package com.qti.acg.apps.controllers.finch;

import android.os.ParcelFileDescriptor;
import android.util.Log;



public class ControllerContext {

    private static final String TAG = "ControllerContext";
    FinchControllerStorage storage;

    int batteryLevel = 100;

    static {
        System.loadLibrary("sharedmem");
    }

    private static native void updateNativeState(int ptr, int state, int btns,
                                                            float pos0, float pos1, float pos2,
                                                            float rot0, float rot1, float rot2, float rot3,
                                                            float gyro0, float gyro1, float gyro2,
                                                            float acc0, float acc1, float acc2,
                                                            int timestamp,
                                                            int touchpads, float x, float y, float trigger);

    private static native void updateNativeConnectionState(int ptr, int connectionState);

    private static native int createNativeContext(int fd, int size);
    public native void getLastData(float gx, float gy, float gz, float ax, float ay, float az, float mx,  float my,  float mz);
    public native void ReadMyState(int ptr, int state, int dev_id);
    private static native void getPoseData(int ptr, float[] pos, float[] rot, long[] timestamp);
    private static native int createNativePoseContext(int fd, int size);
    int id = -1;
    int deviceId = -1;

    FinchControllerService.DeviceInfo di = null;

    int fd = -1;
    int fdSize = 0;
    public static int qvrFd = -1;
    public static int qvrFdSize = 0;
    int nativeContextPtr = 0;
    public static int nativePoseContextPtr = 0;
    float[] prev_frame_quat = new float[4];


    public static void getHMDPoseData(float[] pos,  float[] rot, long[] timestamp) {
        getPoseData(nativePoseContextPtr, pos, rot, timestamp);
    }
    /**
     *
     * @return
     */
    //----------------------------------------------------------------------------------------------
    public static ControllerContext CreateFor(int handle, String desc, ParcelFileDescriptor pfd, int fdSize, ParcelFileDescriptor pfdPose, int fdPoseSize, FinchControllerService.DeviceInfo deviceInfo)
    {
        Log.e(TAG, "CreateFor ");
        ControllerContext controllerContext = new ControllerContext();
        controllerContext.id= handle;
        controllerContext.fd = pfd.getFd();
        controllerContext.fdSize = fdSize;
        if(controllerContext.qvrFd == -1) {
            controllerContext.qvrFd = pfdPose.getFd();
            controllerContext.qvrFdSize = fdPoseSize;
        }
        controllerContext.di= deviceInfo;
        return controllerContext;
    }

    /**
     *
     */
    //----------------------------------------------------------------------------------------------
    private void updateNativeState()
    {
        Log.e(TAG, "updateNativeState ");

    }

    //----------------------------------------------------------------------------------------------
    Runnable stateListener = new Runnable() {
        @Override
        public void run() {
            updateNativeState();
        }
    };

    //
    //
    //----------------------------------------------------------------------------------------------
    public String InitializeIfNeeded(boolean init)
    {
        Log.e(TAG, "Init ");
        storage = FinchControllerStorage.getInstance();
        RegisterCallback(di.identifier);
        return storage.InitializeIfNeeded(init);
    }

    //
    //
    //
    //----------------------------------------------------------------------------------------------
    public boolean Start()
    {
        Log.e(TAG, "Start fd " + fd);
        boolean bResult = storage.Start(di.identifier);
        nativeContextPtr = createNativeContext(fd, fdSize);
        if(nativePoseContextPtr == 0) {
            nativePoseContextPtr = createNativePoseContext(qvrFd, qvrFdSize);
        }
        Log.e(TAG, "Start bResult " + bResult + " NativeContextPtr " + Integer.toHexString(nativeContextPtr) + " size " + fdSize);
        if(bResult == true) {
            updateNativeConnectionState(nativeContextPtr, 2);
        }
        prev_frame_quat[0] = 0.0f;
        prev_frame_quat[1] = 0.0f;
        prev_frame_quat[2] = 0.0f;
        prev_frame_quat[3] = 0.0f;
        return bResult;
    }

    //
    //
    //----------------------------------------------------------------------------------------------
    public void Stop()
    {
        Log.e(TAG, "Stop ");
        storage.Stop(di.identifier);
    }

    //
    //
    //----------------------------------------------------------------------------------------------
    public void reset() {
        Log.e(TAG, "reset ");
        storage.reset();

    }

    //
    //
    //----------------------------------------------------------------------------------------------
    public void sendEvent(int what, int arg1, int arg2)
    {
        Log.e(TAG, "Send Event " + what + " -- " + arg1 + " --- " + arg2);
        switch( what )
        {
            case 1:
                Log.e(TAG, " Trigger Vibration ");
                int strength = 20; //(20~100) %
                int frequency = 0; //(default 0)
                int duration = 500; //ms
				/* Sachin
                XDeviceApi.sendMessage(deviceId, XDeviceConstants.kMessage_TriggerVibration,
                        (int) ((strength <= 0 ? 20 : strength) | ((frequency << 16) & 0xFFFF0000)),
                        (int) (duration * 1000));
				*/
                break;
            case 201:
                break;
        }
    }

    public int getBatteryLevel()
    {
        return storage.getBatteryLevel(di.identifier);
    }

    public void RegisterCallback(int index) {
        storage.dataCallback[index] = new ControllerDataCallbacks() {
            public void dataAvailable(long[] timestamp, float[] pos, float[] quat, float[] posVar,
                                      float[] quatVar, float[] vel, float[] angVel, float[] accel,
                                      float[] accelOffset, float[] gyroOffset, float[] gravityVector,
                                      int[] buttons, float[] touchPad,
                                      int[] isTouching, float[] trigger) {
            /*public void dataAvailable(long[] timestamp, float[] pos, float[] quat, float[] posVar,
                                      float[] quatVar, float[] vel, float[] angVel, float[] accel,
                                      float[] accelOffset, float[] gyroOffset, float[] gravityVector,
                                      int[] button_down, int[] button_up,
                                      int[] last_state, int[] touchpad) {*/
                //Log.e(TAG, "Received DataAvailable " + this + " nativeContextPtr " + nativeContextPtr);
                /*
                if(pos[0] == 0.0 && pos[1] == 0.0 && pos[2] == 0.0) {
                    Log.e(TAG, "FinchControllerLog Index " + di.identifier + " Error with Position data... skipping");
                    return;
                }*/
                /*float[] new_quat = new float[4];
                new_quat[0] = Math.abs(new_quat[0]*100/100);
                new_quat[1] = Math.abs(new_quat[1]*100/100);
                new_quat[2] = Math.abs(new_quat[2]*100/100);
                new_quat[3] = Math.abs(new_quat[3]*100/100);
                Log.e(TAG, "FinchControllerLog Index " + di.identifier + " new_quat=("+new_quat[0]+"," + new_quat[1] + "," + new_quat[2] + "," + new_quat[3]);

                if(new_quat[0] == 0.5 && new_quat[1] == 0.5 && new_quat[2] == 0.5 && new_quat[3] == 0.5) {
                    Log.e(TAG, "FinchControllerLog Index " + di.identifier + " Skipping worked");
                }*/
                //double distance = Math.sqrt(Math.pow((prev_frame_quat[0]-quat[0]),2) + Math.pow((prev_frame_quat[1]-quat[1]),2) + Math.pow((prev_frame_quat[2]-quat[2]),2) + Math.pow((prev_frame_quat[2]-quat[2]),2));
                //Log.e(TAG, "FinchControllerLog Index " + di.identifier + "touchPad x=" + touchPad[0] + "touchPad y=" + touchPad[1]+ " pos=(" + pos[0] + ", " + pos[1] + ", " + pos[2]+"), Rot=(" + quat[1] + ", " + quat[2] + ", " + quat[3] + ", " +  quat[0] + "), distance="+distance);
                /*if(distance < 0.25 || (prev_frame_quat[0] == 0.0 && prev_frame_quat[1] == 0.0 && prev_frame_quat[2] == 0.0 && prev_frame_quat[3] == 0.0)) {
                    prev_frame_quat[0] = quat[0];
                    prev_frame_quat[1] = quat[1];
                    prev_frame_quat[2] = quat[2];
                    prev_frame_quat[3] = quat[3];
                } else {
                    Log.e(TAG, "FinchControllerLog Index " + di.identifier + " Error with Rotation data... skipping");
                    return;
                }*/

                updateNativeState(nativeContextPtr,
                        2,
                        buttons[0],
                        //pos[0], pos[1], pos[2],
                        //quat[0], quat[1], quat[2], quat[3],
                         -pos[0], -pos[1], pos[2], // Unity to SVR space this is required when head pose data is used
                        quat[0], quat[1], -quat[2], quat[3], //Unity to SVR space, this is required when head pose data is used.

                        //-(quat[1]), quat[2], quat[3], quat[0], //Original code
                        angVel[0], angVel[1], angVel[2], //state.gyroscope[0], state.gyroscope[1],state.gyroscope[2],
                        accel[0], accel[1], accel[2], //state.accelerometer[0], state.accelerometer[1], state.accelerometer[2],
                        (int)timestamp[0], //state.timestamp,
                        isTouching[0], touchPad[0], touchPad[1], trigger[0]); //touchpads, state.axes[2], state.axes[3]);

            }
        };
    }

}
