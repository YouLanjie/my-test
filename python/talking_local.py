#!/usr/bin/env python
# Created:2025.10.18

from pathlib import Path
from datetime import datetime
from getpass import getpass
from typing import Callable
import uuid
import hashlib
import json
import os

def get_strtime(dt:datetime|float = datetime.now()) -> str:
    """格式化时间"""
    if isinstance(dt, float):
        dt = datetime.fromtimestamp(dt)
    if isinstance(dt, datetime):
        t = dt.strftime("%Y-%m-%d ")
        t += "一二三四五六日"[dt.weekday()]
        t += dt.strftime(" %H:%M:%S")
        return t
    return ""

class User:
    """User"""
    def __init__(self, name:str, passwd:str, note:str="") -> None:
        self.name = name
        self.passwd = passwd
        self.note = note
        # self.preference = {}
        self.timestamp = datetime.now().timestamp()
        self._id = str(uuid.uuid1())
        self.login_record = []
    @property
    def id(self) -> str:
        """UUID1，用于标识用户"""
        return self._id
    @property
    def passwd(self) -> str:
        """密码属性"""
        return self._passwd
    @passwd.setter
    def passwd(self, value: str):
        self._passwd = hashlib.md5(value.encode()).hexdigest()
    def check_passwd(self, passwd:str) -> bool:
        """检查密码"""
        passwd = hashlib.md5(passwd.encode()).hexdigest()
        return passwd == self.passwd
    def dump_data(self) -> dict:
        "将数据转出成json可用类型"
        return vars(self)
    def load_data(self, data:dict) -> None:
        "从词典载入数据"
        # if data.keys() != vars(self).keys():
            # return
        keys = set(self.__dict__.keys())
        filted_data = {k:v for k,v in data.items() if k in keys}
        self.__dict__.update(filted_data)

class Message:
    "signal message"
    def __init__(self, user:User, content:str) -> None:
        self.timestamp = datetime.now().timestamp()
        self.owner = user.id
        self.content = content
        self._id = str(uuid.uuid1())
    @property
    def id(self) -> str:
        """UUID1，用于标识该条聊天记录"""
        return self._id
    def dump_data(self) -> dict:
        "dump data for json"
        return vars(self)
    def load_data(self, data:dict) -> None:
        "load data from dict"
        keys = set(self.__dict__.keys())
        filted_data = {k:v for k,v in data.items() if k in keys}
        self.__dict__.update(filted_data)

class System:
    """系统服务"""
    def __init__(self) -> None:
        self.users : list[User] = []
        self.messages : list[Message] = []
        self.now_user : User|None = None
        self.savefile = Path("SAVEDATA.json")

        system = User("SYSTEM", "", "系统内置服务用用户")
        system.load_data({"_passwd":"db10fa5fb2467f50c7242356ee42ca86",
                          "timestamp":1753027641.0,
                          "_id":"7d87fb06-64c9-45bc-8b24-397c60d6001b"})
        self.users.append(system)
        self.system = system
        self.load()
    def print_users(self) -> None:
        """print all users"""
        for u in self.users:
            print(f"({get_strtime(u.timestamp)})[{u.name}]:\"{u.note}\"")
    def print_self(self) -> None:
        """打印登录用户自身信息"""
        if not self.now_user:
            print("[WARN] 你尚未登录")
            return
        u = self.now_user
        print(f"名字: '{u.name}'")
        print(f"注册时间: '{get_strtime(u.timestamp)}'")
        print(f"备注: '{u.note}'")
        print(f"UUID: '{u.id}'")
        print(f"密码md5值: '{u.passwd}'")
        login_record = "\n".join("> "+f"在 {get_strtime(i)} 登录过" for i in u.login_record)
        print(f"登录记录:\n{login_record}")
    def register(self) -> None:
        """add new user"""
        try:
            while True:
                name = input("[INPUT] 用户名:")
                if name in {u.name for u in self.users}:
                    print(f"[WARN] 用户 '{name}' 已存在")
                    print("[INFO] 请重试(C-d取消)")
                elif not name:
                    print("[WARN] 用户名不能为空")
                    print("[INFO] 请重试(C-d取消)")
                else:
                    break
            while True:
                passwd = getpass("[INPUT] 密码(不会显示):")
                if passwd != getpass("[INPUT] 再次输入:"):
                    print("[INFO] 请重试(C-d取消)")
                else:
                    break
        except EOFError:
            print("[INFO] 操作取消")
            return
        u = User(name, passwd)
        self.users.append(u)
        if input("[ASK] 自动登录？(Y/n)").lower() != "n":
            self.login(u)
    def login(self, user:User|None = None) -> None:
        """login"""
        if self.now_user:
            print("[WARN] 你已经登录")
            return
        if not user:
            try:
                name = input("[INPUT] 用户名:")
                li = {u.name:u for u in self.users}
                if name not in li:
                    print(f"[WARN] 用户 '{name}' 不存在")
                    return
                passwd = getpass("[INPUT] 密码(不会显示):")
                if not li[name].check_passwd(passwd):
                    print("[WARN] 密码错误")
                    return
                user = li[name]
            except EOFError:
                print("[INFO] 操作取消")
                return
        self.now_user = user
        if user.login_record:
            print(f"[INFO] 上次登录：{get_strtime(user.login_record[-1])}")
        user.login_record.append(datetime.now().timestamp())
    def logout(self):
        """登出"""
        if not self.now_user:
            print("[WARN] 你还没有登录")
        self.now_user = None
    def save(self) -> None:
        """保存数据"""
        if self.savefile.is_dir():
            print("[WARN] savefile is dir")
            return
        data = {"users":[u.dump_data() for u in self.users],
                "messages":[m.dump_data() for m in self.messages],
                "hash":["", ""]}
        data["hash"] = [hashlib.md5(str(data["users"]).encode()).hexdigest(),
                        hashlib.md5(str(data["messages"]).encode()).hexdigest()]
        # t = json.dumps(data, ensure_ascii=False, indent=2)
        t = json.dumps(data)
        self.savefile.write_text(t, encoding="utf-8")
        # print(t)
    def load(self) -> None:
        """读取数据"""
        if not self.savefile.is_file():
            print("[WARN] 数据文件不存在")
            self.syslog("[INFO] 聊天室建立")
            return
        try:
            try:
                data = json.loads(self.savefile.read_text(encoding="utf-8"))
            except UnicodeDecodeError:
                data = json.loads(self.savefile.read_text(encoding="gbk"))
        except json.JSONDecodeError:
            data = None
        if not isinstance(data, dict):
            print("[ERROR] 读取数据失败")
            return
        for u in data.get("users") or []:
            if u.get("_id") in {u.id for u in self.users}:
                continue
            user = User("", "")
            user.load_data(u)
            self.users.append(user)
        tmpu = User("", "")
        for m in data.get("messages") or []:
            if m.get("_id") in {m.id for m in self.messages}:
                continue
            message = Message(tmpu, "")
            message.load_data(m)
            self.messages.append(message)
        hashs = data.get("hash")
        if not isinstance(hashs, list) or not hashs:
            self.syslog("[WARN] 用户数据和聊天记录缺乏安全验证")
        else:
            if hashs[0] != hashlib.md5(str(data.get("users")).encode()).hexdigest():
                self.syslog("[WARN] 用户数据可能被篡改")
            if hashs[1] != hashlib.md5(str(data.get("messages")).encode()).hexdigest():
                self.syslog("[WARN] 聊天记录可能被篡改")

    def send_message(self, message:str|None = None, user:User|None = None) -> None:
        """发送信息"""
        if self.now_user and not user:
            user = self.now_user
        if not user:
            print("[WARN] 尚未登录")
            return
        try:
            check = message is None
            while check:
                message = input("[INPUT] 输入消息：")
                check = input("[ASK] 确认？(Y/n)").lower() == "n"
            self.messages.append(Message(user, str(message)))
        except EOFError:
            print("[INFO] 取消操作")
            return
    def syslog(self, message:str) -> None:
        """记录系统运行日志"""
        self.send_message(message, self.system)
    def show_message(self) -> None:
        """显示所有历史信息"""
        userlist = {u.id:u.name for u in self.users}
        seperator = "-"*30
        if self.messages:
            print(seperator)
        for m in self.messages:
            name = userlist.get(m.owner)
            if name is None:
                name = "<未知用户>"
            print(f"[{name}]在({get_strtime(m.timestamp)})说:")
            print("\n".join("> "+i for i in  m.content.splitlines()))
            print(seperator)
    def note_user(self, note:str|None = None, user:User|None = None) -> None:
        """修改用户自身的备注"""
        if self.now_user and not user:
            user = self.now_user
        if not user:
            print("[WARN] 尚未登录")
            return
        try:
            check = note is None
            print(f"[INFO] 原备注：'{user.note}'")
            while check:
                note = input("[INPUT] 输入备注：")
                check = input("[ASK] 确认？(Y/n)").lower() == "n"
            user.note = str(note)
        except EOFError:
            print("[INFO] 取消操作")
            return

def main():
    """主函数"""
    system = System()
    c = ""
    right = True
    menu : dict[str,tuple[str,Callable]] = {
            "save":("保存数据",system.save),
            "load":("加载数据",system.load),
            "help":("打印命令列表", lambda:print("\n".join(
                ["↓命令↓     -   ↓解释↓"]+[(f"{k:10} -   {v[0]}") for k,v in menu.items()]))),
            "list":("列出所有用户", system.print_users),
            "reg":("注册", system.register),
            "login":("登录", system.login),
            "logout":("登出",system.logout),
            "info":("显示登录后用户的详细信息",system.print_self),
            "renote":("修改用户自身的备注",system.note_user),
            "p":("打印历史消息",system.show_message),
            "send":("发送消息",system.send_message),
            }
    menu["p"][1]()
    print("="*10+"以上为历史信息"+"="*10)
    # menu["help"][1]()
    print("[INFO] 使用 help 加回车获取命令列表")
    print("[INFO] 使用命令进行操作时记得切下输入法")
    while c.lower() != "q":
        color = [f"\x1b[{32 if right else 31}m", "\x1b[0m"] if os.name == "posix" else\
                [">"*10+"命令分隔符"+"<"*10+"\n", ""]
        c = input(f"{color[0]}$ {color[1]}")
        if c in menu:
            menu[c][1]()
            right = True
        else:
            right = False
        system.save()

if __name__ == "__main__":
    main()
