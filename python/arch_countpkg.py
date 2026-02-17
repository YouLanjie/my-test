#!/usr/bin/python
# Created:2026.02.16

import re
import tarfile
import argparse
import pickle
from dataclasses import dataclass
from pathlib import Path
import pprint

try:
    import rich
    import rich.table
except ModuleNotFoundError:
    rich = None

try:
    import tqdm
except ModuleNotFoundError:
    tqdm = None

@dataclass
class Package:
    """arch pacman软件包数据"""
    name:str
    repo:str
    desc:str
    depends:tuple[str, ...] = tuple()
    optdepends:tuple[str, ...] = tuple()
    makedepends:tuple[str, ...] = tuple()
    provides:tuple[str, ...] = tuple()
    groups:tuple[str, ...] = tuple()
    count:int = 0

class Counter:
    """data structer"""
    def __init__(self, db_path: Path) -> None:
        self.db_path = db_path
        self.pkg_list = []
        if not db_path.is_dir():
            print(f"'{db_path}' 不是文件夹")
            return
        self.pkg_list = self._get_data()
        self.provides_list = self._get_provides_list()
        self.is_counted = False
    def _get_data(self) -> list[Package]:
        """从指定路径获取数据库"""
        db_path = self.db_path
        if not db_path.is_dir():
            print(f"'{db_path}' 不是文件夹")
            return []
        dbs = [i for i in db_path.glob("**/*.db") if i.is_file()]
        if not dbs:
            print(f"未在'{db_path}/' 找到数据库文件")
            return []
        all_pkgs : list[Package] = []
        for repo in dbs:
            data = []
            with tarfile.open(repo, "r:*") as tar:
                files = [tar.extractfile(m) for m in tar.getmembers() \
                        if m.isfile and m.name.endswith("desc")]
                data = [f.read().decode().split("\n\n") for f in files if f]
                data = [{i.splitlines()[0]:tuple(i.splitlines()[1:]) \
                        for i in d \
                        if i and i.splitlines()} for d in data]
            for pkg in data:
                package = Package(
                        "|".join(pkg.get("%NAME%") or []),
                        repo.stem,
                        "\n".join(pkg.get("%DESC%") or []),
                        pkg.get("%DEPENDS%") or tuple(),
                        pkg.get("%OPTDEPENDS%") or tuple(),
                        pkg.get("%MAKEDEPENDS%") or tuple(),
                        pkg.get("%PROVIDES%") or tuple(),
                        pkg.get("%GROUPS%") or tuple()
                        )
                all_pkgs.append(package)
        return all_pkgs
    def _get_provides_list(self) -> dict[str, list[Package]]:
        """返回depends的查询列表"""
        pkgs : dict[str, list[Package]] = {}
        for p in self.pkg_list:
            tags = {"|".join(re.split(r"[>=<]+", j)[:1]) \
                    for i in [[p.name], p.provides]\
                    if i for j in i}
            for t in tags:
                if t not in pkgs:
                    pkgs[t] = []
                pkgs[t].append(p)
        return pkgs
    def count_diret_depend(self):
        """直接依赖计数"""
        if self.is_counted:
            return
        pkg_not_found={}
        for pkg in self.pkg_list:
            for t in pkg.depends:
                t = "|".join(re.split(r"[>=<]+", t)[:1])
                if t not in self.provides_list:
                    name = f"{pkg.repo}/{pkg.name}"
                    if name not in pkg_not_found:
                        pkg_not_found[name] = []
                    pkg_not_found[name].append(t)
                    continue
                for p in self.provides_list[t]:
                    p.count+=1
        if pkg_not_found:
            print("无法满足的依赖:")
            pprint.pp(pkg_not_found)
        self.is_counted = True


def _count_all_depend(pkg:Package,
                      provides_list:dict[str, list[Package]],
                      pkg_not_found:dict[str,list[str]]|None = None,
                      visit_stack:list[str]|None = None,
                      accced:set[str]|None = None):
    if visit_stack is None:
        visit_stack = []
    if pkg_not_found is None:
        pkg_not_found={}
    if accced is None:
        accced = set()

    if f"{pkg.repo}/{pkg.name}" in accced:
        return
    accced.add(f"{pkg.repo}/{pkg.name}")

    for depend in pkg.depends:
        depend = "|".join(re.split(r"[>=<]+", depend)[:1])
        if depend not in provides_list:
            name = f"{pkg.repo}/{pkg.name}"
            if name not in pkg_not_found:
                pkg_not_found[name] = []
            pkg_not_found[name].append(depend)
            continue
        for p in provides_list[depend]:
            if set([p.name]+list(p.provides)) & set(visit_stack):
                continue
            _count_all_depend(p, provides_list, pkg_not_found,
                              visit_stack+[pkg.name], accced)
            p.count+=1
    return

def count_all_depend(counter: Counter):
    """包括间接依赖的依赖计数"""
    pkg_not_found : dict[str,list[str]] = {}
    for p in tqdm.tqdm(counter.pkg_list, desc="依赖计算") if tqdm else counter.pkg_list:
        _count_all_depend(p, counter.provides_list, pkg_not_found=pkg_not_found)
    if pkg_not_found:
        print("无法满足的依赖:")
        pprint.pp(pkg_not_found)

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="simple tool about pacman")
    parser.add_argument("path", type=Path, nargs="?",
                        const="/var/lib/pacman/sync/",
                        help="db file search path")
    parser.add_argument("-f","--file",type=Path, help="读写的pickle文件")
    parser.add_argument("-s","--search", help="搜索")
    parser.add_argument("-r","--recount", help="recount pkg")
    parser.add_argument("-t","--top", type=int, default=100, help="top 100")
    parser.add_argument("-d","--diret-deps", action="store_true", help="仅计算直接依赖")
    args = parser.parse_args()
    try:
        database = args.file.read_bytes()
        print("[INFO] 读取pickle文件")
        database = pickle.loads(database)
        if not isinstance(database, Counter):
            print("[WARN] 非法的pickle文件")
            raise AttributeError
        if args.recount:
            for i in database.pkg_list:
                i.count = 0
            database.is_counted = False
    except (AttributeError,
            FileNotFoundError, IsADirectoryError,
            PermissionError, pickle.UnpicklingError) as e:
        if type(e) not in [AttributeError]:
            print(f"[WARN] {e.__class__.__name__}: {e}")
        db_path = Path(args.path or "/var/lib/pacman/sync/")
        if not db_path.is_dir():
            print(f"'{db_path}' is not dir")
            return
        database = Counter(db_path)
        if not database.pkg_list:
            print("EXIT")
            return
    if args.file and not args.file.is_dir():
        args.file.write_bytes(pickle.dumps(database))
    print(f"[INFO] 数据库加载完成(共计{len(database.pkg_list)}条)")

    if args.recount or not database.is_counted:
        if args.diret_deps:
            print("[INFO] 仅计算直接依赖")
            database.count_diret_depend()
        else:
            count_all_depend(database)
            database.is_counted = True

        if args.file and not args.file.is_dir():
            args.file.write_bytes(pickle.dumps(database))

    all_pkgs=sorted(database.pkg_list, key=lambda x:x.count, reverse=True)
    if args.search:
        try:
            pattern = re.compile(args.search, re.I)
            print(f"[INFO] 搜索目标： '{args.search}'")
            all_pkgs = [i for i in all_pkgs \
                    if pattern.search(f"{i.name}: ({i.provides}) {i.desc}")]
        except re.error as e:
            print(f"[WARN] re.error: {e}")

    # pprint.pp(all_pkgs[:30])
    top = args.top
    top = 100 if top <= 0 else top
    if not rich:
        print("[INFO] Tips: 考虑安装rich包改善输出结果")
        print(f"============PKG LIST Top{top}============")
        for i in all_pkgs[:top]:
            pprint.pp((i.count, i.name, i.provides[:5], i.desc))
        return
    print("[INFO] C: 被直接或间接依赖的次数")
    print("[INFO] Repo: "+", ".join({p.repo for p in database.pkg_list}))
    table = rich.table.Table(title=f"PKG LIST Top{top}",
                             row_styles=["", "dim"])
    table.add_column("C")
    table.add_column("Repo",max_width=5)
    table.add_column("Name", style="green", max_width=12)
    table.add_column("Description", style="italic")
    for i in all_pkgs[:top]:
        table.add_row(str(i.count), i.repo, i.name, i.desc)
    rich.print(table)

if __name__ == "__main__":
    main()
