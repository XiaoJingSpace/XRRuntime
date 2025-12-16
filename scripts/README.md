# 脚本说明

## md_to_docx.py

Markdown 转 Word 文档转换脚本。

### 功能

- 将 Markdown 章节文件转换为 Word 文档
- 应用标准技术书籍格式
- 合并所有章节为一个完整文档
- 自动处理格式（标题、段落、列表、代码块等）

### 使用方法

```bash
python scripts/md_to_docx.py
```

### 输出

- 输出文件: `book/XRRuntime框架设计与开发实践指南.docx`
- 如果原文件被占用，会保存为: `book/XRRuntime框架设计与开发实践指南_temp.docx`

### 格式处理

- **标题**: 自动识别并应用格式
- **列表**: 支持有序和无序列表
- **代码块**: 等宽字体，灰色背景
- **加粗标记**: 自动去掉 `**` 标记
- **行内代码**: 保留 `code` 格式

### 依赖

- Python 3.6+
- python-docx 库

### 安装依赖

```bash
pip install python-docx
```

