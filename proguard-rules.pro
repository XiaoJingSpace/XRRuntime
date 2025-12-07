# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.

# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep JNI bridge
-keep class com.xrruntime.** { *; }

# Keep service
-keep class com.xrruntime.XRRuntimeService { *; }

# Don't obfuscate native interface
-keepclasseswithmembernames,includedescriptorclasses class * {
    native <methods>;
}

