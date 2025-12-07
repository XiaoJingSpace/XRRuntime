package com.qti.acg.apps.controllers.finch;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import com.finchtechnologies.android.sdk.Finch;
import com.finchtechnologies.android.sdk.NodeListener;
import com.finchtechnologies.android.sdk.definition.BodyRotationMode;
import com.finchtechnologies.android.sdk.definition.Bone;
import com.finchtechnologies.android.sdk.definition.Chirality;
import com.finchtechnologies.android.sdk.definition.ControllerElement;
import com.finchtechnologies.android.sdk.definition.ControllerType;
import com.finchtechnologies.android.sdk.definition.EventType;
import com.finchtechnologies.android.sdk.definition.FinchUpdateType;
import com.finchtechnologies.android.sdk.definition.NodeType;
import com.finchtechnologies.android.sdk.definition.Platform;
import com.finchtechnologies.android.sdk.definition.Quaternion;
import com.finchtechnologies.android.sdk.definition.RecenterMode;
import com.finchtechnologies.android.sdk.definition.ScannerType;
import com.finchtechnologies.android.sdk.definition.UpdateError;
import com.finchtechnologies.android.sdk.definition.Vector2;
import com.finchtechnologies.android.sdk.definition.Vector3;

import static com.qti.acg.apps.controllers.finch.FinchMath.isNan;

public class FinchService extends Service {
    private final static String VERSION = "1.0.0";
    private final static String TAG = FinchService.class.getSimpleName();

    private final IBinder binder = new FinchBinder();
    private final Finch finch = Finch.getInstance();
    private SharedPreferences preferences;
    private FinchUpdateType mode = FinchUpdateType.Internal;

    private FinchUpdateType forcedMode = FinchUpdateType.Unknown; // Change to force set Mode.
    private boolean countervailHmdShift = true;
    private boolean fixStartPosition = true;

    private Vector3 phmd = new Vector3(0, 0, 0);
    private Quaternion qhmd = new Quaternion(0, 0, 0, 1);

    private boolean firstUpdate = false;
    private boolean twoHandedMode = true;

    private long oldCalibrationTime = 0;

    private Quaternion oldHmdRecenterQuaternionQvrCS = new Quaternion(0, 0, 0, 1);
    private FinchMath.Transform hmdRecenterTransformQvrCS = new FinchMath.Transform();
    private Vector3 hmdPositionShift = new Vector3(0, 0, 0);
    private float startHmdPositionLength = 0.1f;
    private Vector3 startHmdPosition = new Vector3(0, 0, -0.1f);

    private FinchMath.CoordinateSystem qvrCS = new FinchMath.CoordinateSystem(new Vector3(0, -1, 0), new Vector3(0, 0, 1), new Vector3(-1, 0, 0));
    private FinchMath.CoordinateSystem rawHmdCS = new FinchMath.CoordinateSystem(new Vector3(0, 0, 1), new Vector3(0, 1, 0), new Vector3(1, 0, 0));

    private static Quaternion getQHmdOffset(Quaternion qHmdQvrCS) {
        Vector3 fwd = new Vector3(0, 0, 1);
        Vector3 qFwd = FinchMath.apply(qHmdQvrCS, fwd);
        Vector3 horQFwd = FinchMath.normalized(new Vector3(qFwd.x, 0, qFwd.z));
        return FinchMath.fromToRotation(fwd, horQFwd);
    }

    private static Quaternion getLocalRotation(Quaternion qCS, Quaternion qGlobal) {
        return FinchMath.mult(FinchMath.conjugated(qCS), qGlobal);
    }

    private static FinchMath.Transform getLocalTransform(FinchMath.Transform tCS, FinchMath.Transform tGlobal) {
        Vector3 delta = FinchMath.plus(tGlobal.Position, FinchMath.minus(tCS.Position));
        Quaternion localRotation = getLocalRotation(tCS.Orientation, tGlobal.Orientation);
        Vector3 localDelta = FinchMath.apply(FinchMath.conjugated(tCS.Orientation), delta);

        FinchMath.Transform result = new FinchMath.Transform();
        result.Orientation = localRotation;
        result.Position = localDelta;
        return result;
    }

    private static Vector3 predictHmdShift(Quaternion hmdRecQBefore, Quaternion hmdRecQAfter) {
        Quaternion deltaRot = FinchMath.mult(FinchMath.conjugated(hmdRecQBefore), hmdRecQAfter);
        float predictedLen = predictLen(deltaRot);
        Log.d(TAG, "len = " + predictedLen);

        float predictedAng = (deltaRot.y * deltaRot.w < 0) ? getHorizontalAngle(hmdRecQAfter) : (float) Math.PI + getHorizontalAngle(hmdRecQAfter);

        float yobannayaDelta = Math.copySign(getAbsAngle(deltaRot), (deltaRot.y * deltaRot.w));
        predictedAng -= yobannayaDelta / 2;
        predictedAng = getAngleBetweenMinusPIAndPI(predictedAng);
        return getPredictedShift(predictedLen, predictedAng);
    }

    private static float getAbsAngle(Quaternion q) {
        float signedAngle = (float) Math.acos(q.w) * 2.0f;
        float alignedSignedAngle = getAngleBetweenMinusPIAndPI(signedAngle);
        float alignedUnsignedAngle = Math.abs(alignedSignedAngle);
        return alignedUnsignedAngle;
    }

    private static float getAngleBetweenMinusPIAndPI(float angle) {
        while (angle > Math.PI)
            angle -= (float) 2 * Math.PI;
        while (angle < -Math.PI)
            angle += (float) 2 * Math.PI;

        return angle;
    }

    private static float getHorizontalAngle(Quaternion q) {
        return (float) Math.atan2(q.y, q.w) * 2.0f;
    }

    private static float predictLen(Quaternion q) {
        float len = (float) (Math.sqrt(Math.asin(Math.abs(q.y)) + 0.271) - 0.523) / 3.84f;
        return len;
    }

    private static Vector3 getPredictedShift(float len, float angle) {
        return new Vector3(len * (float) Math.cos(angle), 0.0f, len * (float) Math.sin(angle));
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    public void init(ControllerType controllerType) {
        try {
            Log.i(TAG, "FinchService version: " + VERSION);
            preferences = getSharedPreferences("FinchSharedPreferences", Context.MODE_PRIVATE);
            finch.init(FinchService.this, controllerType, Platform.Internal);
            finch.setCs(new Vector3(0, -1, 0), new Vector3(0, 0, 1), new Vector3(-1, 0, 0));
            finch.setBodyRotationMode(BodyRotationMode.ShoulderRotation);
            firstUpdate = true;

            finch.addNodeListener(new NodeListener(NodeType.RightHand) {
                @Override
                public void onConnected(String address) {
                    Log.i(TAG, node + " connected: " + address);
                }

                @Override
                public void onDisconnected(String address) {
                    Log.i(TAG, node + " disconnected: " + address);
                }

                @Override
                public void onBatteryLevelChanged(String address, int charge) {
                    Log.i(TAG, node + " battery charge: " + charge);
                }
            });
            finch.addNodeListener(new NodeListener(NodeType.LeftHand) {
                @Override
                public void onConnected(String address) {
                    Log.i(TAG, node + " connected: " + address);
                }

                @Override
                public void onDisconnected(String address) {
                    Log.i(TAG, node + " disconnected: " + address);
                }

                @Override
                public void onBatteryLevelChanged(String address, int charge) {
                    Log.i(TAG, node + " battery charge: " + charge);
                }
            });
            finch.addNodeListener(new NodeListener(NodeType.RightUpperArm) {
                @Override
                public void onConnected(String address) {
                    Log.i(TAG, node + " connected: " + address);
                    bindUpperArm(node, address);
                }

                @Override
                public void onDisconnected(String address) {
                    Log.i(TAG, node + " disconnected: " + address);
                }

                @Override
                public void onBatteryLevelChanged(String address, int charge) {
                    Log.i(TAG, node + " battery charge: " + charge);
                }
            });
            finch.addNodeListener(new NodeListener(NodeType.LeftUpperArm) {
                @Override
                public void onConnected(String address) {
                    Log.i(TAG, node + " connected: " + address);
                    bindUpperArm(node, address);
                }

                @Override
                public void onDisconnected(String address) {
                    Log.i(TAG, node + " disconnected: " + address);
                }

                @Override
                public void onBatteryLevelChanged(String address, int charge) {
                    Log.i(TAG, node + " battery charge: " + charge);
                }
            });

            finch.startScan(ScannerType.Bonded, 1000, -100, true);
            Log.i(TAG, "init FinchService");
        } catch (Exception e) {
            Log.e(TAG, "fail init FinchService");
            Log.e(TAG, Log.getStackTraceString(e));
        }
    }

    public void recenter() {
        if (finch.bindsControllers()) {
            Log.i(TAG, "bindsControllers");
            update();
        }

        long currentTime = System.nanoTime();

        oldHmdRecenterQuaternionQvrCS = hmdRecenterTransformQvrCS.Orientation;
        hmdRecenterTransformQvrCS = getHmdTransformInQvrCS(qhmd, phmd);
        hmdRecenterTransformQvrCS.Orientation = getQHmdOffset(hmdRecenterTransformQvrCS.Orientation);
        startHmdPosition = FinchMath.apply(FinchMath.conjugated(hmdRecenterTransformQvrCS.Orientation), new Vector3(0, 0, -startHmdPositionLength));
        if (Math.abs(oldCalibrationTime - currentTime) > (long) 5000000) //5 * 10^-3 s
            hmdPositionShift = predictHmdShift(oldHmdRecenterQuaternionQvrCS, hmdRecenterTransformQvrCS.Orientation);
        Log.d(TAG, "shift = " + hmdPositionShift);
        oldCalibrationTime = currentTime;

        finch.calibration(Chirality.Both, RecenterMode.Forward);
        finch.recenter(Chirality.Both, RecenterMode.Forward);

        finch.hapticPulse(NodeType.RightHand, 500);
        finch.hapticPulse(NodeType.LeftHand, 500);
    }

    private FinchMath.Transform getHmdTransformInQvrCS(Quaternion qHmdOffset, Vector3 pHmdOffset) {
        FinchMath.Transform result = new FinchMath.Transform();
        result.Position = translateFromQvrRawHmdCSToQvrCS(pHmdOffset);
        result.Orientation = translateFromQvrRawHmdCSToQvrCS(qHmdOffset);
        return result;
    }

    private Quaternion translateFromQvrRawHmdCSToQvrCS(Quaternion qInQvrRawHmdCS) {
        Quaternion qInQvrCS = qvrCS.transform(rawHmdCS.inverseTransform(qInQvrRawHmdCS));
        qInQvrCS.w = -qInQvrCS.w;
        qInQvrCS.z = -qInQvrCS.z;
        return qInQvrCS;
    }

    private Vector3 translateFromQvrRawHmdCSToQvrCS(Vector3 pInQvrRawHmdCS) {
        return qvrCS.transform(rawHmdCS.inverseTransform(pInQvrRawHmdCS));
    }

    public boolean update(FinchUpdateType mode, Vector3 phmd, Quaternion qhmd) {
        if (FinchMath.equal(phmd.x, 0.0f) && FinchMath.equal(phmd.y, 0.0f) && FinchMath.equal(phmd.z, 0.0f)
                && FinchMath.equal(qhmd.x, 0.0f) && FinchMath.equal(qhmd.y, 0.0f) && FinchMath.equal(qhmd.z, 0.0f) && FinchMath.equal(qhmd.w, 0.0f)) {
            phmd = this.phmd;
            qhmd = this.qhmd;
        }

        mode = (forcedMode == FinchUpdateType.Unknown) ? mode : forcedMode;

        boolean modeWasChanged = false;

        if (this.mode != mode) {
            this.mode = mode;
            modeWasChanged = true;
            if (mode == FinchUpdateType.HmdTransform) {
                Log.i(TAG, "Finch in 6DOF mode");
            } else if (mode == FinchUpdateType.HmdRotation) {
                Log.i(TAG, "Finch in 3DOF mode");
            } else {
                Log.i(TAG, "Finch in no HMD data mode");
            }
        }

        boolean leftUAConnectedAndCorrectly = finch.isNodeConnected(NodeType.LeftUpperArm) && finch.isNodeDataCorrectly(NodeType.LeftUpperArm);
        boolean rightUAConnectedAndCorrectly = finch.isNodeConnected(NodeType.RightUpperArm) && finch.isNodeDataCorrectly(NodeType.RightUpperArm);
        boolean bothUAConnectedAndCorrectly = leftUAConnectedAndCorrectly && rightUAConnectedAndCorrectly;
        boolean switchArmMode = false;
        if (bothUAConnectedAndCorrectly != twoHandedMode) {
            twoHandedMode = bothUAConnectedAndCorrectly;
            switchArmMode = true;
            Log.d(TAG, "Arm mode switched to " + (twoHandedMode ? "two-handed" : "one-handed"));
        }
        Log.d(TAG, !rightUAConnectedAndCorrectly ? "Rua is not connected" : "Rua connected");
        Log.d(TAG, !leftUAConnectedAndCorrectly ? "Lua is not connected" : "Lua connected");


        if (firstUpdate || modeWasChanged) {
            firstUpdate = false;

            if (this.mode == FinchUpdateType.Internal) {
                finch.setRootOffset(new Vector3(0.0f, -0.25f, 0.0f));
            } else {
                finch.setRootOffset(new Vector3(0.0f, 0.0f, 0.0f));
            }
        }

        if (switchArmMode) {
            if (twoHandedMode)
                finch.setBodyRotationMode(BodyRotationMode.ShoulderRotation);
            else if (this.mode == FinchUpdateType.Internal)
                finch.setBodyRotationMode(BodyRotationMode.None);
            else
                finch.setBodyRotationMode(BodyRotationMode.HmdRotation);
        }

        this.phmd = phmd;
        this.qhmd = qhmd;
        /*
        if (phmd != null && qhmd != null) {
            Log.i(TAG, "hmd position: " + phmd + " rot: " + qhmd);
        } else if (qhmd != null) {
            Log.i(TAG, "hmd rot: " + qhmd);
        }*/

        if (modeWasChanged)
            return false;

        bindsUpperArms();
        UpdateError updateError = update();
        if (updateError != UpdateError.None) {
            Log.e(TAG, "updateError: " + updateError);
            return false;
        }

        if (finch.isNodeConnected(NodeType.RightHand) && !finch.isNodeDataCorrectly(NodeType.RightHand)) {
            Log.w(TAG, "RightHand data is not correctly");
        }
        if (finch.isNodeConnected(NodeType.LeftHand) && !finch.isNodeDataCorrectly(NodeType.LeftHand)) {
            Log.w(TAG, "LeftHand data is not correctly");
        }
        if (finch.isNodeConnected(NodeType.RightUpperArm) && !finch.isNodeDataCorrectly(NodeType.RightUpperArm)) {
            Log.w(TAG, "RightUpperArm data is not correctly");
        }
        if (finch.isNodeConnected(NodeType.LeftUpperArm) && !finch.isNodeDataCorrectly(NodeType.LeftUpperArm)) {
            Log.w(TAG, "LeftUpperArm data is not correctly");
        }

        return true;
    }

    public void getCurrentData(int index, long[] timestamp,
                               float[] pos, float[] rot, float[] posVar, float[] rotVar,
                               float[] vel, float[] angVel, float[] accel, float[] accelOffset, float[] gyroOffset, float[] gravityVector,
                               int[] buttons, float[] touchPad, int[] isTouching, float[] trigger) {
        NodeType node = NodeType.RightHand;
        Chirality chirality = Chirality.Right;
        Bone bone = Bone.RightHand;
        if (index == 1) {
            node = NodeType.LeftHand;
            chirality = Chirality.Left;
            bone = Bone.LeftHand;
        }

        boolean nanAlarm = false;
        String nanAlarmMessage = "";

        Quaternion q = finch.getControllerRotation(chirality);
        if (isNan(q)) {
            q = new Quaternion(0, 0, 0, 1);
            nanAlarm = true;
            nanAlarmMessage += chirality.toString() + " controller rotation is NAN!\n";
        }
        rot[0] = q.x;
        rot[1] = q.y;
        rot[2] = q.z;
        rot[3] = q.w;

        Vector3 v = finch.getControllerPosition(chirality, true);
        if (isNan(v)) {
            v = new Vector3(0, 0, 0);
            nanAlarm = true;
            nanAlarmMessage += chirality.toString() + " controller position is NAN!\n";
        }
        pos[0] = v.x;
        pos[1] = v.y;
        pos[2] = -v.z;

        Vector3 la = finch.getBoneLinearAcceleration(bone, false);
        if (isNan(la)) {
            la = new Vector3(0, 0, 0);
            nanAlarm = true;
            nanAlarmMessage += chirality.toString() + " controller local accel is NAN!\n";
        }
        accel[0] = la.x;
        accel[1] = la.y;
        accel[2] = la.z;

        Vector3 av = finch.getBoneAngularVelocity(bone, false);
        if (isNan(av)) {
            av = new Vector3(0, 0, 0);
            nanAlarm = true;
            nanAlarmMessage += chirality.toString() + " controller local gyro is NAN!\n";
        }
        angVel[0] = av.x;
        angVel[1] = av.y;
        angVel[2] = av.z;

        trigger[0] = finch.getIndexTrigger(chirality);

        if (finch.getEvent(node, ControllerElement.ButtonZero, EventType.Process)) {
            buttons[0] |= 0x100;
        }

        if (finch.getEvent(node, ControllerElement.ButtonOne, EventType.Process) ||
                finch.getEvent(node, ControllerElement.ButtonTwo, EventType.Process) ||
                finch.getEvent(node, ControllerElement.ButtonThree, EventType.Process)) {
            buttons[0] |= 0x200;
        }
        if (finch.getEvent(node, ControllerElement.ButtonGrip, EventType.Process)) {
            buttons[0] |= 0x1000;
        }
        if (finch.getEvent(node, ControllerElement.Touch, EventType.Process)) {
            isTouching[0] = 1;
        }
        if (finch.getEvent(node, ControllerElement.ButtonThumb, EventType.Process)) {
            buttons[0] |= 0x8000;
        }
        if (finch.getEvent(node, ControllerElement.IndexTrigger, EventType.Process)) {
            buttons[0] |= (1 << 13);
        }

        Vector2 touch = finch.getTouchAxes(chirality);
        if (Float.isNaN(touch.x) || Float.isNaN(touch.y)) {
            touch = new Vector2(0, 0);
            nanAlarm = true;
            nanAlarmMessage += chirality.toString() + " controller TOUCH AXIS BLEATH is NAN!\n";
        }
        touchPad[0] = touch.x;
        touchPad[1] = touch.y;

        if (nanAlarm) {
            Log.e(TAG, nanAlarmMessage);
        }
    }

    public int getBatteryLevel(NodeType node) {
        return finch.getNodeCharge(node);
    }

    public boolean hapticPulse(NodeType node, int ms) {
        return finch.hapticPulse(node, ms);
    }

    public boolean powerOff(NodeType node) {
        return finch.sendDataToNode(node, new byte[]{0, 6});
    }

    public void exit() {
        try {
            finch.exit();
            Log.i(TAG, "exit FinchService");
        } catch (Exception e) {
            Log.e(TAG, "fail exit FinchService");
            Log.e(TAG, Log.getStackTraceString(e));
        }
    }

    private UpdateError update() {
        switch (mode) {
            case HmdTransform: {
                FinchMath.Transform hmdTransformInQvrCS = getHmdTransformInQvrCS(qhmd, phmd);
                FinchMath.Transform localHmdTransformInQvrCS = getLocalTransform(hmdRecenterTransformQvrCS, hmdTransformInQvrCS);
                if (countervailHmdShift)
                    localHmdTransformInQvrCS.Position = FinchMath.plus(localHmdTransformInQvrCS.Position, hmdPositionShift);
                if (fixStartPosition)
                    localHmdTransformInQvrCS.Position = FinchMath.plus(localHmdTransformInQvrCS.Position,
                            FinchMath.plus(startHmdPosition, FinchMath.apply(localHmdTransformInQvrCS.Orientation, FinchMath.minus(startHmdPosition))));
                Log.i(TAG, "Calibrated hmd rot = " + localHmdTransformInQvrCS.Orientation.toString());
                if (Float.compare(FinchMath.magnitudeSqr(localHmdTransformInQvrCS.Orientation), 1.0f) != 0) {
                    localHmdTransformInQvrCS.Orientation = FinchMath.normalized(localHmdTransformInQvrCS.Orientation);
                }

                return finch.hmdTransformUpdate(localHmdTransformInQvrCS.Orientation, localHmdTransformInQvrCS.Position, System.nanoTime());
            }
            case HmdRotation: {
                Quaternion hmdOrientationInQvrCS = translateFromQvrRawHmdCSToQvrCS(qhmd);
                Quaternion localHmdOrientationInQvrCS = getLocalRotation(hmdRecenterTransformQvrCS.Orientation, hmdOrientationInQvrCS);
                Log.i(TAG, "Calibrated hmd rot = " + localHmdOrientationInQvrCS.toString());
                if (Float.compare(FinchMath.magnitudeSqr(localHmdOrientationInQvrCS), 1.0f) != 0) {
                    localHmdOrientationInQvrCS = FinchMath.normalized(localHmdOrientationInQvrCS);
                }

                return finch.hmdTransformUpdate(localHmdOrientationInQvrCS, new Vector3(0, 0, 0), System.nanoTime());
            }
            case Internal: {
                return finch.update(System.nanoTime());
            }
            default:
                return UpdateError.IllegalArgument;
        }
    }

    private boolean bindsUpperArms() {
        if (finch.getEvent(NodeType.LeftUpperArm, ControllerElement.ButtonZero, EventType.Begin)) {
            Log.i(TAG, "bindsUpperArms");
            finch.swapNodes(NodeType.RightUpperArm, NodeType.LeftUpperArm);
            SharedPreferences.Editor editor = preferences.edit();
            editor.putString(NodeType.RightUpperArm.toString(), finch.getNodeAddress(NodeType.RightUpperArm));
            editor.putString(NodeType.LeftUpperArm.toString(), finch.getNodeAddress(NodeType.LeftUpperArm));
            editor.apply();
            return true;
        }
        return false;
    }

    private void bindUpperArm(NodeType node, String address) {
        String n = preferences.getString(node.toString(), null);
        String c = preferences.getString(node.conjugate().toString(), null);
        if (c != null && c.equals(address)) {
            finch.swapNodes(NodeType.RightUpperArm, NodeType.LeftUpperArm);
        } else if (n == null || !n.equals(address)) {
            SharedPreferences.Editor editor = preferences.edit();
            editor.putString(node.toString(), address);
            editor.apply();
        }
    }


    public class FinchBinder extends Binder {
        public FinchService getService() {
            return FinchService.this;
        }
    }
}
