package com.qualcomm.sxrapi.xrcasting;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.hardware.display.DisplayManager;
import android.util.Log;
import android.view.Display;
import android.view.View;

public class SxrPresentationManager {
    private String TAG = "sxr";
    private SxrPresentation mPresentation = null;
    private boolean mIsInitialized = false;
    private int mActivityDisplayId = -1;
    private Display mPresentationDisplay = null;
    private Activity mContext;
    private DisplayManager mDisplayManager;

    private final DisplayManager.DisplayListener mDisplayListener =
            new DisplayManager.DisplayListener() {
                @Override
                public void onDisplayAdded(int displayId) {
                    Log.d(TAG, "Display #" + displayId + " added.");
                    evaluateDisplaysForPresentation();
                }

                @Override
                public void onDisplayChanged(int displayId) {
                    Log.d(TAG, "Display #" + displayId + " changed.");
                    evaluateDisplaysForPresentation();
                }

                @Override
                public void onDisplayRemoved(int displayId) {
                    Log.d(TAG, "Display #" + displayId + " removed.");
                    if (displayId == mPresentationDisplay.getDisplayId())
                    {
                        evaluateDisplaysForPresentation();
                    }
                }
            };

    private final DialogInterface.OnDismissListener mOnDismissListener =
            new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialog) {
                    if (dialog == mPresentation) {
                        mPresentation.mSurfaceView.setVisibility(View.INVISIBLE);
                        mPresentation = null;
                        mPresentationDisplay = null;
                        Log.i(TAG, "Presentation was dismissed.");
                    }
                }
            };

    public SxrPresentationManager(Activity context, int display_id) {
        Log.d(TAG, "SxrPresentationManager Constructor context=" + context + " display_id=" + display_id);
        mContext = context;
        mActivityDisplayId = display_id;
        mDisplayManager = (DisplayManager)(context.getSystemService(Context.DISPLAY_SERVICE));
        initialize();
    }

    private void initialize() {
        Log.d(TAG, "Initializing SxrPresentationManager");
        if (!mIsInitialized) {
            mDisplayManager.registerDisplayListener(mDisplayListener, null);
            mIsInitialized = true;
            evaluateDisplaysForPresentation();
        }
    }


    public void deinitialize() {
        Log.d(TAG, "Deinitializing SxrPresentationManager");
        if (mIsInitialized) {
            mDisplayManager.unregisterDisplayListener(mDisplayListener);
            if (mPresentation != null)
            {
                mPresentation.dismiss();
            }
            mIsInitialized = false;
            Log.d(TAG, "Deinitialized successfully");
        }
    }

    private void evaluateDisplaysForPresentation() {
        Display[] displays = mDisplayManager.getDisplays();
        int newPresentationDisplay = 0;
        if (displays.length > 0) {
            Display[] suitableDisplays = new Display[displays.length];
            int suitableDisplayCount = 0;
            Display[] suitablePresentationDisplays = new Display[displays.length];
            int suitablePresentationDisplayCount = 0;

            for (int i = 0; i < displays.length; ++i) {
                if (displays[i].getDisplayId() != mActivityDisplayId &&
                        !displays[i].getName().equals("scrcpy")) {
                    if ((displays[i].getFlags() & Display.FLAG_PRESENTATION) == Display.FLAG_PRESENTATION) {
                        Log.d(TAG, "Suitable FLAG_PRESENTATION display found for presentation #" + displays[i].getDisplayId());
                        suitablePresentationDisplays[suitablePresentationDisplayCount++] = displays[i];
                    } else {
                        Log.d(TAG, "Suitable non-FLAG_PRESENTATION display found for presentation #" + displays[i].getDisplayId());
                        suitableDisplays[suitableDisplayCount++] = displays[i];
                    }
                }
            }

            if (suitablePresentationDisplayCount != 0) {
                mPresentationDisplay = suitablePresentationDisplays[suitablePresentationDisplayCount - 1];
            } else if (suitableDisplayCount != 0) {
                mPresentationDisplay = suitableDisplays[suitableDisplayCount - 1];
            } else {
                Log.d(TAG, "No suitable display found for presentation");
                if (mPresentation != null)
                {
                    mPresentation.dismiss();
                }
                return;
            }

            if (mPresentation != null)
            {
                if (mPresentationDisplay.getDisplayId() == mPresentation.getDisplay().getDisplayId())
                {
                    Log.d(TAG, "Presentation exists on suitable display");
                    return;
                }
                else
                {
                    Log.d(TAG, "Presentation exists but a more suitable display has been found.");
                    mPresentation.dismiss();
                }
            }

            Log.d(TAG, "Constructing Presentation");
            mPresentation = new SxrPresentation(mContext, mPresentationDisplay);
            mPresentation.setOnDismissListener(mOnDismissListener);
            Log.d(TAG, "Showing Presentation");
            mPresentation.show();
            mPresentation.mSurfaceView.setVisibility(View.VISIBLE);
            Log.d(TAG, "Initialized successfully");
        }
    }
}
