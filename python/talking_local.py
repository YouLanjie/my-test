#!/usr/bin/env python
# Created:2025.10.18
# 基于python的简陋聊天室程序，向下兼容至python3.8
# Filename: 聊天室v0.0.4.py

from pathlib import Path
from datetime import datetime
from getpass import getpass
from typing import Callable, Union
from importlib import import_module
import argparse
import threading
import subprocess
import uuid
import hashlib
import json
import os
import shutil
import time

# web ui
import http.server
import socketserver
import urllib.parse
from html import escape
from string import Template

if os.name == "posix":
    # Better Input In Linux
    try:
        import_module("readline")
    except ModuleNotFoundError:
        pass

RESCOURSES = {"css":"""
body {
    font-family: Arial, sans-serif;
    max-width: 800px;
    margin: 0 auto;
    padding: 20px;
    background-color: #f5f5f5;
}
.container {
    background-color: white;
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    margin-bottom: 1em;
}
h1 {
    color: #333;
    text-align: center;
}
.form-group {
    margin-bottom: 20px;
}
label {
    display: block;
    margin-bottom: 5px;
    font-weight: bold;
}
input, textarea {
    width: 100%;
    padding: 10px;
    border: 1px solid #ddd;
    border-radius: 4px;
    box-sizing: border-box;
}
button {
    background-color: #4CAF50;
    color: white;
    padding: 12px 20px;
    border: none;
    border-radius: 4px;
    cursor: pointer;
    font-size: 16px;
}
button:hover {
    background-color: #45a049;
}
.result {
    margin-top: 20px;
    padding: 15px;
    background-color: #e7f3ff;
    border-radius: 4px;
}
.messages, .users {
    background-color: white;
    padding: 5px 20px 5px 20px;
    border-radius: 5px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    margin-bottom: 0.5em;
    max-height: 70vh;
    overflow: auto;
    text-overflow: ellipsis;
}
.msg_name, .user_name {
    font-weight: bold;
    color: blue;
}
.msg_time, .user_time {
    color: gray;
    float: right;
    margin-right: 0em;
    font-size: 60%;
}
.messages::-webkit-scrollbar, .users::-webkit-scrollbar {
    width: 8px;
}
.messages::-webkit-scrollbar-track, .users::-webkit-scrollbar-track {
    background: #f1f1f1;
    border-radius: 4px;
}
.messages::-webkit-scrollbar-thumb, .users::-webkit-scrollbar-thumb {
    background: #3498db;
    border-radius: 4px;
}

.pager a {
    color: #07b;
}
.pager a:visited {
    color: #666;
}

.header-padding {
    padding: 1.3em;
}
.header ul {
    list-style-type: none;
    margin: 0;
    padding: 0;
    overflow: auto;
    background-color: #333;
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
}
.header li {
    float: left;
}
.header li a {
    display: block;
    color: white;
    text-align: center;
    padding: 14px 16px;
    text-decoration: none;
}
/*鼠标移动到选项上修改背景颜色 */
.header li a:hover {
    background-color: #111;
}
""", # ==================
"template":"""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
${meta}
  <title>${title}</title>
  <style>${css}
  </style>
</head>
<body>
  <div class="header">
    <ul>
      <li><a href="/">消息列表</a></li>
      <li><a href="/userlist">用户列表</a></li>
      ${loginstatus}
    </ul>
  </div>
  <div class="header-padding"></div>
${content}
</body>
</html>
""", # ==================
"msg_list":"""
<div class="container">
  <h1>Python聊天室(Web UI)</h1>
${pages}
${messages}
${pages}
${send_window}
</div>
""", # ==================
"send_window":"""
<form method="POST" action="/send_message">
<div class="form-group">
<label for="message">填写要发送的消息:</label><textarea name="message" rows="4" required></textarea>
</div>
<button type="submit">发送</button></form>
""", # ==================
"send_window2":"""
<p>您尚未登陆，请登陆或注册后再留言</p>
""", # ==================
"userlist":"""
<div class="container">
  <h1>用户列表</h1>
  <p>Tips: 可点击对应用户名称实现快速登陆</p>
${users}
</div>
""", # ==================
"dashboard":"""
<div class="container">
  <h1>个人仪表板</h1>
  ${userdata}
</div>
""", # ==================
"dashboard-data":"""
<div class='users'>
  <p><span class='user_name'>${name}</span>
  <span class='user_time'>注册时间: ${timestamp}</span></p>
  <p>${note}</p>
</div>
<div class='container'>
  <p>UUID: '${id}'</p>
  <p>密码md5值: '${passwd}'</p>
  <details>
    <summary>登录记录</summary>
    <p>${login_record}</p>
  </details>
</div>

<div class='container'>
  <form method="POST" action="/renote">
    <div class="form-group">
      <label for="note">修改备注:</label>
      <textarea name="note" rows="4" required>${note}</textarea>
    </div>
    <button type="submit">提交修改</button>
  </form>
</div>

<form method="POST" action="/logout">
  <button type="submit" style="background-color:red;">退出登录</button>
</form>
""", # ==================
"login":"""
<div class="container">
  <h1>登录</h1>
  <form method="POST" action="/login">
     <div class="form-group">
      <label for="name">用户名:</label><input type="text" name="username"${value} required>
      <label for="passwd">密码:</label><input type="password" name="passwd" required>
    </div>
    <button type="submit">登录</button>
  </form>
</div>
""", # ==================
"register":"""
<div class="container">
  <h1>注册</h1>
  <form method="POST" action="/register">
    <div class="form-group">
      <label for="name">用户名:</label><input type="text" name="username" required>
      <label for="passwd">密码（暂不支持注册后修改或找回）:</label><input type="password" name="passwd" required>
      <label for="passwd2">再次输入相同的密码确认:</label><input type="password" name="passwd2" required>
    </div>
    <button type="submit">注册</button>
  </form>
</div>
""", # ==================
"404":"""
<div class="container">
  <h1>404 Page Not Found</h1>
  <p>页面将在3秒钟后自动跳转到<a href="/">主页</a></p>
</div>
""", # =================
"response":"""
<div class="container">
  <div class="container">
${content}
  </div>
  <a href="/" class="back-button">
  <button type="submit">返回主页(部分页面会自动跳转)</button>
  </a>
</div>
"""}

# 由于在py3.8时仍不支持将联合类型写成 X|Y ，心碎了
def get_strtime(dt:Union[datetime,float] = datetime.now()) -> str:
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
        self._id = str(uuid.uuid4())
        self.login_record = []
    @property
    def id(self) -> str:
        """UUID4，用于标识用户"""
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
        self._id = str(uuid.uuid4())
    @property
    def id(self) -> str:
        """UUID4，用于标识该条聊天记录"""
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
    def __init__(self, savefile = "SAVEDATA.json") -> None:
        self.users : list[User] = []
        self.messages : list[Message] = []
        self.now_user : dict[str,User] = {}
        self.savefile = Path(str(savefile))
        self.st_mtime = 0.0
        self.md5_hash = []
        self.httpd : Union[None, socketserver.TCPServer] = None

        system = User("SYSTEM", "", "系统内置服务用用户")
        system.load_data({"_passwd":"db10fa5fb2467f50c7242356ee42ca86",
                          "timestamp":1753027641.0,
                          "_id":"7d87fb06-64c9-45bc-8b24-397c60d6001b"})
        self.users.append(system)
        self.system = system
        self.load()
    def print_users(self) -> None:
        """打印所有用户信息"""
        for u in self.users:
            print(f"[{u.name}] ({get_strtime(u.timestamp)})\n  -> \"{u.note}\"\n")
    def print_self(self) -> None:
        """打印登录用户自身信息"""
        if not self.now_user.get("commandline"):
            print("[WARN] 你尚未登录")
            return
        u = self.now_user["commandline"]
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
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 操作取消")
            return
        u = User(name, passwd)
        self.users.append(u)
        if input("[ASK] 自动登录？(Y/n)").lower() != "n":
            self.login(u)
    def login(self, user:Union[User,None] = None) -> None:
        """login"""
        if self.now_user.get("commandline"):
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
            except (KeyboardInterrupt, EOFError):
                print("[INFO] 操作取消")
                return
        self.now_user["commandline"] = user
        if user.login_record:
            print(f"[INFO] 上次登录：{get_strtime(user.login_record[-1])}")
        user.login_record.append(datetime.now().timestamp())
    def logout(self):
        """登出"""
        if not self.now_user.get("commandline"):
            print("[WARN] 你还没有登录")
        self.now_user.pop("commandline")
    def save(self, readable = False) -> None:
        """保存数据"""
        if self.savefile.is_dir():
            print("[WARN] savefile is dir")
            return
        if self.savefile.is_file():
            self.load()
        data = {"users":[u.dump_data() for u in self.users],
                "messages":[m.dump_data() for m in self.messages],
                "hash":["", ""]}
        data["hash"] = [hashlib.md5(str(data["users"]).encode()).hexdigest(),
                        hashlib.md5(str(data["messages"]).encode()).hexdigest()]
        if self.md5_hash == data["hash"] and not readable:
            return
        self.md5_hash = data["hash"]
        if readable:
            t = json.dumps(data, ensure_ascii=False, indent=4)
        else:
            t = json.dumps(data)
        try:
            self.savefile.write_text(t, encoding="utf-8")
        except PermissionError as e:
            print(e)
            print(f"[ERROR] 权限错误：{self.savefile.resolve()}")
            print("[INFO] 若是在windows下首次打开程序出现此报错")
            print("[INFO] 可尝试重新启动本程序解决")
        # print(t)
    def load(self) -> bool:
        """读取数据"""
        if not self.savefile.is_file():
            print(f"[WARN] 数据文件'{self.savefile.resolve()}'不存在")
            self.syslog("[INFO] 聊天室建立")
            self.save()
            return False
        st_mtime = self.savefile.stat().st_mtime
        if st_mtime <= self.st_mtime:
            return False
        try:
            try:
                data = json.loads(self.savefile.read_text(encoding="utf-8"))
            except UnicodeDecodeError:
                data = json.loads(self.savefile.read_text(encoding="gbk"))
        except json.JSONDecodeError:
            data = None
        if not isinstance(data, dict):
            print("[ERROR] 读取数据失败")
            tsp = int(datetime.now().timestamp())
            backupf = Path(f"{self.savefile.stem}_bak_{tsp}{self.savefile.suffix}")
            print(f"[INFO] 移动原文件 '{self.savefile.resolve()}' 到 '{backupf.resolve()}'")
            shutil.move(self.savefile, backupf)
            return False
        self.st_mtime = st_mtime
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
            if not self.md5_hash:
                self.md5_hash = hashs
        self.messages = sorted(self.messages, key=lambda x:x.timestamp)
        return True

    def send_message(self, message:Union[str,None] = None, user:Union[User,None] = None) -> None:
        """发送信息"""
        if self.now_user.get("commandline") and not user:
            user = self.now_user.get("commandline")
        if not user:
            print("[WARN] 尚未登录")
            return
        try:
            check = message is None
            while check:
                message = input("[INPUT] 输入消息：")
                check = input("[ASK] 确认？(Y/n)").lower() == "n"
            self.messages.append(Message(user, str(message)))
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 取消操作")
            return
    def syslog(self, message:str) -> None:
        """记录系统运行日志"""
        self.send_message(message, self.system)
    def print_in_page(self, content: str, limit = 12) -> None:
        """将传入的内容分页显示"""
        content = str(content)
        pages = content.splitlines()
        all_pages = len(pages)//limit + (len(pages)%limit!=0)
        pages = ["\n".join(pages[i*limit:(i+1)*limit]) for i in range(all_pages)]
        try:
            ind = 0
            while ind < len(pages):
                seperator = "-"*15+f" {ind+1}/{len(pages)} "+"-"*15
                print(seperator)
                print(pages[ind])
                print(seperator)
                number = input("[INPUT] 翻页器(h获取帮助):")
                try:
                    number = int(number)
                    if 0 < number <= len(pages):
                        ind = number - 1
                except ValueError:
                    if str(number).lower() == "q":
                        ind = len(pages)
                    elif str(number) == "g":
                        ind = -1
                    elif str(number) == "G":
                        ind = len(pages)-2
                    elif str(number).lower().startswith("h"):
                        print("[INFO] g回到第一页, G跳到最后一页")
                        print("[INFO] 输入数字页码跳转到对应页面")
                        print("[INFO] h开头字符命令打印此信息")
                        print("[INFO] q退出程序(均需要回车确认)")
                        ind -= 1
                    ind += 1
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 取消操作")
            return
        return
    def show_message(self, pager = False) -> None:
        """显示所有历史信息"""
        userlist = {u.id:u.name for u in self.users}
        if self.messages:
            print()
        colors = ("\x1b[34m", "\x1b[0m", "\x1b[2m") if os.name == "posix" else ("", "", "")
        s = ""
        for m in self.messages:
            name = userlist.get(m.owner)
            if name is None:
                name = "<未知用户>"
            s += f"{colors[0]}[{name}]在({get_strtime(m.timestamp)})说:{colors[1]}\n"
            content = m.content
            if len(content.splitlines()) > 12:
                content = "\n".join(content.splitlines()[:12]) +\
                        "\n"+"="*40+"\n"+\
                        "【以下内容由于行数超过12被系统自动截断】\n"+\
                        f"【使用show命令查看全部内容】\n【消息ID:'{m.id}'】"
            elif len(content) > 500:
                content = content[:500]  +\
                        "\n"+"="*40+"\n"+\
                        "【以下内容由于字符数量超过500被系统自动截断】\n"+\
                        f"【使用show命令查看全部内容】\n【消息ID:'{m.id}'】"
            s += "\n".join(colors[2]+"> "+colors[1]+i for i in  content.splitlines()) + "\n\n"
        if pager and len(s.splitlines()) > 12:
            self.print_in_page(s)
        else:
            print(s, end="")
    def select_message(self) -> Union[Message, None]:
        """过滤选择消息"""
        msg_list = {}
        userlist = {u.id:u.name for u in self.users}
        for n,m in enumerate(self.messages):
            name = userlist.get(m.owner)
            if name is None:
                name = "<未知用户>"
            msg = m.content.splitlines()[:1]
            msg = msg[0][:25] if msg else ""
            s = f"[{n}] ({get_strtime(m.timestamp)})[{name}]:'{msg}……'"
            msg_list[s] = m

        obj_msg = None
        key = None
        try:
            while len(msg_list) > 1:
                print(" "*30)
                if len(msg_list) > 12:
                    print("[INFO] 需要退出分页模式再使用关键词匹配过滤")
                    self.print_in_page("\n".join(msg_list.keys()))
                else:
                    print("\n".join(msg_list.keys()) + "\n")
                print("[INFO] 以上为待选项，通过多个关键词匹配得到对应消息")
                key = input("[INPUT] 搜索关键词:")
                msg_list = {k:v for k,v in msg_list.items() if key in k}
            if len(msg_list) == 0:
                print("[WARN] 不存在可选项")
            else:
                obj_msg = list(msg_list)[0]
                print("[INFO] 最终选项：")
                print(obj_msg)
                obj_msg = msg_list[obj_msg]
                if input("[ASK] 确认？(Y/n)").lower() == "n":
                    print("[INFO] 取消操作")
                    return None
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 取消操作")
            return None
        if not obj_msg:
            return None
        return obj_msg
    def show_sigal_message(self) -> None:
        """显示特定历史信息"""
        obj_msg = self.select_message()
        if not obj_msg:
            return
        self.print_in_page(obj_msg.content)

    def note_user(self, note:Union[str,None] = None, user:Union[User,None] = None) -> None:
        """修改用户自身的备注"""
        if self.now_user.get("commandline") and not user:
            user = self.now_user.get("commandline")
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
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 取消操作")
            return
    def get_html(self, ip:Union[dict,str] = "", tag = "") -> str:
        """生成html相应页面"""
        data = {}
        if isinstance(ip, dict):
            data = ip
            ip = str(ip.get("ip"))
        s = []
        if self.now_user.get(ip):
            u = self.now_user[ip]
            s.append(f"""<a href="/dashboard">{escape(u.name)}</a>""")
        else:
            s.append("""<a href="/register">注册</a></li>""")
            s.append("""<a href="/login">登录</a>""")
        loginstatus = "\n".join([f"""<li style="float:right;">{i}</li>""" for i in s])

        meta = ""
        title = ""
        s = ""
        # RESCOURSES[]
        self.load()
        if tag == "msg_list":
            limit = 12
            now_page = [i[2:] for i in (data.get("page") or ["p=1"]) if i.startswith("p=")]
            all_pages = len(self.messages)//limit + (len(self.messages)%limit!=0)
            try:
                now_page = int(now_page[0])
            except IndexError:
                now_page = 1
            except ValueError:
                if now_page[0] == "last_msg":
                    now_page = all_pages
                else:
                    now_page = 1
            pages = [i for i in range(now_page-3, now_page+3) if 0 < i <= all_pages]
            pages = [f'<a href="?p={i}">{i}</a>' if i!=now_page else \
                    f'<b><a href="?p={i}">{i}</a></b>' for i in pages]
            if now_page-3 > 2:
                pages = ['...'] + pages
            if now_page+3 < all_pages-1:
                pages = pages + ['...']
            if now_page-3 > 1:
                pages = ['<a href="?p=1">1</a>'] + pages
            if now_page+3 < all_pages:
                pages = pages + [f'<a href="?p={now_page}">{all_pages}</a>']
            if now_page > 1:
                pages = [f'<a href="?p={now_page-1}">上一页</a>'] + pages
            if now_page < all_pages:
                pages = pages + [f'<a href="?p={now_page+1}">下一页</a>']
            pages = '<p class="pager">Pages: '+" | ".join(pages)
            pages += f'<a href="?p={all_pages}#last_msg" style="float:right;">点击查看最新消息</a></p>'

            userlist = {u.id:u.name for u in self.users}
            for m in self.messages[(now_page-1)*limit:now_page*limit]:
                name = userlist.get(m.owner)
                if name is None:
                    name = "<未知用户>"
                is_lastest = ' id="last_msg"' if m == self.messages[-1] else ""
                s += f"<div class='messages'{is_lastest}>\n"
                s += f"<p><span class='msg_name'>{escape(name)}</span> "
                s += f"<span class='msg_time'>{escape(get_strtime(m.timestamp))}</span></p>\n"
                s += "<p>"+ "<br/>".join(m.content.splitlines()) + "</p>\n"
                s += "</div>\n"

            data = {
                "messages":s,
                "send_window": RESCOURSES["send_window" if self.now_user.get(ip) \
                        else "send_window2"],
                "pages":pages}
            title = "Python聊天室"
        elif tag == "userlist":
            for u in self.users:
                s += f'<div class="users">\n'
                s += f'<p><a href="/login?id={u.id}" class="user_name">{escape(u.name)}</a> '
                s += f"<span class='user_time'>注册时间: {escape(get_strtime(u.timestamp))}</span></p>"
                s += "<p>"+ "<br/>".join(u.note.splitlines()) + "</p>\n"
                s += "</div></a>\n"
            data = {"users":s}
            title = "用户列表"
        elif tag == "login":
            title = "登录"
            userlist = {u.id:u.name for u in self.users}
            if data.get("id") and data.get("id") in userlist:
                data = {"value":' value="'+escape(userlist[str(data.get("id"))])+'"'}
            else:
                data = {"value":""}
        elif tag == "register":
            # <p>[WARN] 用户 '{name}' 不存在, 可尝试注册<p>
            title = "注册"
        elif tag == "dashboard":
            if self.now_user.get(ip):
                u = self.now_user[ip]
                data = {
                    "name":escape(u.name), "timestamp":escape(get_strtime(u.timestamp)),
                    "note":"<br/>".join(escape(u.note).splitlines()),
                    "id":escape(u.id), "passwd":escape(u.passwd),
                    "login_record":"<br/>".join(\
                            escape("> "+f"在 {escape(get_strtime(i))} 登录过") \
                            for i in u.login_record)
                    }
                data = {"userdata": Template(RESCOURSES["dashboard-data"]).safe_substitute(data)}
            else:
                data = {"userdata":"<p>你尚未登录</p>"}
            title = "个人仪表板"
        elif tag == "response":
            title = str(data.get("title"))
            meta = str(data.get("meta"))
            s = str(data.get("content"))
        else:
            tag = "404"
            title = "404 Page Not Found"
            meta = """<meta http-equiv="refresh" content="2;url=/">"""

        if tag and tag in RESCOURSES:
            s = Template(RESCOURSES[tag]).safe_substitute(data)
        s = Template(RESCOURSES["template"]).safe_substitute({
            "content":s, "css":RESCOURSES["css"],
            "meta":meta, "title":f"{title} - {ip}",
            "loginstatus":loginstatus})
        return s

# 自定义请求处理器
class SimpleWebUI(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # 解析URL路径
        parsed_path = urllib.parse.urlparse(self.path)
        path = parsed_path.path
        # print([self.path, parsed_path])
        # 如果访问根路径，返回主页
        if path in ('/', '/userlist', '/login', '/register', '/login', '/dashboard'):
            ip = self.client_address[0]
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            # 生成HTML页面(生成主页面HTML)
            if path == "/login":
                has_id = [i for i in parsed_path.query.split("&") if i.startswith("id=")]
                if has_id:
                    ip = {"ip":ip, "id":has_id[0][3:]}
            if path == "/":
                path = "msg_list"
                ip = {"ip":ip, "page":parsed_path.query.split("&")}
            else:
                path = path[1:]
            html_content = SYSTEM.get_html(ip, path)
        else:
            # # 对于其他路径，使用默认的文件服务行为
            # super().do_GET()
            self.send_response(404)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            html_content = SYSTEM.get_html(self.client_address[0], "404")
        self.wfile.write(html_content.encode())

    def do_POST(self):
        # 处理POST请求
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        # 解析表单数据
        parsed_data = urllib.parse.parse_qs(post_data)
        data = {"content":"<p>???? 你似乎进入到了一个禁忌之地 ???</p>",
                "meta":"""<meta http-equiv="refresh" content="1;url=/">""",
                "ip":self.client_address[0],
                "title":"中转界面"}

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        parsed_path = urllib.parse.urlparse(self.path)
        if parsed_path.path == '/login' and "username" in parsed_data:
            name = parsed_data.get("username")
            name = escape(str(name[0] if name else ""))
            passwd = parsed_data.get("passwd")
            passwd = str(passwd[0] if passwd else "")
            li = {u.name:u for u in SYSTEM.users}
            s = ""
            if name not in li:
                s += f"<p>[WARN] 用户 '{name}' 不存在, 可尝试注册<p>"
                data["meta"] = """<meta http-equiv="refresh" content="2;url=/register">"""
            elif li[name].check_passwd(passwd):
                SYSTEM.now_user[self.client_address[0]] = li[name]
                li[name].login_record.append(datetime.now().timestamp())
                s += """<p>欢迎回来！</p>"""
                s += f"""<p>{escape(name)}</p>"""
            else:
                s += """<p>很抱歉，登录失败了，也许你的密码输错了？</p>"""
                data["meta"] = """<meta http-equiv="refresh" content="2;url=/login">"""
            data["content"] = s
        elif parsed_path.path == '/register' and "username" in parsed_data:
            name = parsed_data.get("username")
            name = escape(str(name[0] if name else ""))
            passwd = parsed_data.get("passwd")
            passwd = str(passwd[0] if passwd else "")
            passwd2 = parsed_data.get("passwd2")
            passwd2 = str(passwd2[0] if passwd2 else "")
            li = {u.name:u for u in SYSTEM.users}
            s = ""
            if name in li:
                s += """<p>注册失败！</p>"""
                s += f"<p>[WARN] 用户 '{name}' 已存在<p>"
                data["meta"] = """<meta http-equiv="refresh" content="2;url=/register">"""
            elif passwd != passwd2:
                s += """<p>注册失败！</p>"""
                s += """<p>两次输入的密码不一样</p>"""
                data["meta"] = """<meta http-equiv="refresh" content="2;url=/register">"""
            else:
                u = User(name, passwd)
                SYSTEM.users.append(u)
                SYSTEM.now_user[self.client_address[0]] = u
                u.login_record.append(datetime.now().timestamp())
                s += """<p>注册成功，欢迎！</p>"""
                s += f"""<p><b>{escape(name)}</b></p>"""
            data["content"] = s
        elif parsed_path.path == '/renote' and "note" in parsed_data:
            note = parsed_data.get("note")
            note = escape(str(note[0] if note else ""))
            if SYSTEM.now_user.get(self.client_address[0]):
                SYSTEM.now_user[self.client_address[0]].note = note
                data["content"] = """<p>修改备注成功！</p>"""
                data["meta"] = """<meta http-equiv="refresh" content="1;url=/dashboard">"""
        elif parsed_path.path == '/send_message' and "message" in parsed_data:
            message = parsed_data.get("message")
            message = escape(str(message[0] if message else ""))
            if SYSTEM.now_user.get(self.client_address[0]):
                SYSTEM.messages.append(Message(SYSTEM.now_user[self.client_address[0]], str(message)))
                data["content"] = """<p>留言成功！</p>"""
                data["meta"] = """<meta http-equiv="refresh" content="1;url=/?p=last_msg#last_msg">"""
        elif parsed_path.path == '/logout':
            if SYSTEM.now_user.get(self.client_address[0]):
                SYSTEM.now_user.pop(self.client_address[0])
                data["content"] = """<p>登出成功！</p>"""
                data["meta"] = """<meta http-equiv="refresh" content="1;url=/">"""
        else:
            for key,value in parsed_data.items():
                data[key] = escape(value[0])
            # 生成响应页面

        response_html = SYSTEM.get_html(data, "response")
        self.wfile.write(response_html.encode())
        SYSTEM.load()
        SYSTEM.save()

# 启动服务器
def run_server(port=8000):
    if os.name != "nt":
        socketserver.TCPServer.allow_reuse_address = True
    for i in range(port, port+100):
        try:
            with socketserver.TCPServer(("", i), SimpleWebUI) as httpd:
                print(f"[INFO] 服务器(WebUI)运行在 http://localhost:{i}/")
                print("[INFO] 浏览器或将自动打开")
                try:
                    import_module("webbrowser").open(f"http://localhost:{i}/")
                except ModuleNotFoundError:
                    print("[INFO] 无法自动打开浏览器")
                SYSTEM.httpd = httpd
                # print("按 Ctrl+C 停止服务器")
                try:
                    httpd.serve_forever()
                except KeyboardInterrupt:
                    print("\n[INFO] 服务器已停止")
        except OSError:
            continue
        break
    SYSTEM.httpd = None

def module_check(module_list:list, system = None) -> list:
    """检查并安装需要的模块"""
    install_list = []
    avaliable_list = []
    for i in module_list:
        try:
            import_module(i)
            avaliable_list.append(i)
        except ModuleNotFoundError:
            install_list.append(i)
    if install_list:
        mirr_url = "https://pypi.tuna.tsinghua.edu.cn/simple"
        ret = subprocess.run(["python", "-m", "pip", "config", "get", "global.index-url"],
                             capture_output=True, check=False)
        if ret.returncode != 0:
            subprocess.run(["python", "-m", "pip", "config", "set", "global.index-url", mirr_url],
                           capture_output=True, check=False)
    for i in install_list:
        ret = subprocess.run(["python", "-m", "pip", "install", i],
                             capture_output=True, check=False)
        if ret.returncode == 0:
            if isinstance(system, System):
                system.syslog(f"[INFO] 模块'{i}'安装成功")
    for i in install_list:
        try:
            import_module(i)
            avaliable_list.append(i)
        except ModuleNotFoundError:
            if isinstance(system, System):
                system.syslog(f"[WARN] 模块'{i}'仍不可用")
    return avaliable_list

def extra_funtions():
    """提供诸如反极域软件下载、更新等功能"""
    system = SYSTEM
    def download_anti_program():
        outf = Path("反极域软件.exe")
        md5_hash = "bcbbe129e6032fdbee6e2df28fef55e3"
        fsize = 4712960
        has_file = outf.is_file() and outf.stat().st_size == fsize
        has_file = has_file and hashlib.md5(outf.read_bytes()).hexdigest() == md5_hash
        if outf.is_file() and not has_file:
            outf.unlink()
        req = import_module("urllib.request")
        errors = import_module("urllib.error")
        print("[INFO] 正在尝试下载反极域软件")
        url = "github.com/imengyu/JiYuTrainer/releases/download/1.7.6/JiYuTrainer.exe"
        urls = (f"http://ghfast.top/{url}", f"http://{url}")
        for i in urls:
            try:
                req.urlretrieve(i, str(outf))
                break
            except errors.HTTPError as e:
                print(f"[WARN] 链接不可用: {i} ({e})")
            except errors.URLError as e:
                print(f"[WARN] 域名无法访问: {i} ({e})")
        if outf.is_file():
            print(f"[INFO] 反极域软件已保存到'{outf.resolve()}'")
        return
    def self_update():
        req = import_module("urllib.request")
        errors = import_module("urllib.error")
        print("[INFO] 正在尝试更新本程序")
        url = "raw.githubusercontent.com/youlanjie/my-test/refs/heads/main/python/talking_local.py"
        urls = (f"http://ghfast.top/{url}", f"http://{url}")
        ret = None
        for i in urls:
            try:
                ret = req.urlopen(i)
                if ret.getcode() == 200:
                    break
                ret = None
            except errors.HTTPError as e:
                print(f"[WARN] 链接不可用: {i} ({e})")
            except errors.URLError as e:
                print(f"[WARN] 域名无法访问: {i} ({e})")
        if not ret:
            print(f"[INFO] 下载失败 - '{url}'")
            return
        content = ret.read()
        try:
            filename = str(content.decode().splitlines()[3])
            if not filename.startswith("# Filename: "):
                raise EOFError
            filename = filename[12:]
        except (UnicodeDecodeError, IndexError, EOFError):
            print("[INFO] 获取聊天室版本错误，使用默认名称")
            filename = "聊天室_版本未知.py"
        outf = Path(filename)
        outf.write_bytes(content)
        if outf.is_file():
            print(f"[INFO] 新版文件已保存到'{outf.resolve()}'")
            system.syslog(f"[INFO] 更新版本到'{outf.resolve()}'")
        return
    li = [lambda:None,download_anti_program, self_update]
    print("""\
##################################
#          额外功能菜单          #
##################################
#  1. 下载反极域（反控制）软件   #
#  2. 更新本程序                 #
#  0. 返回命令行                 #
##################################""")
    num = 0
    try:
        num = int(input("[INPUT] 请选择(数字):"))
        print(f"[INFO] 将运行功能 '{li[num]}'")
        if input("[ASK] 确认？(y/N)").lower() != "y":
            raise KeyboardInterrupt
        print(f"[INFO] 正在运行 '{li[num]}'(后台进行)")
        threading.Thread(target=li[num]).start()
    except (KeyboardInterrupt, EOFError):
        print("[INFO] 取消操作")
    except ValueError:
        print("[INFO] 无效值（非数字）")
    except IndexError:
        print("[INFO] 超出选择范围")

def main(port = 8000):
    """主函数"""
    system = SYSTEM
    # SYSTEM["commandline"] = SYSTEM
    c = ""
    right = True
    menu : dict[str,tuple[str,Callable]] = {
            "more":("更多可选操作（更新等）",extra_funtions),
            "save":("保存数据",system.save),
            "save2":("以人类易读形式保存数据", lambda:system.save(readable=True)),
            "load":("加载数据",system.load),
            "help":("打印命令列表", lambda:print("\n".join(
                ["↓命令↓     -   ↓解释↓"]+[(f"{k:10} -   {v[0]}") for k,v in menu.items()]))),
            "q":("退出程序", lambda: None),
            "ls":("列出所有用户", system.print_users),
            "reg":("注册", system.register),
            "login":("登录", system.login),
            "logout":("登出",system.logout),
            "info":("显示登录后用户的详细信息",system.print_self),
            "renote":("修改用户自身的备注",system.note_user),
            "p":("打印历史消息",system.show_message),
            "p2":("打印历史消息(分页)", lambda: system.show_message(True)),
            "show":("打印选择的特定历史消息",system.show_sigal_message),
            "send":("发送消息",system.send_message),
            }
    menu["p"][1]()
    print("="*10+"以上为历史信息"+"="*10)
    # menu["help"][1]()
    print("[INFO] 使用 help 加回车获取命令列表")
    print("[INFO] 使用命令进行操作时记得切下输入法")
    threading.Thread(target=run_server, args=(port,)).start()
    for i in range(100):
        time.sleep(0.01)
        if SYSTEM.httpd:
            break

    while c.lower() != "q":
        color = [f"\x1b[{32 if right else 31}m", "\x1b[0m"] if os.name == "posix" else\
                [">"*10+"命令分隔符"+("" if right else "(上条命令不正确)")+"<"*10+"\n", ""]
        try:
            c = input(f"{color[0]}$ {color[1]}")
        except (KeyboardInterrupt, EOFError):
            c = "q"
        system.load()
        if c in menu:
            menu[c][1]()
            right = True
        else:
            right = False
        system.save()
    if SYSTEM.httpd:
        SYSTEM.httpd.shutdown()

def parse_arguments() -> argparse.Namespace:
    """解释参数"""
    parser = argparse.ArgumentParser(description='python本地(局域网)聊天室')
    parser.add_argument('-i', '--input', default="SAVEDATA.json", help='存档文件')
    parser.add_argument('-p', '--port', default=8000, type=int, help='端口号')
    # parser.add_argument('-x', '--debug', action="store_true", help='调试')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    ARGS = parse_arguments()
    SYSTEM = System(ARGS.input)
    main(ARGS.port)
