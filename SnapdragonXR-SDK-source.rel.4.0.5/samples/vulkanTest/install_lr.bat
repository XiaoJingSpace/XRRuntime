SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.sgs.svr.vulkantest
adb install -g %MY_PATH%vulkanTest-lr-release.apk
pause