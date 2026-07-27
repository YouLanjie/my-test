#!/usr/bin/env python
# Created:2026.07.27
"""将旧的json聊天室迁移到sqlite3聊天室格式"""

import json
import sqlite3
from pathlib import Path
from enum import Enum, unique

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

@unique
class Messagetype(Enum):
    """消息类型枚举"""
    TEXT = 0
    MONO = 1
    HTML = 2
    MD = 3
    ORG = 4

def init_db(conn: sqlite3.Connection):
    """初始化数据库"""
    conn.executescript("""\
PRAGMA foreign_keys=ON;
PRAGMA journal_mode=WAL;

CREATE TABLE IF NOT EXISTS "users" (
	-- uid
	uuid TEXT PRIMARY KEY,
	name TEXT UNIQUE NOT NULL,
	passwd TEXT,
	note TEXT,
	time DOUBLE NOT NULL,  -- 注册时间
	type INTEGER
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
	datetime(m.time, 'unixepoch', 'localtime') AS '时间',
	COALESCE(u.name, '') AS '发送者',
	m.content AS '内容'
FROM messages m
LEFT JOIN users u ON u.uuid = m.owner;
""")
    conn.commit()

def translate_data(conn: sqlite3.Connection, data:dict):
    """将旧聊天室数据转换到sqlite3.db里"""
    cur = conn.cursor()
    for u in data["users"]:
        try:
            cur.execute("INSERT INTO users (uuid,name,passwd,note,time,type) VALUES(?,?,?,?,?,?)",
                        (u["_id"],u["name"], u["_passwd"], u["note"], u["timestamp"],
                         Usertype.NORM.value))
            for rec_time in u["login_record"]:
                cur.execute("INSERT INTO event (uid,time,type) VALUES(?,?,?)",
                             (u["_id"], rec_time, Activetype.LOGIN.value))
        except sqlite3.IntegrityError as e:
            print(f"在导入用户'{u["name"]}'时遇到问题：{e}")
    for m in data["messages"]:
        try:
            cur.execute("INSERT INTO messages (uuid,owner,time,content,type) VALUES(?,?,?,?,?)",
                         (m["_id"],m["owner"], m["timestamp"],
                          "\n".join(str(m["content"]).splitlines()),
                          Messagetype.TEXT.value))
            for edit in m["edit_history"]:
                cur.execute("INSERT INTO edit_hist (mid,time) VALUES(?,?)",
                            (m["_id"], edit))
        except sqlite3.IntegrityError as e:
            print(f"在导入消息'{m["_id"]}'(uuid)时遇到问题：{e}")
    conn.commit()

def main():
    """主函数"""
    data = json.loads(Path("./data/SAVEDATA.json").read_bytes())
    db_path = Path("./test.db")
    with sqlite3.connect(db_path) as conn:
        init_db(conn)
        translate_data(conn, data)
        # cur = conn.cursor()
        # cur.execute("SELECT * FROM timeline")
        # data = cur.fetchall()
        # print(data)

if __name__ == "__main__":
    main()
