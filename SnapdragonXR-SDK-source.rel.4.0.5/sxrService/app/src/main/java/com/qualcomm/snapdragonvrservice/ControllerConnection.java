//-----------------------------------------------------------------------------
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

package com.qualcomm.snapdragonvrservice;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;

import com.qualcomm.svrapi.controllers.IControllerInterface;
import com.qualcomm.svrapi.controllers.IControllerInterfaceCallback;

import java.io.IOException;

/**
 * ControllerConnection is a middle layer to handle the connect operations with controller module.
 * Including allocate memory, bind the controller module service, start/stop tracking the controller
 * state, also read the controller state from shared memory and fill the data to ControllerState
 * instance.
 */

public class ControllerConnection {

    static ControllerConnection sInstance;
    static final String TAG = "ControllerConnection";

    static final int BATTERY_LEVEL_INVALID = -1;
    static final int CONTROLLER_CLIENT_ID = 100;
    static final String CONTROLLER_CLIENT_DESC = "SVRControllerDebug";

    ControllerState.ConnectionState mConnectionStatus = ControllerState.ConnectionState.DISCONNECTED;

    int nativeRingBufferPtr = 0;
    int DEFAULT_RINGBUFFER_SIZE = 80;
    ParcelFileDescriptor pfd = null;
    int fdSize = 0;

    IControllerInterface controllerInterface;

    Handler mUIHandler = null;

    ServiceConnection serviceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
            LogUtil.log(TAG, "onServiceConnected");
            controllerInterface = IControllerInterface.Stub.asInterface(iBinder);
            startTracking();
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            LogUtil.log(TAG, "onServiceDisconnected");
        }
    };

    private IControllerInterfaceCallback mControllerCallback = new IControllerInterfaceCallback.Stub() {
        @Override
        public void onStateChanged(int handle, int what, int arg1, int arg2)
                throws RemoteException {
            LogUtil.log(TAG, "onStateChanged: handle=" + handle + ",what=" + what);
        }
    };

    private ControllerConnection() {
    }

    public static ControllerConnection getInstance() {
        if (sInstance == null) {
            sInstance = new ControllerConnection();
        }
        return sInstance;
    }

    public ControllerState.ConnectionState getConnectionState() {
        return mConnectionStatus;
    }

    public boolean allocateMemory() {
        nativeRingBufferPtr = AllocateSharedMemory(DEFAULT_RINGBUFFER_SIZE);
        int fd = GetFileDescriptor(nativeRingBufferPtr);
        pfd = ParcelFileDescriptor.adoptFd(fd);
        fdSize = GetFileDescriptorSize(nativeRingBufferPtr);
        return true;
    }

    public void freeMemory() {
        try {
            if (pfd != null) {
                pfd.close();
            }
            if (nativeRingBufferPtr != 0) {
                FreeSharedMemory(nativeRingBufferPtr);
            }
            pfd = null;
            nativeRingBufferPtr = 0;
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public boolean bindControllerService(Context context, Intent intent, Handler handler) {
        if (intent == null || handler == null) {
            return false;
        }
        mUIHandler = handler;
        context.bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
        return true;
    }

    public boolean startTracking() {
        try {
            controllerInterface.registerCallback(mControllerCallback);
            LogUtil.log(TAG, "controller api start");
            controllerInterface.Start(CONTROLLER_CLIENT_ID, CONTROLLER_CLIENT_DESC, pfd, fdSize,
                    null, 0);
        } catch (RemoteException e) {
            e.printStackTrace();
            return false;
        }
        mUIHandler.sendEmptyMessage(ControllerDebugFragment.MSG_CONNECTED);
        mConnectionStatus = ControllerState.ConnectionState.CONNECTED;
        return true;
    }

    public int getBatteryLevel() {
        int battery = BATTERY_LEVEL_INVALID;
        if (mConnectionStatus == ControllerState.ConnectionState.CONNECTED) {
            try {
                battery = controllerInterface.QueryInt(CONTROLLER_CLIENT_ID,
                        ControllerState.SvrControllerQueryType.BATTERY_REMAINING);
            } catch (RemoteException e) {
                e.printStackTrace();
                return BATTERY_LEVEL_INVALID;
            }
        } else {
            return BATTERY_LEVEL_INVALID;
        }
        return battery;
    }

    public void updateControllerState() {
        GetControllerState(nativeRingBufferPtr);
        ControllerState.getInstance().mBatteryLevel = getBatteryLevel();
    }

    public boolean unbindControllerService(Context context) {
        context.unbindService(serviceConnection);
        return true;
    }

    public boolean stopTracking() {
        try {
            controllerInterface.Stop(CONTROLLER_CLIENT_ID);
        } catch (RemoteException e) {
            e.printStackTrace();
            return false;
        }
        mUIHandler.sendEmptyMessage(ControllerDebugFragment.MSG_DISCONNECTED);
        mConnectionStatus = ControllerState.ConnectionState.DISCONNECTED;
        return true;
    }

    private static native int AllocateSharedMemory(int sz);

    private static native int GetFileDescriptor(int ptr);

    private static native int GetFileDescriptorSize(int ptr);

    private static native void FreeSharedMemory(int fd);

    private static native void GetControllerState(int ptr);

}
