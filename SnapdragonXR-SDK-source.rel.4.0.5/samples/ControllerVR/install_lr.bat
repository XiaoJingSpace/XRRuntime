SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.controller
adb install -g %MY_PATH%controllervr-lr-release.apk
pause