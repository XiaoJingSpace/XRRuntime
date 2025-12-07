//=============================================================================
// FILE: SxrNativeActivity.java
//
//                  Copyright (c) 2019 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
package com.qualcomm.sxr.multidisplayvr;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.Toast;
import com.qualcomm.sxr.multidisplayvr.MultiDisplayVRHelper;

public class SxrNativeActivityReceiver extends BroadcastReceiver
{
    SxrNativeActivity mSxrNativeActivity;
    private void LogClass(String str) {MultiDisplayVRHelper.Log(this.getClass().getName() + ":" + str);}

    public SxrNativeActivityReceiver(SxrNativeActivity sxrNativeActivity)
    {
		LogClass("SxrNativeActivityReceiver(sxrNativeActivity=" + sxrNativeActivity + ")");
        mSxrNativeActivity = sxrNativeActivity;
    }

    @Override
    public void onReceive(Context context, Intent intent)
    {
        LogClass("intent received");
        mSxrNativeActivity.sxrNativeActivityReceiverIntent(intent);
    }
}
