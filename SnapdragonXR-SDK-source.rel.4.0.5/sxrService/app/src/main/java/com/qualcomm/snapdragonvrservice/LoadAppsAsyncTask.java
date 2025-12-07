// Copyright (c) 2016 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential.

package com.qualcomm.snapdragonvrservice;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.os.AsyncTask;
import android.os.Bundle;

import java.util.ArrayList;
import java.util.List;

/**
 *
 */
public class LoadAppsAsyncTask extends AsyncTask<Context, Bundle, Void> {

    Listener listener;
    List<ServiceInfo> listOfApps = new ArrayList<>();


    public interface Listener {
        void onUpdate(Bundle appInfo);
        void onComplete(List<ServiceInfo> listOfApps);
    }
    //----------------------------------------------------------------------------------------------
    public LoadAppsAsyncTask(Listener listener)
    {
        this.listener = listener;
    }

    //----------------------------------------------------------------------------------------------
    @Override
    protected Void doInBackground(Context... contexts) {
        PackageManager packageManager = contexts[0].getPackageManager();
        Intent intent = new Intent();
        intent.setAction("android.intent.action.MAIN");
        intent.addCategory("com.qualcomm.snapdragonvr.controllerprovider");
        List<ResolveInfo> list = packageManager.queryIntentServices(intent, PackageManager.MATCH_ALL);
        for(int i=0;i<list.size();i++)
        {
            ServiceInfo si = list.get(i).serviceInfo;
            listOfApps.add(si);
        }

        return null;
    }

    //----------------------------------------------------------------------------------------------
    @Override
    protected void onPostExecute(Void aVoid) {
        super.onPostExecute(aVoid);
        if( listener != null )
        {
            listener.onComplete(listOfApps);
        }
    }
}
