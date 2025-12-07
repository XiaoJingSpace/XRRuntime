SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.sxr.multidisplayvr
adb install -g %MY_PATH%multidisplayvr-lr-release.apk

pause