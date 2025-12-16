# XR Runtime 系统部署脚本
# 用途: 将编译好的 libxrruntime.so 推送到 Android 系统分区

param(
    [switch]$Vendor = $false,  # 推送到 /vendor 而不是 /system
    [switch]$Help = $false
)

if ($Help) {
    Write-Host @"
XR Runtime 系统部署脚本

用法:
    .\deploy_to_system.ps1              # 推送到 /system/lib64
    .\deploy_to_system.ps1 -Vendor      # 推送到 /vendor/lib64

前提条件:
    - 设备已连接并启用 USB 调试
    - 设备已 root
    - 已编译生成 libxrruntime.so

文件位置:
    源文件: app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so
    目标: /system/lib64/libxrruntime.so 或 /vendor/lib64/libxrruntime.so
"@
    exit 0
}

$ErrorActionPreference = "Stop"

# 配置
$sourceFile = "app\build\outputs\jniLibs\arm64-v8a\libxrruntime.so"
$targetBase = if ($Vendor) { "/vendor/lib64" } else { "/system/lib64" }
$targetFile = "$targetBase/libxrruntime.so"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "XR Runtime 系统部署工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查源文件
Write-Host "[1/6] 检查源文件..." -ForegroundColor Yellow
if (-not (Test-Path $sourceFile)) {
    Write-Host "错误: 找不到源文件 $sourceFile" -ForegroundColor Red
    Write-Host "请先编译项目: .\gradlew assembleDebug" -ForegroundColor Yellow
    exit 1
}

$fileSize = (Get-Item $sourceFile).Length / 1MB
Write-Host "✓ 找到源文件: $sourceFile ($([math]::Round($fileSize, 2)) MB)" -ForegroundColor Green
Write-Host ""

# 2. 检查 ADB 连接
Write-Host "[2/6] 检查设备连接..." -ForegroundColor Yellow
$devices = adb devices | Select-String -Pattern "device$"
if ($devices.Count -eq 0) {
    Write-Host "错误: 未找到已连接的设备" -ForegroundColor Red
    Write-Host "请确保设备已连接并启用 USB 调试" -ForegroundColor Yellow
    exit 1
}
Write-Host "✓ 设备已连接" -ForegroundColor Green
Write-Host ""

# 3. 获取 root 权限
Write-Host "[3/6] 获取 root 权限..." -ForegroundColor Yellow
$rootOutput = adb root 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 无法获取 root 权限" -ForegroundColor Red
    Write-Host "输出: $rootOutput" -ForegroundColor Yellow
    exit 1
}
Start-Sleep -Seconds 2  # 等待 root 生效
Write-Host "✓ Root 权限已获取" -ForegroundColor Green
Write-Host ""

# 4. 重新挂载分区
Write-Host "[4/6] 重新挂载分区为可写..." -ForegroundColor Yellow
$mountPoint = if ($Vendor) { "/vendor" } else { "/system" }
$remountOutput = adb remount 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "警告: remount 可能失败，尝试手动挂载..." -ForegroundColor Yellow
    adb shell "mount -o remount,rw $mountPoint" 2>&1 | Out-Null
}
Write-Host "✓ 分区已挂载为可写" -ForegroundColor Green
Write-Host ""

# 5. 推送文件
Write-Host "[5/6] 推送库文件到设备..." -ForegroundColor Yellow
Write-Host "  源: $sourceFile" -ForegroundColor Gray
Write-Host "  目标: $targetFile" -ForegroundColor Gray

$pushOutput = adb push $sourceFile $targetFile 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 推送失败" -ForegroundColor Red
    Write-Host "输出: $pushOutput" -ForegroundColor Yellow
    exit 1
}
Write-Host "✓ 文件推送成功" -ForegroundColor Green
Write-Host ""

# 6. 设置权限和所有权
Write-Host "[6/6] 设置文件权限..." -ForegroundColor Yellow
adb shell "chmod 644 $targetFile" | Out-Null
adb shell "chown root:root $targetFile" | Out-Null

# 设置 SELinux 上下文（如果支持）
$selinuxCheck = adb shell "getenforce" 2>&1
if ($selinuxCheck -match "Enforcing") {
    Write-Host "  检测到 SELinux Enforcing 模式，设置上下文..." -ForegroundColor Gray
    $context = if ($Vendor) { "u:object_r:vendor_lib_file:s0" } else { "u:object_r:system_lib_file:s0" }
    adb shell "chcon $context $targetFile" 2>&1 | Out-Null
}

Write-Host "✓ 权限设置完成" -ForegroundColor Green
Write-Host ""

# 7. 验证部署
Write-Host "验证部署..." -ForegroundColor Yellow
$verify = adb shell "ls -la $targetFile" 2>&1
if ($verify -match "libxrruntime.so") {
    Write-Host "✓ 文件验证成功" -ForegroundColor Green
    Write-Host ""
    Write-Host "文件信息:" -ForegroundColor Cyan
    Write-Host $verify -ForegroundColor Gray
} else {
    Write-Host "警告: 文件验证失败" -ForegroundColor Yellow
    Write-Host "输出: $verify" -ForegroundColor Gray
}

# 8. 重新挂载为只读
Write-Host ""
Write-Host "重新挂载分区为只读..." -ForegroundColor Yellow
adb shell "mount -o remount,ro $mountPoint" 2>&1 | Out-Null
Write-Host "✓ 分区已重新挂载为只读" -ForegroundColor Green
Write-Host ""

# 完成
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "部署完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "库文件已安装到: $targetFile" -ForegroundColor Green
Write-Host ""
Write-Host "下一步:" -ForegroundColor Yellow
Write-Host "1. 重启设备使更改生效: adb reboot" -ForegroundColor White
Write-Host "2. 查看日志验证: adb logcat | grep -i xrruntime" -ForegroundColor White
Write-Host ""

