package com.qti.acg.apps.controllers.finch;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Process;
import android.support.v7.app.AppCompatActivity;
import android.widget.Button;
import android.widget.TextView;

import com.finchtechnologies.android.sdk.definition.ControllerType;
import com.finchtechnologies.android.sdk.definition.Quaternion;
import com.finchtechnologies.android.sdk.definition.Vector3;

import com.finchtechnologies.android.sdk.definition.FinchUpdateType;
interface ControllerDataCallbacks {
    public void dataAvailable(long[] timestamp, float[] pos, float[] quat, float[] posVar,
                       float[] quatVar, float[] vel, float[] angVel, float[] accel,
                       float[] accelOffset, float[] gyroOffset, float[] gravityVector,
                       int[] buttons, float[] touchPad,
                       int[] isTouching,float[] trigger);
}

public class MainActivity extends AppCompatActivity {
    public static final String TAG = MainActivity.class.getSimpleName();

    private FinchService finchService;
    private boolean bound = false;
    private TextView info;
    private Button hapticPulseButton;
    private boolean firstTime = true;
    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            FinchService.FinchBinder binder = (FinchService.FinchBinder) service;
            firstTime = false;
            finchService = binder.getService();
            bound = true;
            long[] timestampHMD = new long[1];
            final long[] timestamp = new long[1];

            finchService.init(ControllerType.Shift);

            new Thread(new Runnable() {
                @Override
                public void run() {

                    Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_DISPLAY);

                    Runnable guiRunnable = new Runnable() {
                        @Override
                        public void run() {
                            float[] pos = new float[3];
                            float[] quat = new float[4];
                            float[] posVar = new float[3];
                            float[] quatVar = new float[4];
                            float[] vel = new float[3];
                            float[] angVel = new float[3];
                            float[] accel = new float[3];
                            float[] accelOffset = new float[3];
                            float[] gyroOffset = new float[3];
                            float[] gravityVector = new float[3];
                            int[] buttons = new int[1];
                            float[] touchPad = new float[4];
                            int[] isTouching = new int[1];
                            float[] trigger = new float[1];
                            Vector3 phmd = new Vector3(0, 0, 0);
                            Quaternion qhmd = new Quaternion(0, 0, 0,0 );
                            finchService.update(FinchUpdateType.Internal, phmd, qhmd);
                            finchService.getCurrentData(0, timestamp, pos,quat, posVar, quatVar, vel, angVel, accel, accelOffset, gyroOffset, gravityVector, buttons, touchPad, isTouching, trigger);
                            info.setText("TimeStamp="+timestamp + "Position=" + pos[0] + "," + pos[1] + "," + pos[2] + " Rotation=" +
                                    quat[0] + "," + quat[1] + "," + quat[2] + "," + quat[3]);

                        }
                    };

                    while (true) {
                        if (bound)
                            runOnUiThread(guiRunnable);
                        //Log.d("FINCH", "Left Hand " + finchService.update());
                        try {
                            Thread.sleep(16);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }).start();

        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            bound = false;
        }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        info = (TextView) findViewById(R.id.info);
/*
        hapticPulseButton = (Button) findViewById(R.id.haptic_pulse);
        hapticPulseButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (bound)
                    finchService.hapticPulse();
            }
        });
*/
        // Bind to FinchService1.
        //Intent intent = new Intent(MainActivity.this, FinchService.class);
        //bindService(intent, connection, Context.BIND_AUTO_CREATE);

    }

    @Override
    protected void onDestroy() {
        finchService.exit();
        unbindService(connection);
        super.onDestroy();
    }
}

/*
public class MainActivity extends AppCompatActivity  {
    private String dataFile;
    private CSVWriter dataWriter;
    private static final int REQUEST_CODE = 0x11;
    private ubeacon ub = null;
    private ubeaconadsp uba = null;
    private boolean startLogging = false;
    private boolean isInitialized = false;
    private boolean startTracking = false;
    private FinchControllerStorage storage;
    private static final String IS_INITIALIZED = "is_initialized";
    private static final String START_TRACKING = "start_tracking";
    private static final String UBEACON = "ubeacon";
    static {
        //System.loadLibrary("Finchc_skel");
        //System.loadLibrary("ubeacon");
        System.loadLibrary("ubeaconadsp");
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == REQUEST_CODE) {
            if (grantResults.length == 1) {
                if (grantResults.length == 1 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(getApplicationContext(), "PERMISSION_GRANTED", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(getApplicationContext(), "PERMISSION_DENIED", Toast.LENGTH_SHORT).show();
                }
            }
        }
    }

    private void createFile(){
        // save file
        String fileTime = new SimpleDateFormat("yyyy-MM-dd-HHmmss").format(new Date());
        String filename = "ubeacon_" + fileTime + ".csv";
        File baseDir = new File(android.os.Environment.getExternalStorageDirectory(), "uBeaconData");
        Log.d("external directory: ", android.os.Environment.getExternalStorageDirectory().getAbsolutePath());
        if (!baseDir.exists()) {
            baseDir.mkdir();
        }
        dataFile = baseDir.getAbsolutePath()+"/"+filename;
        try {
            dataWriter = new CSVWriter(new FileWriter(dataFile));
            String[] dataHeader = {"Timestamp", "pos[0]","pos[1]","pos[2]",
                    "quat[0]","quat[1]","quat[2]","quat[3]", "pos_var[0]","pos_var[1]","pos_var[2]",
                    "quat_var[0]","quat_var[1]","quat_var[2]","quat_var[3]", "vel[0]","vel[1]","vel[2]",
                    "ang_vel[0]","ang_vel[1]","ang_vel[2]", "accel[0]","accel[1]","accel[2]", "top_button", "bottom_button"};
            dataWriter.writeNext(dataHeader);
        }catch (IOException e) {
            Log.e("startLogging", e.getMessage());
        }

    }
    public void RegisterCallback(int index) {
        storage.dataCallback[index] = new ControllerDataCallbacks() {
            public void dataAvailable(long[] timestamp, float[] pos, float[] quat, float[] posVar,
                               float[] quatVar, float[] vel, float[] angVel, float[] accel,
                               float[] accelOffset, float[] gyroOffset, float[] gravityVector,
                               int[] button_down_events, int[] button_up_events,
                               int[] last_state_bottom, int[] touchPad) {
                //Log.e("MainActivity", "Received DataAvailable");
                TextView timeView = (TextView) findViewById(R.id.timeText);
                TextView posView = (TextView) findViewById(R.id.posText);
                TextView quatView = (TextView) findViewById(R.id.quatText);
                TextView topButtonView = (TextView) findViewById(R.id.topButtonText);
                TextView bottomButtonView = (TextView) findViewById(R.id.bottomButtonText);

                timeView.setText(Long.toString(timestamp[0]));
                posView.setText(Arrays.toString(pos));
                quatView.setText(Arrays.toString(quat));
                topButtonView.setText(Integer.toString(touchPad[0]));
                bottomButtonView.setText(Integer.toString(last_state_bottom[0]));

                if (startLogging) {
                    String[] csvData = new String[]{
                            Long.toString(timestamp[0]),
                            Float.toString(pos[0]),
                            Float.toString(pos[1]),
                            Float.toString(pos[2]),
                            Float.toString(quat[0]),
                            Float.toString(quat[1]),
                            Float.toString(quat[2]),
                            Float.toString(quat[3]),
                            Float.toString(posVar[0]),
                            Float.toString(posVar[1]),
                            Float.toString(posVar[2]),
                            Float.toString(quatVar[0]),
                            Float.toString(quatVar[1]),
                            Float.toString(quatVar[2]),
                            Float.toString(quatVar[3]),
                            Float.toString(vel[0]),
                            Float.toString(vel[1]),
                            Float.toString(vel[2]),
                            Float.toString(angVel[0]),
                            Float.toString(angVel[1]),
                            Float.toString(angVel[2]),
                            Float.toString(accel[0]),
                            Float.toString(accel[1]),
                            Float.toString(accel[2]),
                            Integer.toString(touchPad[0]),
                            Integer.toString(last_state_bottom[0])
                    };
                    dataWriter.writeNext(csvData);
                }

            }
        };
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setTheme(R.style.AppTheme);

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        String[] permissions = {"android.permission.WRITE_EXTERNAL_STORAGE"};
        ActivityCompat.requestPermissions(this, permissions, REQUEST_CODE);
        final TextView status = (TextView) findViewById(R.id.initStatus);
        Log.e("MainActivity", "onCreate: calling ubeacon" );
        final Button button = (Button)findViewById(R.id.start_button);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Code here executes on main thread after user presses button
                //createFile();
                startLogging = false;
                if(isInitialized == false) {
                    storage = FinchControllerStorage.getInstance();
                    storage.setAppContext(getApplicationContext());
                    isInitialized = true;
                }
                if(startTracking == false) {
                    startTracking = true;
                    RegisterCallback(0);
                    String result = storage.InitializeIfNeeded(true);
                    status.setText(result);
                    storage.Start(0);
                }
            }
        });
        final Button button2 = (Button)findViewById(R.id.stop_button);
        button2.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Code here executes on main thread after user presses button
                if(startTracking == true) {
                    storage.Stop(0);

                }

                startLogging = false;
            }
        });
    }
    @Override
    public void onStop() {
        super.onStop();
        isInitialized = false;
        Log.e("MainActivity", "onStop: setting isInitialized to false" );

    }
    @Override
    public void onResume() {
        super.onResume();
        Log.e("MainActivity", "onResume: current isInitialized =" + isInitialized );

    }
    @Override
    public void onDestroy() {
        super.onDestroy();
        try {
            dataWriter.close();
            //w.ubeacon_terminate();
        } catch(IOException e){
            Log.e("onDestroy", e.getMessage());
        }

    }
    @Override
    protected void onSaveInstanceState(final Bundle outState) {
        super.onSaveInstanceState(outState);

        // Save the state of item position
        outState.putBoolean(IS_INITIALIZED,isInitialized);
        outState.putBoolean(START_TRACKING,startTracking);
        //outState.putParcelable(UBEACON,ub); //(START_TRACKING,startTracking);
    }

    @Override
    protected void onRestoreInstanceState(final Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);

        // Read the state of item position
        isInitialized = savedInstanceState.getBoolean(IS_INITIALIZED);
        startTracking = savedInstanceState.getBoolean(START_TRACKING);
    }
}
*/