SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.foveated
adb install -g %MY_PATH%foveatedvr-ll-release.apk

pause