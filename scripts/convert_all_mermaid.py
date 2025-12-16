#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
批量转换所有 Markdown 文件中的 Mermaid 图表为图片
"""

import os
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).parent))
from mermaid_to_image import extract_mermaid_from_markdown, convert_mermaid_to_image
import hashlib

def find_all_markdown_files(root_dir):
    """查找所有 Markdown 文件"""
    md_files = []
    for root, dirs, files in os.walk(root_dir):
        # 跳过 images 目录
        if 'images' in root:
            continue
        for file in files:
            if file.endswith('.md'):
                md_files.append(Path(root) / file)
    return md_files

def convert_all_mermaid_diagrams():
    """转换所有 Mermaid 图表"""
    book_dir = Path('book')
    output_dir = book_dir / 'images' / 'mermaid'
    output_dir.mkdir(parents=True, exist_ok=True)
    
    md_files = find_all_markdown_files(book_dir / 'chapters')
    md_files.extend(find_all_markdown_files(book_dir / 'appendices'))
    
    total_diagrams = 0
    converted = 0
    
    for md_file in md_files:
        print(f"\n处理: {md_file}")
        diagrams = extract_mermaid_from_markdown(md_file)
        
        if not diagrams:
            continue
        
        for name, code in diagrams:
            total_diagrams += 1
            # 使用内容哈希生成文件名
            diagram_hash = hashlib.md5(code.encode('utf-8')).hexdigest()[:8]
            diagram_name = f"mermaid_{diagram_hash}.png"
            output_path = output_dir / diagram_name
            
            # 如果已存在，跳过
            if output_path.exists():
                print(f"  [OK] {name} (已存在)")
                converted += 1
                continue
            
            print(f"  转换: {name} -> {diagram_name}")
            if convert_mermaid_to_image(code, output_path):
                print(f"    [OK] 成功")
                converted += 1
            else:
                print(f"    [FAIL] 失败")
    
    print(f"\n总计: {total_diagrams} 个图表，成功转换: {converted} 个")

if __name__ == '__main__':
    convert_all_mermaid_diagrams()

