SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.protectedadvancedlayer
adb install -g %MY_PATH%protectedadvancedlayer-ll-release.apk

pause