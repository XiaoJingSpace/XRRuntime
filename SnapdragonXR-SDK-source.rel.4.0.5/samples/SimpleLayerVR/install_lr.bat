SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.layer
adb install -g %MY_PATH%simplelayervr-lr-release.apk
pause