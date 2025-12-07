// Copyright (c) 2016 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential.

package com.qualcomm.snapdragonvrservice;

import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.IBinder;
import android.os.RemoteException;
import android.preference.PreferenceManager;

/**
 *
 */
public class SvrService extends Service {

    //----------------------------------------------------------------------------------------------
    @Override
    public IBinder onBind(Intent intent) {
        return new ISvrServiceInterface.Stub() {
            //--------------------------------------------------------------------------------------
            @Override
            public Intent GetControllerProviderIntent(String desc) throws RemoteException {

                SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(SvrService.this);

                //Get Selected ControllerProvider
                String value = sharedPrefs.getString(SettingsActivity.PREF_CONTROLLER_PROVIDER, null);

                //Get PackageName and Service Name from the value
                Intent intent = null;
                if( value != null )
                {
                    String[] parts = value.split("#");
                    if( parts.length == 2 ) {
                        intent = new Intent();
                        intent.setPackage(parts[0]);
                        intent.setClassName(parts[0], parts[1]);
                    }
                    else {
                    intent = new Intent();
                        intent.setPackage("None");
                        intent.setClassName("None", "None");
                    }
                }

                return intent;
            }
            //--------------------------------------------------------------------------------------
            @Override
            public int GetControllerRingBufferSize() throws RemoteException
            {
                SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(SvrService.this);
                int sizeValue = sharedPrefs.getInt(SettingsActivity.PREF_CONTROLLER_RINGBUFFER_SIZE, 80);
                return sizeValue;
            }
        };
    }

    private static final String TAG = "SvrService";
}
