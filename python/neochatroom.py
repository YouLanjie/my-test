#!/usr/bin/env python
# Created:2026.07.27
"""作为talking_local.py的非兼容py3.8、非兼容windows、剔除无用功能的升级版"""

import re
import time
import uuid
import sqlite3
import hashlib
import argparse
import threading
from pathlib import Path
from getpass import getpass
from dataclasses import dataclass
from importlib import import_module
from functools import lru_cache
from typing import Callable
from enum import Enum, unique

# web ui
import http.server
import socketserver
import urllib.parse
import urllib.request
import urllib.error
from html import escape
from string import Template

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
    BLOB = 5

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
    owner : str   # 用户名(非uid)
    owner_id: str
    time : float
    content : str
    typ : Messagetype

class System:
    """操作类"""
    db_path = Path("SAVEDATA.db")
    _admi_name = "SYSTEM"
    _admi_uuid = "7d87fb06-64c9-45bc-8b24-397c60d6001b"
    _admi_psswd = "db10fa5fb2467f50c7242356ee42ca86"
    _admi_sid = "SYSTEM-LOG-SERVER"
    cli_sid = "CLI-SESSION-UUID"
    def __init__(self) -> None:
        exists = self.db_path.is_file()
        self.conn = sqlite3.connect(self.db_path)
        self.conn.create_function("regexp", 2, regexp)
        self.conn.create_function("iregexp", 2, iregexp)
        if not exists:
            # 减少重复初始化
            self.init_db()
            self.syslog("[INFO] 聊天室建立")
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
                          (self._admi_uuid, self._admi_name,self._admi_psswd, "系统内置服务用用户",
                           1753027641.486, Usertype.ADMI.value))
        self.conn.commit()
    def close(self):
        """关闭SQL连接"""
        self.conn.close()
    def get_userlist(self, uid:str|None=None) -> list[User]:
        """获取用户列表(或指定uid查询)"""
        cur = self.conn.cursor()
        base_sql = "SELECT uuid,name,note,time,type FROM users"
        args = ()
        if not uid is None:
            base_sql += " WHERE uuid = ?"
            args = (uid,)
        base_sql += " ORDER BY time ASC"
        cur.execute(base_sql, args)
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
        ret = {"user":user[0], "activities":[]}
        cur = self.conn.cursor()
        # 获取活动记录，时间降序
        cur.execute("SELECT time,type FROM event WHERE uid = ? ORDER BY time DESC", (uid,))
        ret["activities"] = [(j,Activetype(k)) for j,k in cur.fetchall()]
        cur.execute("SELECT COUNT(uuid),COALESCE(SUM(LENGTH(content)), 0) "
                    "FROM messages WHERE uuid = ?", (uid,))
        ret["summary"] = cur.fetchone()
        return (True, ret)
    def get_messages_len(self) -> int:
        """获取消息的数量"""
        msg_num = self.conn.execute("SELECT COUNT(uuid) FROM messages").fetchone()[0]
        return msg_num
    def get_messages(self, pagenum = -1, limit = 12) -> tuple[list[Message],dict[str,int]]:
        """获取分页的消息"""
        msg_num = self.get_messages_len()
        cur = self.conn.cursor()
        if limit < 1:
            limit = msg_num or 1
        total_page = int(msg_num/limit)+(1 if msg_num%limit else 0)
        if pagenum == 0:
            pagenum = 1
        elif pagenum > total_page or pagenum < 0:
            pagenum = total_page
        # 包含情况：(offset, offset+limit]
        offset = (pagenum-1)*limit
        cur.execute("SELECT m.uuid,u.name,m.owner,m.time,content,m.type "
                    "FROM messages m "
                    "LEFT JOIN users u ON m.owner = u.uuid "
                    "ORDER BY m.time ASC "
                    "LIMIT ? OFFSET ?", (limit,offset))
        li = []
        for mid,owner,uid,ctime,content,typ in cur.fetchall():
            li.append(Message(mid,owner,uid,ctime,content,typ))
        return (li, {"msg_num":msg_num, "total_page":total_page,
                     "now_page":pagenum,"limit":limit})
    def get_message_by_mid(self, mid) -> Message|None:
        """通过消息id获取消息内容"""
        cur = self.conn.cursor()
        cur.execute("SELECT m.uuid,u.name,m.time,content,m.type "
                    "FROM messages m "
                    "LEFT JOIN users u ON m.owner = u.uuid "
                    "WHERE m.uuid = ?", (mid))
        ret = cur.fetchone()
        if not ret:
            return None
        return Message(*ret)
    def register(self, name:str, passwd:str) -> tuple[bool,str]:
        """注册，成功返回uid"""
        name = str(name)
        cur = self.conn.cursor()
        cur.execute("SELECT uuid FROM users WHERE name = ?", (name,))
        if cur.fetchone():
            return (False, "用户已存在")
        passwd = hashlib.md5(str(passwd).encode("utf8")).hexdigest()
        uid = str(uuid.uuid4())
        cur.execute("INSERT INTO users (uuid,name,passwd,time,type) VALUES(?,?,?,?,?)",
                    (uid,name,passwd,time.time(),Usertype.NORM.value,))
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
        """向内部event表记录事件，需要手动commit()以减少commit数量"""
        self.conn.execute("INSERT INTO event (uid,time,type) VALUES(?,?,?)",
                          (uid,time.time(),ev.value))
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
    def send_message(self, sid:str, msg:str,
                     msg_type:Messagetype=Messagetype.TEXT) -> tuple[bool, str]:
        """发送文本消息"""
        if isinstance(msg,bytes) or msg_type == Messagetype.BLOB:
            return (False, "发送消息失败：不应该发送二进制消息")
        if sid == self._admi_sid:
            uid = self._admi_uuid
        else:
            uid = self.get_uid_by_sid(sid)
            if not uid[0]:
                return (False, f"发送消息失败：{uid[1]}")
            uid = uid[1]
        mid = str(uuid.uuid4())
        self.conn.execute("INSERT INTO messages "
                          "(uuid,owner,time,content,type) "
                          "VALUES(?,?,?,?,?)",
                          (mid,uid,time.time(),msg,msg_type.value,))
        self.conn.commit()
        return (True, mid)
    def syslog(self, msg:str):
        """使用系统账户记录通知日志(发送消息)"""
        return self.send_message(self._admi_sid, msg)

class Rescourses:
    """资源类"""
    def __init__(self) -> None:
        self.tf_css = Path(__file__).parent/"ncr_res/main.css"
        self.tf_dcss = Path(__file__).parent/"ncr_res/dark.css"
        self.tf_html = Path(__file__).parent/"ncr_res/template.html"
        self.css = ""
        self.darkcss = ""
        # 键：meta, title, loginstatus, content
        self.index = Template("<html><head>${meta}</head><body>${content}</body></html>")
        self.template :dict[str,Template] = {}
        self.load_template()
    def _is_newer(self, f1:Path, f2:Path) -> bool:
        if not f1.exists() or not f2.exists():
            return False
        return f1.stat().st_mtime > f2.stat().st_mtime
    def check_update(self):
        """检查模板文件更新"""
        f = Path(__file__)
        if self._is_newer(self.tf_css, f) or self._is_newer(self.tf_dcss, f)\
                or self._is_newer(self.tf_html, f):
            self.load_template()
    def load_template(self):
        """加载模板文件"""
        if self.tf_css.is_file():
            self.css = self.tf_css.read_text()
        if self.tf_dcss.is_file():
            self.darkcss = self.tf_dcss.read_text()
        if not self.tf_html.is_file():
            return
        ret = re.split(r"<!-- template:\s*([A-Za-z0-9_]+) -->\n", self.tf_html.read_text())[1:]
        if len(ret) % 2:
            ret = ret[:-1]
        for i in range(int(len(ret)/2)):
            self.template[ret[i*2]] = Template(ret[i*2+1])
        if t:=self.template.get("index"):
            self.index = t
    def get(self, name:str, data:dict) -> str:
        """自动根据数据应用模板"""
        if name not in self.template:
            return ""
        return self.template[name].safe_substitute(data)

class InterfaceWeb(http.server.SimpleHTTPRequestHandler):
    """Web交互 && 自定义请求处理器"""
    system : None|System = None
    httpd : None|socketserver.TCPServer = None
    rescourses = Rescourses()
    @classmethod
    def _start_server(cls, port):
        if cls.httpd:
            return
        cls.rescourses.check_update()
        # if os.name != "nt":
        socketserver.TCPServer.allow_reuse_address = True
        for i in range(port, port+100):
            try:
                with socketserver.TCPServer(("", i), cls) as httpd:
                    print(f"[INFO] 服务器(WebUI)运行在 http://localhost:{i}/")
                    cls.httpd = httpd
                    # print("按 Ctrl+C 停止服务器")
                    try:
                        httpd.serve_forever()
                    except KeyboardInterrupt:
                        print("\n[INFO] 服务器已停止")
            except OSError:
                continue
            break
        cls.httpd = None
    @classmethod
    def start_server(cls, port=8000):
        """开启服务器"""
        threading.Thread(target=cls._start_server, args=(port,)).start()
    def get_sid(self):
        """获得cookie的SID"""
        cookies = (self.headers.get("Cookie", "") or "").split(";")
        sid = ""
        for i in cookies:
            i = i.strip()
            if not i.startswith("sid="):
                continue
            sid = i[4:]
        return sid
    def get_querys(self) -> dict[str,str]:
        """获取url?xxx=yyy参数"""
        query = urllib.parse.urlparse(self.path).query
        querys = {i.split("=",1)[0]:i.split("=",1)[1] for i in query.split("&") if "=" in i}
        return querys
    def get_base_html(self, content:str, title:str="新·聊天室(WebUI)", meta:str="") -> str:
        """套上基础html模板"""
        sid = self.get_sid()
        s = []
        if self.system and (sid := self.get_sid()) and (uid:=self.system.get_uid_by_sid(sid)[1]) \
                and (u:=self.system.get_userlist(uid)[0]):
            s.append(f"""<a href="/dashboard">{escape(u.name)}</a>""")
        else:
            s.append("""<a href="/register">注册</a></li>""")
            s.append("""<a href="/login">登录</a>""")
        loginstatus = "\n".join([f"""<li style="float:right;">{i}</li>""" for i in s])
        # 键：meta, title, loginstatus, content
        return self.rescourses.index.safe_substitute({
            "meta":meta,
            "title":title,
            "loginstatus":loginstatus,
            "content":content,
            })
    @lru_cache
    def get_msg_content_html(self, content:str, msgid:str) -> str:
        """生成消息渲染后的html"""
        # try:
        #     if not self.orgreader:
        #         raise ModuleNotFoundError
        #     if not content.startswith("# USE ORG"):
        #         raise ValueError
        #     doc = self.orgreader.Document(content,
        #                                   file_name=msgid+".org",
        #                                   setting={"id_prefix":"org_"+msgid+"_"})
        #     doc.root.line.s = ""
        #     visitor = self.orgreader.HtmlExportVisitor()
        #     msg = visitor.toc_to_html(doc) + doc.root.accept(visitor)
        #     if doc.status["footnotes"]:
        #         msg += visitor.fns_to_html(doc)
        #     return msg
        # except (ModuleNotFoundError, ValueError, OSError, FileNotFoundError):
        return "<br/>".join(escape(content).splitlines())
    def gen_pager(self, now_page:int, all_pages:int, lst=True) -> str:
        """生成翻页器"""
        if all_pages == 0:
            return ""
        req = "&".join(f"{k}={v}" for k,v in self.get_querys().items() if k != "p")
        if req:
            req += "&"

        pages = []
        fmt = '<a href="?%sp=%d">%s</a>'
        start,current,end = 1, now_page, all_pages
        vals = sorted({start, end} | set(range(max(start, current-2), min(end, current+2)+1)))
        for i, x in enumerate(vals):
            if i == 0 and start < current:
                pages.append(fmt%("",current-1,"上一页"))
            if i == len(vals)-1 and current < end:
                pages.append(fmt%("",current+1,"下一页"))
            if i > 0 and x - vals[i-1] > 1:
                pages.append(fmt%(req,vals[i-1]+1,vals[i-1]+1) if x-vals[i-1] == 2 else "...")
            pages.append(f"<b>{fmt%(req,x,x)}</b>" if x == current else fmt%(req,x,x))
        pages = '<p class="pager">Pages: '+" | ".join(pages)
        if lst:
            pages += f'&nbsp;<a href="?{req}p={all_pages}#last_msg" style="float:right;">'
            pages += '点击查看最新消息</a>'
        pages += '</p>'
        return pages
    def gen_message_list(self, messages:list[Message]) -> str:
        """生成消息列表(html)"""
        if not self.system:
            return ""
        s = ""
        last_msg_id = msg_list[0].uuid if (msg_list:=self.system.get_messages(limit=1)[0]) else ""
        now_uid = ret[1] if (ret:=self.system.get_uid_by_sid(self.get_sid())) else ""
        # now_utype = ret[0].typ if now_uid and (ret:=self.system.get_userlist(now_uid))
        # else Usertype.BAN
        for m in messages:
            msg_id = ' id="last_msg"' if m.uuid == last_msg_id else ''
            msg = self.get_msg_content_html(m.content, m.uuid)
            if len(messages) == 1:
                msg_id += ' style="max-height:100%;"'
                if (time.time() - m.time < 60*10 and m.owner == now_uid):
                    msg += "<br/><a href='/edit?id="+m.uuid+"'><button>编辑留言</button></a>"
            s += self.rescourses.get("msg_data", {
                "id":msg_id,
                "msgid":m.uuid,
                "name":escape(m.owner),
                "owner":escape(m.owner_id),
                "timestamp":escape(pytools.get_strtime(m.time)),
                "ind":messages.index(m)+1,
                "msg":msg,
                })
        return s
    def get_msglist(self) -> str:
        """返回消息列表"""
        if not self.system:
            return ""
        querys = self.get_querys()
        try:
            limit = int(str(querys.get("page_limit")))
        except ValueError:
            limit = 12
        now_page = str(querys.get("p") or "1")
        try:
            now_page = int(now_page)
        except ValueError:
            if now_page == "last_msg":
                now_page = -1
            else:
                now_page = 1
        is_login = self.system.get_uid_by_sid(self.get_sid())[0]
        msgs,stat = self.system.get_messages(pagenum=now_page,limit=limit)
        s = self.gen_message_list(msgs)
        total_page = stat["total_page"]
        now_page = stat["now_page"]
        limit = stat["limit"]
        pager = self.gen_pager(now_page, total_page)
        return self.get_base_html(self.rescourses.get("msg_list", {
            "messages": s,
            "send_window" : self.rescourses.get("send_window" if is_login else "send_window2",{}),
            "pages": pager,
            }))
    def ret_404(self):
        """响应404界面"""
        t = self.rescourses.template.get("404")
        t = t.safe_substitute() if t else "404 NOT FOUND"
        t = self.get_base_html(t)
        self.send_response(404)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(t.encode())
    def do_GET(self):
        """解析URL路径"""
        parsed_path = urllib.parse.urlparse(self.path)
        path = parsed_path.path
        handler :dict[str,Callable] = {
                "/":self.get_msglist,
                }
        css_list = {"/main.css":self.rescourses.css,
                    "/dark.css":self.rescourses.darkcss}
        if path not in handler and path not in css_list:
            self.ret_404()
            return
        html_content = ""
        if path in handler:
            html_content = handler[path]()
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
        if path in css_list:
            html_content = css_list[path]
            self.send_response(200)
            self.send_header('Content-type', 'text/css')
            self.end_headers()
        self.wfile.write(html_content.encode())

class InterfaceCLI:
    """CLI交互"""
    def __init__(self, system:System|None=None) -> None:
        self.system = system or System()
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
        if self.system.get_uid_by_sid(self.sid)[0]:
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
        print(f"备注: '{u.note or ''}'")
        print(f"注册: '{pytools.get_strtime(u.time)}'")
        print(f"UUID: '{u.uuid}'")
        # print(f"密码md5值: '{u.passwd}'")
        acti = [j for j,k in stat.get("activities") or [] if k == Activetype.LOGIN]
        login_record = "\n".join("> "+f"在 {pytools.get_strtime(i)} 登录过" for i in acti[:5])
        print(f"登录记录:(共{len(acti)}条{'，只显示最近5条' if len(acti)>5 else ''})\n{login_record}")
    def note_user(self) -> None:
        """修改用户自身的备注"""
        ret = self.system.get_uid_by_sid(self.sid)
        if not ret[0]:
            print(f"[INFO] 获取uid错误：{ret[1]}")
            return
        uid = ret[1]
        ok,stat = self.system.get_userinfo(uid)
        if not ok:
            print("[INFO] 获取用户信息失败")
            return
        note = stat["user"].note
        print(f"[INFO] 原备注：'{note}'")
        try:
            check = True
            while check:
                note = input("[INPUT] 输入备注：")
                check = input("[ASK] 确认？(Y/n)").lower() == "n"
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 取消操作")
            return
        self.system.set_usernote(self.sid, note)
    def print_in_page(self, content: str|list[str], limit = 12) -> None:
        """将传入的内容分页显示"""
        if isinstance(content, str):
            content = content.splitlines()
        lines = []
        all_pages = 1
        for i in content:
            i = i.splitlines()
            if len(lines)+len(i)+1 > all_pages*limit and len(lines) % limit != 0:
                lines += [""]*(limit-len(lines)%limit-1)
            lines += [""]
            lines += i
            all_pages = (len(lines)-1)//limit+1
        pages = ["\n".join(lines[i*limit:(i+1)*limit]) for i in range(all_pages)]
        hint = ""
        try:
            ind = 0
            while ind < len(pages):
                seperator = f"{'-'*15} {ind+1}/{len(pages)} {'-'*15}"
                print(hint or seperator+"\n"+pages[ind])
                print(seperator)
                hint = ""
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
                        hint = "\n".join([
                            "[INFO] g回到第一页, G跳到最后一页",
                            "[INFO] 输入数字页码跳转到对应页面",
                            "[INFO] h开头字符命令打印此信息",
                            "[INFO] q退出程序(均需要回车确认)",
                            ])
                        ind -= 1
                    ind += 1
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 退出分页器")
            return
        return
    def msg_fommater(self, messages:list[Message]):
        """格式化消息"""
        li :list[str] = []
        colors = ("\x1b[34m", "\x1b[0m", "\x1b[2m")
        for m in messages:
            s = f"{colors[0]}[{m.owner}]在({pytools.get_strtime(m.time)})说:{colors[1]}\n"
            content = m.content
            if len(content.splitlines()) > 12:
                content = "\n".join(content.splitlines()[:12]) +\
                        "\n"+"="*40+"\n"+\
                        "【以下内容由于行数超过12被系统自动截断】\n"+\
                        f"【使用show命令查看全部内容】\n【消息ID:'{m.uuid}'】"
            elif len(content) > 500:
                content = content[:500]  +\
                        "\n"+"="*40+"\n"+\
                        "【以下内容由于字符数量超过500被系统自动截断】\n"+\
                        f"【使用show命令查看全部内容】\n【消息ID:'{m.uuid}'】"
            s += "\n".join(colors[2]+"> "+colors[1]+i for i in  content.splitlines()) + "\n"
            li.append(s)
        return li
    def print_recent_msg(self):
        """打印最新消息"""
        messages,stat = self.system.get_messages(limit=20)
        li = self.msg_fommater(messages)
        print("\n".join(li), end="")
        if stat["total_page"] > 1:
            print(f"\n[NOTE] 只打印了最新{len(li)}条消息")
    def print_pager_msg(self):
        """打印经过分页的全部消息"""
        messages,_ = self.system.get_messages(limit=-1)
        li = self.msg_fommater(messages)
        if len(("\n".join(li)).splitlines()) > 12:
            self.print_in_page(li, limit=24)
        else:
            print("\n".join(li))
    def select_message(self) -> Message|None:
        """过滤选择消息"""
        msg_list :dict[str,Message] = {}
        for m in self.system.get_messages(limit=-1)[0]:
            msg = m.content.splitlines()[:1]
            msg = (msg[0][:25]+"……" if len(msg[0])>25 else msg[0]) if msg else ""
            s = f"[{m.uuid}] ({pytools.get_strtime(m.time)})[{m.owner}]:'{msg}'"
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
    def send_message(self):
        """发送消息"""
        if not self.system.get_uid_by_sid(self.sid)[0]:
            print("[WARN] 尚未登录")
            return
        try:
            check = True
            message = ""
            while check:
                message = input("[INPUT] 输入消息：")
                check = input("[ASK] 确认？(Y/n)").lower() == "n"
            ret = self.system.send_message(self.sid, message)
        except (KeyboardInterrupt, EOFError):
            print("[INFO] 取消操作")
            return
        if not ret[0]:
            print(f"[WARN] 发送消息失败：{ret[1]}")

    def main(self):
        """主函数"""
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
                "renote":("修改用户自身的备注",self.note_user),
                "p":("打印历史消息",self.print_recent_msg),
                "p2":("打印历史消息(分页)", self.print_pager_msg),
                "show":("打印选择的特定历史消息",self.show_sigal_message),
                "send":("发送消息",self.send_message),
                }
        menu["p"][1]()
        print("="*10+"以上为历史信息"+"="*10)
        # menu["help"][1]()
        print("[INFO] 使用 help 加回车获取命令列表")
        print("[INFO] 使用命令进行操作时记得切下输入法")
        while c.lower() != "q":
            color = [f"\x1b[{32 if right else 31}m", "\x1b[0m"]
            try:
                c = input(f"{color[0]}$ {color[1]}")
            except (KeyboardInterrupt, EOFError):
                print("\n[INFO] C-c/C-d 退出")
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
    try:
        cli = InterfaceCLI()
    except sqlite3.DatabaseError as e:
        print(f"[ERROR] sqlite3错误：{e}")
        print("[ERROR] 程序将直接退出")
        return
    InterfaceWeb.system = cli.system
    InterfaceWeb.start_server()
    cli.main()
    if InterfaceWeb.httpd:
        InterfaceWeb.httpd.shutdown()
    cli.close()

if __name__ == "__main__":
    main()
