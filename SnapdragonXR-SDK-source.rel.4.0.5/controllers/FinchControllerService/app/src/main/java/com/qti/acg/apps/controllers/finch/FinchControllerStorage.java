package com.qti.acg.apps.controllers.finch;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import com.finchtechnologies.android.sdk.definition.ControllerType;
import com.finchtechnologies.android.sdk.definition.NodeType;
import com.finchtechnologies.android.sdk.definition.Quaternion;
import com.finchtechnologies.android.sdk.definition.Vector3;
import com.finchtechnologies.android.sdk.definition.FinchUpdateType;


/**
 * Created by sachina on 8/31/2017.
 */

public class FinchControllerStorage {
    private static final String TAG = "QualcommControllerStorage";
    public static final int MAX_CONTROLLERS = 2;
    private static FinchControllerStorage myObj = null;
    private Context m_context;

    private FinchService finchService;
    private boolean bound = false;
    private Runnable r = null;
    private Handler handler = null;
    private boolean bReadForUpdates = false;
    private boolean[] ControllerState = new boolean[MAX_CONTROLLERS];
    public ControllerDataCallbacks[] dataCallback  = new ControllerDataCallbacks[MAX_CONTROLLERS];
    private FinchUpdateType  mode = FinchUpdateType.Internal;
    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            FinchService.FinchBinder binder = (FinchService.FinchBinder) service;
            finchService = binder.getService();
            bound = true;
            long[] timestampHMD = new long[1];
            final long[] timestamp = new long[1];

            finchService.init(ControllerType.UniversalController);
            Log.d(TAG, "Service Connected");
            /*
            new Thread(new Runnable() {
                @Override
                public void run() {

                    Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_DISPLAY);

                    while (true) {
                        if (bound)
                            //runOnUiThread(guiRunnable);
                        Log.d("FINCH", "Left Hand " + finchService.update());
                        try {
                            Thread.sleep(16);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }).start();
            */

        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            bound = false;
        }
    };

    public int getBatteryLevel(int index) {
        if (index == 1) {
            return finchService.getBatteryLevel(NodeType.LeftHand);
        } else {
            return finchService.getBatteryLevel(NodeType.RightHand);
        }
    }
    private FinchControllerStorage(){
        if (handler == null) {
            handler = new Handler();
        }
        for(int i = 0; i < MAX_CONTROLLERS; i++) {
            ControllerState[i] = false;
        }
        for(int i = 0; i < MAX_CONTROLLERS; i++) {
            dataCallback[i] = null;
        }

    }

    /**
     * Create a static method to get instance.
     */
    public static FinchControllerStorage getInstance(){
        Log.e(TAG, "getInstance before" + myObj);
        if(myObj == null){
            myObj = new FinchControllerStorage();
        }
        Log.e(TAG, "getInstance after" + myObj);
        return myObj;
    }
    public static void freeInstance(){
        myObj = null;
    }
    public Context getAppContext(){
        // do something here
        Log.e(TAG, "getAppContext " + m_context);
        return m_context;
    }
    public void setAppContext(Context obj){
        // do something here
        Log.e(TAG, "setAppContext " + obj);
        m_context = obj;
    }


    public boolean Start(int id) {
        //Log.e(TAG, "Start " + id);
        // Bind to FinchService1.

        boolean bResult = false;
        ControllerState[id] = true;
        if (bReadForUpdates == false) {

            Log.e(TAG, "Starting thread ");
            if (r == null) {
                r = createRunnable();
            } else {
                Log.e(TAG, "Not starting thread / Reusing");
            }
            if (handler == null) {
                Log.e(TAG, "starting thread");
                handler = new Handler();
            } else {
                Log.e(TAG, "Not creating handler / Reusing");
            }
            //controllerIndex = 0;
            handler.postDelayed(r, 1000);
            bReadForUpdates = true;
            bResult = true;
        } else {
            Log.e(TAG, "bReadForUpdates was already true");
            bResult = true;
        }
        return bResult;
    }
    //
    //
    //----------------------------------------------------------------------------------------------
    public void Stop(int id)
    {
        Log.e(TAG, "Stop ");
        ControllerState[id] = false;
        for(int i = 0; i < MAX_CONTROLLERS; i++) {
            if(ControllerState[i] == true)
            {
                return;
            }
        }
        Log.e(TAG, "Stop Calling terminate");
        //Turn off tracking only when all controllers are stopped
        bReadForUpdates = false;
        // Sachin uba.ubeaconadsp_terminate();
    }

    private Runnable createRunnable(){
        //handler = new Handler();
        Runnable aRunnable = new Runnable(){
            public void run() {
                //Log.e(TAG, "createRunnable: inside run");
                float[] HMDpos = new float[3];
                float[] HMDquat = new float[4];
                final long[] HMDtimestamp = new long[1];
                ControllerContext.getHMDPoseData(HMDpos, HMDquat, HMDtimestamp);
                Vector3 phmd = new Vector3(HMDpos[0], HMDpos[1], -(HMDpos[2]));
                Quaternion qhmd = new Quaternion(HMDquat[0], HMDquat[1], HMDquat[2], HMDquat[3] );
                if(HMDpos[0] == 0 && HMDpos[1] == 0 && HMDpos[2] == 0 &&
                        HMDquat[0] == 0 && HMDquat[1] == 0 && HMDquat[2] == 0 && HMDquat[3] == 0) {
                    mode = FinchUpdateType.Internal;
                } else if (HMDpos[0] == 0 && HMDpos[1] == 0 && HMDpos[2] == 0 &&
                        (HMDquat[0] != 0 || HMDquat[1] != 0 || HMDquat[2] != 0 || HMDquat[3] != 0)) {
                    mode = FinchUpdateType.HmdRotation;
                } else {
                    mode = FinchUpdateType.HmdTransform;
                }
                finchService.update(mode, phmd, qhmd);
                for (int index = 0; index < MAX_CONTROLLERS; index++) {
                    if (ControllerState[index] == true) {
                        final long[] timestamp = new long[1];
                        float[] pos = new float[3];
                        float[] quat = new float[4];
                        float[] posVar = new float[3];
                        float[] quatVar = new float[4];
                        float[] vel = new float[3];
                        float[] angVel = new float[3];
                        float[] accel = new float[3];
                        float[] accelOffset = new float[3];
                        float[] gyroOffset = new float[3];
                        float[] gravityVector = new float[3];
                        int[] buttons = new int[1];
                        float[] touchPad = new float[4];
                        int[] isTouching = new int[1];
                        float[] trigger = new float[1];

                        finchService.getCurrentData(index, timestamp, pos,quat, posVar, quatVar, vel, angVel, accel, accelOffset, gyroOffset, gravityVector, buttons, touchPad, isTouching, trigger);
                        /*if(success == false) {
                            Log.d(TAG, "index= " + index + " Error Collecting Data");
                            continue;
                        }*/
                        //Log.d(TAG, "index= " + index + " TimeStamp="+timestamp[0] + "Position=" + pos[0] + "," + pos[1] + "," + pos[2] + " Rotation=" +
                        //        quat[0] + "," + quat[1] + "," + quat[2] + "," + quat[3]);
                        if (dataCallback[index] != null) {
                            dataCallback[index].dataAvailable(timestamp, pos, quat, posVar, quatVar, vel, angVel, accel,
                                    accelOffset, gyroOffset, gravityVector, buttons, touchPad, isTouching, trigger);
                        }
                    }
                //}
                }
                if (bReadForUpdates == true) {
                    handler.postDelayed(this, 10);
                }
            }
        };
        return aRunnable;
    }
    public void reset()
    {
        Log.d(TAG, "reset called");
        finchService.recenter();
    }
    public String InitializeIfNeeded(boolean init)
    {
        //Log.e(TAG, "InitializeIfNeeded " + init);
        FinchControllerStorage storage = FinchControllerStorage.getInstance();
        byte[] msg = new byte[100];
        int result=0;
        if(bound == false) {
            Intent intent = new Intent(m_context, FinchService.class);
            m_context.bindService(intent, connection, Context.BIND_AUTO_CREATE);
        }
        //ub.ubeacon_set_context(storage.getAppContext());
        /* Sachin if( init == true) {
            if (ub.ubeacon_is_initialized() == 1) {
                try {
                    uba.ubeaconadsp_terminate();
                } catch(Exception e) {
                    Log.e(TAG, "Error trying to terminate ubeacon lib " + e.getStackTrace());
                }
            }
            Log.e(TAG, "Initialize()");
            if (uba.ubeaconadsp_initialize() != 0) {
                Log.e(TAG, "Initialize() error");
                ub.ubeacon_get_error_message(msg, 100);
                return new String(msg).trim();
            }
        }
   */
        return "Success";
    }
}
