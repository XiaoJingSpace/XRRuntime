SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.cameralayer
adb install -g %MY_PATH%cameralayer-ll-release.apk

pause