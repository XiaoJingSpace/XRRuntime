//=============================================================================
// FILE: MultiDisplayVRHelper.java
//
//                  Copyright (c) 2019 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//=============================================================================
package com.qualcomm.sxr.multidisplayvr;

import android.util.Log;
import android.content.res.Resources;

public class MultiDisplayVRHelper
{
    public static void Log(String str) {Log.i("MultiDisplayVR", str);}

    public static String ChangeColorIntentString(String packageName) {return packageName + ".CHANGE_COLOR_INTENT";}
}
