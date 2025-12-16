# Word 文档图表和截图完整指南

## 🎯 目标

在生成的 Word 文档中自动包含：
- ✅ Mermaid 框架图、流程图、逻辑图、引导图
- ✅ 开发实现截图
- ✅ 自动格式化和布局

## 📊 实现方案

### 方案概述

1. **Mermaid 图表**: Markdown → PNG 图片 → Word
2. **截图**: 图片文件 → Markdown 引用 → Word
3. **自动处理**: 转换脚本自动识别并插入

## 🔧 实施步骤

### 步骤1: 转换 Mermaid 图表为图片

#### 方法A: 批量转换（推荐）

```bash
python scripts/convert_all_mermaid.py
```

这会：
- 扫描所有章节文件
- 提取 Mermaid 图表
- 转换为 PNG 图片
- 保存到 `book/images/mermaid/`

#### 方法B: 手动转换单个文件

```bash
python scripts/mermaid_to_image.py book/chapters/part1/chapter01.md
```

#### 方法C: 使用 Mermaid Live Editor（在线）

1. 访问 https://mermaid.live/
2. 粘贴 Mermaid 代码
3. 点击 "Download PNG"
4. 保存到 `book/images/mermaid/`，命名为 `mermaid_<hash>.png`

**如何获取 hash**: 运行转换脚本会显示每个图表的 hash 值

### 步骤2: 准备截图

#### 截图清单

按照 [截图添加指南](images/截图添加指南.md) 准备所有截图：

- 第2章：4个截图
- 第3章：3个截图
- 第4章：8个截图
- 第5章：3个截图
- 第15章：5个截图
- 第16章：3个截图
- 第17章：3个截图
- 第19章：5个截图

**总计**: 34个截图

#### 截图要求

- **格式**: PNG（推荐）或 JPG
- **分辨率**: 1920x1080 或更高
- **大小**: < 2MB
- **命名**: `chapterXX_sectionXX_description.png`
- **位置**: `book/images/` 或子目录

#### 在 Markdown 中添加截图引用

```markdown
![Android Studio 安装界面](images/chapter02_section01_android_studio_install.png)
```

### 步骤3: 生成 Word 文档

```bash
python scripts/md_to_docx.py
```

脚本会自动：
1. ✅ 识别 Mermaid 图表代码块
2. ✅ 查找对应的 PNG 图片（基于内容 hash）
3. ✅ 如果图片存在，插入到 Word 文档
4. ✅ 如果图片不存在，显示占位符和提示
5. ✅ 识别 Markdown 中的图片引用
6. ✅ 插入截图到 Word 文档
7. ✅ 居中显示图表和截图
8. ✅ 设置合适的图片大小

## 📋 当前状态检查

### 检查 Mermaid 图表转换状态

```bash
# Windows PowerShell
Get-ChildItem book\images\mermaid\*.png | Measure-Object | Select-Object Count

# 应该显示已转换的图表数量
```

### 检查截图文件

```bash
# Windows PowerShell
Get-ChildItem book\images\chapter*.png -Recurse | Measure-Object | Select-Object Count
```

### 检查 Word 文档

打开生成的 Word 文档，检查：
- [ ] Mermaid 图表是否显示为图片
- [ ] 截图是否正确插入
- [ ] 图片是否居中显示
- [ ] 图片大小是否合适

## 🎨 图片格式设置

### Mermaid 图表

- **宽度**: 14cm（自动设置）
- **对齐**: 居中
- **间距**: 上下各 6pt

### 截图

- **宽度**: 12cm（自动设置）
- **对齐**: 居中
- **间距**: 根据上下文自动调整

## 🔍 故障排除

### 问题1: Mermaid 图表显示为占位符

**原因**: 图片文件不存在

**解决方案**:
1. 运行 `python scripts/convert_all_mermaid.py`
2. 检查 `book/images/mermaid/` 目录
3. 确认图片文件已生成

### 问题2: 截图未显示

**原因**: 图片路径错误或文件不存在

**解决方案**:
1. 检查 Markdown 中的图片路径
2. 确认图片文件存在
3. 路径应该是相对路径：`images/filename.png`

### 问题3: 图片插入失败

**原因**: 文件过大或格式不支持

**解决方案**:
1. 检查图片大小（建议 < 2MB）
2. 确认格式为 PNG 或 JPG
3. 压缩图片如果太大

### 问题4: 在线转换失败

**原因**: 网络问题或 API 限制

**解决方案**:
1. 检查网络连接
2. 使用 Mermaid Live Editor 手动转换
3. 安装 mermaid-cli 进行离线转换

## 📝 完整工作流程示例

### 一次性完成所有步骤

创建 `scripts/build_complete.bat`:

```batch
@echo off
echo ========================================
echo XRRuntime 文档完整构建流程
echo ========================================
echo.

echo [1/3] 转换 Mermaid 图表为图片...
python scripts/convert_all_mermaid.py
echo.

echo [2/3] 检查截图文件...
echo 提示: 请确保所有截图已准备好
echo.

echo [3/3] 生成 Word 文档...
python scripts/md_to_docx.py
echo.

echo ========================================
echo 构建完成！
echo ========================================
echo.
echo 输出文件: book\XRRuntime框架设计与开发实践指南.docx
echo.
pause
```

## ✅ 验证清单

生成 Word 文档后，检查：

- [ ] 所有 Mermaid 图表已转换为图片并插入
- [ ] 所有截图已正确插入
- [ ] 图片显示清晰
- [ ] 图片大小合适
- [ ] 图片居中显示
- [ ] 没有占位符（或占位符有说明）
- [ ] 文档格式正确

## 📚 相关文档

- [图表清单](图表清单.md) - 完整图表清单
- [图表添加总结](图表添加总结.md) - 图表添加总结
- [截图添加指南](images/截图添加指南.md) - 详细截图指南
- [脚本说明](../scripts/README_图表处理.md) - 脚本使用说明

## 🎯 下一步

1. **转换图表**: 运行 `python scripts/convert_all_mermaid.py`
2. **准备截图**: 按照指南准备所有截图
3. **生成文档**: 运行 `python scripts/md_to_docx.py`
4. **检查结果**: 打开 Word 文档检查所有图表和截图
5. **优化调整**: 根据需要调整图片大小和位置

