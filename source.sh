#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#
#   文件名称：source.sh
#   创 建 者：youlanjie
#   创建日期：2023年05月01日
#   描    述：设置颜色样式
#
#================================================================

bold="\033[1m"
red="\033[31m"
green="\033[32m"
yellow="\033[33m"
blue="\033[34m"
clean="\033[0m"

line="$bold$red-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*$clean"

Info() {
    echo "$bold$blue" "Info:$yellow " "$1" "$clean"
}

Info2() {
    echo "$bold$blue" "Info:$green " "$1" "$clean"
}

Warn() {
    echo "$bold$yellow" "Waring: $1" "$clean"
}
