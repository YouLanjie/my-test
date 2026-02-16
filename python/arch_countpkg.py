#!/usr/bin/env python
# Created:2026.02.16

import re
import tarfile
import argparse
import pickle
from dataclasses import dataclass
from pathlib import Path
import pprint

@dataclass
class Package:
    """pkg info"""
    name:str
    repo:str
    desc:str
    depends:tuple[str, ...] = tuple()
    optdepends:tuple[str, ...] = tuple()
    makddepends:tuple[str, ...] = tuple()
    provides:tuple[str, ...] = tuple()
    groups:tuple[str, ...] = tuple()
    count:int = 0

def get_data(db_path: Path) -> tuple[list[Package], dict[str, list[Package]]]:
    if not db_path.is_dir():
        print(f"'{db_path}' is not dir")
        return ([], {})
    dbs = [i for i in db_path.glob("**/*.db") if i.is_file()]
    if not dbs:
        print("No db files found")
        return ([], {})
    all_pkgs : list[Package] = []
    pkgs : dict[str, list[Package]] = {}
    for repo in dbs:
        data = []
        with tarfile.open(repo, "r:*") as tar:
            files = [tar.extractfile(m) for m in tar.getmembers() \
                    if m.isfile and m.name.endswith("desc")]
            data = [f.read().decode().split("\n\n") for f in files if f]
            data = [{i.splitlines()[0]:tuple(i.splitlines()[1:]) for i in d \
                    if i and i.splitlines()} for d in data]
        for pkg in data:
            package = Package("|".join(pkg.get("%NAME%") or []),
                              repo.stem,
                              "\n".join(pkg.get("%DESC%") or []),
                              pkg.get("%DEPENDS%") or tuple(),
                              pkg.get("%OPTDEPENDS%") or tuple(),
                              pkg.get("%MAKEDEPENDS%") or tuple(),
                              pkg.get("%PROVIDES%") or tuple(),
                              pkg.get("%GROUPS%") or tuple(),
                              )
            all_pkgs.append(package)
            tags = {"|".join(re.split(r"[>=<]+", j)[:1]) \
                    for i in [pkg.get("%NAME%"),
                              pkg.get("%GROUPS%"),
                              pkg.get("%PROVIDES%")]\
                    if i for j in i}
            for t in tags:
                if t not in pkgs:
                    pkgs[t] = []
                pkgs[t].append(package)
    return all_pkgs, pkgs

def main():
    parser = argparse.ArgumentParser(description="simple tool about pacman")
    parser.add_argument("path", type=Path, nargs="?",
                        const="/var/lib/pacman/sync/",
                        help="db file search path")
    parser.add_argument("-r","--read",type=Path, help="读取pickle文件")
    parser.add_argument("-w","--write",type=Path, help="写入pickle文件")
    parser.add_argument("-s","--search", help="搜索")
    args = parser.parse_args()
    db_path = Path(args.path or "/var/lib/pacman/sync/")
    if not db_path.is_dir():
        print(f"'{db_path}' is not dir")
        return
    if args.read.is_file():
        data = pickle.loads(args.read.read_bytes())
        if not isinstance(data, tuple) or len(data) != 2:
            return
        all_pkgs, pkgs = data
    else:
        all_pkgs, pkgs = get_data(db_path)
        if not all_pkgs:
            return
    if args.write:
        args.write.write_bytes(pickle.dumps(tuple([all_pkgs, pkgs])))

    pkg_not_found={}
    for pkg in all_pkgs:
        for t in pkg.depends:
            t = "|".join(re.split(r"[>=<]+", t)[:1])
            if t not in pkgs:
                name = f"{pkg.repo}/{pkg.name}"
                if name not in pkg_not_found:
                    pkg_not_found[name] = []
                pkg_not_found[name].append(t)
                continue
            for p in pkgs[t]:
                p.count+=1
    if pkg_not_found:
        print("无法满足的依赖:")
        pprint.pp(pkg_not_found)
    all_pkgs=sorted(all_pkgs, key=lambda x:x.count, reverse=True)
    if args.search:
        try:
            pattern = re.compile(args.search, re.I)
            all_pkgs = [i for i in all_pkgs \
                    if pattern.search(f"{i.name}: ({i.provides}) {i.desc}")]
        except re.error as e:
            print(f"[WARN] re.error: {e}")

    # pprint.pp(all_pkgs[:30])
    print(f"[INFO] {len(all_pkgs)} in total")
    for i in all_pkgs[:100]:
        pprint.pp((i.count, i.name, i.provides[:5], i.desc))

if __name__ == "__main__":
    main()
