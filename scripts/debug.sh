#!/bin/bash

# Debug script for XR Runtime
# Usage: ./scripts/debug.sh [command] [options]

set -e

COMMAND=${1:-logcat}

case "$COMMAND" in
    logcat)
        echo "Starting logcat for XRRuntime..."
        adb logcat -c
        adb logcat | grep -E "(XRRuntime|OpenXR)"
        ;;
    
    gdb)
        PACKAGE_NAME="com.xrruntime"
        PID=$(adb shell pidof -s "$PACKAGE_NAME")
        if [ -z "$PID" ]; then
            echo "Error: Process not found. Please start the app first."
            exit 1
        fi
        echo "Attaching GDB to process $PID..."
        adb shell "gdbserver :5039 --attach $PID" &
        sleep 2
        $ANDROID_NDK_HOME/prebuilt/linux-x86_64/bin/gdb -ex "target remote :5039" -ex "continue"
        ;;
    
    perf)
        echo "Starting performance profiling..."
        PACKAGE_NAME="com.xrruntime"
        PID=$(adb shell pidof -s "$PACKAGE_NAME")
        if [ -z "$PID" ]; then
            echo "Error: Process not found. Please start the app first."
            exit 1
        fi
        echo "Profiling process $PID..."
        adb shell "perf record -p $PID -g sleep 10"
        adb pull /data/local/tmp/perf.data
        echo "Perf data saved to perf.data"
        ;;
    
    memcheck)
        echo "Starting memory check..."
        PACKAGE_NAME="com.xrruntime"
        PID=$(adb shell pidof -s "$PACKAGE_NAME")
        if [ -z "$PID" ]; then
            echo "Error: Process not found. Please start the app first."
            exit 1
        fi
        adb shell dumpsys meminfo "$PACKAGE_NAME"
        ;;
    
    *)
        echo "Usage: $0 [logcat|gdb|perf|memcheck]"
        echo ""
        echo "Commands:"
        echo "  logcat   - View runtime logs"
        echo "  gdb      - Attach GDB debugger"
        echo "  perf     - Profile performance"
        echo "  memcheck - Check memory usage"
        exit 1
        ;;
esac

