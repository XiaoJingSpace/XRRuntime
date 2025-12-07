//=============================================================================
// FILE: SxrNativeActivity.java
//
//                  Copyright (c) 2019 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
package com.qualcomm.sxr.multidisplayvr;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.AssetManager;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class SxrNativeActivity extends android.app.NativeActivity
{
    SxrNativeActivityReceiver mSxrNativeActivityReceiver;
    public native void nativeColorScalarSet(float colorScalar);

    private void LogClass(String str) {MultiDisplayVRHelper.Log(this.getLocalClassName() + ":" + str);}

	public static final String FIRST_TIME_TAG = "first_time";
	public static final  String ASSETS_SUB_FOLDER_NAME = "raw";
	public static final int BUFFER_SIZE = 1024;

    void sxrNativeActivityReceiverIntent(Intent intent)
    {
        float colorScalar = intent.getFloatExtra(getString(R.string.intentColorId), -1);
        LogClass("sxrNativeActivityReceiverIntent()=" + colorScalar);
        nativeColorScalarSet(colorScalar);
    }

    @Override
	protected void onPause() 
    {
        super.onPause();
        LogClass("onPause()");
    }

    @Override
	protected void onResume() 
    {
        super.onResume();
        LogClass("onResume()");
        
        View view = findViewById(android.R.id.content);
        Context context = view.getContext();
        Toast.makeText(context, "Resuming SxrNativeActivity on secondary display", Toast.LENGTH_SHORT).show();
    }

	@Override 
	public void onWindowFocusChanged (boolean hasFocus)
	{
        super.onWindowFocusChanged(hasFocus);
        LogClass("onWindowFocusChanged; hasFocus=" + hasFocus);

		if(hasFocus) 
		{
			getWindow().getDecorView().setSystemUiVisibility(
					View.SYSTEM_UI_FLAG_LAYOUT_STABLE
					| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
					| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
					| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
					| View.SYSTEM_UI_FLAG_FULLSCREEN
					| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
			getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) 
    {
		LogClass("onCreate()");
        super.onCreate(savedInstanceState);

        mSxrNativeActivityReceiver = new SxrNativeActivityReceiver(this);
	    LocalBroadcastManager.getInstance(this).registerReceiver(
            mSxrNativeActivityReceiver, 
            new IntentFilter(MultiDisplayVRHelper.ChangeColorIntentString(getApplicationContext().getPackageName())));

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		if (!prefs.getBoolean(FIRST_TIME_TAG, false)) 
        {
			SharedPreferences.Editor editor = prefs.edit();
			editor.putBoolean(FIRST_TIME_TAG, true);
			editor.apply();
			copyAssetsToExternal();
		}
	}

	@Override
	protected void onDestroy() 
    {
        LogClass("onDestroy()");
        super.onDestroy();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mSxrNativeActivityReceiver);
    }

	/*
	 * copy the Assets from assets/raw to app's external file dir
	 */
	public void copyAssetsToExternal() 
    {
		AssetManager assetManager = getAssets();
		String[] files = null;
		try 
        {
			InputStream in = null;
			OutputStream out = null;

			files = assetManager.list(ASSETS_SUB_FOLDER_NAME);
			for (int i = 0; i < files.length; i++) 
            {
				in = assetManager.open(ASSETS_SUB_FOLDER_NAME + "/" + files[i]);
				String outDir = getExternalFilesDir(null).toString() + "/";

				File outFile = new File(outDir, files[i]);
				out = new FileOutputStream(outFile);
				copyFile(in, out);
				in.close();
				in = null;
				out.flush();
				out.close();
				out = null;
			}
		} 
        catch (IOException e) 
        {
			Log.e("tag", "Failed to get asset file list.", e);
		}
		File file = getExternalFilesDir(null);
		Log.d("tag", "file:" + file.toString());
	}
    /*
     * read file from InputStream and write to OutputStream.
     */
	private void copyFile(InputStream in, OutputStream out) throws IOException 
    {
		byte[] buffer = new byte[BUFFER_SIZE];
		int read;
		while ((read = in.read(buffer)) != -1) 
        {
			out.write(buffer, 0, read);
		}
	}
}
