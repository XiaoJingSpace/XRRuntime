package com.xrruntime;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class XRRuntimeService extends Service {
    private static final String TAG = "XRRuntimeService";
    
    static {
        System.loadLibrary("xrruntime");
    }
    
    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "XRRuntimeService created");
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "XRRuntimeService started");
        return START_STICKY;
    }
    
    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "XRRuntimeService destroyed");
    }
    
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}

