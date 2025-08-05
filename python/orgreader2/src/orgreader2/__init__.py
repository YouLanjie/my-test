#!/usr/bin/env python
from .orgreader2 import Document

def to_html(s:str) -> str:
    """将输入org转换为html"""
    if not isinstance(s, str):
        return ""
    return Document(s.splitlines()).to_html()
