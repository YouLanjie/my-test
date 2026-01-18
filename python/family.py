#!/usr/bin/env python
# data structure of 人物关系

from pathlib import Path
import json
import argparse

try:
    from . import pytools
except ImportError:
    import pytools

class People:
    """many人"""
    cfg_template = {".*":{
        "male":True,
        "comment":"",
        "child":[],
        "father":None,
        "mother":None,
        "partner":set(),
        "level":1,
        "fixed":False,
        }}
    def __init__(self, source:Path|str|bytes|dict):
        self.cfg = self.cfg_template.copy()
        cfg = {}
        if not isinstance(source, dict):
            if isinstance(source, Path):
                if source.is_file():
                    source = source.read_bytes()
                else:
                    source = "{}"
            try:
                cfg = json.loads(source or "{}")
            except json.JSONDecodeError as e:
                pytools.print_err(str(e))
        else:
            cfg = source
        if not isinstance(cfg, dict):
            pytools.print_err(f"Err type: {type(cfg)}")
            cfg = {}
        pytools.merge_dict(self.cfg, cfg, True)
        self._build_relationship()
    def _build_relationship(self) -> dict:
        data = self.cfg
        for i in data:
            for j in data[i]["child"]:
                if j not in data:
                    continue
                data[j]["father" if data[i]["male"] else "mother"] = i
        for i in data:
            if data[i]["father"] and data[i]["mother"]:
                data[data[i]["father"]]["partner"].add(data[i]["mother"])
                data[data[i]["mother"]]["partner"].add(data[i]["father"])
        for i in data:
            self._build_generation(i)
        offset = min([i["level"] for _,i in data.items()] or [0])
        for i in data:
            data[i]["level"] -= (offset-1)
        return data
    def _build_generation(self, now:str, level=1, prefix="/"):
        if now not in self.cfg:
            return
        if self.cfg[now]["fixed"]:
            return
        parent = self.cfg[now]["father"] or self.cfg[now]["mother"]
        if parent in self.cfg and self.cfg[parent]["fixed"] and\
                level != self.cfg[parent]["level"]+1:
            level = self.cfg[parent]["level"]+1
        self.cfg[now]["level"] = level
        self.cfg[now]["fixed"] = True
        # import rich
        # rich.print(f"[INFO] SET '{now}' to lv:{level} (path:'{prefix}')")

        for j in self.cfg[now]["partner"]:
            self._build_generation(j, level=level, prefix=prefix+f"({now})")
        for j in  ("father", "mother"):
            if not self.cfg[now][j]:
                continue
            self._build_generation(self.cfg[now][j], level=level-1, prefix=f"{prefix}../({now})")
        for i in self.cfg[now]["child"]:
            self._build_generation(i, level=level+1, prefix=f"{prefix}{now}/")
    def to_levels(self) -> dict:
        data = self.cfg
        levels = {}
        for k,v in data.items():
            if v["level"] not in levels:
                levels[v["level"]] = {}
            levels[v["level"]][k] = v
        levels = {i:levels[i] for i in sorted(levels)}
        return levels
    def print_config_template(self):
        """打印模板json"""
        cfg = {".*":{ "male":True, "comment":"", "child":[], }}
        pytools.merge_dict(cfg, self.cfg)
        print(json.dumps(cfg, ensure_ascii=False, indent='\t'))

def main():
    args = parse_arg()
    config = People(args.config)
    if args.print_config:
        config.print_config_template()
        return
    try:
        # __import__('rich').print(config.cfg)
        # __import__('rich').print(config.to_levels())
        cfg = {".*":{"level":0, "male":True}}
        pytools.merge_dict(cfg, {i:config.cfg[i] for i in
                                 sorted(config.cfg, key=lambda x:config.cfg[x]["level"])})
        __import__('rich').print(cfg)
    except ModuleNotFoundError:
        __import__('pprint').pprint(config.cfg)
        __import__('pprint').pprint(config.to_levels())
        cfg = {".*":{ "male":True, "comment":"", "child":[], "level":0 }}
        pytools.merge_dict(cfg, {i:config.cfg[i] for i in
                                 sorted(config.cfg, key=lambda x:config.cfg[x]["level"])})
        __import__('pprint').pprint(cfg)

def parse_arg() -> argparse.Namespace:
    """解释参数"""
    parser=argparse.ArgumentParser(description="gen")
    parser.add_argument("-c", "-i", "--config", type=Path, default=Path("config.json"),
                        help="指定配置文件(不可用时命令行配置才有用)")
    parser.add_argument("-C", "--print-config", action="store_true", help="打印配置文件模板")
    return parser.parse_args()

if __name__ == "__main__":
    main()
