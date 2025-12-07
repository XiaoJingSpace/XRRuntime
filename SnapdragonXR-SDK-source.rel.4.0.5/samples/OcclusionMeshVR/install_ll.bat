SETLOCAL
SET MY_PATH=%~dp0

adb uninstall com.qualcomm.svr.occlusionmesh
adb install -g %MY_PATH%occlusionmesh-ll-release.apk

pause