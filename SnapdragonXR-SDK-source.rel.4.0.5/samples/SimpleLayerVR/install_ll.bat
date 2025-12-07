SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.layer
adb install -g %MY_PATH%simplelayervr-ll-release.apk

pause