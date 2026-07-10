#!/usr/bin/env python
# Created:2026.07.10
"""
检查目录下是否有重复文件(just for fun)
最好还是自己去找rdfind这样的工具吧
"""

import sys
import time
import pickle
import hashlib
import argparse
from dataclasses import dataclass
from pathlib import Path

@dataclass
class File:
    """文件数据"""
    path:Path
    size:int = 0
    md5:str = ""
    def get_file_hash(self, chunk_size=1024*1024*5):
        """计算文件的MD5值，分块读取避免内存爆炸"""
        if self.md5:
            return self.md5
        hasher = hashlib.md5()
        with open(self.path, 'rb') as f:
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break
                hasher.update(chunk)
        self.md5 = hasher.hexdigest()
        return self.md5

@dataclass
class Database:
    """数据集(pickle保存读取用)"""
    files:list[File]
    hashes:dict[str, list[File]]
    hashed:bool = False
    def filter_by_size(self):
        """获取文件大小一致的文件列表"""
        sizegroup :dict[int, list[File]] = {}
        for f in self.files:
            if f.size not in sizegroup:
                sizegroup[f.size] = []
            sizegroup[f.size].append(f)
        sizegroup = {k:v for k,v in sizegroup.items() if len(v) > 1}
        return sizegroup
    def duplicated(self):
        """计算重复文件"""
        if self.hashed:
            return
        sizegroup = self.filter_by_size()
        totallen = sum(len(v) for _,v in sizegroup.items())
        totalsize = sum(sum(i.size for i in v) for _,v in sizegroup.items())
        print(f"计算md5排查中，待计算文件总大小{totalsize/1024/1024:.2f}MiB",
              file=sys.stderr)
        barwidth = 35
        count = 0
        size = 0
        t1 = time.monotonic()
        t2 = t1
        for _,fl in sizegroup.items():
            for f in fl:
                if f.get_file_hash() not in self.hashes:
                    self.hashes[f.md5] = []
                self.hashes[f.md5].append(f)
                size += f.size
                count += 1
            t2 = time.monotonic()
            eta = (totalsize-size)/size*(t2-t1) if size else float('inf')
            prog = size/totalsize
            bartext = f"[{"#"*int(barwidth*prog)+" "*int(barwidth*(1-prog))}] {prog*100:5.1f}% "
            print(f"\r{bartext} ({count}/{totallen}) took {t2-t1:.0f}s ETA {eta:.1f}s ",
                  end='\r', file=sys.stderr)
        self.hashed = True
        print(f"\n计算总耗时：{t2-t1:.1f}s", file=sys.stderr)

def parse_arg():
    """读参数"""
    parser = argparse.ArgumentParser(description="检查目录下是否有重复文件")
    parser.add_argument("path", nargs="?", default="./", help="路径")
    parser.add_argument("-c", "--cache", default=".cache_uniqf.pickle", help="pickle缓存文件")
    args = parser.parse_args()
    return {"path":args.path, "cache":args.cache}

def main():
    """主函数"""
    args = parse_arg()
    pickf = Path(args["cache"]) if args["cache"] else None
    inpdir = Path(args["path"])
    if not inpdir.is_dir():
        print(f"[WARN] 不是文件夹: {inpdir}", file=sys.stderr)
        return

    if pickf and pickf.is_file():
        print(f"加载pickle文件({pickf})", file=sys.stderr)
        db : Database = pickle.loads(pickf.read_bytes())
    else:
        print(f"读取文件列表并记录大小: {inpdir}", file=sys.stderr)
        db = Database([File(i, i.stat().st_size)
                       for i in inpdir.glob("**/*") if i.is_file()], {})
        if pickf:
            print(f"保存到pickle文件({pickf})", file=sys.stderr)
            pickf.write_bytes(pickle.dumps(db))

    print(f"总共{len(db.files)}个文件，总大小{sum(i.size for i in db.files)/1024/1024:.2f}MiB")

    sizegroup = db.filter_by_size()
    print(f"共有{len(sizegroup)}组等大小文件，"
          f"其中{len([k for k in sizegroup if k == 0])}个空组，"
          f"最长组为{max(len(v) for k,v in sizegroup.items() if k != 0)}")

    if not db.hashed:
        db.duplicated()
        if pickf:
            print(f"重新保存结果到pickle文件({pickf})", file=sys.stderr)
            pickf.write_bytes(pickle.dumps(db))
    duplicated = {v[0].md5:v for _,v in db.hashes.items() if len(v) > 1}

    for md5,files in duplicated.items():
        print(f"MD5({md5[:5]}...{md5[-5:]}):")
        for f in files:
            print(f"- {f.path}")

if __name__ == "__main__":
    main()
