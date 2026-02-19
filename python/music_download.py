#!/usr/bin/env python

from pathlib import Path
import json
import re
import subprocess
import argparse
import shutil
import requests

try:
    import readline
    del readline
except ModuleNotFoundError:
    pass

# 设置请求头
headers = {
    'User-Agent':'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'
}

def download_file(url:str, file:Path) -> bool:
    """下载音频文件"""
    if file.is_file():
        print(f"INFO 跳过已下载文件 {file}")
        return True
    try:
        response = requests.get(url, headers=headers, timeout=5)  # 发送 GET 请求
        response.raise_for_status()  # 如果响应状态码不是 200，抛出异常
        if response.url == 'https://music.163.com/404':
            print("ERR: 下载失败！(被重定向到404页,或许这是vip可听曲)")
            return False
        file.write_bytes(response.content)
        print(f'INFO 下载完成！ {file}')  # 打印下载完成信息
    except requests.exceptions.RequestException as e:
        print(f'INFO 下载失败！({e})')  # 打印下载失败信息
        return False
    return True

def get_real_link(link:str) -> str:
    """将手机复制的分享链接转含歌曲id的普通链接"""
    match = re.match(r".*(https://163cn.tv/[a-zA-Z0-9]+)", link)
    link = match.group(1) if match else link
    try:
        response = requests.head(link, timeout=5)
        response.raise_for_status()
        link = response.headers["Location"]
    except requests.exceptions.RequestException as e:
        print('请求出错:', e)
    return link

class Song:
    """歌曲类"""
    def __init__(self, data: int|str|dict) -> None:
        self.song_data = {}
        if isinstance(data, dict):
            self.song_data = data
        if isinstance(data, int):
            data = str(int)
        if not isinstance(data, str):
            return
        url = data
        try:
            int(url)
        except ValueError:
            if "https://163cn.tv/" in url:
                url = get_real_link(url)
            match = re.match(r".*id=(\d+)", url)
            if not match:
                return
            url = match.group(1)
        url = f"http://music.163.com/api/song/detail/?ids=[{url}]"
        url = url.encode()
        try:
            response = requests.get(url, headers=headers, timeout=5)
            response.raise_for_status()
            result = response.json()
            self.song_data = result.get("songs")[0] if result.get("songs") else {}
        except requests.exceptions.RequestException as e:
            print('请求出错:', e)
    def download(self, output_dir:Path, use_safe_name=True, inpd:Path|None=None):
        """下载音频文件"""
        print(f"PROCESS: {self.full_info}")
        if not output_dir.is_dir():
            print(f"ERR: {output_dir} 不存在或非文件夹")
            return

        # 获取基本信息
        output_basename = f"{self.artist} - {self.name}"
        replacement = ["/", "／"]
        if use_safe_name:
            replacement = ["\\/:*?\"<>", "＼／∶＊？＂〈〉｜"]
        for k,l in zip(list(replacement[0]), list(replacement[1])):
            output_basename=output_basename.replace(k, l)
        tmp_outputf = output_dir / Path(f"{output_basename}_tmp.mp3")
        cover_outputf = None
        if self.cover:
            extention = re.match(r".*\.(a-Z)", self.cover)
            extention = extention.group(1) if extention else "png"
            cover_outputf = output_dir / Path(f"{output_basename}.{extention}")
        outputf = output_dir / Path(f"{output_basename}.mp3")

        # 文件下载处理
        if outputf.is_file():
            print("INFO 跳过SKIP")
            return
        url = f"https://music.163.com/song/media/outer/url?id={self.id}.mp3"
        if inpd and inpd.is_dir():
            fl = sorted(inpd.glob("*.mp3"), key=lambda x:x.stat().st_mtime)
            if fl:
                shutil.copy(fl[-1], tmp_outputf)
        if not download_file(url, tmp_outputf):
            return
        if cover_outputf:
            download_file(self.cover, cover_outputf)

        # 文件合并处理
        command = ["ffmpeg", "-v", "error",
                   "-i", repr(str(tmp_outputf.resolve()))]
        if cover_outputf and cover_outputf.is_file():
            command += ["-i", repr(str(cover_outputf.resolve())),
                        "-map","0:0","-map","1:0","-id3v2_version","3"]
        command += ["-c", "copy"]
        command += ["-metadata", f"album={repr(self.album)}",
                    "-metadata", f"artist={repr(self.artist)}",
                    "-metadata", f"title={repr(self.name)}",
                    "-metadata", f"comment='SongId: {self.id}'",
                    "-metadata:s:v", "title='Album cover'",
                    "-metadata:s:v", "comment='Cover (Front)'"]
        command += [repr(str(outputf.resolve())), "&&",
                    "rm", repr(str(tmp_outputf.resolve()))]
        if cover_outputf and cover_outputf.is_file():
            command += ["&&", "rm", repr(str(cover_outputf.resolve()))]
        command = " ".join(command)
        # print(command)
        ret = subprocess.run(command, shell=True, check=False)
        if ret.returncode != 0:
            print(f"ERROR 命令错误: {command}")
            print(f"ERROR {tmp_outputf}")
            print(f"ERROR {str(tmp_outputf).encode()}")
            print(f"ERROR {repr(str(tmp_outputf))}")
        print("INFO 转换完成")
    def filter(self, s:str) -> str:
        """过滤字符串不可见字符"""
        s = "".join(i for i in s if i.isprintable() or i in "\r\n\t")
        return s
    @property
    def full_info(self) -> str:
        """返回易读的信息行"""
        return f"[{self.album}] {self.artist} - {self.name}"
    @property
    def id(self) -> str:
        return str(self.song_data.get('id') or "")
    @property
    def album(self) -> str:
        """返回专辑名"""
        album_data = self.song_data.get("album")
        if not album_data:
            return ""
        album = str(album_data.get("name"))
        # aliases = album_data.get("alias")
        # if aliases:
            # album += f"({",".join(aliases)})"
        # print(album)
        return self.filter(album)
    @property
    def cover(self) -> str:
        """返回封面链接（其实是专辑封面）"""
        album_data = self.song_data.get("album")
        if not album_data:
            return ""
        cover = str(album_data.get("picUrl"))
        return cover
    @property
    def name(self) -> str:
        """返回歌曲名"""
        name = str(self.song_data.get("name"))
        trans_name = self.song_data.get("transName")
        if trans_name:
            name += f"({trans_name})"
        # print(name)
        return self.filter(name)
    @property
    def artist(self) -> str:
        """返回作者"""
        artists = self.song_data.get("artists")
        if not artists:
            return ""
        artists = [i.get("name") for i in artists if i.get("name")]
        return self.filter(",".join(artists))

# 获取指定歌手的歌曲信息
def get_artist_songs(artist_id):
    """
    发送请求获取指定歌手的歌曲信息
    参数:
    artist_id (str): 歌手的 ID
    返回:
    list or None: 成功则返回歌曲列表，否则返回 None
    """
    url = 'https://music.163.com/api/v1/artist/songs'  # 歌手歌曲信息 API 的 URL
    params = {
        'id': artist_id,  # 歌手 ID
        'offset': 0,  # 偏移量
        'total': True,  # 是否获取全部歌曲信息
        'limit': 1000  # 获取歌曲数量
    }
    try:
        response = requests.get(url, headers=headers, params=params, timeout=5)  # 发送 GET 请求
        response.raise_for_status()  # 如果响应状态码不是 200，抛出异常
        result = json.loads(response.text)  # 将响应的文本内容转为 JSON 格式
        # print(response.text)
        songs = result['songs']  # 获取歌曲列表
        return [Song(i) for i in songs]
    except requests.exceptions.RequestException as e:
        print('请求出错:', e)  # 打印请求错误信息
        return []

def get_playlist_songs(playlist_id):
    """获取播放列表的歌曲"""
    url = "https://music.163.com/api/playlist/detail"
    params = {
        'id': playlist_id,  # 歌手 ID
        'offset': 0,  # 偏移量
        'total': True,  # 是否获取全部歌曲信息
        'limit': 1000  # 获取歌曲数量
    }
    try:
        response = requests.get(url, headers=headers, params=params, timeout=5)  # 发送 GET 请求
        response.raise_for_status()  # 如果响应状态码不是 200，抛出异常
        result = response.json()  # 将响应的文本内容转为 JSON 格式
        result = result.get("result")
        print(response)
        if not result:
            return []
        songs = result.get('tracks')  # 获取歌曲列表
        return [Song(i) for i in songs]
    except requests.exceptions.RequestException as e:
        print('请求出错:', e)  # 打印请求错误信息
        return []

def example1():
    # example 1:
    li = [Song("27961159"),
          Song("https://music.163.com/song?id=1468651128"),
          Song("https://music.163.com/song?id=1368929712"),
          Song("https://music.163.com/song?id=2615855920")]
    for i in li:
        i.download(Path("."))

def example2():
    # 下载歌手的热门歌曲和播放列表的歌曲
    artist_id = '41993'
    songs = get_artist_songs(artist_id)  # 获取歌手的歌曲信息
    songs += get_playlist_songs("14139872015")  # 获取播放列表的歌曲信息
    save_dir = Path(".")  # 保存目录
    for i in songs:
        print(i.full_info)
        i.download(save_dir)

def main():
    # 主函数
    parser = argparse.ArgumentParser(description="网易云音乐下载器")
    parser.add_argument("-o", "--output-dir", default=".", help="设置输出文件夹")
    parser.add_argument("-i", "--input-dir", help="设置备用输入文件夹(抽里面的mp3当作音源)")
    args = parser.parse_args()
    print("输入歌曲id或者链接，自动下载到当前目录，C-c退出")
    try:
        while True:
            link = input("请输入歌曲链接或者id：")
            inpd = Path(args.input_dir) if args.input_dir else None
            Song(link).download(Path(args.output_dir), inpd=inpd)
    except EOFError:
        print("侦测到C-d,退出")
    except KeyboardInterrupt:
        print("侦测到C-c,退出")

if __name__ == "__main__":
    main()
