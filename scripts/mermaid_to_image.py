#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Mermaid 图表转图片工具
将 Mermaid 代码转换为 PNG 图片
"""

import os
import re
import subprocess
import requests
from pathlib import Path
import base64
import json

def convert_mermaid_to_image_online(mermaid_code, output_path):
    """
    使用 Mermaid Live Editor API 在线转换
    """
    try:
        # 方法1: 使用 mermaid.ink API
        api_url = "https://mermaid.ink/img"
        
        # 将代码编码为 base64
        encoded = base64.urlsafe_b64encode(mermaid_code.encode('utf-8')).decode('utf-8')
        url = f"{api_url}/{encoded}"
        
        # 下载图片
        response = requests.get(url, timeout=30)
        if response.status_code == 200 and len(response.content) > 1000:  # 检查是否是有效图片
            with open(output_path, 'wb') as f:
                f.write(response.content)
            return True
        
        # 方法2: 使用 mermaid.live API (备用)
        api_url2 = "https://mermaid.live/api/v1/svg"
        payload = {"code": mermaid_code}
        response2 = requests.post(api_url2, json=payload, timeout=30)
        if response2.status_code == 200:
            # SVG 转 PNG 需要额外处理，这里先保存 SVG
            svg_path = output_path.with_suffix('.svg')
            with open(svg_path, 'wb') as f:
                f.write(response2.content)
            print(f"  注意: 生成了 SVG 文件，需要手动转换为 PNG: {svg_path}")
            return False
        
        return False
    except Exception as e:
        print(f"  在线转换失败: {e}")
        return False

def convert_mermaid_to_image_cli(mermaid_code, output_path):
    """
    使用 mermaid-cli 命令行工具转换
    """
    try:
        # 创建临时 .mmd 文件
        temp_mmd = output_path.with_suffix('.mmd')
        with open(temp_mmd, 'w', encoding='utf-8') as f:
            f.write(mermaid_code)
        
        # 使用 mmdc 转换
        result = subprocess.run(
            ['mmdc', '-i', str(temp_mmd), '-o', str(output_path)],
            capture_output=True,
            timeout=30
        )
        
        # 删除临时文件
        if temp_mmd.exists():
            temp_mmd.unlink()
        
        return result.returncode == 0
    except FileNotFoundError:
        print("mermaid-cli 未安装，使用在线转换")
        return False
    except Exception as e:
        print(f"CLI 转换失败: {e}")
        return False

def convert_mermaid_to_image(mermaid_code, output_path):
    """
    转换 Mermaid 图表为图片
    优先使用 CLI，失败则使用在线 API
    """
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    # 尝试使用 CLI
    if convert_mermaid_to_image_cli(mermaid_code, output_path):
        return True
    
    # 使用在线 API
    if convert_mermaid_to_image_online(mermaid_code, output_path):
        return True
    
    return False

def extract_mermaid_from_markdown(md_file):
    """
    从 Markdown 文件中提取所有 Mermaid 图表
    返回: [(图表名称, mermaid代码), ...]
    """
    diagrams = []
    with open(md_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 匹配 ```mermaid ... ``` 代码块
    pattern = r'```mermaid\n(.*?)```'
    matches = re.finditer(pattern, content, re.DOTALL)
    
    for idx, match in enumerate(matches):
        mermaid_code = match.group(1).strip()
        # 生成图表名称
        diagram_name = f"{Path(md_file).stem}_diagram_{idx+1}"
        diagrams.append((diagram_name, mermaid_code))
    
    return diagrams

if __name__ == '__main__':
    import sys
    
    if len(sys.argv) < 2:
        print("用法: python mermaid_to_image.py <markdown_file> [output_dir]")
        sys.exit(1)
    
    md_file = Path(sys.argv[1])
    output_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else Path('book/images/mermaid')
    
    if not md_file.exists():
        print(f"文件不存在: {md_file}")
        sys.exit(1)
    
    diagrams = extract_mermaid_from_markdown(md_file)
    print(f"找到 {len(diagrams)} 个 Mermaid 图表")
    
    for name, code in diagrams:
        output_path = output_dir / f"{name}.png"
        print(f"转换: {name} -> {output_path}")
        if convert_mermaid_to_image(code, output_path):
            print(f"  ✓ 成功")
        else:
            print(f"  ✗ 失败")

