//-----------------------------------------------------------------------------
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

package com.qualcomm.snapdragonvrservice;

import android.util.Log;

/**
 * LogUtil is used to enable/disable the debug log.
 */

public class LogUtil {
    static boolean sDebug = true;

    public static void enableDebug(boolean enable) {
        sDebug = enable;
    }

    public static void log(String tag, String msg) {
        if (sDebug) {
            Log.d(tag, msg);
        }
    }
}
