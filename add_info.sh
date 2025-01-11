#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2024 YouLanjie
#   
#   文件名称：info_add_by_name.sh
#   创 建 者：youlanjie
#   创建日期：2024年06月15日
#   描    述：
#
#================================================================

app_name="${0##*/}"

F_B='\033[1m'
F_I='\033[3m'
F_U='\033[4m'
F_S='\033[5m'
F_R='\033[7m'

F_black='\033[30m'
F_red='\033[31m'
F_green='\033[32m'
F_yellow='\033[33m'
F_blue='\033[34m'
F_purple='\033[35m'
F_cyan='\033[36m'
F_white='\033[37m'

F_C='\033[0m'

F_line="$F_B$F_red-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*$F_C"

# Show an INFO message
# $1: message string
# $2: style string
_msg_info() {
	local _msg="${1}"
	local _color="${2}"
	[[ "${quiet}" == "y" ]] || printf "$F_B${F_green}[%s] INFO:$F_cyan$F_I$_color %s$F_C\n" "${app_name}" "${_msg}"
}

# Show a WARNING message
# $1: message string
_msg_warning() {
	local _msg="${1}"
	printf "$F_B${F_yellow}[%s] WARNING:$F_I %s$F_C\n" "${app_name}" "${_msg}" >&2
}

# Show an ERROR message then exit with status
# $1: message string
# $2: exit code number (with 0 does not exit)
_msg_error() {
	local _msg="${1}"
	local _error=${2}
	printf "$F_B${F_red}[%s] ERROR:$F_I %s$F_C\n" "${app_name}" "${_msg}" >&2
	if (( _error > 0 )); then
	    exit "${_error}"
	fi
}

usage() {
	echo "\
usage: $app_name [options]
  options:
     -i           设置输入文件夹
     -t           设置标题
     -A           设置作曲家
     -a           设置专辑
     -p           设置封面
     -h           帮助信息"
	exit $1
}

get_info() {
	# 设置文件名（没有扩展名）
	format="`echo "$1" |sed "s/.*\/\(.*\)\.mp3/\1/"`"
	# 获取音频信息
	[[ $title_1 == "" ]] && title=`echo "$1" |sed -n "s/.* - \(.*\)\.mp3/\1/p"`
	[[ $artist_1 == "" ]] && artist=`echo "$1" |sed -n "s/.*\/\(.*\) - .*/\1/p"`
	[[ $album_1 == "" ]] && album=`ffprobe -v quiet -show_format "$1"|grep "TAG:album="|sed 's|^TAG:album=||'`
	[[ $album == "" ]] && album=$title
	[[ $icon == "" ]] && icon=$(cd "$input_dir/";find ./ -maxdepth 1 -iregex ".*/$format\.\(png\|jpg\|jpeg\)"|sed -n '1p'|sed 's|^./||')
	_msg_info "文件名:$1"
	_msg_info "音乐名称:$title"
	_msg_info "作者:$artist"
	_msg_info "专辑:$album"
}

check_dir() {
	# 设置输出目录
	if [[ -e "./out" ]] {
		if [[ ! -d "./out" ]] {
			_msg_warning "存在输出目录同名文件，为防止出错，退出"
			exit -1
		}
	}
	if [[ ! -d "./out" ]] {
		_msg_info "不存在输出目录，将创建" "$F_yellow"
		mkdir "./out" || (_msg_warning "创建文件夹失败，退出" && exit -1)
	}
}

add_info() {
	# 输入文件
	input_f="$1"
	# 输出文件
	out_f="./out/${format}.mp3"
	[[ -f "$icon" ]] && icon_cmd="-i \"$icon\" -map 0:0 -map 1:0 -id3v2_version 3" || icon_cmd=""
	cmd="\
ffmpeg -i \"$input_f\" $icon_cmd -c copy -metadata album=\"$album\" -metadata artist=\"$artist\" -metadata title=\"$title\" \
-metadata:s:v title='Album cover' -metadata:s:v comment='Cover (Front)' \"$out_f\" -v error"

	# 检查
	check_dir
	if [[ -e $out_f ]] {
		_msg_info "输出文件已存在，跳过" "$F_yellow"
		return 0
	}

	#_msg_info "Command:$cmd"
	#_msg_info "以下为程序输出："
	(zsh -c $cmd && _msg_info "'$input_f' 完成转换！" "$F_yellow") || _msg_warning "'$input_f' 转换出现问题！"
}

running() {
	for i ($input_dir/*.mp3) {
		echo $F_line
		i=$(echo $i|sed "s|//|/|g")
		get_info $i
		add_info $i
	}
}

title=""
artist=""
album=""
icon=""
title_1=""
artist_1=""
album_1=""

format=""

input_dir="."
while {getopts "i:t:A:a:p:nh" OPTION} {
	case $OPTION {
		i) input_dir=$OPTARG ;;
		t) title=$OPTARG && title_1="1" ;;
		A) artist=$OPTARG && artist_1="1" ;;
		a) album=$OPTARG && album_1="1" ;;
		p) icon=$OPTARG ;;
		h|?) usage ;;
		*) usage -1 ;;
	}
}

running

