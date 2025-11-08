#!/usr/bin/env python
# Created:2025.11.01

import http.server
import socketserver
import urllib.parse
from html import escape
# from pathlib import Path
from string import Template

import talking_local as tl

SYSTEM = tl.System()

RESCOURSES = {"./main_page.html":"""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Python聊天室</title>
	<style>
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
		}
		.msg_name, .user_name {
			font-weight: bold;
			color: blue;
		}
		.msg_time, .user_time {
			color: gray;
			float: right;
			margin-right: 1em;
		}
	</style>
</head>
<body>
	<div class="container">
		<h1>Python聊天室(Web UI)</h1>
		<p>以下是消息列表（右边滑动条滑动到最下面查看最新消息）</p>
${messages}
${send_window}
	</div>
	<div class="container">
		<h1>用户列表</h1>
${users}
	</div>
	<div class="container">
		<h1>登录状态</h1>
${userdata}
	</div>
</body>
</html>
""", "./404.html":"""
<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="refresh" content="3;url=/">
</head>
<body>
    <p>页面将在3秒钟后自动跳转到<a href="/">HomePage</a></p>
</body>
</html>
""", "./response_page.html":"""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
${meta}
	<title>提交结果</title>
	<style>
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
		}
		.msg_name, .user_name {
			font-weight: bold;
			color: blue;
		}
		.msg_time, .user_time {
			color: gray;
			float: right;
			margin-right: 1em;
		}
	</style>
</head>
<body>
	<div class="container">
${content}
		<a href="/" class="back-button">返回表单</a>
	</div>
</body>
</html>
"""}

def gen_html(name:str, data = None) -> str:
    """return html file"""
    if not isinstance(data, dict):
        data = {}
    # file = Path(name)
    # if not file.is_file():
    if not name in RESCOURSES:
        return """<!DOCTYPE html><html lang="zh-CN">"""+\
                """<head><title>404<title/><meta http-equiv="refresh" content="2;url=/"><head/>"""+\
                """<body><p>'{file}' could not be found<p/><body/>"""
    # content = Template(file.read_text())
    content = Template(RESCOURSES[name])
    return content.safe_substitute(data)

def get_info() -> dict:
    """从SYSTEM获取信息并处理"""
    data = {}
    userlist = {u.id:u.name for u in SYSTEM.users}
    s = ""
    for m in SYSTEM.messages:
        name = userlist.get(m.owner)
        if name is None:
            name = "<未知用户>"
        s += "<div class='messages'>\n"
        s += f"<p><span class='msg_name'>{escape(name)}</span> <span class='msg_time'>{escape(tl.get_strtime(m.timestamp))}</span></p>\n"
        s += "<p>"+ "<br/>".join(m.content.splitlines()) + "</p>\n"
        s += "</div>\n"
    data["messages"] = s
    s = ""
    for u in SYSTEM.users:
        s += "<div class='users'>\n"
        s += f"<p><span class='user_name'>{escape(u.name)}</span> <span class='user_time'>注册时间: {escape(tl.get_strtime(u.timestamp))}</span></p>\n"
        s += "<p>"+ "<br/>".join(u.note.splitlines()) + "</p>\n"
        s += "</div>\n"
    data["users"] = s
    s = ""
    if SYSTEM.now_user:
        u = SYSTEM.now_user
        s += "<div class='users'>\n"
        s += f"<p><span class='user_name'>{escape(u.name)}</span> <span class='user_time'>注册时间: {escape(tl.get_strtime(u.timestamp))}</span></p>\n"
        s += "<p>备注: <br/>"+ "<br/>".join(u.note.splitlines()) + "</p>\n"
        s += "</div>\n"
        s += f"<p>UUID: '{escape(u.id)}'<p>"
        s += f"<p>密码md5值: '{escape(u.passwd)}'<p>"
        login_record = "<br/>".join("> "+f"在 {escape(tl.get_strtime(i))} 登录过" for i in u.login_record)
        s += f"<p>登录记录:</br>{login_record}<p>\n"

        s += """<form method="POST" action="/renote">"""
        s += """<div class="form-group">"""
        s += f"""<label for="note">修改备注:</label><textarea name="note" rows="4" required>{escape(u.note)}</textarea>"""
        s += """</div>"""
        s += """<button type="submit">提交修改</button></form>\n"""

        s += """<p></p><form method="POST" action="/logout">"""
        s += """<button type="submit" style="background-color:red;">退出登录</button></form>\n"""
    else:
        # s += "<div class='messages'>\n"
        s += "<p>你尚未登录,请登录或注册（登录失败再注册）</p>\n"
        s += """<form method="POST" action="/login">"""
        s += """<div class="form-group">"""
        s += """<label for="name">用户名:</label><input type="text" name="username" required>"""
        s += """<label for="passwd">密码:</label><input type="password" name="passwd" required>"""
        # <textarea id="message" name="message" rows="4" required></textarea>
        s += """</div>"""
        s += """<button type="submit">登录</button></form>"""
        # s += "</div>\n"
    data["userdata"] = s
    s = ""
    if SYSTEM.now_user:
        s += """<form method="POST" action="/send_message">"""
        s += """<div class="form-group">"""
        s += """<label for="message">填写要发送的消息:</label><textarea name="message" rows="4" required></textarea>"""
        s += """</div>"""
        s += """<button type="submit">发送</button></form>"""
    data["send_window"] = s
    return data

# 自定义请求处理器
class SimpleWebUI(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # 解析URL路径
        parsed_path = urllib.parse.urlparse(self.path)
        # print([self.path, parsed_path])
        # 如果访问根路径，返回主页
        if parsed_path.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            # 生成HTML页面(生成主页面HTML)
            # SYSTEM.show_message()
            html_content = gen_html("./main_page.html", get_info())
            self.wfile.write(html_content.encode())
        else:
            # # 对于其他路径，使用默认的文件服务行为
            # super().do_GET()
            self.send_response(404)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            html_content = gen_html("./404.html")
            self.wfile.write(html_content.encode())
    
    def do_POST(self):
        # 处理POST请求
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        # 解析表单数据
        parsed_data = urllib.parse.parse_qs(post_data)
        data = {"content":"<p>???? 你似乎进入到了一个禁忌之地 ???</p>",
                "meta":"""<meta http-equiv="refresh" content="1;url=/">"""}

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        parsed_path = urllib.parse.urlparse(self.path)
        if parsed_path.path == '/login' and "username" in parsed_data:
            name = parsed_data.get("username")
            name = escape(str(name[0] if name else ""))
            passwd = parsed_data.get("passwd")
            passwd = str(name[0] if name else "")
            li = {u.name:u for u in SYSTEM.users}
            s = ""
            if name not in li:
                s += f"<p>[WARN] 用户 '{name}' 不存在, 可尝试注册<p>"
                s += """<form method="POST" action="/register">"""
                s += """<div class="form-group">"""
                s += f"""<label for="name">用户名:</label><input type="text" name="username" value={repr(name)} required>"""
                s += """<label for="passwd">密码:</label><input type="password" name="passwd" required>"""
                s += """<label for="passwd2">再次输入相同的密码确认:</label><input type="password" name="passwd2" required>"""
                s += """</div>"""
                s += """<button type="submit">注册</button></form>"""
                data["meta"] = ""
            elif li[name].check_passwd(passwd):
                SYSTEM.now_user = li[name]
                li[name].login_record.append(tl.datetime.now().timestamp())
                s += """<p>欢迎回来！</p>"""
                s += f"""<p>{escape(name)}</p>"""
            else:
                s += """<p>很抱歉，登录失败了，也许你的密码输错了？</p>"""
            data["content"] = s
        elif parsed_path.path == '/register' and "username" in parsed_data:
            name = parsed_data.get("username")
            name = escape(str(name[0] if name else ""))
            passwd = parsed_data.get("passwd")
            passwd = str(name[0] if name else "")
            passwd2 = parsed_data.get("passwd2")
            passwd2 = str(name[0] if name else "")
            li = {u.name:u for u in SYSTEM.users}
            s = ""
            if name in li:
                s += """<p>注册失败！</p>"""
                s += f"<p>[WARN] 用户 '{name}' 已存在<p>"
            elif passwd != passwd2:
                s += """<p>注册失败！</p>"""
                s += f"""<p>两次输入的密码不一样</p>"""
            else:
                u = tl.User(name, passwd)
                SYSTEM.users.append(u)
                SYSTEM.now_user = u
                u.login_record.append(tl.datetime.now().timestamp())
                s += """<p>注册成功，欢迎！</p>"""
                s += f"""<p><b>{escape(name)}</b></p>"""
            data["content"] = s
        elif parsed_path.path == '/renote' and "note" in parsed_data:
            note = parsed_data.get("note")
            note = escape(str(note[0] if note else ""))
            if SYSTEM.now_user:
                SYSTEM.now_user.note = note
                data["content"] = """<p>修改备注成功！</p>"""
        elif parsed_path.path == '/send_message' and "message" in parsed_data:
            message = parsed_data.get("message")
            message = escape(str(message[0] if message else ""))
            if SYSTEM.now_user:
                SYSTEM.messages.append(tl.Message(SYSTEM.now_user, str(message)))
                data["content"] = """<p>留言成功！</p>"""
        elif parsed_path.path == '/logout':
            if SYSTEM.now_user:
                SYSTEM.now_user = None
                data["content"] = """<p>登出成功！</p>"""
        else:
            for key,value in parsed_data.items():
                data[key] = escape(value[0])
            # 生成响应页面
        response_html = gen_html("./response_page.html", data)
        # response_html = gen_html("./main_page.html", get_info())
        self.wfile.write(response_html.encode())
        SYSTEM.save()

# 启动服务器
def run_server(port=8000):
    socketserver.TCPServer.allow_reuse_address = True
    for i in range(100):
        try:
            with socketserver.TCPServer(("", port), SimpleWebUI) as httpd:
                print(f"服务器运行在 http://localhost:{port}")
                print("按 Ctrl+C 停止服务器")
                try:
                    httpd.serve_forever()
                except KeyboardInterrupt:
                    print("\n服务器已停止")
        except OSError:
            port += 1
            continue
        break

if __name__ == "__main__":
    run_server()

