SETLOCAL
SET MY_PATH=%~dp0

adb %MYDEV% uninstall com.qti.acg.apps.controllers.chirp
adb %MYDEV% install -r %MY_PATH%..\app\build\outputs\apk\ChirpControllerService-debug.apk
adb shell pm grant com.qti.acg.apps.controllers.chirp android.permission.READ_EXTERNAL_STORAGE
adb shell pm grant com.qti.acg.apps.controllers.chirp android.permission.WRITE_EXTERNAL_STORAGE
pause