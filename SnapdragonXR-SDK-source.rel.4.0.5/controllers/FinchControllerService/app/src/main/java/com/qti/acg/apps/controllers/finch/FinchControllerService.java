package com.qti.acg.apps.controllers.finch;

import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;

import com.qualcomm.svrapi.controllers.SvrControllerApi;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class FinchControllerService extends SvrControllerModule {

    private static final String TAG = "QualcommControllerService";
    HashMap<Integer, ControllerContext> listOfControllers = new HashMap<>(2);
    List<DeviceInfo> listOfDevices = new ArrayList<>();
    boolean bInitialized = false;
    FinchControllerStorage storage;
    boolean ControllerStated = false;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class DeviceInfo {
        int identifier;
        boolean isInUse;

        DeviceInfo(int s)
        {
            identifier = s;
            isInUse = false;
        }

        void MarkInUse()
        {
            isInUse = true;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void updateDeviceList()
    {
        listOfDevices.add(new DeviceInfo(0));
        listOfDevices.add(new DeviceInfo(1));
        listOfDevices.add(new DeviceInfo(0));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    @Override
    public void onCreate() {
        Log.e(TAG, "onCreate");
        super.onCreate();
        updateDeviceList();
        storage = FinchControllerStorage.getInstance();
        storage.setAppContext(this);
        initializeIfNeeded();
        //System.loadLibrary("Finchc_skel");
        //System.loadLibrary("ubeacon");
        //System.loadLibrary("ubeaconadsp");

        bInitialized = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    @Override
    public void ControllerStop(int id)
    {
        Log.e(TAG, "ControllerStop");
        ControllerContext controllerContext = listOfControllers.get(id);
        if( controllerContext != null ) {
            listOfControllers.remove(id);
            listOfDevices.get(id).isInUse = false;
            controllerContext.Stop();
            controllerContext.di.isInUse = false;
        }
        if(listOfControllers.isEmpty()) {
            Log.e(TAG, "All Controllers are removed, freeing storage instance");
            bInitialized = false;
            //storage.Stop();
            //Newly added
            //storage = null;
            //FinchControllerStorage.freeInstance();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    DeviceInfo GetAvailableDevice()
    {
        DeviceInfo di = null;
        for(int i=0;i<listOfDevices.size();i++)
        {
            if( listOfDevices.get(i).isInUse == false )
            {
                di = listOfDevices.get(i);
                break;
            }
        }
        if(di != null) {
            Log.e(TAG, "GetAvailableDevice " + di.identifier);
        } else {
            Log.e(TAG, "GetAvailableDevice di=" + di);
        }
        return di;
    }

    /**
     * Initialize
     */
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void initializeIfNeeded()
    {
        storage.setAppContext(getApplicationContext());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ControllerContext GetAvailableController(int handle, String desc, ParcelFileDescriptor pfd, int fdSize,ParcelFileDescriptor qvrFd, int qvrFdSize)
    {
        ControllerContext controllerContext = null;

        DeviceInfo deviceInfo = GetAvailableDevice();
        if( deviceInfo != null ) {
            controllerContext = ControllerContext.CreateFor(handle, desc, pfd, fdSize, qvrFd, qvrFdSize, deviceInfo);
            if( controllerContext != null )
            {
                deviceInfo.MarkInUse();
                listOfControllers.put(controllerContext.id, controllerContext);
            }
        }

        return controllerContext;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    @Override
    synchronized protected void ControllerStart(int handle, String desc, ParcelFileDescriptor pfd, int fdSize, ParcelFileDescriptor qvrFd, int qvrFdSize) {

            Log.e(TAG, "ControllerStart");
            //synchronized (storage) {
                ControllerContext controllerContext = GetAvailableController(handle, desc, pfd, fdSize, qvrFd, qvrFdSize);
                //Log.e(TAG, "ControllerStart start Synchronized");
                if (controllerContext != null) {
                    String result = controllerContext.InitializeIfNeeded(!bInitialized);
                    bInitialized = true;

                    if (true == controllerContext.Start()) {
                        Log.e(TAG, "ControllerStart sending Controller_connected");
                        //OnDeviceStateChanged(handle, SvrControllerApi.CONTROLLER_CONNECTING);
                        OnDeviceStateChanged(handle, SvrControllerApi.CONTROLLER_CONNECTED);
                    } else {
                        OnDeviceStateChanged(handle, SvrControllerApi.CONTROLLER_ERROR);
                    }
                } else {
                    //TODO: Not Available
                }
                //Log.e(TAG, "ControllerStart End Synchronized");
           // }

    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    protected void ControllerExit()
    {
        if( bInitialized )
        {

            bInitialized = false;
            stopSelf();
            Process.killProcess(Process.myPid());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    @Override
    protected void ControllerStopAll() {
        int size = listOfControllers.size();
        for(int i=0;i<size;i++)
        {
            ControllerStop(i);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    protected int ControllerQueryInt(int handle, int what)
    {
        int result = -1;
        ControllerContext controllerContext = listOfControllers.get(handle);
        if( controllerContext != null )
        {
            switch(what)
            {
                case SvrControllerApi.svrControllerQueryType_kControllerQueryBatterRemaining:
                    result = controllerContext.getBatteryLevel();
                    break;
                case SvrControllerApi.svrControllerQueryType_kControllerQueryControllerCaps:
                    //Log.e(TAG, "ControllerStart Query Capability");
                    result = 1;
                    break;
                /*case SvrControllerApi.svrControllerQueryType_kControllerQueryNumControllers:
                    Log.e(TAG, "ControllerStart Query Num Controller");
                    result = 2;
                    break;*/
                case SvrControllerApi.svrControllerQueryType_kControllerQueryActiveButtons:
                    //Log.e(TAG, "ControllerStart Query ActiveButtons");
                    result = 8;
                    break;
                case SvrControllerApi.svrControllerQueryType_kControllerQueryActive2DAnalogs:
                    //Log.e(TAG, "ControllerStart Query Active2DAnalogs");
                    result = 2;
                    break;
                case SvrControllerApi.svrControllerQueryType_kControllerQueryActive1DAnalogs:
                    //Log.e(TAG, "ControllerStart Query Active1DAnalogs");
                    result = 0;
                    break;
                case SvrControllerApi.svrControllerQueryType_kControllerQueryActiveTouchButtons:
                    //Log.e(TAG, "ControllerStart Query ActiveTouchButtons");
                    result = 2;
                    break;
            }

        }
        return result;
    }
    protected String ControllerQueryString(int handle, int what)
    {
        String result = null;
        ControllerContext controllerContext = listOfControllers.get(handle);
        if( controllerContext != null )
        {
            switch(what)
            {
                case SvrControllerApi.svrControllerQueryType_kControllerQueryDeviceManufacturer:
                    Log.e(TAG, "ControllerStart Query DeviceManufacturer");
                    result = "Finch Microsystem";
                    break;
                case SvrControllerApi.svrControllerQueryType_kControllerQueryDeviceIdentifier:
                    Log.e(TAG, "ControllerStart Query DeviceIdentifier");
                    result = "12345678";
                    break;
            }

        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    @Override
    protected void ControllerRecenter(Message msg) {
        Log.e(TAG, "ControllerRecenter");
        ControllerContext controllerContext = listOfControllers.get(msg.arg1);
        if( controllerContext != null ) {
            controllerContext.reset();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    @Override
    protected void ControllerSendMessage(int handle, int what, int arg1, int arg2) {
        //TODO:
        ControllerContext controllerContext = listOfControllers.get(handle);
        if( controllerContext != null )
        {
            switch(what)
            {
                case SvrControllerApi.svrControllerMessageType_kControllerMessageRecenter:
                    controllerContext.reset();
                    break;
            }
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    public void OnDeviceStateChanged(int deviceId, int state) {

        ControllerContext cc = listOfControllers.get(deviceId);
        try {
            callback.onStateChanged(cc.id, state, 0, 0);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

}
