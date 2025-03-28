#!/usr/bin/python

"""
本程序由ai生成
"""

import os
import re
import sys
from fuzzywuzzy import fuzz
from collections import defaultdict

class UnionFind:
    def __init__(self, size):
        self.parent = list(range(size))
    
    def find(self, x):
        if self.parent[x] != x:
            self.parent[x] = self.find(self.parent[x])
        return self.parent[x]
    
    def union(self, x, y):
        fx = self.find(x)
        fy = self.find(y)
        if fx != fy:
            self.parent[fy] = fx

def find_music_files(directory, extensions=None):
    if extensions is None:
        extensions = ['.mp3', '.flac', '.wav', '.m4a', '.ogg', '.aac']
    music_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            if any(file.lower().endswith(ext) for ext in extensions):
                music_files.append(os.path.join(root, file))
    return music_files

def preprocess_filename(filename):
    base = os.path.basename(filename)
    name = os.path.splitext(base)[0]
    name = re.sub(r'[\(\[](.*?)[\)\]]', '', name)
    name = re.sub(r'[-–_&+]+', ' ', name)
    keywords = ['live', 'remastered', 'version', 'flac', '320k', '24bit',
                'official', 'video', 'audio', 'album', 'edit', 'demo',
                'acoustic', 'cover', 'remix', 'instrumental']
    for kw in keywords:
        name = re.sub(r'\b' + re.escape(kw) + r'\b', '', name, flags=re.IGNORECASE)
    name = name.lower().strip()
    name = re.sub(r'\s+', ' ', name)
    return name

def main(directory, threshold=80):
    files = find_music_files(directory)
    processed = [(f, preprocess_filename(f)) for f in files]
    n = len(processed)
    uf = UnionFind(n)
    
    # 预计算所有处理后的名称
    names = [p[1] for p in processed]
    
    # 比较并合并相似项
    for i in range(n):
        for j in range(i+1, n):
            if fuzz.ratio(names[i], names[j]) >= threshold:
                uf.union(i, j)
    
    # 分组结果
    groups = defaultdict(list)
    for idx in range(n):
        groups[uf.find(idx)].append(processed[idx][0])
    
    # 输出分组结果
    duplicates_found = False
    for group in groups.values():
        if len(group) > 1:
            duplicates_found = True
            print("可能的重复文件组：")
            for file in group:
                print(f"  • {file}")
            print()
    
    if not duplicates_found:
        print("未找到重复文件")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("使用方法：python3 find_duplicates.py <目录> [阈值]")
        print("示例：python3 find_duplicates.py ~/Music 85")
        sys.exit(1)
    
    directory = sys.argv[1]
    threshold = int(sys.argv[2]) if len(sys.argv) >=3 else 80
    
    if not os.path.isdir(directory):
        print(f"错误：'{directory}' 不是有效目录")
        sys.exit(1)
    
    main(directory, threshold)
