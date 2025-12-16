@echo off
REM Build script for XR Runtime (Windows)
REM Usage: scripts\build.bat [debug|release]

setlocal enabledelayedexpansion

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=debug

echo Building XR Runtime (%BUILD_TYPE%)...

REM Check if Android SDK is set
if "%ANDROID_HOME%"=="" (
    echo Error: ANDROID_HOME is not set
    echo Please set ANDROID_HOME environment variable to your Android SDK path
    exit /b 1
)

REM Check if NDK is available
if "%ANDROID_NDK_HOME%"=="" (
    echo Warning: ANDROID_NDK_HOME is not set, trying to find NDK...
    if exist "%ANDROID_HOME%\ndk" (
        for /d %%d in ("%ANDROID_HOME%\ndk\*") do (
            set ANDROID_NDK_HOME=%%d
            echo Found NDK: !ANDROID_NDK_HOME!
            goto :found_ndk
        )
    )
    echo Error: Could not find Android NDK
    echo Please set ANDROID_NDK_HOME environment variable
    exit /b 1
)
:found_ndk

REM Build using Gradle
if "%BUILD_TYPE%"=="release" (
    call gradlew.bat assembleRelease
) else (
    call gradlew.bat assembleDebug
)

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo Build complete!
echo SO library location:
if "%BUILD_TYPE%"=="release" (
    dir /s /b app\build\intermediates\cmake\release\obj\arm64-v8a\libxrruntime.so 2>nul
) else (
    dir /s /b app\build\intermediates\cmake\debug\obj\arm64-v8a\libxrruntime.so 2>nul
)

endlocal

