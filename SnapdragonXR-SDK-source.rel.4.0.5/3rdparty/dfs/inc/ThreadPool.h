#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/******************************************************************************
* File: ThreadPool.h
*
* Copyright (c) 2013, 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <exception>

#include <sys/resource.h>
#include <sys/syscall.h>
#include <pthread.h>

template< int NTHREADS, bool ENABLE_AFFINITY = false >
class _ThreadPool
{
   _ThreadPool( const _ThreadPool& ) = delete; // no copy constructor
   _ThreadPool( const _ThreadPool&& ) = delete; // no move constructor
   _ThreadPool& operator=( const _ThreadPool& ) = delete; // no copy assignment operator
   _ThreadPool& operator=( const _ThreadPool&& ) = delete; // no move assignment operator

private:
   std::vector<std::thread> _threads;
   std::mutex _mutex;
   std::condition_variable _cvRun;
   std::condition_variable _cvDone;
   std::condition_variable _cvExit;
   unsigned int _runMask;
   unsigned int _exitMask;
   std::function<void()> _workerFunc;
   unsigned int _numThreadsCreated;

   _ThreadPool() : _runMask( 0 ), _exitMask( 0 ), _numThreadsCreated( 0 )
   {
      for ( int i = 0; i < NTHREADS; ++i )
      {
         _threads.push_back( std::thread( &_ThreadPool::_workerThread, this ) );
      }
   }

   ~_ThreadPool()
   {
      {
         std::unique_lock<std::mutex> lck( _mutex );
         _exitMask = ( 1 << NTHREADS ) - 1;
         _cvRun.notify_all();
         _cvExit.wait( lck, [this](){ return _exitMask == 0; } );
      }

      for ( auto& th : _threads )
      {
#ifdef _WIN32
         // Workaround for a bug in Visual Studio.
         // Calling std::thread::join() after main() exits will result in deadlock.
         th.detach();
#else
         th.join();
#endif
      }
   }

   int _workerThread()
   {
      // lock the work list
      std::unique_lock<std::mutex> lck( _mutex );

      int threadId = _numThreadsCreated++;
      unsigned int threadMask = 1 << threadId;

#if defined(__ANDROID__) && !defined(__arm_linux__)
      pthread_setname_np( pthread_self(), "cp_thread" );
#if 0
      //commented this out - AC
      if ( ENABLE_AFFINITY == true )
      {
         unsigned long mask = 1<<(threadId+4);
         syscall( __NR_sched_setaffinity, 0, 4, &mask );
      }
#endif
#endif

      while ( ( _exitMask & threadMask ) == 0 )
      {
         // wait until there is work to do
         _cvRun.wait( lck, [=](){ return ( _runMask & threadMask ) != 0 || ( _exitMask & threadMask ) != 0; } );

         if ( ( _exitMask & threadMask ) != 0 )
         {
            break;
         }

         // free the work list for others
         lck.unlock();

         _workerFunc();   // do work.

         //mark that one more job is complete.
         lck.lock();

         _runMask &= ~threadMask;
         if ( _runMask == 0 )
         {
            _cvDone.notify_all();
         }
      }

      _exitMask &= ~threadMask;
      if ( _exitMask == 0 )
      {
         _cvExit.notify_all();
      }

      return 0;
   }

   int _run( std::function<void()> callback )
   {
      std::unique_lock<std::mutex> lck( _mutex );

      _workerFunc = callback;

      _runMask = ( 1 << NTHREADS ) - 1;

      _cvRun.notify_all();

      _cvDone.wait( lck, [ this ](){ return _runMask == 0; } );

      return 0;
   }

#ifndef _MSC_VER
   static _ThreadPool _instance;
#endif

public:
   static int run( std::function<void()> callback )
   {
      static std::mutex mtx;
#ifdef _MSC_VER
      _ThreadPool _instance;
#endif

      // Allow only one thread to run at any time
      std::unique_lock<std::mutex> lck( mtx );

      return _instance._run( callback );
   }
};

// Workaround for buggy std::thread in Visual Studio 2013.
// Creating threads before entering main(), and joining threads after exiting
// main() often results in deadlock.
#ifndef _MSC_VER
template< int NTHREADS, bool ENABLE_AFFINITY >
_ThreadPool<NTHREADS, ENABLE_AFFINITY> _ThreadPool<NTHREADS, ENABLE_AFFINITY>::_instance;
#endif


typedef _ThreadPool<4> ThreadPool;

#endif
