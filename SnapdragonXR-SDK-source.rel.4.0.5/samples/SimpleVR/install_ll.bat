SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.simple
adb install -g %MY_PATH%simplevr-ll-release.apk

pause