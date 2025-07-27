#!/usr/bin/python

import argparse
from pathlib import Path
import re
import orgreader2
import argcomplete
import pytools

try:
    import natsort
except ModuleNotFoundError as e:
    i = input("tyr: pip install natsort ? (y/N)")
    if i in ("y", "yes"):
        import pip
        if not pip.main(["install","natsort", "beautifulsoup4"]):
            print("安装成功，请重新启动程序")
    raise e

def safety_name(s:str)->str:
    return re.sub(r"([][])", r"\\\1", s)

def get_output(files:list[Path], title:str, subtitle:str, link:tuple[Path|None, Path|None]|None, args:argparse.Namespace) -> str:
    navigation = ""
    if link:
        navigation = "|上一页|本页文件总数|下一页|\n|-|\n"
        navigation += f"| [[./{safety_name(str(link[0]))}][{safety_name(link[0].stem)}]] |" \
                if link[0] else "| |"
        navigation += f"| {len(files)} "
        navigation += f"| [[./{safety_name(str(link[1]))}][{safety_name(link[1].stem)}]] |" \
                if link[1] else "| |"
    ret = f"""\
#+title: {title}
#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='{args.css_file}'/>
#+HTML_LINK_UP: ../
#+HTML_LINK_HOME: ./
#+HTML_HEAD: <style>li{"{margin: 0px}"}</style>
{f"* Header\n{navigation}" if link else ""}
* {subtitle}
"""
    li = [f"- [[{"./" if safety_name(str(i))[0] != "/" else "file://"}{safety_name(str(i))}]]" \
            for i in files]
    if args.split:
        nl = []
        for index,item in enumerate(li):
            if index % args.split == 0:
                nl.append(f"** {index+1}-{index+10}")
            nl.append(item)
        li = nl
    ret += "\n".join(li)
    ret += f"\n* Tail\n{navigation}" if link else ""
    return ret

def get_input_dir(inp:Path, pattern:re.Pattern)->list[Path]:
    dir_list = [i for i in inp.iterdir() \
            if i.is_dir() and \
            [j for j in i.iterdir() \
            if j.is_file() and pattern.match(j.name)]]
    dir_list = natsort.natsorted(dir_list)
    return dir_list

def save_file(f:Path, s:str):
    if f.exists():
        if not f.is_file():
            pytools.print_err(f"ERROR 目标输出是已存在文件夹 - {f}")
            return
        pytools.print_err(f"WARN 覆盖文件 - {f}")
    f.write_text(s, encoding="utf8")

def main():
    img_exts=["png", "jpg", "jpeg", "gif", "webp"]
    vid_exts=["mp4", "mkv", "3gp", "webm", "mov",
              "mpeg", "m4a", "mp3", "wav"]
    args = parse_arguments()
    extra_exts=[i for i in args.extern.split("|") if i!=""]
    pattern = re.compile(r".*\.(?:"+"|".join(img_exts+vid_exts+extra_exts)+")", re.I)
    input_d = Path(args.input)
    output_d = Path(args.output)
    if not input_d.is_dir() or not output_d.is_dir():
        pytools.print_err("ERROR 指定的输入或输出文件夹不是文件夹")
        return

    title = input_d.absolute().name
    if args.title:
        title = args.title

    dir_list : list[Path] = get_input_dir(input_d, pattern)
    if not dir_list and \
            [j for j in input_d.iterdir() if j.is_file() and pattern.match(j.name)]:
        dir_list = [input_d]

    if len(dir_list) > 1:
        print("INFO 创建首页")
        output = get_output([pytools.calculate_relative(Path(f"{i}.html"), output_d) for i in dir_list], f"INDEX:{title}", title, None, args)
        if args.save_org or args.no_export:
            save_file(Path(f"{output_d}/index.org"), output)
        if not args.no_export:
            save_file(Path(f"{output_d}/index.html"),
                      str(orgreader2.Document(output.splitlines()).to_html()))
        # print(output)
    elif dir_list:
        print("INFO 单文件(无章节)")
    else:
        pytools.print_err("WARN 好像没有找到识别范围内文件呢喵")
    dir_lists = [(Path(f"{i}.html") if i else None, j, Path(f"{k}.html") if k else None) \
            for i,j,k in zip([None]+dir_list, dir_list, dir_list[1:]+[None])]
    for lastdir,dirs,nextdir in dir_lists:
        file_list = [pytools.calculate_relative(i, output_d) \
                for i in dirs.iterdir() \
                if i.is_file() and pattern.match(i.name)]
        file_list = natsort.natsorted(file_list)
        output = get_output(file_list, title,
                f"{dir_list.index(dirs)+1} / {dirs}" if len(dir_list) > 1 else f"{dirs}",
                (lastdir, nextdir), args)
        objname = dirs.name
        if len(dir_list) <= 1:
            objname = "index"
        if args.save_org or args.no_export:
            save_file(Path(f"{output_d}/{objname}.org"), output)
        if not args.no_export:
            save_file(Path(f"{output_d}/{objname}.html"),
                      str(orgreader2.Document(output.splitlines()).to_html()))
        # print(output)

def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='用于生成一堆org文件然后导出成html（看漫画用的）')
    parser.add_argument('-i', '--input', default="./", help='设置输入目录(默认./)')
    parser.add_argument('-o', '--output', default="./", help='设置输出目录(默认./)')
    parser.add_argument('-c', '--css-file', default="../main.css", help='设置css文件(默认../main.css)')
    parser.add_argument('-e', '--extern', default="", help='设置额外查找文件后缀(`|`分割)')
    parser.add_argument('-t', '--title', default=None, help='设置标题')
    parser.add_argument('-s', '--split', type=int, nargs="?", default=0, const=10,
            help='在图片间插入标题用于快速跳转(指定分组)')
    parser.add_argument('-N', '--no-export', action="store_true", help='保存org文件但不导出')
    parser.add_argument('-S', '--save-org', action="store_true", help='保存org文件并导出')
    argcomplete.autocomplete(parser)
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    main()
