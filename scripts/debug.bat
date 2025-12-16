@echo off
REM Debug script for XR Runtime (Windows)
REM Usage: scripts\debug.bat [command] [options]

setlocal enabledelayedexpansion

set COMMAND=%1
if "%COMMAND%"=="" set COMMAND=logcat

if "%COMMAND%"=="logcat" (
    echo Starting logcat for XRRuntime...
    adb logcat -c
    adb logcat | findstr /R /C:"XRRuntime" /C:"OpenXR"
    goto :end
)

if "%COMMAND%"=="gdb" (
    set PACKAGE_NAME=com.xrruntime
    for /f "tokens=*" %%p in ('adb shell pidof -s %PACKAGE_NAME%') do set PID=%%p
    if "!PID!"=="" (
        echo Error: Process not found. Please start the app first.
        exit /b 1
    )
    echo Attaching GDB to process !PID!...
    start /b adb shell "gdbserver :5039 --attach !PID!"
    timeout /t 2 /nobreak >nul
    if "%ANDROID_NDK_HOME%"=="" (
        echo Error: ANDROID_NDK_HOME is not set
        exit /b 1
    )
    "%ANDROID_NDK_HOME%\prebuilt\windows-x86_64\bin\gdb.exe" -ex "target remote :5039" -ex "continue"
    goto :end
)

if "%COMMAND%"=="perf" (
    set PACKAGE_NAME=com.xrruntime
    for /f "tokens=*" %%p in ('adb shell pidof -s %PACKAGE_NAME%') do set PID=%%p
    if "!PID!"=="" (
        echo Error: Process not found. Please start the app first.
        exit /b 1
    )
    echo Profiling process !PID!...
    adb shell "perf record -p !PID! -g sleep 10"
    adb pull /data/local/tmp/perf.data
    echo Perf data saved to perf.data
    goto :end
)

if "%COMMAND%"=="memcheck" (
    set PACKAGE_NAME=com.xrruntime
    for /f "tokens=*" %%p in ('adb shell pidof -s %PACKAGE_NAME%') do set PID=%%p
    if "!PID!"=="" (
        echo Error: Process not found. Please start the app first.
        exit /b 1
    )
    adb shell dumpsys meminfo %PACKAGE_NAME%
    goto :end
)

echo Usage: %0 [logcat^|gdb^|perf^|memcheck]
echo.
echo Commands:
echo   logcat   - View runtime logs
echo   gdb      - Attach GDB debugger
echo   perf     - Profile performance
echo   memcheck - Check memory usage
exit /b 1

:end
endlocal

