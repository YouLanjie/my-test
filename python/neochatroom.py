#!/usr/bin/env python
# Created:2026.07.27
"""作为talking_local.py的非兼容py3.8、非兼容windows、剔除无用功能的升级版"""

import os
import re
import time
import uuid
import sqlite3
import argparse
import hashlib
from pathlib import Path
from getpass import getpass
from dataclasses import dataclass
from importlib import import_module
from typing import Callable
from enum import Enum, unique
import pytools

# Better Input In Linux
try:
    import_module("readline")
except ModuleNotFoundError:
    pass


# 注册正则表达式函数(py sqlite3不自带REGEXP函数)
def regexp(pattern, item):
    """供sqlite3用的正则函数"""
    if item is None:
        return False
    try:
        return re.search(pattern, item, re.M) is not None
    except re.error:
        return False
def iregexp(pattern, item):
    """供sqlite3用的正则函数(大小写不敏感)"""
    if item is None:
        return False
    try:
        return re.search(pattern, item, re.I+re.M) is not None
    except re.error:
        return False

@unique
class Usertype(Enum):
    """用户类型枚举"""
    BAN = 0
    VISIT = 1
    NORM = 2
    ADMI = 3
@unique
class Activetype(Enum):
    """活动类型枚举"""
    VISIT = 0
    LOGIN = 1
    LOGOUT = 2
    EDIT_NOTE = 3
    EDIT_MSG = 4
@unique
class Messagetype(Enum):
    """消息类型枚举"""
    TEXT = 0
    MONO = 1
    HTML = 2
    MD = 3
    ORG = 4

@dataclass
class User:
    """用户数据类"""
    uuid : str
    name : str
    # passwd : str
    note : str
    time : float
    typ  : Usertype
@dataclass
class Message:
    """消息类"""
    uuid : str
    owner : str   # 用户名(非id)
    time : float
    content : str
    typ : Messagetype

class System:
    """操作类"""
    db_path = Path("SAVEDATA.db")
    admi_uuid = "7d87fb06-64c9-45bc-8b24-397c60d6001b"
    admi_psswd = "db10fa5fb2467f50c7242356ee42ca86"
    cli_sid = "CLI-SESSION-UUID"
    def __init__(self) -> None:
        self.conn = sqlite3.connect(self.db_path)
        self.conn.create_function("regexp", 2, regexp)
        self.conn.create_function("iregexp", 2, iregexp)
        self.init_db()
    def init_db(self):
        """初始化数据库"""
        self.conn.executescript("""\
PRAGMA foreign_keys=ON;
PRAGMA journal_mode=WAL;

CREATE TABLE IF NOT EXISTS "users" (
	-- uid
	uuid TEXT PRIMARY KEY,
	name TEXT UNIQUE NOT NULL,
	passwd TEXT,
	note TEXT,
	time DOUBLE NOT NULL,  -- 注册时间
	type INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS "event" (
	uid TEXT,               -- 执行者uuid，NULL或<0表示为游客
	time DOUBLE NOT NULL,   -- 事件时间
	type INTEGER NOT NULL,  -- 事件类型
	PRIMARY KEY (uid, time, type),
	-- 外键约束
	FOREIGN KEY ("uid") REFERENCES "users"(uuid)
);

CREATE TABLE IF NOT EXISTS "messages" (
	-- mid
	uuid TEXT PRIMARY KEY,
	owner TEXT NOT NULL,
	time DOUBLE NOT NULL,
	content TEXT NOT NULL,
	type INTEGER NOT NULL,
	FOREIGN KEY ("owner") REFERENCES "users"(uuid)
);

CREATE TABLE IF NOT EXISTS "edit_hist" (
	mid TEXT NOT NULL,  -- 消息id
	time DOUBLE NOT NULL,
	diff TEXT,    -- diff新旧文本结果
	PRIMARY KEY (mid, time, diff),
	FOREIGN KEY ("mid") REFERENCES "messages"(uuid)
);

CREATE TABLE IF NOT EXISTS "tags" (
	-- tid
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	tag TEXT UNIQUE NOT NULL
);

CREATE TABLE IF NOT EXISTS "msgs_tags" (
	mid TEXT,
	tid INTEGER,
	-- 联合主键
	PRIMARY KEY (mid, tid),
	-- 外键关联+自动删除
	FOREIGN KEY ("mid") REFERENCES "messages"(id) ON DELETE CASCADE,
	FOREIGN KEY ("tid") REFERENCES "tags"(id) ON DELETE CASCADE
);
CREATE INDEX IF NOT EXISTS "idx_tid" ON "msgs_tags"(tid);

CREATE TABLE IF NOT EXISTS "sessions" (
	sid TEXT PRIMARY KEY,
	uid TEXT NOT NULL,
	ctime INTEGER,
	FOREIGN KEY ("uid") REFERENCES "users"(uuid) ON DELETE CASCADE
);


CREATE VIEW IF NOT EXISTS view_msgs AS
SELECT 
	datetime(m.time, 'unixepoch', 'localtime') AS '时间',
	COALESCE(u.name, '') AS '发送者',
	m.content AS '内容'
FROM messages m
LEFT JOIN users u ON u.uuid = m.owner;
""")
        # 设置系统用户
        self.conn.execute("INSERT OR REPLACE INTO users (uuid,name,passwd,note,time,type)"
                          " VALUES(?,?,?,?,?,?)",
                          (self.admi_uuid, "SYSTEM",self.admi_psswd, "系统内置服务用用户",
                           1753027641.0, Usertype.ADMI.value))
        self.conn.commit()
    def close(self):
        """关闭SQL连接"""
        self.conn.close()
    def get_userlist(self, uid:str|None=None) -> list[User]:
        """获取用户列表(或指定uid查询)"""
        cur = self.conn.cursor()
        if uid is None:
            cur.execute("SELECT uuid,name,note,time,type FROM users")
        else:
            cur.execute("SELECT uuid,name,note,time,type FROM users WHERE uid = ?",
                        (uid,))
        li = []
        for u_uid,name,note,ctime,typ in cur.fetchall():
            li.append(User(u_uid,name,note,ctime,Usertype(typ)))
        cur.close()
        return li
    def get_uid_by_sid(self, sid:str) -> tuple[bool,str]:
        """根据会话sid获取用户uid"""
        cur = self.conn.cursor()
        cur.execute("SELECT uid FROM sessions WHERE sid = ?", (sid,))
        ret = cur.fetchone()
        if ret:
            return (True, ret[0])
        return (False, "会话未登录")
    def get_userinfo(self, uid:str) -> tuple[bool, dict]:
        """通过uid获取用户详细信息"""
        user = self.get_userlist(uid)
        if not user:
            return (False, {"msg":"获取用户信息失败"})
        ret = {"user":user, "activities":[]}
        cur = self.conn.cursor()
        # 获取活动记录，时间降序
        cur.execute("SELECT time,type FROM event WHERE uid = ? ORDER BY time DESC", (uid,))
        ret["activities"] = [(j,Activetype(k)) for j,k in cur.fetchall()]
        cur.execute("SELECT COUNT(uuid),COALESCE(SUM(LENGTH(content)), 0) "
                    "FROM event WHERE uid = ?", (uid,))
        ret["summary"] = cur.fetchone()
        return (True, ret)
    def get_messages(self, pagenum = 1, limit = 12) -> tuple[list[Message],dict[str,int]]:
        """获取分页的消息"""
        cur = self.conn.cursor()
        msg_num = cur.execute("SELECT COUNT(*) FROM messages").fetchone()
        if limit < 1:
            limit = msg_num or 1
        total_page = int(msg_num/limit)+(1 if msg_num%limit else 0)
        if pagenum == 0:
            pagenum = 1
        elif pagenum > total_page or pagenum < 0:
            pagenum = total_page
        # 包含情况：(offset, offset+limit]
        offset = (pagenum-1)*limit
        cur.execute("SELECT uuid,owner,time,content,type "
                    "FROM messages m "
                    "LEFT JOIN users u ON m.owner = u.uuid "
                    "ORDER m.time ASC "
                    "LIMIT ? OFFSET ?", (limit,offset))
        li = []
        for mid,owner,ctime,content,typ in cur.fetchall():
            li.append(Message(mid,owner,ctime,content,typ))
        return (li, {"msg_num":msg_num, "total_page":total_page,
                     "now_page":pagenum,"limit":limit})
    def register(self, name:str, passwd:str) -> tuple[bool,str]:
        """注册，成功返回uid"""
        name = str(name)
        cur = self.conn.cursor()
        cur.execute("SELECT uuid FROM users WHERE name = ?", (name,))
        if cur.fetchone():
            return (False, "用户已存在")
        passwd = hashlib.md5(str(passwd).encode("utf8")).hexdigest()
        uid = str(uuid.uuid4())
        cur.execute("INSERT INTO users (uuid,name,passwd,time,type) VALUES(?,?,?)",
                    (uid,name,passwd,time.time(),Usertype.NORM,))
        cur.close()
        self.conn.commit()
        return (True, uid)
    def login(self, sid:str, name:str, passwd:str) -> tuple[bool,str]:
        """登录（sid无则留空），返回状态和sid"""
        if self.get_uid_by_sid(sid)[0]:
            return (False, "当前会话已登录")
        name = str(name)
        passwd = hashlib.md5(str(passwd).encode("utf8")).hexdigest()
        cur = self.conn.cursor()
        cur.execute("SELECT uuid FROM users WHERE name = ? AND passwd = ?",
                    (name, passwd))
        ret = cur.fetchone()
        if not ret:
            return (False, "用户名或密码不正确")
        if sid == "":
            sid = str(uuid.uuid4())
        cur.execute("INSERT INTO sessions (sid,uid,ctime) VALUES(?,?,?)",
                    (sid,ret[0],int(time.time()),))
        cur.close()
        self.logevent(ret[0], Activetype.LOGIN)
        self.conn.commit()
        return (True, sid)
    def logout(self, sid:str) -> tuple[bool,str]:
        """登出"""
        ret = self.get_uid_by_sid(sid)
        if not ret[0]:
            return (False, "你不能在未登录的时候登出")
        self.conn.execute("DELETE FROM sessions WHERE sid = ?", (sid,))
        self.logevent(ret[1], Activetype.LOGOUT)
        self.conn.commit()
        return (True, "已登出")
    def logevent(self, uid:str, ev:Activetype):
        """需要手动commit()以减少commit数量"""
        self.conn.execute("INSERT event INTO uid,time,type VALUES(?,?,?)",
                          (uid,int(time.time()),ev.value))
    def set_usernote(self, sid:str, new_note:str) -> tuple[bool, str]:
        """修改设置用户备注"""
        uid = self.get_uid_by_sid(sid)
        if not uid[0]:
            return (False, "你不能在未登录的时候修改备注")
        uid = uid[1]
        self.conn.execute("UPDATE users SET note = ? WHERE uid = ?",
                          (new_note,uid,))
        self.logevent(uid, Activetype.EDIT_NOTE)
        self.conn.commit()
        return (True, "修改备注成功")

class InterfaceCLI:
    """CLI交互"""
    def __init__(self) -> None:
        self.system = System()
        self.sid = self.system.cli_sid
    def close(self):
        """关闭数据库连接"""
        self.system.close()
    def listuser(self):
        """打印用户列表"""
        users = self.system.get_userlist()
        print(f"用户列表 ({len(users)})")
        for u in users:
            print(f"[{u.name}] ({pytools.get_strtime(u.time)}) <{u.typ}>\n  -> \"{u.note}\"\n")
    def register(self) -> None:
        """处理注册输入"""
        usernames = [u.name for u in self.system.get_userlist()]
        try:
            while True:
                name = input("[INPUT] 用户名:")
                if name in usernames:
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
        self.system.register(name, passwd)
        if input("[ASK] 自动登录？(Y/n)").lower() != "n":
            self.system.login(self.sid, name, passwd)
    def login(self) -> None:
        """交互式dl处理"""
        if self.system.get_uid_by_sid(self.sid):
            print("[WARN] 你已经登录")
            return
        try:
            name = input("[INPUT] 用户名:")
            passwd = getpass("[INPUT] 密码(不会显示):")
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 操作取消")
            return
        ret = self.system.login(self.sid, name, passwd)
        if not ret[0]:
            print(f"[INFO] 登录失败：{ret[1]}")
            return
        stat = self.system.get_userinfo(self.system.get_uid_by_sid(self.sid)[1])[1]
        acti = [j for j,k in stat.get("activities") or [] if k == Activetype.LOGIN]
        if acti and len(acti) > 1:
            print(f"[INFO] 上次登录：{pytools.get_strtime(acti[1])}")
    def logout(self):
        """登出"""
        ret = self.system.logout(self.sid)
        if not ret[0]:
            print(f"[INFO] 登出失败：{ret[1]}")
    def info(self):
        """打印自身状态信息"""
        ret = self.system.get_uid_by_sid(self.sid)
        if not ret[0]:
            print(f"[INFO] 获取uid错误：{ret[1]}")
            return
        ret,stat = self.system.get_userinfo(ret[1])
        if not ret:
            print("[INFO] 获取信息错误")
            return
        u :User = stat["user"]
        print(f"名字: '{u.name}'")
        print(f"注册时间: '{pytools.get_strtime(u.time)}'")
        print(f"备注: '{u.note}'")
        print(f"UUID: '{u.uuid}'")
        # print(f"密码md5值: '{u.passwd}'")
        acti = [j for j,k in stat.get("activities") or [] if k == Activetype.LOGIN]
        login_record = "\n".join("> "+f"在 {pytools.get_strtime(i)} 登录过" for i in acti)
        print(f"登录记录:({len(stat["login"])})\n{login_record}")
    def main(self):
        c = ""
        right = True
        menu : dict[str,tuple[str,Callable]] = {
                "help":("打印命令列表", lambda:print("\n".join(
                    ["↓命令↓     -   ↓解释↓"]+[(f"{k:10} -   {v[0]}") for k,v in menu.items()]))),
                "q":("退出程序", lambda: None),
                "ls":("列出所有用户", self.listuser),
                "reg":("注册", self.register),
                "login":("登录", self.login),
                "logout":("登出",self.logout),
                "info":("显示登录后用户的详细信息",self.info),
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
        while c.lower() != "q":
            color = [f"\x1b[{32 if right else 31}m", "\x1b[0m"] if os.name == "posix" else\
                    [">"*10+"命令分隔符"+("" if right else "(上条命令不正确)")+"<"*10+"\n", ""]
            try:
                c = input(f"{color[0]}$ {color[1]}")
            except (KeyboardInterrupt, EOFError):
                c = "q"
            if c in menu:
                menu[c][1]()
                right = True
            else:
                right = False

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='python本地(局域网)聊天室(非py3.8兼容版)')
    parser.add_argument('-i', '--input', default="SAVEDATA.db", help='存档文件')
    parser.add_argument('-p', '--port', default=8000, type=int, help='端口号')
    parser.add_argument('-S', '--pure-http-server', action="store_true", help='纯服务器(前台运行)')
    args = parser.parse_args()
    # 指定数据库文件
    System.db_path = Path(args.input)
    cli = InterfaceCLI()
    cli.main()
    cli.close()

if __name__ == "__main__":
    main()
