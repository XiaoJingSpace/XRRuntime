package com.qualcomm.sxrapi.xrcasting;

import android.app.Presentation;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class SxrPresentation extends Presentation implements SurfaceHolder.Callback{
    String TAG = "sxr";
    SxrPresentationSurfaceView mSurfaceView;

    public SxrPresentation(Context context, Display display) {
        super(context, display);
        Log.d(TAG, "SxrPresentation constructed");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Be sure to call the super class.
        super.onCreate(savedInstanceState);
        CharSequence text =  "SxrPresentation Name: " + getDisplay().getName() + " DisplayId: "+ getDisplay().getDisplayId();
        Log.d(TAG, "SxrPresentation onCreate: " + text);
        Log.d(TAG, "SxrPresentation new SxrPresentationGLSurfaceView(getContext()");
        LinearLayout outerlayout = new LinearLayout(getContext());
        outerlayout.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        outerlayout.setOrientation(LinearLayout.VERTICAL);
		
		// NOTE: Disabling TextView to reduce XRCast rendering load and compositing complexity
		//       Leaving code commented out for ease of reenablement for debugging.
        //TextView textView = new TextView(getContext());
        //textView.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        //textView.setGravity(android.view.Gravity.CENTER);
        //int height_dip = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 40, getResources().getDisplayMetrics());
        //textView.setHeight(height_dip);
        //textView.setBackgroundColor(Color.WHITE);
        //textView.setTextColor(0xFF3554DB);
        //textView.setText(text);
        //int textHeight_dip = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 4, getResources().getDisplayMetrics());
        //textView.setTextSize(textHeight_dip);
        //outerlayout.addView(textView);

        mSurfaceView = new SxrPresentationSurfaceView(getContext());
        mSurfaceView.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1));
        mSurfaceView.getHolder().addCallback(this);
        outerlayout.addView(mSurfaceView);
        setContentView(outerlayout);
    }

    @Override
    protected void onStart()
    {
        super.onStart();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
    }

    public static native void sxrSetPresentationSurfaceDestroyed();

    public static native void sxrSetPresentationSurfaceChanged(Surface surface, int format, int w, int h);

    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "SxrPresentationSurfaceView surfaceCreated");
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "SxrPresentationSurfaceView surfaceDestroyed");
        synchronized (this) {
            sxrSetPresentationSurfaceDestroyed();
        }
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.d(TAG, "SxrPresentationSurfaceView surfaceChanged(" +
                holder.getSurface() + ", " +
                format  + ", " +
                w  + ", " +
                h);

        synchronized (this) {
            sxrSetPresentationSurfaceChanged(holder.getSurface(), format, w, h);
        }
    }

    public class SxrPresentationSurfaceView extends SurfaceView
    {
        public SxrPresentationSurfaceView(Context context)
        {
            super(context);
            Log.d(TAG, "SxrPresentationSurfaceView constructed");
        }
    }
}