#ifndef ROW_PIPELINE_H
#define ROW_PIPELINE_H

/******************************************************************************
* File: RowPipeline.h
*
* Copyright (c) 2013, 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#include <vector>
#include <cstdio>
#include <stdint.h>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "ThreadPool.h"

class RowOperationBase
{
   RowOperationBase() = delete; // no default constructor
   RowOperationBase( const RowOperationBase& ) = delete; // no copy constructor
   RowOperationBase& operator=( const RowOperationBase& ) = delete; // no copy assignment operator
   RowOperationBase( const RowOperationBase&& ) = delete; // no move constructor
   RowOperationBase& operator=( const RowOperationBase&& ) = delete; // no move assignment operator

public:
   enum ErrorStatus
   {
      ALL_ROWS_DONE = -1,
      BLOCKED_BY_SELF_DEPENDENCY = -2,
      BLOCKED_BY_BACKWARD_DEPENDENCY = -3,
      BLOCKED_BY_FORWARD_DEPENDENCY = -4,
      MUTEX_TRYLOCK_FAILED = -5
   };

   int doWork()
   {
      int y = schedule();
      if ( y >= 0 )
      {
         perform( y );
         y = done( y );
      }

      return y;
   }

protected:
   RowOperationBase( unsigned int numRows, unsigned int ahead, int lag = 0 )
      : _numRows( numRows ), _ahead( ahead ),
        _prevOpPtr( nullptr ), _nextOpPtr( nullptr ), _lag( lag )
   {
      init();
   }

   virtual ~RowOperationBase() = default;

   virtual void perform( unsigned int  y ) = 0;

   virtual void release() {}

   virtual void postprocess() {}

   virtual void preprocess() {}

private:
   // Backward dependency check.
   // Has this operation produced enough rows for the next operation to process row y?
   bool hasEnoughRows( int y, unsigned int numRows ) const
   {
      return ( (int)_numRowsDone * (int)numRows > y * (int)_numRows || _numRowsDone >= _numRows );
   }

   // Forward dependency check.
   // Does this operation need more rows from previous operation to process row y?
   bool needsMoreInputRows( int y, unsigned int numRows ) const
   {
      return ( y * (int)_numRows <= ( (int)_nextY + (int)_ahead + _lag ) * (int)numRows );
   }

   void setPrevOpPtr( const RowOperationBase* ptr )
   {
      _prevOpPtr = ptr;
   }

   void setNextOpPtr( const RowOperationBase* ptr ) const
   {
      _nextOpPtr = ptr;
   }

   // Schedule this operation after checking dependencies.
   // Returns scheduled row number if all dependencies are satisfied.
   // Returns negative value as error status otherwise.
   int schedule()
   {
      std::unique_lock<std::mutex> lck( _mutex, std::try_to_lock );

      int y;
      if ( lck )
      {
         if ( _nextY >= _numRows )
         {
            y = ALL_ROWS_DONE;
         }
         else if ( _nextY >= _numRowsDone + _ahead )
         {
            y = BLOCKED_BY_SELF_DEPENDENCY;
         }
         else if ( _prevOpPtr != nullptr && !_prevOpPtr->hasEnoughRows( _nextY + _lag, _numRows ) )
         {
            y = BLOCKED_BY_BACKWARD_DEPENDENCY;
         }
         else if ( _nextOpPtr != nullptr && !_nextOpPtr->needsMoreInputRows( _numRowsDone, _numRows ) )
         {
            y = BLOCKED_BY_FORWARD_DEPENDENCY;
         }
         else
         {
            y = _nextY;
            _nextY += 1;
         }
      }
      else
      {
         y = MUTEX_TRYLOCK_FAILED;
      }

      return y;
   }

   // Performs bookkeeping after row y is processed.
   // Returns the delta in numRowsDone caused by row y being done.
   // Nonzero delta means the next operation may become unblocked.
   unsigned int done( unsigned int y )
   {
      std::lock_guard<std::mutex> lck( _mutex );

      // Update rowMask
      int bitPos = y - _numRowsDone;
      _rowMask |= (1 << bitPos);

      // Check to see if we can move numRowsDone further
      int moreRowsDone = 0;
      while ( _rowMask & 1 )
      {
         _rowMask >>= 1;
         ++moreRowsDone;
      }

      // Move numRowsDone
      _numRowsDone += moreRowsDone;

      bool isOperationDone = ( moreRowsDone > 0 && _numRowsDone >= _numRows );

      // Free memory in worker threads
      // May cause stability problems if cv::Mat::release() is not thread safe
      if ( isOperationDone )
      {
         postprocess();
         //release();
      }

      return moreRowsDone;
   }

   const unsigned int _numRows;
   const unsigned int _ahead;
   const RowOperationBase* _prevOpPtr;
   mutable const RowOperationBase* _nextOpPtr;
   const int _lag;

   std::mutex _mutex;
   unsigned int _nextY;
   unsigned int _numRowsDone;
   uint32_t _rowMask;

   void init()
   {
      _nextY = 0;
      _numRowsDone = 0;
      _rowMask = 0;

      preprocess();
   }

   template< int NUMBER_OF_THREADS, bool AFFINITY > friend class _OperationPipeline;



   // Deprecated below
protected:
   RowOperationBase( unsigned int numRows, unsigned int ahead, const RowOperationBase* prevOpPtr, int lag = 0 )
      : _numRows( numRows ), _ahead( ahead ), _prevOpPtr( prevOpPtr ), _nextOpPtr( nullptr ),
        _lag( lag ), _nextY( 0 ), _numRowsDone( 0 ), _rowMask( 0 )
   {
      if ( prevOpPtr != nullptr )
      {
         prevOpPtr->setNextOpPtr( this );
      }
   }
};

template< int NUMBER_OF_THREADS,  bool AFFINITY = false >
class _OperationPipeline
{
   _OperationPipeline( const _OperationPipeline& ) = delete; // no copy constructor
   _OperationPipeline& operator=( const _OperationPipeline& ) = delete; // no copy assignment operator
   _OperationPipeline( const _OperationPipeline&& ) = delete; // no move constructor
   _OperationPipeline& operator=( const _OperationPipeline&& ) = delete; // no move assignment operator

public:
   _OperationPipeline() = default;
   ~_OperationPipeline() = default;

   // Append RowOperation to the end of this pipeline.
   template< typename T, typename... Args >
   std::shared_ptr<T>
   addOperation( Args&&... args )
   {
      std::shared_ptr<T> opPtr = std::make_shared<T>( args... );
      if ( !_operations.empty() )
      {
         opPtr->setPrevOpPtr( _operations.back().get() );
         _operations.back()->setNextOpPtr( opPtr.get() );
      }

      _operations.push_back( opPtr );

      return opPtr;
   }

   // Run this pipeline in worker threads
   void run()
   {
      init();

      _ThreadPool<NUMBER_OF_THREADS, AFFINITY>::run( [this](){ runEachThread(); } );
   }

   // Run this pipeline sequentially in caller's context
   // The operations are called in a simple for loop.
   // There is no dependency check and no dynamic scheduling.
   void runSequential()
   {
      init();

      for ( auto& opPtr : _operations )
      {
         for ( int y = 0; y < opPtr->_numRows; ++y )
         {
            opPtr->perform( y );
         }
         opPtr->postprocess();
      }
   }

private:
   // Entry function which will be called by ThreadPool
   void runEachThread()
   {
      if ( _operations.size() > 0 )
      {
         int lastOpIndex = 0;
         int firstOpIndex = 0;

         while ( true )
         {
            bool shouldSleep = true;

            // Remember the current values of lastOpIndex and firstOpIndex,
            // which may be modified in the for loop.
            int lastOpIndexTmp = lastOpIndex;
            int firstOpIndexTmp = firstOpIndex;
            for ( int i = lastOpIndexTmp; i >= firstOpIndexTmp; --i )
            {
               int status = _operations[i]->doWork();

               if ( status > 0 )
               {
                  // Work was done for operation i and state had been changed.

                  bool shouldNotify = false;

                  // The block below is protected by mutex.
                  // Indicate that sleeping threads should wake up.
                  {
                     std::lock_guard<std::mutex> lck( _mutex );
                     if ( _numSleepingThreads > 0 )
                     {
                        _wakeFlag = true;
                        shouldNotify = true;
                     }
                  }

                  // notify_all is expensive. Do only when necessary.
                  if ( shouldNotify )
                  {
                     // No lock is required to call notify_all.
                     _cvWake.notify_all();
                  }

                  shouldSleep = false;
                  break;
               }
               else if ( status == 0 )
               {
                  // Work was done for operation i and state was unchanged.
                  // Start over from the end of the pipeline.
                  shouldSleep = false;
                  break;
               }
               else if ( status == RowOperationBase::ALL_ROWS_DONE )
               {
                  if ( i < static_cast<int>( _operations.size() ) - 1 )
                  {
                     // Operation i is done so we can skip operation 0 ... i from now on.
                     firstOpIndex = i + 1;

                     // Make sure lastOpIndex is no less than firstOpIndex
                     if ( lastOpIndex < firstOpIndex )
                        lastOpIndex = firstOpIndex;

                     shouldSleep = false;
                     break;
                  }
                  else
                  {
                     // Last operation in the pipeline is done.

                     bool shouldNotify = false;

                     // The block below is protected by mutex.
                     // Indicate the entire pipeline is done,
                     // and sleeping threads should wake up.
                     {
                        std::lock_guard<std::mutex> lck( _mutex );
                        _isDone = true;
                        if ( _numSleepingThreads > 0 )
                        {
                           _wakeFlag = true;
                           shouldNotify = true;
                        }
                     }

                     // notify_all is expensive. Do only when necessary.
                     if ( shouldNotify )
                     {
                        // No lock is required to call notify_all.
                        _cvWake.notify_all();
                     }

                     return;
                  }
               }
               else if ( status == RowOperationBase::BLOCKED_BY_FORWARD_DEPENDENCY )
               {
                  // Operation i is blocked because operation i+1 cannot catch up.

                  if ( lastOpIndex < i + 1 )
                  {
                     // This thread never processed operation i+1 due to lastOpIndex.
                     // Adjust lastOpIndex and start over.
                     lastOpIndex = i + 1;
                     shouldSleep = false;
                     break;
                  }
               }
               else if ( status == RowOperationBase::MUTEX_TRYLOCK_FAILED )
               {
                  // Cannot process operation i due to contention with another thread.
                  // Do not go to sleep because there might still be work for operation i.
                  shouldSleep = false;
               }
            }

            // Go to sleep if we cannot find any work in all operations.
            if ( shouldSleep )
            {
               std::unique_lock<std::mutex> lck( _mutex );
               // Before going to sleep, check if _isDone flag is set.
               if ( _isDone )
               {
                  return;
               }
               else
               {
                  ++_numSleepingThreads;
                  _wakeFlag = false;
                  _cvWake.wait( lck, [&](){ return _wakeFlag; } );
                  --_numSleepingThreads;
               }
            }
         }
      }
   }

   std::vector<std::shared_ptr<RowOperationBase>> _operations;
   std::mutex _mutex;
   std::condition_variable _cvWake;

   bool _wakeFlag;
   bool _isDone;
   unsigned int _numSleepingThreads;

   void init()
   {
      _isDone = false;
      _numSleepingThreads = 0;
      _wakeFlag = false;

      for ( auto& opPtr : _operations )
      {
         opPtr->init();
      }
   }



   // Deprecated below
public:
   void addOperation( RowOperationBase& op )
   {
      std::shared_ptr<RowOperationBase> opPtr( &op, []( void* ) {} );

      _operations.push_back( opPtr );
   }
};

typedef _OperationPipeline<4> OperationPipeline;

#endif //ROW_PIPELINE_H
