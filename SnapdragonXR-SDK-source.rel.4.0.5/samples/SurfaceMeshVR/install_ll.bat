SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.surfacemesh
adb install -g %MY_PATH%surfacemesh-ll-release.apk

pause