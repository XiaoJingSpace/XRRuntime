SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.foveated
adb install -g %MY_PATH%foveatedvr-lr-release.apk
pause