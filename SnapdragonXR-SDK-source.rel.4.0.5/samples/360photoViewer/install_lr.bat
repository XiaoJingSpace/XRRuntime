SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.apps.photoViewer
adb install -g %MY_PATH%360photoViewer-lr-debug.apk
pause