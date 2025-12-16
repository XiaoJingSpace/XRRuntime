# 图表处理脚本说明

## 脚本列表

1. **md_to_docx.py** - 主转换脚本（已更新，支持图表和截图）
2. **mermaid_to_image.py** - Mermaid 图表转图片工具
3. **convert_all_mermaid.py** - 批量转换所有 Mermaid 图表

## 使用流程

### 步骤1: 转换 Mermaid 图表为图片

#### 方法1: 批量转换（推荐）

```bash
python scripts/convert_all_mermaid.py
```

这会自动：
- 扫描所有章节文件
- 提取所有 Mermaid 图表
- 转换为 PNG 图片
- 保存到 `book/images/mermaid/`

#### 方法2: 转换单个文件

```bash
python scripts/mermaid_to_image.py book/chapters/part1/chapter01.md
```

#### 方法3: 手动转换

1. 访问 [Mermaid Live Editor](https://mermaid.live/)
2. 粘贴 Mermaid 代码
3. 导出为 PNG
4. 保存到 `book/images/mermaid/`，命名为 `mermaid_<hash>.png`

### 步骤2: 添加截图

1. 按照 [截图添加指南](../book/images/截图添加指南.md) 准备截图
2. 将截图保存到 `book/images/` 对应目录
3. 在 Markdown 中使用 `![描述](images/filename.png)` 引用

### 步骤3: 生成 Word 文档

```bash
python scripts/md_to_docx.py
```

脚本会自动：
- 识别 Mermaid 图表并插入对应图片
- 识别图片引用并插入截图
- 如果图片不存在，显示占位符

## Mermaid 图表转换方法

### 方法1: 在线 API（默认）

脚本会自动使用 Mermaid Live Editor API 在线转换，无需安装额外工具。

### 方法2: mermaid-cli（可选）

如果需要离线转换或批量处理：

```bash
# 安装 Node.js 和 npm
# 然后安装 mermaid-cli
npm install -g @mermaid-js/mermaid-cli

# 转换单个文件
mmdc -i diagram.mmd -o diagram.png

# 批量转换
mmdc -i book/chapters/**/*.md -o book/images/mermaid/
```

## 图片命名规则

### Mermaid 图表

- 格式: `mermaid_<hash>.png`
- Hash: 基于图表内容的 MD5 前8位
- 位置: `book/images/mermaid/`

### 截图

- 格式: `chapterXX_sectionXX_description.png`
- 位置: `book/images/` 或子目录

## 故障排除

### Mermaid 图表未显示

1. 检查图片文件是否存在: `book/images/mermaid/mermaid_*.png`
2. 运行转换脚本: `python scripts/convert_all_mermaid.py`
3. 检查网络连接（如果使用在线 API）

### 截图未显示

1. 检查图片路径是否正确
2. 确认图片文件存在
3. 检查 Markdown 中的引用格式: `![alt](path)`

### 图片插入失败

1. 检查图片文件大小（建议 < 2MB）
2. 确认图片格式（PNG 或 JPG）
3. 检查文件权限

## 注意事项

1. **Mermaid 图表**: 必须先转换为图片才能插入 Word
2. **截图**: 需要手动准备，然后通过 Markdown 引用
3. **图片路径**: 使用相对路径，从 `book/` 目录开始
4. **文件大小**: 建议每个图片不超过 2MB，以保证 Word 文档性能

## 自动化流程

创建 `scripts/build_complete.sh` (Linux/macOS) 或 `scripts/build_complete.bat` (Windows):

```bash
#!/bin/bash
# 完整构建流程

echo "1. 转换 Mermaid 图表..."
python scripts/convert_all_mermaid.py

echo "2. 生成 Word 文档..."
python scripts/md_to_docx.py

echo "完成！"
```

