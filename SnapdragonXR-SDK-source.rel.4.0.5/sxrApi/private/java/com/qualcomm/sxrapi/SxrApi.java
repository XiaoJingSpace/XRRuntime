//=============================================================================
// FILE: SxrApi.java
//                  Copyright (c) 2016-2017 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================

package com.qualcomm.sxrapi;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Point;
import android.content.ActivityNotFoundException;
import android.view.View;
import android.view.Display;
import android.view.WindowManager;
import android.view.Choreographer;
import android.view.Surface;
import android.util.Log;
import android.opengl.Matrix;
// BEGIN SxrPresentation region
import com.qualcomm.sxrapi.xrcasting.SxrPresentationManager;
// END SxrPresentation region

public class SxrApi implements android.view.Choreographer.FrameCallback
{
    public static final String TAG = "SxrApi";
    public static SxrApi handler = new SxrApi();

    static
	{
        System.loadLibrary("sxrapi");
    }

    public static native void nativeVsync(long lastVsyncNano);

    public static native void sxrInitialize(Activity javaActivity);

    public static native void sxrBeginXr(Activity javaActivity, sxrBeginParams mBeginParams);

    public static native void sxrSubmitFrame(Activity javaActivity, sxrFrameParams mSvrFrameParams);

    public static native void sxrEndXr();

    public static native void sxrShutdown();

    public static native String sxrGetVersion();

    public static native String sxrGetXrServiceVersion();

    public static native String sxrGetXrClientVersion();

    public static native float sxrGetPredictedDisplayTime();

    public static native float sxrGetPredictedDisplayTimePipelined(int depth);

    public static native sxrHeadPoseState sxrGetPredictedHeadPose(float predictedTimeMs);

    public static native void sxrSetPerformanceLevels(sxrPerfLevel cpuPerfLevel, sxrPerfLevel gpuPerfLevel);

    public static native sxrDeviceInfo sxrGetDeviceInfo();

    public static native void sxrRecenterPose();

    public static native void sxrRecenterPosition();

    public static native void sxrRecenterOrientation(boolean yawOnly);

    public static native int sxrGetSupportedTrackingModes();

    public static native void sxrSetTrackingMode(int trackingModes);

    public static native int sxrGetTrackingMode();

    public static native void sxrBeginEye(sxrWhichEye whichEye);

    public static native void sxrEndEye(sxrWhichEye whichEye);

    public static native void sxrSetWarpMesh(sxrWarpMeshEnum whichMesh, float[] pVertexData, int vertexSize, int nVertices, int[] pIndices, int nIndices);

    public static native void sxrGetOcclusionMesh(sxrWhichEye whichEye, int[] pTriangleCount, int[] pVertexStride, float[] pTriangles);

    public static Choreographer choreographerInstance;

    public static void NotifyNoVr( final Activity act )
    {
        act.runOnUiThread(new Runnable() {
                public void run() {

                    try
                    {
                    AlertDialog.Builder alertBuilder = new AlertDialog.Builder(act);
                    alertBuilder.setMessage("SnapdragonXR not supported on this device!");
                    alertBuilder.setCancelable(true);

                    alertBuilder.setPositiveButton(
                        "Close",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                dialog.cancel();
                            }
                        });

                    AlertDialog alertDlg = alertBuilder.create();
                    alertDlg.show();
                    }
                    catch (Exception e)
                    {
                        Log.e(TAG, "Exception displaying dialog box!");
                        Log.e(TAG, e.getMessage());
                    }
            }
        });
    }

    public static void NotifyHeadless( final Activity act )
    {
        act.runOnUiThread(new Runnable() {
                public void run() {

                    try
                    {
                    AlertDialog.Builder alertBuilder = new AlertDialog.Builder(act);
                    alertBuilder.setMessage("Headset disconnected, enter Headless mode!");
                    alertBuilder.setCancelable(true);

                    alertBuilder.setPositiveButton(
                        "Close",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                dialog.cancel();
                            }
                        });

                    AlertDialog alertDlg = alertBuilder.create();
                    alertDlg.show();
                    }
                    catch (Exception e)
                    {
                        Log.e(TAG, "Exception displaying dialog box!");
                        Log.e(TAG, e.getMessage());
                    }
            }
        });
    }

    public static long getVsyncOffsetNanos( Activity act)
    {
         final Display disp = act.getWindowManager().getDefaultDisplay();
		 return disp.getAppVsyncOffsetNanos();
    }

	public static float getRefreshRate( Activity act )
	{
		final Display disp = act.getWindowManager().getDefaultDisplay();
		return disp.getRefreshRate();
	}

	public static int getDisplayWidth( Activity act )
	{
		final Display disp = act.getWindowManager().getDefaultDisplay();
		Point outSize = new Point();
		disp.getRealSize(outSize);
		return outSize.x;
	}

	public static int getDisplayHeight( Activity act )
	{
		final Display disp = act.getWindowManager().getDefaultDisplay();
		Point outSize = new Point();
		disp.getRealSize(outSize);
		return outSize.y;
	}

	public static int getDisplayOrientation( Activity act )
	{
		final Display disp = act.getWindowManager().getDefaultDisplay();
		int rot = disp.getRotation();
		if(rot == Surface.ROTATION_0)
		{
			return 0;
		}
		else if(rot == Surface.ROTATION_90)
		{
			return 90;
		}
		else if(rot == Surface.ROTATION_180)
		{
			return 180;
		}
		else if(rot == Surface.ROTATION_270)
		{
			return 270;
		}
		else
		{
			return -1;
		}
	}

	public static int getAndroidOsVersion( )
	{
		return android.os.Build.VERSION.SDK_INT;
	}

    public static void startVsync( Activity act )
    {
    	act.runOnUiThread( new Thread()
    	{
		 @Override
    		public void run()
    		{
				choreographerInstance = Choreographer.getInstance();
				choreographerInstance.removeFrameCallback(handler);
				choreographerInstance.postFrameCallback(handler);
    		}
    	});
	}

    public static void stopVsync( Activity act )
    {
    	act.runOnUiThread( new Thread()
    	{
		 @Override
    		public void run()
    		{
				if (choreographerInstance != null) {
					choreographerInstance.removeFrameCallback(handler);
				}
    		}
    	});
	}

    public void doFrame(long frameTimeNanos)
    {
		nativeVsync(frameTimeNanos);
		choreographerInstance.postFrameCallback(this);
	}

    private static final int SXR_MAX_RENDER_LAYERS = 16;
    private static final int SXR_MAJOR_VERSION = 2;
    private static final int SXR_MINOR_VERSION = 1;
    private static final int SXR_REVISION_VERSION = 1;
    public static SxrResult sxrResult;
    public static sxrWarpMeshType warpMeshType;

    public enum SxrResult
    {
        SXR_ERROR_NONE(0),
        SXR_ERROR_UNKNOWN(1),
        SXR_ERROR_UNSUPPORTED(2),
        SXR_ERROR_VRMODE_NOT_INITIALIZED(3),
        SXR_ERROR_VRMODE_NOT_STARTED(4),
        SXR_ERROR_VRMODE_NOT_STOPPED(5),
        SXR_ERROR_QVR_SERVICE_UNAVAILABLE(6),
        SXR_ERROR_JAVA_ERROR(7);

        private int vResult;
        SxrResult(int value) { this.vResult = value; }

    }
   //enum permits only private constructors
    public void setSvrResult(int enumIndex)
    {
        switch (enumIndex)
        {
            case 0:
                sxrResult = SxrResult.SXR_ERROR_NONE;
                break;
            case 1:
                sxrResult = SxrResult.SXR_ERROR_UNKNOWN;
                break;
            case 2:
                sxrResult = SxrResult.SXR_ERROR_UNSUPPORTED;
                break;
            case 3:
                sxrResult = SxrResult.SXR_ERROR_VRMODE_NOT_INITIALIZED;
                break;
            case 4:
                sxrResult = SxrResult.SXR_ERROR_VRMODE_NOT_STARTED;
                break;
            case 5:
                sxrResult = SxrResult.SXR_ERROR_VRMODE_NOT_STOPPED;
                break;
            case 6:
                sxrResult = SxrResult.SXR_ERROR_QVR_SERVICE_UNAVAILABLE;
                break;
            case 7:
                sxrResult = SxrResult.SXR_ERROR_JAVA_ERROR;
                break;
        }
    }

    public void setSvrWarpMeshType(int enumIndex)
    {
        switch (enumIndex)
        {
            case 0:
                warpMeshType = sxrWarpMeshType.kMeshTypeColumsLtoR;
                break;
            case 1:
                warpMeshType = sxrWarpMeshType.kMeshTypeColumsRtoL;
                break;
            case 2:
                warpMeshType = sxrWarpMeshType.kMeshTypeRowsTtoB;
                break;
            case 3:
                warpMeshType = sxrWarpMeshType.kMeshTypeRowsBtoT;
                break;
        }
    }

    public enum sxrEventType
    {
        kEventNone(0),
        kEventSdkServiceStarting(1),
        kEventSdkServiceStarted(2),
        kEventSdkServiceStopped(3),
        kEventControllerConnecting(4),
        kEventControllerConnected(5),
        kEventControllerDisconnected(6),
        kEventThermal(7),
        kEventSensorError(8);

        private int vSvrEventType;
        sxrEventType(int value) { this.vSvrEventType = value; }

        @Override
        public String toString()
        {
            return String.valueOf(this.vSvrEventType);
        }
    }

    public class sxrVector3
    {
        public float x, y, z;

        public sxrVector3() { x = y = z = 0.f; }

        public sxrVector3(float x, float y, float z)
        {
            this.x = x; this.y = y; this.z = z;
        }
    }

    public class sxrVector4
    {
        public float x, y, z, w;

        public sxrVector4()
        {
            x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f;
        }

        public sxrVector4(float x, float y, float z, float w)
        {
            this.x = x; this.y = y; this.z = z; this.w = w;
        }

        public void set(float x, float y, float z, float w)
        {
            this.x = x; this.y = y; this.z = z; this.w = w;
        }

    }
    public class sxrQuaternion
    {
        public float x, y, z, w;
        public float[] element = null;

        public sxrQuaternion() { setIdentity(); }

        public void setIdentity()
        {
            x = y = z = 0;
            w = 1;
        }
        public sxrMatrix4 quatToMatrix()
        {

            if (null == element) { element = new float[16]; }

            final float xx = x * x;
            final float xy = x * y;
            final float xz = x * z;
            final float xw = x * w;
            final float yy = y * y;
            final float yz = y * z;
            final float yw = y * w;
            final float zz = z * z;
            final float zw = z * w;

            element[0] = 1.0f - 2 * (yy + zz);
            element[1] = 2 * (xy + zw);
            element[2] = 2 * (xz - yw);
            element[3] = 0.0f;

            element[4] = 2 * (xy - zw);
            element[5] = 1 - 2 * (xx + zz);
            element[6] = 2 * (yz + xw);
            element[7] = 0.0f;

            element[8] = 2 * (xz + yw);
            element[9] = 2 * (yz - xw);
            element[10] = 1 - 2 * (xx + yy);
            element[11] = 0.0f;

            element[12] = 0.0f;
            element[13] = 0.0f;
            element[14] = 0.0f;
            element[15] = 1.0f;

            return new sxrMatrix4(element);
        }

        public sxrQuaternion set(float x, float y, float z, float w)
        {
            this.x = x; this.y = y; this.z = z; this.w = w;
            return this;
        }

        public boolean equals(final Object targetObj)
        {
            if (this == targetObj) return true;
            if (!(targetObj instanceof sxrQuaternion)) return false;
            final sxrQuaternion temp = (sxrQuaternion) targetObj;
            return this.x == temp.x && this.y == temp.y && this.z == temp.z && this.w == temp.w;
        }
    }
    public class sxrMatrix4
    {
        float mtx[];
        public sxrMatrix4()
        {
            float[] mtx =
            {
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f
            };
        }
        public sxrMatrix4(float[] m) { mtx = m; }
        public float[] queueInArray() { return mtx; }
    }
    public enum sxrWhichEye
    {
        kLeftEye(0),
        kRightEye(1),
        kNumEyes(2);
        private int vWhichEye;

        sxrWhichEye(int value) { this.vWhichEye = value; }
    }

    public enum sxrEyeMask
    {
        kEyeMaskLeft(1),
        kEyeMaskRight(2),
        kEyeMaskBoth(3);
        private int vEyeMask;

        sxrEyeMask(int value) { this.vEyeMask = value; }
        public int getEyeMask(){ return vEyeMask; }
    }

    public enum sxrLayerFlags
    {
        kLayerFlagNone(0),
        kLayerFlagHeadLocked(1),
        kLayerFlagOpaque(2);
        private int vLayerFlags;

        sxrLayerFlags(int value){ this.vLayerFlags = value; }
    }

    public enum sxrColorSpace
    {
        kColorSpaceLinear(0),
        kColorSpaceSRGB(1),
        kNumColorSpaces(2);
        private int vColorSpace;

        sxrColorSpace(int value){ this.vColorSpace = value; }
    }
    public class sxrHeadPose
    {
        public sxrQuaternion rotation;
        public sxrVector3 position;

        public sxrHeadPose()
        {
            rotation = new sxrQuaternion();
            position = new sxrVector3();
        }
    }

    public enum sxrTrackingMode
    {
        kTrackingRotation(1),
        kTrackingPosition(2);

        private int vTrackingMode;
        sxrTrackingMode(int value) { this.vTrackingMode = value; }
        public int getTrackingMode(){ return vTrackingMode; }
    }
    public class sxrHeadPoseState
    {
        public sxrHeadPose pose;
        public int poseStatus;
        public long poseTimeStampNs;
        public long poseFetchTimeNs;
        public long expectedDisplayTimeNs;

        public sxrHeadPoseState()
        {
            pose = new sxrHeadPose();
            poseStatus = 0;
            poseTimeStampNs = 0;
            poseFetchTimeNs = 0;
            expectedDisplayTimeNs = 0;
        }
    }

    public enum sxrPerfLevel
    {
        kPerfSystem(0),
        kPerfMinimum(1),
        kPerfMedium(2),
        kPerfMaximum(3),
        kNumPerfLevels(4);

        private int vPerfLevel;
        sxrPerfLevel(int value) { this.vPerfLevel = value; }
    }

    public class sxrBeginParams
    {
        public int mainThreadId;
        public sxrPerfLevel cpuPerfLevel;
        public sxrPerfLevel gpuPerfLevel;
        public Surface surface;
        public int optionFlags;
        public sxrColorSpace   colorSpace;
        public sxrBeginParams(Surface mSurface)
		{
            mainThreadId = 0;
            cpuPerfLevel = sxrPerfLevel.kPerfMaximum;
            gpuPerfLevel = sxrPerfLevel.kPerfMaximum;
            surface = mSurface;
            optionFlags = 0;
            colorSpace = sxrColorSpace.kColorSpaceLinear;
        }
    }

    public enum sxrFrameOption
    {
        kDisableDistortionCorrection(1),
        kDisableReprojection(2),
        kEnableMotionToPhoton(4),
        kDisableChromaticCorrection(8);

        private int vFrameOption;
        sxrFrameOption(int value) { this.vFrameOption = value; }
        public int getFrameOption(){ return vFrameOption; }
    }

    public enum sxrTextureType
    {
        kTypeTexture(0),
        kTypeTextureArray(1),
        kTypeImage(2),
        kTypeEquiRectTexture(3),
        kTypeEquiRectImage(4),
        kTypeVulkan(5);
        private int vTextureType;
        sxrTextureType(int value) { this.vTextureType = value; }
    }

    public class sxrVulkanTexInfo
    {
        public int memSize;
        public int width;
        public int height;
        public int numMips;
        public int bytesPerPixel;
        public int renderSemaphore;

        public sxrVulkanTexInfo()
        {
            memSize = 0;
            width = 0;
            height = 0;
            numMips = 0;
            bytesPerPixel = 0;
            renderSemaphore = 0;
        }
    }
    public enum sxrWarpType
    {
        kSimple(0);
        private int vWarpType;

        sxrWarpType(int value) { this.vWarpType = value; }
    }
    public enum sxrWarpMeshType
    {
        kMeshTypeColumsLtoR(0),
        kMeshTypeColumsRtoL(1),
        kMeshTypeRowsTtoB(2),
        kMeshTypeRowsBtoT(3);
        private int vWarpMeshType;
        sxrWarpMeshType(int value) { this.vWarpMeshType = value; }
    }

    public enum sxrWarpMeshEnum
    {
        kMeshEnumLeft(0),
        kMeshEnumRight(1),
        kMeshEnumUL(2),
        kMeshEnumUR(3),
        kMeshEnumLL(4),
        kMeshEnumLR(5),
        kWarpMeshCount(6);
        private int vWarpMeshEnum;
        sxrWarpMeshEnum(int value) { this.vWarpMeshEnum = value; }
    }

    public class sxrLayoutCoords
    {
        public float[] LowerLeftPos = new float[4];
        public float[] LowerRightPos = new float[4];
        public float[] UpperLeftPos = new float[4];
        public float[] UpperRightPos = new float[4];

        public float[] LowerUVs = new float[4];
        public float[] UpperUVs = new float[4];
        public float[] TransformMatrix = new float[16];

        public sxrLayoutCoords()
        {
            float[] LowerLeftPos = {0.0f, 0.0f, 0.0f, 0.0f};
            float[] LowerRightPos = {0.0f, 0.0f, 0.0f, 0.0f};
            float[] UpperLeftPos = {0.0f, 0.0f, 0.0f, 0.0f};
            float[] UpperRightPos = {0.0f, 0.0f, 0.0f, 0.0f};
            float[] LowerUVs = {0.0f, 0.0f, 0.0f, 0.0f};
            float[] UpperUVs = {0.0f, 0.0f, 0.0f, 0.0f};
            float[] TransformMatrix =
                    {       0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 0.0f
                    };
        }
    }

    public class sxrRenderLayer
    {
        public int imageHandle;
        public sxrTextureType imageType;
        public sxrLayoutCoords imageCoords;
        public sxrEyeMask eyeMask;
        public int layerFlags;
        public sxrVulkanTexInfo vulkanInfo;

        public sxrRenderLayer()
        {
            imageHandle = 0;
            imageType = sxrTextureType.kTypeTexture;
            imageCoords = new sxrLayoutCoords();
            eyeMask = sxrEyeMask.kEyeMaskLeft;
            layerFlags = 0;
            vulkanInfo = new sxrVulkanTexInfo();
        }
    }

    public class sxrFrameParams
    {
        public int frameIndex;
        public int minVsyncs;
        public sxrRenderLayer[] renderLayers = new sxrRenderLayer[SXR_MAX_RENDER_LAYERS];
        public int frameOptions;
        public sxrHeadPoseState headPoseState;
        public sxrWarpType warpType;
        public float fieldOfView;

        public sxrFrameParams()
        {
            frameIndex = 0;
            minVsyncs = 0;
            for (int i = 0; i < SXR_MAX_RENDER_LAYERS; i++)
            {
                renderLayers[i] = new sxrRenderLayer();
            }
            frameOptions = 0;
            headPoseState = new sxrHeadPoseState();
            warpType = sxrWarpType.kSimple;
            fieldOfView = 0.0f;
        }
    }

    public class sxrViewFrustum
    {
        public float left;
        public float right;
        public float top;
        public float bottom;

        public float near;
        public float far;

        public sxrViewFrustum()
        {
            left = 0.0f;
            right = 0.0f;
            top = 0.0f;
            bottom = 0.0f;
            near = 0.0f;
            far = 0.0f;
        }
    }

    public class sxrDeviceInfo
    {
        public int displayWidthPixels;
        public int displayHeightPixels;
        public float displayRefreshRateHz;
        public int displayOrientation;
        public int targetEyeWidthPixels;
        public int targetEyeHeightPixels;
        public float targetFovXRad;
        public float targetFovYRad;
        public sxrViewFrustum leftEyeFrustum;
        public sxrViewFrustum rightEyeFrustum;
        public float   targetEyeConvergence;
        public float   targetEyePitch;
        public int deviceOSVersion;
        public sxrWarpMeshType warpMeshType;
    }

    public class sxrEventData_Thermal
    {
        long thermalLevel;
    }

    public class sxrEvent
    {
        sxrEventType eventType;
        long deviceId;
        float eventTimeStamp;
        sxrEventData_Thermal eventData;
    }

// BEGIN SxrPresentation region
    private static SxrPresentationManager sSxrPresentationManager = null;

    public static void enablePresentation(final Activity act, final boolean isEnabled) {
        Log.d(TAG, "enablePresentation");
        if (isEnabled) {
            act.runOnUiThread(new Runnable() {
                public void run() {
                    int displayID = act.getWindowManager().getDefaultDisplay().getDisplayId();
                    Log.d(TAG, "enablePresentation run()");
                    try {
                        sSxrPresentationManager = new SxrPresentationManager(act, displayID);
                    } catch (Exception e) {
                        Log.e(TAG, "Exception thrown in enablePresentation");
                        Log.e(TAG, e.getMessage());
                    }
                }
            });
        } else {
            if (sSxrPresentationManager != null) {
                act.runOnUiThread(new Runnable() {
                    public void run() {
                        sSxrPresentationManager.deinitialize();
                        sSxrPresentationManager = null;
                    }
                });
            }
        }
    }
// END SxrPresentation region
}
