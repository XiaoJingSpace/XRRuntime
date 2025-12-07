SETLOCAL
SET MY_PATH=%~dp0

adb root
adb wait-for-device
adb remount
adb wait-for-device

adb push %MY_PATH%svrapi_config_sheldon_WarpOnHost.txt /system/etc/qvr/svrapi_config.txt

adb shell killall qvrservice

pause
