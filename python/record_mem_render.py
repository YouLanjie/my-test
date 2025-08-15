#!/usr/bin/env python

import re
import subprocess
import argparse
import sys
import time
from pathlib import Path
import numpy as np
import pandas as pd

def get_mem_by_free() -> tuple[float,float]:
    vm_rss = 0
    vm_swap = 0
    output = subprocess.run("free|sed '1d'",
                            shell=True,check=False,
                            capture_output=True).stdout.decode().splitlines()
    try:
        vm_rss = int(output[0].split()[2])
        if len(output) > 1:
            vm_swap = int(output[1].split()[2])
    except ValueError:
        pass
    return (vm_rss/1024, vm_swap/1024)

def get_mem_by_grep(grep:str) -> tuple[float, float]:
    pid_list = subprocess.run("ps aux|grep "+repr(grep)+"|sed '1d'|awk '{print $2}'",
                              shell=True,check=False,capture_output=True).stdout.decode().splitlines()
    count = {"RSS":0, "Swap":0}
    for pid in pid_list:
        status = Path(f"/proc/{pid}/status")
        if not status.is_file():
            continue
        status_text = status.read_text(encoding="utf-8")
        vm_rss = re.search(r"VmRSS:\s*(\d+)", status_text)
        vm_swap = re.search(r"VmSwap:\s*(\d+)", status_text)
        count["RSS"] += int(vm_rss.group(1)) if vm_rss else 0
        count["Swap"] += int(vm_swap.group(1)) if vm_swap else 0
    return (count["RSS"]/1024, count["Swap"]/1024)

def record(tag) -> str:
    save_text = [f"Time,SystemVmRSS,SystemVmSwap,AppVmRSS({tag}),AppVmSwap({tag})"]
    counter = []
    lt = ""
    try:
        # print("time | sys | grep")
        print(save_text[-1])
        while True:
            t = time.strftime("%Y-%m-%dT%H:%M:%S+08:00")
            if lt == t:
                time.sleep(0.1)
                continue
            lt = t
            free = get_mem_by_free()
            grep = get_mem_by_grep(tag)
            counter.append((t,free,grep))
            save_text.append(f"{t},{free[0]:.2f},{free[1]:.2f},{grep[0]:.2f},{grep[1]:.2f}")
            # print(f"{t}: {free}, {grep}")
            print(save_text[-1])
    except KeyboardInterrupt:
        print("# C-c 退出", file=sys.stderr)
    return "\n".join(save_text)

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
        save_text = record(args.grep)
        Path(args.output).write_text(save_text, encoding="utf-8")
        return
    render(args)

if __name__ == "__main__":
    main()

