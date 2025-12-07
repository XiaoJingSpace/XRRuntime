REM @ECHO OFF
echo ************************************************************************************
echo Configuring chirp controller
echo ************************************************************************************

SETLOCAL
SET MY_PATH=%~dp0
adb %MYDEV% shell mkdir /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\controller_init_config.json /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\controller_ultra_config.json /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\hmd_init_config.json /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\hmd_ultra_config.json /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\imu_config.json /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\process_model_config.json /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\hmd_imu_config.json /sdcard/chirp/
adb %MYDEV% push %MY_PATH%..\app\src\main\assets\controller_imu_config.json /sdcard/chirp/
pause
adb shell sync

pause