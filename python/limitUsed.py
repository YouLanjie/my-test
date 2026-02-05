#!/usr/bin/env python
# Created:2026.01.24

from dataclasses import dataclass
from pathlib import Path
import sys
import time
import threading
import argparse
import tempfile
import subprocess
import copy
import json
import logging
import pprint
import hashlib
import os

import urllib.request
import urllib.error

DISABLE_AUPD = False
DISABLE_LOCK = False
DISABLE_PYW  = False
EXE = Path(sys.argv[0]).resolve()
logger = logging.getLogger(__name__)

def merge_dict(old:dict, new:dict, warn=False, prefix=""):
    """将new词典的内容依照old的结构和类型合并"""
    if prefix:
        prefix+="/"
    for k in new:
        if ".*" in old:
            old[k] = copy.deepcopy(old[".*"])
        if k not in old:
            if warn:
                logger.warning("[WARN] 词典'/%s'不存在关键词'%s'",
                               prefix, k)
            continue
        if not isinstance(new[k], type(old[k])):
            if warn:
                logger.warning("[WARN] 路径'/%s%s'的类型应该是%s而非%s",
                               prefix, k, type(old[k]), type(new[k]))
            continue
        if isinstance(old[k], dict):
            merge_dict(old[k], new[k], warn=warn, prefix=prefix+k)
            continue
        old[k] = new[k]
    if ".*" in old:
        old.pop(".*")

def read_text(file: Path) -> str:
    if not file.is_file():
        return ""
    try:
        b = file.read_bytes()
    except PermissionError:
        return ""
    s = ""
    for i in ["utf8", "gbk", "utf32", "utf16"]:
        try:
            s = b.decode(i)
            break
        except UnicodeDecodeError:
            pass
    return s

def download_file(urls:list, savefile) -> bytes:
    """从网络上下载文件"""
    req = urllib.request
    errors = urllib.error
    ret = None
    url = ""
    for url in urls:
        try:
            ret = req.urlopen(url)
            if ret.getcode() == 200:
                break
            ret = None
        except errors.HTTPError as e:
            logger.warning("链接不可用: %s (%s)", url, e)
        except errors.URLError as e:
            logger.warning("域名无法访问: %s (%s)", url, e)
    if not ret:
        logger.info("[INFO] 下载失败 - '%s'", url)
        return b""
    content = ret.read()
    if savefile:
        try:
            Path(savefile).write_bytes(content)
        except (OSError, PermissionError) as e:
            logging.error("while saving '%s': %s", savefile, e)
    return content

def auto_update(protect: "Protect"):
    """自动更新函数"""
    if DISABLE_AUPD:
        return
    if time.time() - EXE.stat().st_mtime < 60*60:
        return
    local_content = EXE.read_bytes()
    logging.info("正在尝试自动更新")
    path = "python/limitUsed.py"
    url = f"raw.githubusercontent.com/youlanjie/my-test/refs/heads/main/{path}"
    urls = [f"http://ghfast.top/{url}",
            f"https://youlanjie.github.io/lib/{path}",
            f"http://{url}"]
    content = download_file(urls, "")
    if not content or content == local_content:
        return
    obj = protect.filelist[EXE]
    obj.keep_alive = False
    if obj.pth:
        obj.pth.join()
    EXE.write_bytes(content)
    logging.info("已自动更新")
    protect.start()
    return

def run_python(cmd:list):
    """运行子进程"""
    python = Path(sys.executable)
    if os.name == "nt" and not DISABLE_PYW:
        python = Path(sys.executable).parent/"pythonw.exe"
        if not python.is_file():
            python = Path(sys.executable)
    cmd = [python]+cmd
    logger.debug("运行命令：%s", cmd)
    try:
        subprocess.Popen(cmd)
    except (FileNotFoundError, subprocess.CalledProcessError) as e:
        logger.error("运行命令时遇到错误：%s", e)
        raise e

def check_file(f:Path, count=1000, timeout:int|float=3) -> bool:
    """持续一段时间检测文件是否存在过"""
    for _ in range(count):
        try:
            if f.exists() and time.time()-f.stat().st_mtime<timeout:
                return True
        except FileNotFoundError:
            pass
        time.sleep(0.001)
    return False

class ProgramLock:
    """程序间互斥锁"""
    tempdir = Path(tempfile.gettempdir())
    locks = [tempdir/f"{EXE.stem}_lock1.lock",
             tempdir/f"{EXE.stem}_lock2.lock"]
    def __init__(self, enable = True, is_sub = False) -> None:
        enable = enable and not DISABLE_LOCK
        self.enable = enable
        self.is_sub = bool(is_sub)
        self.flag = True
        self.now = self.locks[is_sub]
        self.another = self.locks[not is_sub]

        if not enable:
            return

        if check_file(self.now):
            logger.warning("[%s进程] 互斥锁存在，退出程序",
                           "主副"[self.is_sub])
            sys.exit()
        self.pth = threading.Thread(target=self.lock)
        self.pth.start()
    def lock(self):
        """创建并保持锁文件存在"""
        if not self.enable:
            logger.info("锁被禁用，上锁取消")
            return
        def _check(f:Path) -> bool:
            try:
                if f.is_file() and f.stat().st_size != 0:
                    return True
            except FileNotFoundError as e:
                logger.error("[%s进程] 文件错误: %s",
                             "主副"[self.is_sub], e)
                self.flag = False
                sys.exit(0)
            return False
        logger.info("创建锁文件 (%s)", self.now)
        self.now.touch()
        while self.flag:
            self.now.touch()
            if _check(self.now):
                break
            if not check_file(self.another, timeout=0.5):
                self.exec()
            time.sleep(0.1)
        if self.another.is_file():
            self.another.write_bytes(b"quit")
        self.now.unlink(True)

        logger.info("[%s进程] 退出程序并移除进程锁", "主副"[self.is_sub])
        self.flag = False
    def stop(self):
        """退出"""
        self.flag = False
        if not self.enable:
            return
        self.pth.join()
    def exec(self):
        """保活"""
        if not self.enable:
            return
        logger.info("[%s进程] 启动%s进程",
                    "主副"[self.is_sub], "副主"[self.is_sub])
        if self.is_sub:
            run_python([i for i in sys.argv if i not in ("-d", "--daemon")])
        else:
            run_python(sys.argv+["-d"])
        time.sleep(1)

@dataclass
class FileObject:
    """保存受保护文件相关信息"""
    file: Path
    sha256 : str = ""
    read_in_mem: bool = True
    keep_alive = True
    pth: threading.Thread|None = None

class Protect:
    """文件保护系统"""
    def __init__(self, cfg=EXE.parent/"filelist.json") -> None:
        self.cfg = {
                "maxsize":1024*1024*200,
                "filelist": {
                    ".*" : {
                        "read_in_mem":True,
                        "sha256":"",
                        "link":"",
                        "archpath":""
                        }
                    },
                "activities": {
                    ".*" : {
                        "cmd": [],
                        "time":"",
                        "type":"exec_once",
                        }
                    }
                }
        self.cfg_f = cfg if cfg and cfg.is_file() else None
        self.workdir = self.cfg_f.parent if self.cfg_f else EXE.parent
        self.filelist : dict[Path, FileObject] = {
                EXE: FileObject(EXE)
                }
        self.memsize = 0

        self._read_cfg()
        self.update_filelist()
        logger.info("保护系统初始化完成")
    def _read_cfg(self):
        if not self.cfg_f or not self.cfg_f.is_file():
            return
        logger.info("找到配置文件: %s", self.cfg_f)
        cfg = {}
        try:
            cfg = json.loads(read_text(self.cfg_f) or "{}")
        except json.JSONDecodeError as e:
            logger.warning(str(e))
        if not isinstance(cfg, dict):
            logger.warning("Err type: %s", type(cfg))
            cfg = {}
        merge_dict(self.cfg, cfg, True)
    def _get_filelist(self):
        """获取文件列表"""
        filelist = {(self.workdir/i).resolve():self.cfg["filelist"][i]
                    for i in self.cfg["filelist"] \
                            if (self.workdir/i).exists()}
        newfl = {}
        for i in filelist:
            if i.is_dir():
                # logger.info("查找目录: %s", i)
                newfl.update({j:filelist[i] for j in i.glob("**/*") if j.is_file()})
            else:
                newfl[i] = filelist[i]
        filelist = newfl
        if EXE.parent/"message.log" in filelist:
            filelist.pop(EXE.parent/"message.log")
        li = [EXE]
        if self.cfg_f:
            li.append(self.cfg_f)
        li += sorted(set(filelist)-set(li), key=lambda x:x.stat().st_size)
        newfl = {}
        for i in li:
            if i not in filelist:
                newfl[i] = FileObject(i)
            else:
                newfl[i] = filelist[i]
        return newfl
    def update_filelist(self):
        """更新待监测文件列表"""
        filelist = self._get_filelist()
        for i,v in filelist.items():
            if i in self.filelist:
                self.filelist[i].keep_alive = True
                continue
            if isinstance(v, FileObject):
                self.filelist[i] = v
            else:
                self.filelist[i] = FileObject(i,
                                              read_in_mem=v["read_in_mem"],
                                              sha256=v["sha256"])
        for i in set(self.filelist)-set(filelist):
            self.filelist[i].keep_alive = False
    def start(self):
        """启动监测线程"""
        for k,v in self.filelist.items():
            if v.pth or not v.keep_alive:
                continue
            pth = threading.Thread(
                    target=self.protect_file,
                    args=[k])
            v.pth = pth
            pth.start()
    def protect_file(self, file:Path):
        """保护文件"""
        if not file.is_file():
            return
        obj = self.filelist.get(file)
        if not obj:
            return
        logger.info("启动新文件保护线程:\n%s",
                    pprint.pformat(obj))

        if not obj.read_in_mem:
            with file.open("rb"):
                while obj.keep_alive and obj.file.is_file():
                    time.sleep(1)
            logger.info("退出保护线程: %s (存在：%s)", file, file.is_file())
            return

        if file == EXE:
            threading.Thread(target=auto_update, args=[self]).start()

        parent = file.parent
        content = file.read_bytes()
        if self.memsize + len(content) > self.cfg["maxsize"]:
            logger.info("因超大小限制(%d/%d)而退出: %s",
                        self.memsize, self.cfg["maxsize"], file)
            return
        sha256 = hashlib.sha256(content).hexdigest()
        if obj.sha256 and obj.sha256 != sha256:
            logger.info("因与预期sha256不等而退出: %s", file)
            return
        stat = file.stat()
        self.memsize += len(content)
        while obj.keep_alive:
            if file.is_file() and file.stat() == stat:
                time.sleep(1)
                continue
            if not parent.is_dir():
                parent.mkdir(parents=True,exist_ok=True)
            if file.is_file() and file.read_bytes() == content:
                stat = file.stat()
                continue
            tips = "修改" if file.is_file() else "删除"
            file.write_bytes(content)
            file.chmod(stat.st_mode)
            stat = file.stat()
            logger.info("恢复被%s的文件 '%s'", tips, file)
            if file == EXE:
                logger.info("程序本体被%s，更新文件列表", tips)
                self.update_filelist()
                self.start()
        if file in self.filelist:
            self.filelist[file].pth = None
            self.filelist[file].keep_alive = False
        self.memsize -= len(content)
        logger.info("退出保护线程: %s", file)
    def quit(self):
        """退出程序"""
        threadlist = [i.pth for _,i in self.filelist.items() if i]
        for _,v in self.filelist.items():
            v.keep_alive = False
        for i in threadlist:
            if not i:
                continue
            i.join()
    def print_config_template(self):
        """打印模板json"""
        print(json.dumps(self.cfg, ensure_ascii=False, indent='\t'))

class ScreenLock:
    inp_cfg = """MBTN_LEFT ignore
MBTN_LEFT_DBL set osc yes
MBTN_RIGHT ignore
WHEEL_UP ignore
WHEEL_DOWN ignore
MOUSE_MOVE ignore"""
    def __init__(self) -> None:
        self.avaliable = False
        try:
            subprocess.run(["mpv","-h"], check=True)
            self.avaliable = True
        except (FileNotFoundError, subprocess.CalledProcessError) as e:
            print(f"[WARN] {e}")
    def run(self, file:Path):
        if not self.avaliable:
            return
        if not file.is_file():
            return
        # subprocess.run(["nircmdc.exe", "mutesysvolume", "0"])
        # subprocess.run(["nircmdc.exe", "setsysvolume", "28000"])
        subprocess.run(["mpv", file, "--fs", "--no-osc", f"--input-conf={file}", "--input-doubleclick-time=50"], check=False)


def parse_arguments() -> argparse.Namespace:
    """解释参数"""
    workdir = EXE.parent
    parser = argparse.ArgumentParser(description='python')
    parser.add_argument('-i', '--input', type=Path, default=workdir/"filelist.json", help='config文件')
    parser.add_argument("-p", "--print-config", action="store_true", help="打印配置文件模板")
    parser.add_argument("-d", "--daemon", action="store_true", help="运行保活用副线程")
    parser.add_argument("-n", "--no-lock", action="store_true", help="无锁")
    parser.add_argument("-v", "--verbose", action="store_true", help="打印调试信息")
    args = parser.parse_args()
    return args

def config_logging(file_name:Path,console_level:int=logging.INFO, file_level:int=logging.DEBUG):
    """定义日志配置"""
    file_handler = logging.FileHandler(file_name, mode='a', encoding="utf8")
    file_handler.setFormatter(logging.Formatter(
        '[%(asctime)s %(levelname)s] [PID:%(process)d] [%(module)s:%(lineno)d %(funcName)s] %(message)s',
        datefmt="%Y/%m/%d %H:%M:%S"
        ))
    file_handler.setLevel(file_level)

    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setFormatter(logging.Formatter(
        '[%(asctime)s %(levelname)s] [PID:%(process)d] %(message)s',
        datefmt="%Y/%m/%d %H:%M:%S"
        ))
    console_handler.setLevel(console_level)

    handlers : list = [file_handler]
    # fixed: 'NoneType' object has no attribute 'isatty'
    if sys.stdout and sys.stdout.isatty():
        handlers.append(console_handler)

    logging.basicConfig(
        level=min(console_level, file_level),
        handlers=handlers,
        )

def main():
    """主函数"""
    args = parse_arguments()
    if os.name == "nt" and not sys.executable.endswith("pythonw.exe") and not DISABLE_PYW:
        run_python(sys.argv)
        return
    config_logging(file_name=EXE.parent/"message.log",
                   console_level=logging.DEBUG if args.verbose else logging.INFO)
    logger.debug("程序启动(后台:%s)", args.daemon)

    lock = ProgramLock(not args.no_lock, args.daemon)
    # 副线程
    if args.daemon:
        while lock.flag:
            try:
                lock.pth.join()
            except (RuntimeError, KeyboardInterrupt) as e:
                logger.warning("[%s进程] Exception: %s",
                               "主副"[args.daemon], e)
        return

    # 主线程
    protect = Protect(args.input)
    if args.print_config:
        protect.print_config_template()
        return

    protect.start()
    try:
        count = 0
        while lock.flag:
            time.sleep(0.5)
            count += 1
            if count % 20 == 0:
                protect.update_filelist()
                protect.start()
    except KeyboardInterrupt:
        logger.info("退出程序")
        protect.quit()
        lock.stop()
        return
    protect.quit()
    lock.stop()

if __name__ == "__main__":
    main()
