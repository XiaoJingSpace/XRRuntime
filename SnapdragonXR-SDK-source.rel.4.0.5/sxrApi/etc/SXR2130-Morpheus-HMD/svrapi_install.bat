SETLOCAL
SET MY_PATH=%~dp0

adb root
adb wait-for-device
adb remount
adb wait-for-device

adb push %MY_PATH%svrapi_config.txt /system/etc/qvr/svrapi_config.txt

adb push %MY_PATH%svrapi_lens_left.csv /system/etc/qvr/.
adb push %MY_PATH%svrapi_lens_right.csv /system/etc/qvr/.

adb shell killall qvrservice

pause
