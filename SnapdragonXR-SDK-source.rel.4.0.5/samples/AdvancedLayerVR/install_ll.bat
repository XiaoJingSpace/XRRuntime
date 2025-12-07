SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.advancedlayer
adb install -g %MY_PATH%advancedlayer-ll-release.apk

pause