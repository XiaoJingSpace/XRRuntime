SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.multiviewvr
adb install -g %MY_PATH%multiviewvr-ll-release.apk

pause