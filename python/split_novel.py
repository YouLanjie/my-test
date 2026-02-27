#!/usr/bin/env python
# Created:2025.11.29
"""
分割全一卷的txt小说文件（一般来自轻小说文库）
转自blogs
"""

import re
import argparse
import json
import html
from pathlib import Path

import pytools

def seperate_str(pattern: re.Pattern, content: str):
    """分割字符串为dict"""
    groups : dict[tuple, list[str]] = {}
    last_match = None
    headline = ("...",)
    for match in pattern.finditer(content):
        # print(match)
        if headline in groups:
            skip = 1
            while tuple(list(headline)+[f"_fsk{skip}"]) in groups:
                skip += 1
            headline = tuple(list(headline)+[f"_fsk{skip}"])
        if last_match:
            groups[headline] = content[last_match.end():match.start()].splitlines()
        else:
            groups[("BEGGINNING",)] = content[:match.start()].splitlines()
        last_match = match
        headline = tuple(" ".join([str(i) for i in last_match.groups()]).split())
    if last_match:
        groups[headline] = content[last_match.end():].splitlines()
    # pprint.pprint({i for i in groups})
    return groups

def get_same(li1:list|tuple, li2:list|tuple) -> list:
    """获取两个列表的相同部分"""
    limit = min(len(li1), len(li2))
    while li1[:limit] != li2[:limit]:
        limit -= 1
    return list(li1[:limit])

class Tree:
    """以类似文件系统的方式建立基于词典的树状结构"""
    def __init__(self) -> None:
        self.tree : dict = {}
        self.count = 0
    def mkdir(self, path: list|tuple) -> None|dict:
        """创建路径"""
        if not path:
            return self.tree
        cwd = self.tree
        for i in path:
            if i not in cwd:
                cwd[i] = {}
                cwd = cwd[i]
            elif not isinstance(cwd[i], dict):
                skip = 1
                f = "%s_sk%d"
                while f % (i,skip) in cwd and not isinstance(f % (i,skip), dict):
                    skip += 1
                cwd[f % (i,skip)] = {}
                cwd = cwd[f % (i,skip)]
            else:
                cwd = cwd[i]
        return cwd
    def touch(self, path: list|tuple, content:None|list[str]=None) -> bool:
        """创建'文件'"""
        # print([self.count, path])
        cwd = self.mkdir(path[:-1])
        if cwd is None:
            return False
        name = path[-1]
        if name in cwd:
            skip = 1
            f = "%s_fsk%d"
            while f % (name,skip) in cwd:
                skip += 1
            name = f % (name,skip)
        if not content is None:
            cwd[name] = [self.count, content]
        elif not path[-1] in cwd:
            cwd[name] = [self.count, []]
        else:
            return False
        self.count+=1
        return True
    def get(self, path: list|tuple) -> None|dict:
        """根据路径获取对应词典"""
        if not path:
            return self.tree
        cwd = self.tree
        for i in path:
            if i not in cwd:
                return None
            if not isinstance(cwd[i], dict):
                return None
            cwd = cwd[i]
        return cwd
    def get_by_id(self, num:int, path=[]) -> None|tuple[list|tuple, None|str]:
        """根据数字id获取'文件'文本"""
        if num < 0:
            return
        if not path:
            cwd = self.tree
        else:
            cwd = self.get(path)
            if not cwd:
                return
        for k,v in cwd.items():
            if isinstance(v, list) and len(v) == 2:
                if v[0] == num:
                    return (path+[k],v[1])
            if not isinstance(v, dict):
                continue
            ret = self.get_by_id(num, path+[k])
            if ret:
                return ret
    def get_id_list(self, node:dict, prefix:None|list=None) -> dict[int, list[str]]:
        """查找dict下所有的“文件”（收集数字收否连续）"""
        if prefix is None:
            prefix = []
        s = {}
        for k,v in node.items():
            if isinstance(v, dict):
                s |= self.get_id_list(v, prefix=prefix+[k])
                continue
            s[v[0]] = prefix+[k]
        return s
    def squash(self, cwd=None, sep=" ") -> None:
        """将单独路径压缩为同一个文件夹"""
        if cwd is None:
            cwd = self.tree
        new_cwd = {}
        for k,v in cwd.items():
            if not isinstance(v, dict):
                new_cwd[k] = v
                continue
            cond = self.get_id_list(v)
            if len(cond) > 1:
                cond = [cond[i] for i in {i for i in cond if i-1 not in cond and i+1 not in cond}]
                for i in cond:
                    if len(i) > 1:
                        continue
                    new_cwd[k+sep+i[0]] = v[i[0]]
                    v.pop(i[0])
            self.squash(v, sep=sep)
            if (len(v) == 1) and k+sep+list(v)[0] not in cwd:
                new_cwd[k+sep+list(v)[0]] = v[list(v)[0]]
            else:
                new_cwd[k] = v
        cwd.clear()
        for k,v in new_cwd.items():
            cwd[k] = v
    def to_list(self, begin=0) -> list[tuple[list[str], None|str]]:
        """将树状结构参照id顺序恢复为list"""
        li = []
        count = begin
        while True:
            ret = self.get_by_id(count)
            if not ret:
                break
            count+=1
            li.append(ret)
        return li

def to_tree(groups:dict[tuple, list[str]],
            blacklist:re.Pattern) -> dict[str,list[str]]:
    """词典转成词典树"""
    tree = Tree()
    for k,v in groups.items():
        tree.touch(k, content=v)
        # tree.touch(k, content=[""])
    tree.squash()
    tree.squash()
    tree.squash()
    # pprint.pprint(tree.tree)
    # pprint.pprint(tree.to_list())

    fix_pattern = re.compile(r"\s*_[f]?sk\d+(?=\s?)")
    def fix(l:list[str]) -> list[str]:
        return [fix_pattern.sub("", i) for i in l]

    ret = {}
    last = []
    print("目录：")
    for path, s in tree.to_list(1):
        if not s:
            continue
        if last:
            same_len = len(get_same(last, path))
        else:
            same_len = 0
        file = " ".join(fix(path[:1]))
        while same_len < len(path):
            if file not in ret:
                ret[file] = []
            headline = " ".join(fix(path[same_len:same_len+1]))
            if not blacklist.search(headline):
                ret[file].append(f"{"*"*(same_len+1)} {headline}")
                print(f"{" "*(same_len)}- {headline}")
            same_len+=1
        ret[file] += s
        last = path
    return ret

def process_unicode(s: str):
    """处理unicode转义"""
    #def sub(match: re.Match):
    #    try:
    #        return chr(int(match.group(1)))
    #    except ValueError:
    #        return ""
    #return re.sub(r'&#(\d+);', sub, s)
    return html.unescape(s)

def simple_process(filename:str):
    """不分卷"""
    file = Path(filename)
    if not file.is_file():
        pytools.print_err(f"[WARN] 文件不存在:{file}")
        return
    s = pytools.read_text(file)
    s = s.splitlines()
    s = "\n\n".join([i for i in s if i])
    s = process_unicode(s)
    print(s)

def parse_arg() -> argparse.Namespace:
    """解释参数"""
    parser=argparse.ArgumentParser(description="分割全一卷的txt小说文件")
    parser.add_argument("-c", "-i", "--config", type=Path, default=Path("config.json"),
                        help="指定配置文件")
    parser.add_argument("-C", "--print-config", action="store_true", help="打印配置文件模板")
    parser.add_argument("-s", "--simple", help="只执行简单处理")
    parser.add_argument("-n", "--dry-run", action="store_true", help="不输出文件")
    return parser.parse_args()

def main():
    """主函数"""
    args = parse_arg()
    if args.simple:
        simple_process(args.simple)
        return
    cfg_f = args.config
    default_pattern = r"^[　]*(第.*卷.*)"
    default_pattern2 = r"插图"
    cfg = {"source_file":"",
           "title":"",
           "setupfile":"./setup.setup",
           "section_pattern":default_pattern,
           "ban_section_pattern":default_pattern2,
           "comment":"这里用来记录点额外信息，不会对结果产生影响",
           "process_unicode":True,
           "words":[["身分","身份"], ["计画","计划"], ["徵","征"],
                    ["乾","干"]],}
    new_cfg = {}
    if cfg_f.is_file():
        try:
            new_cfg = json.loads(cfg_f.read_bytes() or "{}")
        except json.JSONDecodeError as e:
            pytools.print_err(str(e))
    if not isinstance(new_cfg, dict):
        pytools.print_err(f"Err type: {type(cfg)}")
        new_cfg = {}
    pytools.merge_dict(cfg, new_cfg, True)

    if args.print_config:
        print(json.dumps(cfg, ensure_ascii=False, indent='\t'))
        return

    inp = cfg_f.parent/cfg["source_file"]
    if not inp.is_file():
        pytools.print_err(f"[WARN] '{inp}' is not file")
        return
    content = pytools.read_text(inp)

    if cfg["process_unicode"]:
        content = process_unicode(content)

    for w1,w2 in cfg["words"]:
        pytools.print_err(f"替换 '{w1}' 为 '{w2}' : 共计{len(re.findall(w1, content))}处")
        content = re.sub(w1, w2, content)

    content = "\n".join(content.splitlines())
    try:
        pattern = re.compile(cfg["section_pattern"], re.I+re.M)
    except re.error as e:
        pytools.print_err(f"[WARN] re compile error: {e}")
        pattern = re.compile(default_pattern, re.I+re.M)
    try:
        blacklist = re.compile(cfg["ban_section_pattern"], re.I)
    except re.error:
        blacklist = re.compile(default_pattern2, re.I)
    docs = to_tree(seperate_str(pattern, content), blacklist=blacklist)

    ind = 0
    for h1,content in docs.items():
        ind += 1
        outf = Path(f"{ind:03d}_{h1}.org")
        print(f"- 输出到文件: {outf}")
        if outf.is_dir():
            continue
        s = "\n\n".join([i for i in content if i])
        if not args.dry_run:
            outf.write_text(f"""\
#+title: {cfg['title']} {h1}
#+setupfile: {cfg['setupfile']}
""" + s, encoding="utf8")
    if args.dry_run:
        print("(Dry Run)")

if __name__ == "__main__":
    main()
