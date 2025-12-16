@echo off
REM Deploy script for XR Runtime (Windows)
REM Usage: scripts\deploy.bat [device_serial]

setlocal enabledelayedexpansion

set DEVICE_SERIAL=%1

echo Deploying XR Runtime to device...

REM Check if ADB is available
where adb >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: adb not found. Please install Android SDK platform-tools
    exit /b 1
)

REM Check device connection
if not "%DEVICE_SERIAL%"=="" (
    adb -s %DEVICE_SERIAL% devices
) else (
    adb devices
)

REM Build APK if not exists
if not exist "app\build\outputs\apk\debug\app-debug.apk" (
    echo Building APK...
    call gradlew.bat assembleDebug
    if %ERRORLEVEL% neq 0 (
        echo Build failed!
        exit /b %ERRORLEVEL%
    )
)

REM Install APK
echo Installing APK...
if not "%DEVICE_SERIAL%"=="" (
    adb -s %DEVICE_SERIAL% install -r app\build\outputs\apk\debug\app-debug.apk
) else (
    adb install -r app\build\outputs\apk\debug\app-debug.apk
)

if %ERRORLEVEL% neq 0 (
    echo Installation failed!
    exit /b %ERRORLEVEL%
)

REM Push OpenXR loader config
echo Pushing OpenXR loader config...
set PACKAGE_NAME=com.xrruntime
set CONFIG_PATH=/sdcard/Android/data/%PACKAGE_NAME%/files/openxr_loader.json

REM Create config JSON
(
echo {
echo   "runtime": {
echo     "library_path": "/data/data/%PACKAGE_NAME%/lib/arm64/libxrruntime.so",
echo     "name": "Custom XR2 Runtime",
echo     "api_version": "1.1.0"
echo   }
echo }
) > %TEMP%\openxr_loader.json

if not "%DEVICE_SERIAL%"=="" (
    adb -s %DEVICE_SERIAL% push %TEMP%\openxr_loader.json %CONFIG_PATH%
) else (
    adb push %TEMP%\openxr_loader.json %CONFIG_PATH%
)

echo Deployment complete!
echo Runtime library: /data/data/%PACKAGE_NAME%/lib/arm64/libxrruntime.so
echo Config file: %CONFIG_PATH%

endlocal

