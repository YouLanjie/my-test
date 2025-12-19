#!/usr/bin/env python
# Created:2025.10.18
# 基于python的简陋聊天室程序，向下兼容至python3.8
# Filename: 聊天室v0.0.10.py

from pathlib import Path
from datetime import datetime
from getpass import getpass
from typing import Callable, Union, Any
from importlib import import_module
import argparse
import threading
import subprocess
import uuid
import hashlib
import json
import os
import sys
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

def rescourses(key:str, data:dict) -> str:
    """获取被模板化后的资源"""
    return Template(str({
"css":"""
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    max-width: 1000px;
    margin: 0 auto;
    padding: 20px;
    background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
    min-height: 100vh;
    line-height: 1.6;
}

.container {
    background-color: white;
    padding: 25px;
    border-radius: 12px;
    box-shadow: 0 4px 20px rgba(0,0,0,0.08);
    margin-bottom: 1.5em;
    transition: transform 0.3s ease, box-shadow 0.3s ease;
}

h1 {
    color: #2c3e50;
    text-align: center;
    margin-bottom: 1.2em;
    font-weight: 600;
    position: relative;
}

h1:after {
    content: '';
    display: block;
    width: 60px;
    height: 4px;
    background: linear-gradient(90deg, #3498db, #2ecc71);
    margin: 10px auto 0;
    border-radius: 2px;
}

.form-group {
    margin-bottom: 25px;
}

label {
    display: block;
    margin-bottom: 8px;
    font-weight: 600;
    color: #34495e;
}

input, textarea {
    width: 100%;
    padding: 12px 15px;
    border: 1px solid #ddd;
    border-radius: 8px;
    box-sizing: border-box;
    font-family: inherit;
    font-size: 16px;
    transition: all 0.3s ease;
    background-color: #f8f9fa;
}

input:focus, textarea:focus {
    outline: none;
    border-color: #3498db;
    box-shadow: 0 0 0 3px rgba(52, 152, 219, 0.2);
    background-color: white;
}

button {
    background: linear-gradient(135deg, #3498db, #2980b9);
    color: white;
    padding: 14px 25px;
    border: none;
    border-radius: 8px;
    cursor: pointer;
    font-size: 16px;
    font-weight: 600;
    transition: all 0.3s ease;
    box-shadow: 0 4px 6px rgba(50, 50, 93, 0.11), 0 1px 3px rgba(0, 0, 0, 0.08);
}

button:hover {
    transform: translateY(-2px);
    box-shadow: 0 7px 14px rgba(50, 50, 93, 0.1), 0 3px 6px rgba(0, 0, 0, 0.08);
}

button:active {
    transform: translateY(1px);
}

.result {
    margin-top: 20px;
    padding: 18px;
    background-color: #e7f3ff;
    border-radius: 8px;
    border-left: 4px solid #3498db;
}

.messages, .users {
    background-color: white;
    padding: 15px 20px;
    border-radius: 10px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.05);
    margin-bottom: 0.8em;
    max-height: 70vh;
    overflow: auto;
    text-overflow: ellipsis;
    transition: all 0.3s ease;
}

.messages:hover, .users:hover {
    box-shadow: 0 4px 15px rgba(0,0,0,0.08);
}

.msg_name, .user_name {
    font-weight: 700;
    color: #3498db;
    text-decoration: none;
    transition: color 0.2s ease;
}

.msg_name:hover, .user_name:hover {
    color: #2980b9;
    text-decoration: underline;
}

.msg_time, .user_time {
    color: #7f8c8d;
    float: right;
    margin-right: 0em;
    font-size: 75%;
    font-weight: 500;
}

.messages::-webkit-scrollbar, .users::-webkit-scrollbar {
    width: 8px;
}

.messages::-webkit-scrollbar-track, .users::-webkit-scrollbar-track {
    background: #f1f1f1;
    border-radius: 4px;
}

.messages::-webkit-scrollbar-thumb, .users::-webkit-scrollbar-thumb {
    background: linear-gradient(135deg, #3498db, #2ecc71);
    border-radius: 4px;
}

.messages::-webkit-scrollbar-thumb:hover, .users::-webkit-scrollbar-thumb:hover {
    background: linear-gradient(135deg, #2980b9, #27ae60);
}

a {
    color: #3498db;
    text-decoration: none;
    transition: color 0.2s ease;
}

a:visited {
    color: #8e44ad;
}

a:hover {
    color: #2980b9;
}

.header-padding {
    padding: 1.3em;
}

.header ul {
    list-style-type: none;
    margin: 0;
    padding: 0;
    overflow: auto;
    background: linear-gradient(90deg, #2c3e50, #34495e);
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    z-index: 1000;
    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
}

.header li {
    float: left;
}

.header li a {
    display: block;
    color: white;
    text-align: center;
    padding: 16px 20px;
    text-decoration: none;
    transition: all 0.3s ease;
    font-weight: 500;
}

.header li a:hover {
    background-color: rgba(255,255,255,0.1);
}

.header li:last-child {
    float: right;
}

.pager {
    text-align: center;
    margin: 20px 0;
    padding: 15px;
    background-color: #f8f9fa;
    border-radius: 8px;
    overflow: auto;
}

.pager a {
    display: inline-block;
    padding: 8px 15px;
    margin: 0 5px;
    background-color: white;
    border-radius: 6px;
    box-shadow: 0 2px 5px rgba(0,0,0,0.05);
    transition: all 0.2s ease;
}

.pager a:hover {
    background-color: #3498db;
    color: white;
    transform: translateY(-2px);
}

.pager b a {
    background-color: #3498db;
    color: white;
}

details {
    margin: 15px 0;
    border: 1px solid #e1e8ed;
    border-radius: 8px;
    overflow: hidden;
}

summary {
    padding: 15px;
    background-color: #f8f9fa;
    cursor: pointer;
    font-weight: 600;
    transition: background-color 0.2s ease;
}

summary:hover {
    background-color: #e9ecef;
}

details p {
    padding: 15px;
    margin: 0;
    background-color: white;
}

.back-button {
    display: inline-block;
    margin-top: 15px;
}

img {
    max-width: 100%;
}

/* 响应式设计 */
@media (max-width: 768px) {
    body {
        padding: 10px;
    }
    
    .container {
        padding: 20px 15px;
    }
    
    .header li a {
        padding: 14px 12px;
        font-size: 14px;
    }
    
    .msg_time, .user_time {
        float: none;
        display: block;
        margin-top: 5px;
    }
    
    h1 {
        font-size: 1.5em;
    }
}

/* 动画效果 */
@keyframes fadeIn {
    from { opacity: 0; transform: translateY(10px); }
    to { opacity: 1; transform: translateY(0); }
}

.messages, .users, .container {
    animation: fadeIn 0.5s ease;
}

/* 消息气泡样式增强 */
.messages p:last-child {
    margin-bottom: 0;
    padding: 12px 15px;
    background-color: #f8f9fa;
    border-radius: 8px;
    border-left: 3px solid #3498db;
}

/* 暗色模式样式 */
@media (prefers-color-scheme: dark) {
    body {
        background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
        color: #e0e0e0;
    }

    .container {
        background-color: #1e1e2e;
        box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
    }

    h1 {
        color: #bb86fc;
    }

    h1:after {
        background: linear-gradient(90deg, #bb86fc, #03dac6);
    }

    label {
        color: #e0e0e0;
    }

    input, textarea {
        background-color: #2d2d3e;
        border: 1px solid #444;
        color: #e0e0e0;
    }

    input:focus, textarea:focus {
        border-color: #bb86fc;
        box-shadow: 0 0 0 3px rgba(187, 134, 252, 0.2);
        background-color: #363648;
    }

    button {
        background: linear-gradient(135deg, #bb86fc, #9c64f2);
        box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
    }

    button:hover {
        box-shadow: 0 7px 14px rgba(0, 0, 0, 0.4);
    }

    .result {
        background-color: #2a2a3e;
        border-left: 4px solid #bb86fc;
    }

    .messages, .users {
        background-color: #252536;
        box-shadow: 0 2px 10px rgba(0, 0, 0, 0.2);
    }

    .messages:hover, .users:hover {
        box-shadow: 0 4px 15px rgba(0, 0, 0, 0.3);
    }

    .msg_name, .user_name {
        color: #bb86fc;
    }

    .msg_name:hover, .user_name:hover {
        color: #9c64f2;
    }

    .msg_time, .user_time {
        color: #a0a0a0;
    }

    .messages::-webkit-scrollbar-track, .users::-webkit-scrollbar-track {
        background: #2d2d3e;
    }

    .messages::-webkit-scrollbar-thumb, .users::-webkit-scrollbar-thumb {
        background: linear-gradient(135deg, #bb86fc, #03dac6);
    }

    .messages::-webkit-scrollbar-thumb:hover, .users::-webkit-scrollbar-thumb:hover {
        background: linear-gradient(135deg, #9c64f2, #00c9b7);
    }

    a {
        color: #bb86fc;
    }

    a:visited {
        color: #c792ea;
    }

    a:hover {
        color: #9c64f2;
    }

    .header ul {
        background: linear-gradient(90deg, #121212, #1e1e1e);
    }

    .header li a:hover {
        background-color: rgba(255, 255, 255, 0.1);
    }

    .pager {
        background-color: #2d2d3e;
    }

    .pager a {
        background-color: #363648;
        color: #e0e0e0;
    }

    .pager a:hover {
        background-color: #bb86fc;
        color: #121212;
    }

    .pager b a {
        background-color: #bb86fc;
        color: #121212;
    }

    details {
        border: 1px solid #444;
    }

    summary {
        background-color: #2d2d3e;
    }

    summary:hover {
        background-color: #363648;
    }

    details p {
        background-color: #1e1e2e;
    }

    /* 消息气泡样式增强 - 暗色模式 */
    .messages p:last-child {
        background-color: #2d2d3e;
        border-left: 3px solid #bb86fc;
    }
}

/* 手动切换暗色模式的类（可选） */
.dark-mode {
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%) !important;
    color: #e0e0e0 !important;
}

.dark-mode .container {
    background-color: #1e1e2e !important;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3) !important;
}

.dark-mode h1 {
    color: #bb86fc !important;
}

/* 其他暗色模式样式同上，只需将@media内的样式复制到这里，并加上 !important */
""", # ==================
"template":"""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
${meta}
  <title>${title}</title>
  <link rel="stylesheet" type="text/css" href="/main.css">
</head>
<body>
  <div class="header">
    <ul>
      <li><a href="/">消息列表</a></li>
      <li><a href="/userlist">用户列表</a></li>
      <li><a href="/about">About</a></li>
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
"msg_data":"""
<div class='messages'${is_lastest}>
<p><a href='/userlist#${m.owner}' class='msg_name'>${name}</a>
<span class='msg_time'>${timestamp} | #${ind}</span></p>
<p>${msg}
</p>
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
"user-data":"""
<div class='users' id="${id}">
  <p><a href="/login?id=${id}" class="user_name">${name}</a>
  <span class='user_time'>注册时间: ${timestamp}</span></p>
  <p>${note}</p>
</div>
""", # ==================
"dashboard":"""
<div class="container">
  <h1>个人仪表板</h1>
  ${userdata}
</div>
""", # ==================
"dashboard-data":"""
${usercard}
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
  <a href="${url}" class="back-button">
  <button type="submit">返回主页(部分页面会自动跳转)</button>
  </a>
</div>
""", # ==================
"about":"""
<div class="container">
  <h1>About 关于</h1>
  <p>相关链接:</p>
  <ul>
    <li><a href="/self_update">自动更新按钮</a></li>
    <li><a href="http://${url_self_1}">本程序链接(github)</a></li>
    <li><a href="${url_self_2}">本程序链接(github.io)</a></li>
    <li><a href="http://${url_anti_jiyu}">反极域程序链接(github)</a></li>
    <li><a href="http://ghfast.top/${url_anti_jiyu}">反极域程序链接(github 加速)</a></li>
  </ul>
</div>
""", # ==================
"self_update":"""
<div class="container">
  <h1>自动更新启动页</h1>
  <p>自动更新已经启动,结果会在稍后以一条系统消息显示</p>
  <p>页面将在2秒钟后自动跳转到<a href="/">主页</a></p>
</div>
""", # ==================
"url_self_1":"raw.githubusercontent.com/youlanjie/my-test/refs/heads/main/python/talking_local.py",
"url_self_2":"https://youlanjie.github.io/lib/python/talking_local.py",
"url_anti_jiyu":"https://youlanjie.github.io/lib/python/talking_local.py",
}.get(key))).safe_substitute(data)

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

def log_in_file(msg:str, loglevel = "INFO", filename:str = "chat_room.log"):
    """将信息记录到文件"""
    try:
        if ARGS.no_logs:
            return
        with open(filename, "a") as f:
            print(f"[{loglevel}] [{get_strtime()}] {msg}", file=f)
    except (OSError, PermissionError, AttributeError, NameError):
        pass

def print_debug_info(path:Path):
    """打印debug信息(用于解决windows下的玄学问题)"""
    print(f"[DEBUG] CMD: '{sys.argv[0]}'")
    print(f"[DEBUG] Program: '{Path(sys.argv[0]).resolve()}'")
    print(f"[DEBUG] CWD: '{os.getcwd()}'")
    print(f"[DEBUG] CHECK_PATH: '{path}'")
    print(f"[DEBUG] .is_absolute(): {path.is_absolute()}")
    print(f"[DEBUG] .resolve().is_absolute(): {path.resolve().is_absolute()}")

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
    def __init__(self, savefile = "SAVEDATA.json", no_load = False) -> None:
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
        if not no_load:
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
            print(f"[ERROR] 写入权限错误：{self.savefile.resolve()}")
            if os.name == "nt":
                print_debug_info(self.savefile)
                print("[INFO] 若是在windows下首次打开程序出现此报错,可尝试重新启动本程序解决")
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
            print("[ERROR] json解析失败")
            tsp = int(datetime.now().timestamp())
            backupf = Path(f"{self.savefile.stem}_bak_{tsp}{self.savefile.suffix}")
            print(f"[INFO] 移动原文件 '{self.savefile.resolve()}' 到 '{backupf.resolve()}'")
            shutil.move(self.savefile, backupf)
            return False
        except PermissionError as e:
            print(e)
            print(f"[ERROR] 读取权限错误：{self.savefile.resolve()}")
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
    def syslog(self, message:str, p=False) -> None:
        """记录系统运行日志"""
        self.send_message(message, self.system)
    def print_in_page(self, content: Union[str, list], limit = 12) -> None:
        """将传入的内容分页显示"""
        if isinstance(content, list):
            s = ""
            count = 1
            for i in content:
                if len((s+i+"\n").splitlines()) > count*limit:
                    while len(s.splitlines()) % limit != 0:
                        s += "\n"
                s += i + "\n"
                count = len(s.splitlines()) // limit + 1
            content = s
        content = str(content)
        pages = content.splitlines()
        all_pages = len(pages)//limit + (len(pages)%limit!=0)
        pages = ["\n".join(pages[i*limit:(i+1)*limit]) for i in range(all_pages)]
        no_print = False
        try:
            ind = 0
            while ind < len(pages):
                seperator = "-"*15+f" {ind+1}/{len(pages)} "+"-"*15
                print(seperator)
                if not no_print:
                    print(pages[ind])
                    print(seperator)
                no_print = False
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
                        no_print = True
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
        li = []
        for m in self.messages:
            name = userlist.get(m.owner)
            if name is None:
                name = "<未知用户>"
            s = f"{colors[0]}[{name}]在({get_strtime(m.timestamp)})说:{colors[1]}\n"
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
            s += "\n".join(colors[2]+"> "+colors[1]+i for i in  content.splitlines()) + "\n"
            li.append(s)
        if pager and len(("\n".join(li)).splitlines()) > 12:
            self.print_in_page(li, limit=18)
        else:
            print("\n".join(li), end="")
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
            if now_page < 1:
                now_page = 1
            elif now_page > all_pages:
                now_page = all_pages
            pages = [i for i in range(now_page-2, now_page+3) if 0 < i <= all_pages]
            pages = [f'<a href="?p={i}">{i}</a>' if i!=now_page else \
                    f'<b><a href="?p={i}">{i}</a></b>' for i in pages]
            if now_page-3 == 2:
                pages = ['<a href="?p=2">2</a>'] + pages
            elif now_page-3 > 2:
                pages = ['...'] + pages
            if now_page+3 == all_pages-1:
                pages = pages + [f'<a href="?p={all_pages-1}">{all_pages-1}</a>']
            elif now_page+3 < all_pages-1:
                pages = pages + ['...']
            if now_page-3 >= 1:
                pages = ['<a href="?p=1">1</a>'] + pages
            if now_page+3 <= all_pages:
                pages = pages + [f'<a href="?p={all_pages}">{all_pages}</a>']
            if now_page > 1:
                pages = [f'<a href="?p={now_page-1}">上一页</a>'] + pages
            if now_page < all_pages:
                pages = pages + [f'<a href="?p={now_page+1}">下一页</a>']
            pages = '<p class="pager">Pages: '+" | ".join(pages)
            pages += f'&nbsp;<a href="?p={all_pages}#last_msg" style="float:right;">'
            pages += '点击查看最新消息</a></p>'

            userlist = {u.id:u.name for u in self.users}
            for ind,m in enumerate(self.messages[(now_page-1)*limit:now_page*limit]):
                name = userlist.get(m.owner)
                if name is None:
                    name = "<未知用户>"
                is_lastest = ' id="last_msg"' if m == self.messages[-1] else ""
                s += rescourses("msg_data", {
                    "is_lastest":is_lastest,
                    "name":escape(name),
                    "timestamp":escape(get_strtime(m.timestamp)),
                    "ind":ind+(now_page-1)*limit+1,
                    "msg":"<br/>".join(escape(m.content).splitlines()),
                    })

            data = {
                "messages":s,
                "send_window": rescourses("send_window" if self.now_user.get(ip) \
                        else "send_window2", {}),
                "pages":pages}
            title = "Python聊天室"
        elif tag == "userlist":
            for u in self.users:
                s += rescourses(
                        "user-data",
                        {"id":u.id,"name":escape(u.name),
                         "timestamp":escape(get_strtime(u.timestamp)),
                         "note":"<br/>".join(u.note.splitlines())})
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
                    "note":"\n".join(u.note.splitlines()),
                    "id":escape(u.id), "passwd":escape(u.passwd),
                    "login_record":"<br/>".join(\
                            escape("> "+f"在 {escape(get_strtime(i))} 登录过") \
                            for i in u.login_record)
                    }
                data["usercard"] = rescourses("user-data", data)
                data = {"userdata": rescourses("dashboard-data", data)}
            else:
                data = {"userdata":"<p>你尚未登录</p>"}
            title = "个人仪表板"
        elif tag == "response":
            title = str(data.get("title"))
            meta = str(data.get("meta"))
            s = str(data.get("content"))
        elif tag == "about":
            title = "About 关于 | 帮助"
            data = {i:rescourses(i, {}) for i in [
                "url_self_1",
                "url_self_2",
                "url_anti_jiyu",
                ]}
        elif tag == "self_update":
            title = "自动更新启动页"
            meta = """<meta http-equiv="refresh" content="2;url=/?p=last_msg#last_msg">"""
            s = rescourses("self_update", {})
            threading.Thread(target=self_update).start()
        else:
            tag = "404"
            title = "404 Page Not Found"
            meta = """<meta http-equiv="refresh" content="2;url=/">"""

        if tag:
            tmp = rescourses(tag, data)
            if tmp != "None":
                s = tmp
        s = rescourses("template", {
            "content":s,
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
        if path in ('/', '/userlist', '/login', '/register',
                    '/login', '/dashboard', '/about',
                    '/self_update'):
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
        elif path == "/main.css":
            self.send_response(200)
            self.send_header('Content-type', 'text/css')
            self.end_headers()
            html_content = rescourses("css", {})
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
        url_ref = "/"
        data = {"content":"<p>???? 你似乎进入到了一个禁忌之地 ???</p>",
                "meta":"1",
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
                data["meta"], url_ref = "2", "/register"
            elif li[name].check_passwd(passwd):
                SYSTEM.now_user[self.client_address[0]] = li[name]
                li[name].login_record.append(datetime.now().timestamp())
                s += """<p>欢迎回来！</p>"""
                s += f"""<p>{escape(name)}</p>"""
            else:
                s += """<p>很抱歉，登录失败了，也许你的密码输错了？</p>"""
                data["meta"], url_ref = "2", f"/login?id={li[name].id}"
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
                data["meta"], url_ref = "2", "/register"
            elif passwd != passwd2:
                s += """<p>注册失败！</p>"""
                s += """<p>两次输入的密码不一样</p>"""
                data["meta"], url_ref = "2", "/register"
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
            note = str(note[0] if note else "")
            if SYSTEM.now_user.get(self.client_address[0]):
                SYSTEM.now_user[self.client_address[0]].note = note
                data["content"] = """<p>修改备注成功！</p>"""
                url_ref = "/dashboard"
        elif parsed_path.path == '/send_message' and "message" in parsed_data:
            message = parsed_data.get("message")
            message = str(message[0] if message else "")
            if SYSTEM.now_user.get(self.client_address[0]):
                SYSTEM.messages.append(Message(SYSTEM.now_user[self.client_address[0]], str(message)))
                data["content"] = """<p>留言成功！</p>"""
                url_ref = "/?p=last_msg#last_msg"
        elif parsed_path.path == '/logout':
            if SYSTEM.now_user.get(self.client_address[0]):
                SYSTEM.now_user.pop(self.client_address[0])
                data["content"] = """<p>登出成功！</p>"""
        else:
            for key,value in parsed_data.items():
                data[key] = escape(value[0])
            # 生成响应页面

        data["meta"] = f'<meta http-equiv="refresh" content="{data["meta"]};url={url_ref}">'
        data["url"] = url_ref
        response_html = SYSTEM.get_html(data, "response")
        self.wfile.write(response_html.encode())
        SYSTEM.load()
        SYSTEM.save()
    def log_message(self, format: str, *args: Any) -> None:
        s = "%s - %s" % (self.address_string() , format % args)
        log_in_file(s, loglevel="SERVER")
        return None

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
                    if not ARGS.no_browser:
                        import_module("webbrowser").open(f"http://localhost:{i}/")
                except (ModuleNotFoundError, AttributeError):
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

def download_file(urls:list, savefile) -> bytes:
    req = import_module("urllib.request")
    errors = import_module("urllib.error")
    ret = None
    url = ""
    for url in urls:
        try:
            ret = req.urlopen(url)
            if ret.getcode() == 200:
                break
            ret = None
        except errors.HTTPError as e:
            print(f"[WARN] 链接不可用: {url} ({e})")
        except errors.URLError as e:
            print(f"[WARN] 域名无法访问: {url} ({e})")
    if not ret:
        print(f"[INFO] 下载失败 - '{url}'")
        return b""
    content = ret.read()
    if savefile:
        try:
            Path(savefile).write_bytes(content)
        except (OSError, PermissionError) as e:
            print(f"[ERROR] Saving '{savefile}': {e}")
    return content

def download_anti_program():
    outf = Path("反极域软件.exe")
    md5_hash = "bcbbe129e6032fdbee6e2df28fef55e3"
    fsize = 4712960
    has_file = outf.is_file() and outf.stat().st_size == fsize
    has_file = has_file and hashlib.md5(outf.read_bytes()).hexdigest() == md5_hash
    if outf.is_file() and not has_file:
        outf.unlink()
    print("[INFO] 正在尝试下载反极域软件")
    url = rescourses("url_anti_jiyu", {})
    urls = [f"http://ghfast.top/{url}", f"http://{url}"]
    download_file(urls, outf)
    if outf.is_file():
        print(f"[INFO] 反极域软件已保存到'{outf.resolve()}'")
    return

def self_update():
    print("[INFO] 正在尝试更新本程序")
    url = rescourses("url_self_1", {})
    urls = [f"http://ghfast.top/{url}",
            f"http://{url}",
            rescourses("url_self_2", {}),]
    content = download_file(urls, "")
    msg = []
    try:
        filename = str(content.decode().splitlines()[3])
        if not filename.startswith("# Filename: "):
            raise EOFError
        filename = filename[12:]
    except (UnicodeDecodeError, IndexError, EOFError):
        msg.append("[INFO] 获取聊天室版本错误，使用默认名称")
        print(msg[-1])
        filename = "聊天室_版本未知.py"
    outf = Path(filename)
    if outf.exists():
        msg.append(f"[INFO] 文件'{outf.resolve()}'已存在")
        print(msg[-1])
        if outf.read_bytes() == content:
            msg.append("[INFO] 文件内容相同，更新取消")
            print(msg[-1])
        else:
            tsp = int(datetime.now().timestamp())
            backupf = Path(f"{outf.stem}_bak_{tsp}{outf.suffix}")
            msg.append(f"[INFO] 移动原文件 '{outf.resolve()}' 到 '{backupf.resolve()}'")
            print(msg[-1])
            shutil.move(outf, backupf)
    result = False
    if not outf.is_file():
        outf.write_bytes(content)
        if outf.is_file():
            msg.append(f"[INFO] 新版文件已保存到'{outf.resolve()}'")
            print(msg[-1])
            result = True
    SYSTEM.syslog("\n".join(msg))
    return result

def extra_funtions():
    """提供诸如反极域软件下载、更新等功能"""
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
        if input("[ASK] 确认？(Y/n)").lower() == "n":
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
            log_in_file(f"Run Command: '{c}'")
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
    parser.add_argument('-n', '--no-browser', action="store_true", help='不自动打开浏览器')
    parser.add_argument('-S', '--pure-http-server', action="store_true", help='纯服务器(前台运行)')
    parser.add_argument('--no-subprocess', action="store_true", help='不在windows下尝试启动子进程')
    parser.add_argument('--no-logs', action="store_true", help='不使用文件记录web服务器的日志')
    # parser.add_argument('-x', '--debug', action="store_true", help='调试')
    args = parser.parse_args()
    return args

SYSTEM = System(no_load=True)
if __name__ == "__main__":
    ARGS = parse_arguments()
    # 麻烦的windows右键打开方式错误（默认工作目录不正确）
    if not ARGS.no_subprocess and os.name == "nt" and str(Path("SAVEDATA.json").resolve())[1:3] != ":\\":
        print_debug_info(Path())
        print_debug_info(Path("SAVEDATA.json"))
        print("[INFO] 疑似手动选择打开方式导致工作目录问题")
        print("[INFO] 尝试启动子进程以代替(设置工作目录位于程序目录)")
        subprocess.run(["python"]+sys.argv+["--no-subprocess"],
                       cwd = str(Path(sys.argv[0]).parent),
                       check=False)
        sys.exit()

    SYSTEM = System(ARGS.input)
    log_in_file(f"Run Program")
    if ARGS.pure_http_server:
        run_server(ARGS.port)
    else:
        main(ARGS.port)
    log_in_file(f"Quit Program")
