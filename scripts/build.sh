#!/bin/bash

# Build script for XR Runtime
# Usage: ./scripts/build.sh [debug|release]

set -e

BUILD_TYPE=${1:-debug}

echo "Building XR Runtime ($BUILD_TYPE)..."

# Check if Android SDK is set
if [ -z "$ANDROID_HOME" ]; then
    echo "Error: ANDROID_HOME is not set"
    exit 1
fi

# Check if NDK is available
if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "Warning: ANDROID_NDK_HOME is not set, trying to find NDK..."
    if [ -d "$ANDROID_HOME/ndk" ]; then
        NDK_VERSION=$(ls -1 "$ANDROID_HOME/ndk" | head -1)
        export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/$NDK_VERSION"
        echo "Found NDK: $ANDROID_NDK_HOME"
    else
        echo "Error: Could not find Android NDK"
        exit 1
    fi
fi

# Build using Gradle
if [ "$BUILD_TYPE" == "release" ]; then
    ./gradlew assembleRelease
else
    ./gradlew assembleDebug
fi

echo "Build complete!"
echo "SO library location:"
if [ "$BUILD_TYPE" == "release" ]; then
    find . -name "libxrruntime.so" -path "*/release/*" | head -1
else
    find . -name "libxrruntime.so" -path "*/debug/*" | head -1
fi

