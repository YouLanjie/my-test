#!/usr/bin/env python

import re
import argparse
import sys
import time
from pathlib import Path
import psutil
import numpy as np
import pandas as pd

def get_mem_by_free() -> tuple[float,float]:
    vm_rss = psutil.virtual_memory().used
    vm_swap = psutil.swap_memory().used
    return (vm_rss/1024/1024, vm_swap/1024/1024)

def get_mem_by_grep(grep:str) -> tuple[float, float]:
    count = {"RSS":0, "Swap":0}
    for pid in psutil.pids():
        try:
            process = psutil.Process(pid)
            if not re.search(grep, f"{process.username()} {process.name()} {process.cmdline()}"):
                continue
            vm_rss = process.memory_full_info().rss
            vm_swap = process.memory_full_info().swap
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue
        count["RSS"] += vm_rss
        count["Swap"] += vm_swap
    return (count["RSS"]/1024/1024, count["Swap"]/1024/1024)

def record(tag, output):
    text = f"Time,SystemVmRSS,SystemVmSwap,AppVmRSS({tag}),AppVmSwap({tag})\n"
    lt = ""
    with Path(output).open("a",encoding="utf-8") as f:
        try:
            print("time | sys | grep")
            f.write(text)
            while True:
                t = time.strftime("%Y-%m-%dT%H:%M:%S+08:00")
                if lt == t:
                    time.sleep(0.1)
                    continue
                lt = t
                free = get_mem_by_free()
                grep = get_mem_by_grep(tag)
                text = f"{t},{free[0]:.2f},{free[1]:.2f},{grep[0]:.2f},{grep[1]:.2f}\n"
                print(f"{t}: {free}, {grep}")
                f.write(text)
                f.flush()
        except KeyboardInterrupt:
            print("# C-c 退出", file=sys.stderr)
    # return "\n".join(save_text)

def get_args() -> argparse.Namespace:
    argparser = argparse.ArgumentParser(description="记录内存占用并绘制表格")
    argparser.add_argument("-o", "--output", default=None, help="输出文件(css|png)")
    argparser.add_argument("-p","--grep", default="", help="匹配样式")
    argparser.add_argument("-i", "--input", default=None, help="输入文件(不会记录，会绘图)")
    argparser.add_argument("-t", "--title", default=None, help="生成图表的标题")
    argparser.add_argument("-y", "--ylabel", default=None, help="生成图表y轴的label")
    argparser.add_argument("--width", type=int, default=16, help="表宽")
    argparser.add_argument("--heigh", type=int, default=9, help="表高")
    argparser.add_argument("--dashgrep", default=None, help="使用虚线的关键词")
    argparser.add_argument("--precent", action="store_true", help="使用百分比（统一y轴）")
    argparser.add_argument("--log", action="store_true", help="使用对数")
    try:
        __import__("argcomplete").autocomplete(argparser)
    except ModuleNotFoundError:
        pass
    args = argparser.parse_args()
    return args

def render(args:argparse.Namespace):
    args.output = args.output or f"{time.strftime("%Y%m%d_%H%M%S")}{"_"+args.grep if args.grep else ""}.png"
    if not Path(args.input).is_file():
        print("文件不存在")
        return
    df = pd.read_csv(args.input)
    df_cols = list(df)
    if len(df_cols) < 2:
        print("表格没有可对应的数值")
        return
    x_name = df_cols[0]
    y_list = df_cols[1:]
    padding = len(df[x_name])/args.width
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    fig,ax = plt.subplots(1,1,figsize=(args.width, args.heigh))
    index = np.arange(0,len(df[x_name]), padding)
    ax.xaxis.set_major_locator(ticker.MultipleLocator(padding))
    for label in y_list:
        y_data = df[label]
        if args.log:
            y_data = np.log2(y_data+1)
            label+="(log2)"
        if args.precent:
            y_data = y_data/max(y_data)
        if args.dashgrep and re.search(args.dashgrep,label,re.I):
            ax.plot(df[x_name], y_data, linewidth=1, linestyle='--', label=label)
        else:
            ax.plot(df[x_name], y_data, linewidth=1, label=label)
    plt.title(f'{args.title}({args.input})')
    plt.xlabel(x_name,fontsize=5)
    plt.xticks(rotation=45, fontsize=5)
    ax.set_ylabel(args.ylabel)
    # plt.ylabel(args.ylabel)
    plt.grid(True)
    # 添加图例
    plt.legend()
    # plt.savefig('output_chart.png', dpi=3000)  # 保存图片
    plt.rcParams['font.sans-serif'] = ['SimHei']
    plt.savefig(args.output, dpi=300)  # 保存图片
    # plt.show()

def main():
    args = get_args()
    if not args.input:
        args.output = args.output or f"{time.strftime("%Y%m%d_%H%M%S")}{"_"+args.grep if args.grep else ""}.csv"
        record(args.grep, args.output)
        return
    render(args)

if __name__ == "__main__":
    main()
