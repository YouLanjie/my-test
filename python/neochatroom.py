#!/usr/bin/env python
# Created:2026.07.27
"""作为talking_local.py的非兼容py3.8、非兼容windows、剔除无用功能的升级版"""

import re
import time
import uuid
import sqlite3
import argparse
import hashlib
from pathlib import Path
from dataclasses import dataclass
from importlib import import_module
from enum import Enum, unique

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
    EDIT_NOTE = 2
    EDIT_MSG = 3
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

CREATE VIEW IF NOT EXISTS view_msgs AS
SELECT 
	datetime(m.time, 'unixepoch') AS '时间',
	COALESCE(u.name, '') AS '发送者',
	m.content AS '内容'
FROM messages m
LEFT JOIN users u ON u.uuid = m.owner;


CREATE TABLE IF NOT EXISTS "sessions" (
	sid TEXT PRIMARY KEY,
	uid TEXT NOT NULL,
	ctime INTEGER,
	FOREIGN KEY ("uid") REFERENCES "users"(uuid) ON DELETE CASCADE
);
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
        ret["activities"] = cur.fetchall()
        cur.execute("SELECT COUNT(uuid),COALESCE(SUM(LENGTH(content)), 0) "
                    "FROM event WHERE uid = ?", (uid,))
        ret["summary"] = cur.fetchone()
        return (True, ret)
    def get_messages(self, pagenum = 1, limit = 12) -> tuple[list[Message],dict[str,int]]:
        """获取分页的消息"""
        cur = self.conn.cursor()
        msg_num = cur.execute("SELECT COUNT(*) FROM messages").fetchone()
        if limit < 1:
            limit = msg_num
        total_page = int(msg_num/limit)+(1 if msg_num%limit else 0)
        if pagenum < 1:
            pagenum = 1
        elif pagenum > total_page:
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
        stat = {"msg_num":msg_num}
        return (li, stat)
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
        self.conn.commit()
        return (True, sid)
    def logout(self, sid:str) -> tuple[bool,str]:
        """登出"""
        if not self.get_uid_by_sid(sid)[0]:
            return (False, "你不能在未登录的时候登出")
        self.conn.execute("DELETE FROM sessions WHERE sid = ?", (sid,))
        self.conn.commit()
        return (True, "已登出")
    def set_usernote(self, sid:str, new_note:str) -> tuple[bool, str]:
        """修改设置用户备注"""
        uid = self.get_uid_by_sid(sid)
        if not uid[0]:
            return (False, "你不能在未登录的时候修改备注")
        uid = uid[1]
        self.conn.execute("UPDATE users SET note = ? WHERE uid = ?",
                          (new_note,uid,))
        return (True, "修改备注成功")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='python本地(局域网)聊天室(非py3.8兼容版)')
    parser.add_argument('-i', '--input', default="SAVEDATA.db", help='存档文件')
    parser.add_argument('-p', '--port', default=8000, type=int, help='端口号')
    parser.add_argument('-S', '--pure-http-server', action="store_true", help='纯服务器(前台运行)')
    args = parser.parse_args()
    # 指定数据库文件
    System.db_path = Path(args.input)
    system = System()
    system.close()

if __name__ == "__main__":
    main()
