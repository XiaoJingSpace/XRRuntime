SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.camera
adb install -g %MY_PATH%camera-ll-release.apk

pause