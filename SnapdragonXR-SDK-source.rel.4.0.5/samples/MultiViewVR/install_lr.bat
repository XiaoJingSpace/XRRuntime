SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.mutiviewvr
adb install -g %MY_PATH%multiviewvr-lr-release.apk
pause