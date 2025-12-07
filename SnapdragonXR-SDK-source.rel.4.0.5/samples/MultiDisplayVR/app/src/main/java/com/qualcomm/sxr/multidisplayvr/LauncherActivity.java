//=============================================================================
// FILE: LauncherActivity.java
//
//                  Copyright (c) 2019 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
package com.qualcomm.sxr.multidisplayvr;

import android.app.ActivityOptions;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.view.Display;
import android.view.WindowManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;
import com.qualcomm.sxr.multidisplayvr.MultiDisplayVRHelper;

public class LauncherActivity extends android.app.Activity
{
    static boolean s_launchedToSecondaryDisplay = false;
    private void LogClass(String str) {MultiDisplayVRHelper.Log(this.getLocalClassName() + ":" + str);}
    
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
    }

	@Override
	protected void onCreate(Bundle savedInstanceState) 
    {
		LogClass("onCreate()");
        super.onCreate(savedInstanceState);

        final String packageName = getApplicationContext().getPackageName();
        if(s_launchedToSecondaryDisplay == false)
        {
            //launch secondary display
            Intent launchIntent = null;
            final String clazz = "SxrNativeActivity";
            LogClass("getting Launch Intent for package - " + packageName + " and clazz - " + clazz);
            View view = findViewById(android.R.id.content);
            Context context = view.getContext();
        
            launchIntent = new Intent();
            launchIntent.setComponent(new ComponentName(packageName, packageName + "." + clazz));
        
            LogClass("android.R.id.content=" + android.R.id.content + ";context = " + context + ";launchIntent=" + launchIntent);

            if (launchIntent != null) 
            {
                launchIntent.addFlags(  Intent.FLAG_ACTIVITY_NEW_DOCUMENT | //this is necessary so that the SxrNativeActivity (destined for the secondary display) gets its own window on the primary phone display, which in turn allows for proper suspend/resume behavior for LauncherActivity.  (Otherwise, Android seems to randomly choose one of the two activities to get a single window -- and if SxrNativeActivity is randomly chosen, it will not respond to the user tapping it to resume)
                                        Intent.FLAG_ACTIVITY_NO_ANIMATION);

                Bundle aoBundle = null;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                {
                    int id = 0;
                    DisplayManager displayManager = (DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);
                    Display[] displays = displayManager.getDisplays(DisplayManager.DISPLAY_CATEGORY_PRESENTATION);
                    if( (displays != null) && (displays.length > 0) )
                    {
                        LogClass("displayManager.getDisplays().length=" + displays.length);
                        id = displays[0].getDisplayId();
                        
                        Toast.makeText(context, "Launching on Display ["+id + "]", Toast.LENGTH_SHORT).show();
                        aoBundle = ActivityOptions.makeBasic().setLaunchDisplayId(id).toBundle();

                        context.startActivity(launchIntent, aoBundle);
                        s_launchedToSecondaryDisplay = true;
                        LogClass("LauncherActivity: launchedToSecondaryDisplay on display id=" + id);
                    }
                }
            }
        }

        //setup UI
        setContentView(R.layout.button);
        Button button = (Button) findViewById(R.id.button);
        if(s_launchedToSecondaryDisplay)
        {
            button.setOnClickListener(new OnClickListener() 
            {
                public void onClick(View v) 
                {
                    Intent intent = new Intent();
                    String intentFloatId = getString(R.string.intentColorId);
                    intent.setAction(MultiDisplayVRHelper.ChangeColorIntentString(packageName));

                    Button button = (Button) findViewById(R.id.button);
                    String color0 = getString(R.string.color0);
                    String color1 = getString(R.string.color1);
                    if(button.getText().equals(color0))
                    {
                        button.setText(color1);
                        intent.putExtra(intentFloatId, 0.5f);
                    }
                    else
                    {
                        button.setText(color0);
                        intent.putExtra(intentFloatId, 1.0f);
                    }

                    boolean sendBroadcastResult = LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(intent);
                    LogClass("LauncherActivity: sendBroadcast(" + intent + ")=" + sendBroadcastResult);
                }
            });
        }
        else
        {
            button.setText(getString(R.string.noSecondaryDisplay));
        }
    }
}
