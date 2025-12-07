//-----------------------------------------------------------------------------
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

package com.qualcomm.snapdragonvrservice;

import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import com.qualcomm.snapdragonvrservice.ControllerState.ConnectionState;

/**
 * ControllerDebugFragment is the UI for user to select a controller , connect with this controller
 * and view the state data from the controller, including position, rotation, accelerometer,
 * gyroscope, buttons status, touch pad status , battery level and so on.
 */

public class ControllerDebugFragment extends Fragment implements LoadAppsAsyncTask.Listener {

    static {
        System.loadLibrary("controller-debug");
    }

    int mReadIntervalMilliseconds = 16;
    Button mConnectButton;
    Spinner mControllerSpinner;
    TextView mRotationValue;
    TextView mPositionValue;
    TextView mAccelerometerValue;
    TextView mGyroscopeValue;
    TextView mButtonValues;
    TextView mTouchPadValue;
    TextView mBatteryValue;

    boolean exitReading = false;
    Handler mHandler;

    String TAG = "ControllerDebugFragment";

    static final int MSG_CONNECTING = 1;
    static final int MSG_CONNECTED = 2;
    static final int MSG_DISCONNECTING = 3;
    static final int MSG_DISCONNECTED = 4;

    static final String EMPTY_STR = "";
    static final String STR_SPACE = " ";
    static final String STR_PARENTHESE_LEFT = "(";
    static final String STR_PARENTHESE_RIGHT = ")";
    static final String STR_COMMA = ",";

    GetControllerStateThread mThread;

    ControllerState mControllerState;
    ControllerConnection mControllerConnection;

    String [] mControllerButtons;

    HashMap<String, Intent> mControllerMap = new HashMap<>();

    Runnable mUpdateControllerDataRunnable = new Runnable() {
        @Override
        public void run() {
            mRotationValue.setText(getFormattedRotation(mControllerState));
            mPositionValue.setText(getFormattedPosition(mControllerState));
            mAccelerometerValue.setText(getFormattedAccelerometer(mControllerState));
            mGyroscopeValue.setText(getFormattedGyroscope(mControllerState));
            mButtonValues.setText(getFormattedButtonsState(mControllerState));
            mTouchPadValue.setText(getFormattedTouchPadState(mControllerState));
            mBatteryValue.setText(getFormattedBatteryLevel(mControllerState));
        }
    };

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        LogUtil.log(TAG, "onActivityCreated()..");
        new LoadAppsAsyncTask(this).execute(getActivity());
        mHandler = new Handler() {
            public void handleMessage(Message msg) {
                LogUtil.log(TAG, "msg:" + msg.what);
                switch (msg.what) {
                    case MSG_CONNECTING:
                        mConnectButton.setText(R.string.connection_btn_connecting);
                        break;
                    case MSG_CONNECTED:
                        mConnectButton.setText(R.string.connection_btn_disconnect);
                        mConnectButton.setEnabled(true);
                        break;
                    case MSG_DISCONNECTING:
                        mConnectButton.setText(R.string.connection_btn_disconnecting);
                        break;
                    case MSG_DISCONNECTED:
                        mConnectButton.setText(R.string.connection_btn_connect);
                        mConnectButton.setEnabled(true);
                        break;
                    default:
                }
            }
        };
        mControllerButtons = getResources().getStringArray(R.array.controller_btns);
        mThread = new GetControllerStateThread();
        mControllerConnection = ControllerConnection.getInstance();
        mControllerConnection.allocateMemory();
        mControllerSpinner = (Spinner) getActivity().findViewById(R.id.controller_spinner);
        mConnectButton = (Button) getActivity().findViewById(R.id.connect_btn);
        mConnectButton.setText(R.string.connection_btn_connect);
        mConnectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                LogUtil.log(TAG, "onClick");
                if (mControllerConnection
                        .getConnectionState() == ControllerState.ConnectionState.DISCONNECTED) {
                    String key = mControllerSpinner.getSelectedItem().toString();
                    LogUtil.log(TAG, "go to connect: " + key);
                    if (mControllerConnection.bindControllerService(
                            getActivity().getApplicationContext(), mControllerMap.get(key),
                            mHandler)) {
                        exitReading = false;
                        mThread.start();
                        mConnectButton.setEnabled(false);
                    } else {
                        Toast.makeText(getActivity(), "Fail to connect with " + key,
                                Toast.LENGTH_SHORT).show();
                    }
                } else if (mControllerConnection
                        .getConnectionState() == ConnectionState.CONNECTED) {
                    LogUtil.log(TAG, "go to disconnect");
                    disconnect();
                    mConnectButton.setEnabled(false);
                }
            }
        });

        mRotationValue = (TextView) getActivity().findViewById(R.id.rotation_value);
        mPositionValue = (TextView) getActivity().findViewById(R.id.position_value);
        mAccelerometerValue = (TextView) getActivity().findViewById(R.id.acc_value);
        mGyroscopeValue = (TextView) getActivity().findViewById(R.id.gyr_value);
        mButtonValues = (TextView) getActivity().findViewById(R.id.buttons_value);
        mTouchPadValue = (TextView) getActivity().findViewById(R.id.touchpad_value);
        mBatteryValue = (TextView) getActivity().findViewById(R.id.battery_value);
        mControllerState = ControllerState.getInstance();
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.controller_debug_fragment, container, false);
    }

    @Override
    public void onStart() {
        super.onStart();
        LogUtil.log(TAG, "onStart()..");
    }

    @Override
    public void onResume() {
        super.onResume();
        LogUtil.log(TAG, "onResume()..");
    }

    @Override
    public void onPause() {
        super.onPause();
        LogUtil.log(TAG, "onPause()..");
    }

    /*
     * We disconnect with controller when UI is not visible.
     */
    @Override
    public void onStop() {
        super.onStop();
        LogUtil.log(TAG, "onStop()..");
        if (mControllerConnection.getConnectionState() == ConnectionState.CONNECTED) {
            disconnect();
            mConnectButton.setEnabled(false);
        }
    }

    /*
     * Reset all controller state in UI to empty string.
     */
    void resetUIValues() {
        mBatteryValue.setText(EMPTY_STR);
        mRotationValue.setText(EMPTY_STR);
        mPositionValue.setText(EMPTY_STR);
        mAccelerometerValue.setText(EMPTY_STR);
        mGyroscopeValue.setText(EMPTY_STR);
        mTouchPadValue.setText(EMPTY_STR);
        mButtonValues.setText(EMPTY_STR);
    }

    void disconnect() {
        exitReading = true;
        mControllerConnection.stopTracking();
        mControllerConnection.unbindControllerService(getActivity().getApplicationContext());
    }

    @Override
    public void onDestroy() {
        LogUtil.log(TAG, "onDestroy()..");
        super.onDestroy();
        if (mControllerConnection.getConnectionState() == ConnectionState.CONNECTED) {
            disconnect();
        }
        mControllerConnection.freeMemory();
    }

    String formatFloat(float value) {
        return String.format("%.3f", value);
    }

    String getFormattedRotation(ControllerState controllerState) {
        return STR_PARENTHESE_LEFT + formatFloat(controllerState.mRotationX) + STR_COMMA
                + formatFloat(controllerState.mRotationY) + STR_COMMA
                + formatFloat(controllerState.mRotationZ) + STR_COMMA
                + formatFloat(controllerState.mRotationW) + STR_PARENTHESE_RIGHT;
    }

    String getFormattedPosition(ControllerState controllerState) {
        return STR_PARENTHESE_LEFT + formatFloat(controllerState.mPositionX) + STR_COMMA
                + formatFloat(controllerState.mPositionY) + STR_COMMA
                + formatFloat((controllerState.mPositionZ)) + STR_PARENTHESE_RIGHT;

    }

    String getFormattedAccelerometer(ControllerState controllerState) {
        return STR_PARENTHESE_LEFT + formatFloat(controllerState.mAccelerometerX) + STR_COMMA
                + formatFloat(controllerState.mAccelerometerY) + STR_COMMA
                + formatFloat(controllerState.mAccelerometerZ) + STR_PARENTHESE_RIGHT;
    }

    String getFormattedGyroscope(ControllerState controllerState) {
        return STR_PARENTHESE_LEFT + formatFloat(controllerState.mGyroscopeX) + STR_COMMA
                + formatFloat(controllerState.mGyroscopeY) + STR_COMMA
                + formatFloat(controllerState.mGyroscopeZ) + STR_PARENTHESE_RIGHT;

    }

    String getFormattedButtonsState(ControllerState controllerState) {
        String buttonValue = "";
        for (int i = 0; i < mControllerButtons.length; i++) {
            // LogUtil.log(TAG,i + "=>" + String.format("0x%08X",
            // ControllerState.SvrControllerButton.ONE << i));
            if ((controllerState.mButtonState
                    & (ControllerState.SvrControllerButton.ONE << i)) != 0) {
                buttonValue += STR_SPACE + mControllerButtons[i];
            }
        }
        return buttonValue;
    }

    String getFormattedTouchPadState(ControllerState controllerState) {
        String value = "";
        if ((controllerState.mTouchPad
                & ControllerState.SvrControllerTouch.PRIMARY_THUMBSTICK) != 0) {
            value = getString(R.string.controller_touchpad_touch) + STR_PARENTHESE_LEFT
                    + formatFloat(controllerState.mAnalog2DPrimaryX) + STR_COMMA
                    + formatFloat(controllerState.mAnalog2DPrimaryY) + STR_PARENTHESE_RIGHT;
        }
        if ((controllerState.mButtonState
                & ControllerState.SvrControllerButton.PRIMARY_THUMBSTICK) != 0) {
            value += STR_SPACE
                    + getString(R.string.controller_touchpad_clicked) + STR_SPACE;
            if ((controllerState.mButtonState
                    & ControllerState.SvrControllerButton.PRIMARY_THUMBSTICK_UP) != 0) {
                value += getString(R.string.controller_touchpad_up);
            } else if ((controllerState.mButtonState
                    & ControllerState.SvrControllerButton.PRIMARY_THUMBSTICK_DOWN) != 0) {
                value += getString(R.string.controller_touchpad_down);
            } else if ((controllerState.mButtonState
                    & ControllerState.SvrControllerButton.PRIMARY_THUMBSTICK_LEFT) != 0) {
                value += getString(R.string.controller_touchpad_left);
            } else if ((controllerState.mButtonState
                    & ControllerState.SvrControllerButton.PRIMARY_THUMBSTICK_RIGHT) != 0) {
                value += getString(R.string.controller_touchpad_right);
            }
        }
        return value;

    }

    String getFormattedBatteryLevel(ControllerState controllerState) {
        if (controllerState.mBatteryLevel > 0) {
            return controllerState.mBatteryLevel + "%";
        } else {
            return EMPTY_STR;
        }
    }

    public void onComplete(List<ServiceInfo> listOfApps) {
        if ((listOfApps.size() > 0)) {
            for (int i = 0; i < listOfApps.size(); i++) {
                mControllerMap.put(
                        (listOfApps.get(i).loadLabel(getActivity().getPackageManager()).toString()),
                        new Intent().setPackage(listOfApps.get(i).packageName).setClassName(
                                listOfApps.get(i).packageName, listOfApps.get(i).name));
            }
        } else {
            Toast.makeText(getActivity(), "Can't find any controller", Toast.LENGTH_SHORT).show();
            mConnectButton.setEnabled(false);
        }
        List<String> list = new ArrayList<String>();
        for (String key : mControllerMap.keySet()) {
            list.add(key);
            LogUtil.log(TAG, "key:" + key);
            LogUtil.log(TAG, "value:" + mControllerMap.get(key));
        }
        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(getActivity(),
                android.R.layout.simple_spinner_item, list);
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mControllerSpinner.setAdapter(dataAdapter);
    }

    public void onUpdate(Bundle appInfo) {

    }

    class GetControllerStateThread extends Thread {
        public void run() {
            while (!exitReading) {
                mControllerConnection.updateControllerState();
                mHandler.post(mUpdateControllerDataRunnable);
                try {
                    Thread.sleep(mReadIntervalMilliseconds);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    resetUIValues();
                }
            });
        }
    }
}
