//-----------------------------------------------------------------------------
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

package com.qualcomm.snapdragonvrservice;

import android.util.Log;

/**
 * ControllerState is a class to represent the controller state
 */

public class ControllerState {

    static final String TAG = "ControllerState";


    // should maatch with svrControllerState in svrapi.h
    static class SvrControllerButton {
        public static final int NONE                       = 0x00000000;
        public static final int ONE                        = 0x00000001;
        public static final int TWO                        = 0x00000002;
        public static final int THREE                      = 0x00000004;
        public static final int FOUR                       = 0x00000008;
        public static final int DPAD_UP                    = 0x00000010;
        public static final int DPAD_DOWN                  = 0x00000020;
        public static final int DPAD_LEFT                  = 0x00000040;
        public static final int DPAD_RIGHT                 = 0x00000080;
        public static final int START                      = 0x00000100;
        public static final int BACK                       = 0x00000200;
        public static final int RESERVED_A                 = 0x00000400;
        public static final int RESERVED_B                 = 0x00000800;
        public static final int PRIMARY_SHOULDER           = 0x00001000;
        public static final int PRIMARY_INDEX_TRIGGER      = 0x00002000;
        public static final int PRIMARY_HAND_TRIGGER       = 0x00004000;
        public static final int PRIMARY_THUMBSTICK         = 0x00008000;
        public static final int PRIMARY_THUMBSTICK_UP      = 0x00010000;
        public static final int PRIMARY_THUMBSTICK_DOWN    = 0x00020000;
        public static final int PRIMARY_THUMBSTICK_LEFT    = 0x00040000;
        public static final int PRIMARY_THUMBSTICK_RIGHT   = 0x00080000;
        public static final int SECONDARY_SHOULDER         = 0x00100000;
        public static final int SECONDARY_INDEX_TRIGGER    = 0x00200000;
        public static final int SECONDARY_HAND_TRIGGER     = 0x00400000;
        public static final int SECONDARY_THUMBSTICK       = 0x00800000;
        public static final int SECONDARY_THUMBSTICK_UP    = 0x01000000;
        public static final int SECONDARY_THUMBSTICK_DOWN  = 0x02000000;
        public static final int SECONDARY_THUMBSTICK_LEFT  = 0x04000000;
        public static final int SECONDARY_THUMBSTICK_RIGHT = 0x08000000;
        public static final int UP                         = 0x10000000;
        public static final int DOWN                       = 0x20000000;
        public static final int LEFT                       = 0x40000000;
        public static final int RIGHT                      = 0x80000000;
        public static final int ANY                        = ~SvrControllerButton.NONE;
    }

    static class SvrControllerAxis1D {
        public static final int PRIMARY_INDEX_TRIGGER       = 0x00000000;
        public static final int SECONDARY_INDEX_TRIGGER     = 0x00000001;
        public static final int PRIMARY_HAND_TRIGGER        = 0x00000002;
        public static final int SECONDARY_HAND_TRIGGER      = 0x00000003;
    }

    static class SvrControllerAxis2D {
        public static final int PRIMARY_THUMBSTICK          = 0x00000000;
        public static final int SECONDARY_THUMBSTICK        = 0x00000001;
    }

    static class SvrControllerTouch {
        public static final int NONE                  = 0x00000000;
        public static final int ONE                   = 0x00000001;
        public static final int TWO                   = 0x00000002;
        public static final int THREE                 = 0x00000004;
        public static final int FOUR                  = 0x00000008;
        public static final int PRIMARY_THUMBSTICK    = 0x00000010;
        public static final int SECONDARY_THUMBSTICK  = 0x00000020;
        public static final int ANY                   = ~SvrControllerTouch.NONE;
    }

    static class SvrControllerQueryType {
        public static final int BATTERY_REMAINING     = 0;
        public static final int CONTROLLER_CAPS       = 1;
    }

    public enum ConnectionState {
        DISCONNECTED, CONNECTTING, CONNECTED, DISCONNECTTING
    }

    float mRotationX;
    float mRotationY;
    float mRotationZ;
    float mRotationW;

    float mPositionX;
    float mPositionY;
    float mPositionZ;

    float mAccelerometerX;
    float mAccelerometerY;
    float mAccelerometerZ;

    float mGyroscopeX;
    float mGyroscopeY;
    float mGyroscopeZ;

    int mButtonState;
    int mTouchPad;
    int mTimestamp;

    float mAnalog2DPrimaryX;
    float mAnalog2DPrimaryY;

    float mAnalog2DSecondaryX;
    float mAnalog2DSecondaryY;

    float mAnalog1DPrimaryIndex;
    float mAnalog1DSecondaryIndex;
    float mAnalog1DPrimaryHand;
    float mAnalog1DSecondaryHand;

    int mConnectionState;
    int mBatteryLevel = -1;
    static ControllerState sInstance;

    private ControllerState() {
    }

    public static ControllerState getInstance() {
        if (sInstance == null) {
            sInstance = new ControllerState();
        }
        return sInstance;
    }

    public static void updateState(float rotationX, float rotationY, float rotationZ,
            float rotationW,
            float positionX, float positionY, float positionZ,
            float accelerometerX, float accelerometerY, float accelerometerZ,
            float gyroscopeX, float gyroscopeY, float gyroscopeZ,
            int buttonState, int touchPad, int timestamp,
            float analog2DPrimaryX, float analog2DPrimaryY,
            float analog2DSecondaryX, float analog2DSecondaryY,
            float analog1DPrimaryIndex, float analog1DSecondaryIndex,
            float analog1DPrimaryHand, float analog1DSecondaryHand,
            int connectionState) {
        sInstance.mRotationX = rotationX;
        sInstance.mRotationY = rotationY;
        sInstance.mRotationZ = rotationZ;
        sInstance.mRotationW = rotationW;

        LogUtil.log(TAG, "rotation: x=" + rotationX + ",y=" + rotationY + ",z=" + rotationZ + ",w="
                + rotationW);

        sInstance.mPositionX = positionX;
        sInstance.mPositionY = positionY;
        sInstance.mPositionZ = positionZ;

        LogUtil.log(TAG, "position:x=" + positionX + ",y=" + positionY + ",z=" + positionZ);

        sInstance.mAccelerometerX = accelerometerX;
        sInstance.mAccelerometerY = accelerometerY;
        sInstance.mAccelerometerZ = accelerometerZ;

        LogUtil.log(TAG, "accelerometer:x=" + accelerometerX + ",y=" + accelerometerY + ",z="
                + accelerometerZ);

        sInstance.mGyroscopeX = gyroscopeX;
        sInstance.mGyroscopeY = gyroscopeY;
        sInstance.mGyroscopeZ = gyroscopeZ;

        LogUtil.log(TAG, "gyroscope:x=" + gyroscopeX + ",y=" + gyroscopeY + ",z=" + gyroscopeZ);

        sInstance.mButtonState = buttonState;
        sInstance.mTouchPad = touchPad;
        sInstance.mTimestamp = timestamp;

        LogUtil.log(TAG, "button:" + buttonState);
        LogUtil.log(TAG, "mTouchPad:" + touchPad);
        LogUtil.log(TAG, "timestamp:" + timestamp);

        sInstance.mAnalog2DPrimaryX = analog2DPrimaryX;
        sInstance.mAnalog2DPrimaryY = analog2DPrimaryY;
        sInstance.mAnalog2DSecondaryX = analog2DSecondaryX;
        sInstance.mAnalog2DSecondaryY = analog2DSecondaryY;

        sInstance.mConnectionState = connectionState;

        LogUtil.log(TAG, "analog2D-0 :x=" + sInstance.mAnalog2DPrimaryX + ",y="
                + sInstance.mAnalog2DPrimaryY);
        LogUtil.log(TAG, "analog2D-1 :x=" + sInstance.mAnalog2DSecondaryX + ",y="
                + sInstance.mAnalog2DSecondaryY);

        // 1D
        sInstance.mAnalog1DPrimaryIndex = analog1DPrimaryIndex;
        sInstance.mAnalog1DSecondaryIndex = analog1DSecondaryIndex;
        sInstance.mAnalog1DPrimaryHand = analog1DPrimaryHand;
        sInstance.mAnalog1DSecondaryHand = analog1DSecondaryHand;

        LogUtil.log(TAG, "connectionState:" + connectionState);

    }

}
