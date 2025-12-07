# 项目文档总览

本文档提供了 OpenXR Runtime for Qualcomm XR2 项目的完整文档索引和导航。

## 📚 文档结构

```
docs/
├── README.md              # 文档目录（从这里开始）
├── PROJECT_GUIDE.md      # 完整项目指南 ⭐（推荐首先阅读）
├── ARCHITECTURE.md        # 架构文档
├── BUILD.md               # 构建指南
├── DEPLOY.md              # 部署指南
├── DEBUG.md               # 调试指南
├── API_REFERENCE.md       # API 参考
└── QVR_INTEGRATION.md     # QVR 集成指南
```

## 🎯 快速导航

### 新用户入门

1. **[完整项目指南](docs/PROJECT_GUIDE.md)** ⭐
   - 项目概述
   - 架构设计
   - 快速开始
   - 使用示例
   - 故障排除

2. [构建指南](docs/BUILD.md)
   - 环境配置
   - 编译步骤
   - 常见问题

3. [部署指南](docs/DEPLOY.md)
   - 部署方法
   - 配置说明
   - 验证步骤

### 开发者文档

1. [架构文档](docs/ARCHITECTURE.md)
   - 整体架构
   - 核心组件
   - 数据流
   - 线程模型

2. [API 参考](docs/API_REFERENCE.md)
   - API 实现状态
   - 函数说明
   - 使用示例

3. [QVR 集成指南](docs/QVR_INTEGRATION.md)
   - QVR API 概述
   - 集成实现
   - 数据转换
   - 调用流程

### 调试和测试

1. [调试指南](docs/DEBUG.md)
   - 调试工具
   - 性能分析
   - 常见问题
   - 故障排除

## 📖 文档详细说明

### 1. [完整项目指南](docs/PROJECT_GUIDE.md) ⭐

**推荐首先阅读**

包含项目的完整说明，适合所有用户：

- ✅ 项目概述和目标
- ✅ 架构设计详解
- ✅ 技术栈说明
- ✅ 项目结构说明
- ✅ 核心功能介绍
- ✅ 编译和部署指南
- ✅ API 使用示例
- ✅ 开发指南
- ✅ 故障排除

**文件大小**: ~19KB  
**阅读时间**: 约 30 分钟

### 2. [架构文档](docs/ARCHITECTURE.md)

详细的架构设计说明，适合开发者：

- 整体架构图
- 核心组件说明
- 数据流分析
- 线程模型
- 内存管理
- 性能优化策略

**文件大小**: ~6KB  
**阅读时间**: 约 15 分钟

### 3. [构建指南](docs/BUILD.md)

编译项目的详细步骤：

- 前置要求
- 环境配置
- 编译步骤（多种方法）
- 编译选项配置
- 常见问题解决

**文件大小**: ~4KB  
**阅读时间**: 约 10 分钟

### 4. [部署指南](docs/DEPLOY.md)

部署 Runtime 到设备的方法：

- 部署方法（APK、手动、脚本）
- OpenXR Loader 配置
- 启动 Runtime
- 验证部署
- 卸载方法

**文件大小**: ~5KB  
**阅读时间**: 约 10 分钟

### 5. [调试指南](docs/DEBUG.md)

调试和测试 Runtime：

- 调试工具（Logcat、GDB）
- 性能分析
- OpenXR 验证层
- 常见调试场景
- 内存调试
- 线程调试

**文件大小**: ~7KB  
**阅读时间**: 约 15 分钟

### 6. [API 参考](docs/API_REFERENCE.md)

OpenXR API 实现状态：

- 已实现的 API 列表
- API 实现说明
- 扩展支持
- 测试建议
- 已知限制

**文件大小**: ~5KB  
**阅读时间**: 约 10 分钟

### 7. [QVR 集成指南](docs/QVR_INTEGRATION.md)

QVR API 集成详细说明：

- QVR API 概述
- 集成实现
- API 调用流程
- 数据转换
- 追踪模式
- 错误处理

**文件大小**: ~6KB  
**阅读时间**: 约 15 分钟

## 🗺️ 阅读路径建议

### 路径 1: 快速开始（新用户）

```
PROJECT_GUIDE.md → BUILD.md → DEPLOY.md → DEBUG.md
```

### 路径 2: 深入理解（开发者）

```
PROJECT_GUIDE.md → ARCHITECTURE.md → API_REFERENCE.md → QVR_INTEGRATION.md
```

### 路径 3: 问题解决（故障排除）

```
PROJECT_GUIDE.md (故障排除章节) → DEBUG.md → BUILD.md (常见问题)
```

## 📊 文档统计

- **总文档数**: 8 个
- **总大小**: ~60KB
- **总页数**: 约 200+ 页（打印）
- **代码示例**: 50+ 个
- **图表**: 10+ 个

## 🔄 文档更新

文档会随着项目发展持续更新：

- **最后更新**: 2024年12月
- **更新频率**: 随代码更新
- **维护状态**: 活跃维护中

## 💡 使用建议

1. **首次使用**: 从 [PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md) 开始
2. **遇到问题**: 查看对应文档的"故障排除"章节
3. **深入开发**: 阅读 [ARCHITECTURE.md](docs/ARCHITECTURE.md) 和 [QVR_INTEGRATION.md](docs/QVR_INTEGRATION.md)
4. **API 使用**: 参考 [API_REFERENCE.md](docs/API_REFERENCE.md)

## 📞 获取帮助

- 📖 查看文档: `docs/` 目录
- 🐛 报告问题: 提交 Issue
- 💬 讨论交流: 项目讨论区

---

**开始阅读**: [完整项目指南](docs/PROJECT_GUIDE.md) ⭐

