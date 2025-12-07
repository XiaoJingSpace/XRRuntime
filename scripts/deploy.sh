#!/bin/bash

# Deploy script for XR Runtime
# Usage: ./scripts/deploy.sh [device_serial]

set -e

DEVICE_SERIAL=${1:-""}

echo "Deploying XR Runtime to device..."

# Check if ADB is available
if ! command -v adb &> /dev/null; then
    echo "Error: adb not found. Please install Android SDK platform-tools"
    exit 1
fi

# Check device connection
if [ -n "$DEVICE_SERIAL" ]; then
    adb -s "$DEVICE_SERIAL" devices
else
    adb devices
fi

# Build APK if not exists
if [ ! -f "app/build/outputs/apk/debug/app-debug.apk" ]; then
    echo "Building APK..."
    ./gradlew assembleDebug
fi

# Install APK
echo "Installing APK..."
if [ -n "$DEVICE_SERIAL" ]; then
    adb -s "$DEVICE_SERIAL" install -r app/build/outputs/apk/debug/app-debug.apk
else
    adb install -r app/build/outputs/apk/debug/app-debug.apk
fi

# Push OpenXR loader config
echo "Pushing OpenXR loader config..."
PACKAGE_NAME="com.xrruntime"
CONFIG_PATH="/sdcard/Android/data/$PACKAGE_NAME/files/openxr_loader.json"

# Create config JSON
cat > /tmp/openxr_loader.json << EOF
{
  "runtime": {
    "library_path": "/data/data/$PACKAGE_NAME/lib/arm64/libxrruntime.so",
    "name": "Custom XR2 Runtime",
    "api_version": "1.1.0"
  }
}
EOF

if [ -n "$DEVICE_SERIAL" ]; then
    adb -s "$DEVICE_SERIAL" push /tmp/openxr_loader.json "$CONFIG_PATH"
else
    adb push /tmp/openxr_loader.json "$CONFIG_PATH"
fi

echo "Deployment complete!"
echo "Runtime library: /data/data/$PACKAGE_NAME/lib/arm64/libxrruntime.so"
echo "Config file: $CONFIG_PATH"

