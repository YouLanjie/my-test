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
     -h           帮助信息"
	exit $1
}

get_info() {
	# 获取音频信息
	title=`echo $1 |sed "s/.* - \(.*\)\.mp3/\1/"`
	artist=`echo $1 |sed "s/.*\/\(.*\) - .*/\1/"`
	_msg_info "音乐名称:$title"
	_msg_info "作者:$artist"
	# 设置文件名（没有扩展名）
	format="${artist} - ${title}"
	# 获取文件后缀名
	[[ $link != "" ]] && extension=$(echo $link|sed "s/.*\.//")
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
	input_f="${format}.mp3"
	# 输出文件
	out_f="./out/${format}.mp3"
	icon="${format}.jpg"

	[[ $album == "" ]] && album=$title

	# 检查
	check_dir
	if [[ -e $out ]] {
		_msg_info $i
		_msg_info "输出文件已存在，跳过" "$F_yellow"
		echo "$F_line"
		return 0
	}

	echo "$F_line"
	# 设置图标
	_msg_info "以下为程序输出："
	[[ $subtitle != "" ]] && title="${title}(${subtitle})"
	if [[ -f "$icon" ]] {
		(ffmpeg\
	            -i "$input_f"                                 \
	            -i "$icon" -map 0:0 -map 1:0 -id3v2_version 3 \
	            -c copy                                       \
	            -metadata artist="$artist"                    \
	            -metadata title="$title"                      \
	            -metadata TIT3="$subtitle"                    \
	            -metadata:s:v title='Album cover'             \
	            -metadata:s:v comment='Cover (Front)'         \
	            "$out_f" &&
	            _msg_info "'$input_f' 完成转换！" "$F_yellow"
	        ) || _msg_warning "'$input_f' 转换出现问题！"
	} else {
		(ffmpeg\
		    -i "$input_f"                                 \
		    -c copy                                       \
		    -metadata artist="$artist"                    \
		    -metadata title="$title"                      \
	            -metadata TIT3="$subtitle"                    \
		    -metadata:s:v title='Album cover'             \
		    -metadata:s:v comment='Cover (Front)'         \
		    "$out_f" &&
		    _msg_info "'$input_f' 完成转换！" "$F_yellow"
		) || _msg_warning "'$input_f' 转换出现问题！"
	}
}

running() {
	for i ($input_dir/*.mp3) {
		echo $F_line
		get_info $i
		echo "$F_line"
		add_info
	}
}

title=""
subtitle=""
artist=""
album=""

format=""

input_dir="."
while {getopts "i:nh" OPTION} {
	case $OPTION {
		i) input_dir=$OPTARG ;;
		h|?) usage ;;
		*) usage -1 ;;
	}
}

running

