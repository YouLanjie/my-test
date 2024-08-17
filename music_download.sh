#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#   
#   文件名称：music_download.sh
#   创 建 者：youlanjie
#   创建日期：2023年06月09日
#   描    述：下载网易云音乐（一站式解决）
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
     -i           设置备用输入文件夹
     -n           查找备用文件时使用通配符查找
     -l           优先使用本地文件
     -h           帮助信息"
	exit $1
}

get_info() {
	# 获取音频信息
	_msg_info "没有的信息可不填空着"
	_msg_info "音频链接:" && read link
	_msg_info "封面链接:" && read img_link
	_msg_info "音乐名称:" && read title
	_msg_info "别称:" && read subtitle
	_msg_info "作者:" && read artist
	_msg_info "专辑:" && read album
	clear
	# 获取没有后缀的下载链接
	link=$(echo $link|sed "s/?authSecret=.*//")
	# 获取高清的封面链接
	img_link=$(echo "$img_link" |sed 's/\(^.*\.jpg\).*/\1/')
	# 设置文件名（没有扩展名）
	format="${artist} - ${title}(${subtitle})"
	[[ $subtitle == "" ]] && format="${artist} - ${title}"
	# 获取文件后缀名
	[[ $link != "" ]] && extension=$(echo $link|sed "s/.*\.//")
}

local_sound() {
	if [[ $input_dir == "" ]] {
		return
	}
	[[ $flag_name == "true" ]] && grep_f="\.mp3\|\.m4a" || grep_f="${format}\.\(mp3\|\.m4a\)"
	input_f=$(ls "$input_dir"|grep "$grep_f"|sed -n "1p")
	_msg_info "mv \""$input_dir/$name_f"\" \"$name_f\""
	[[ -f "$input_dir/$input_f" ]] && (mv "$input_dir/$input_f" "$name_f" || (_msg_warning "错误！移动文件出错" && exit -1))
}

download_sound() {
	name_f="${format}.${extension}"
	if [[ $flag_local == "true" ]] {
		local_sound
	}
	if [[ $link != "" && $link != "NULL" ]] {
		echo $F_line
		_msg_info "wget \"$link\" -O \"$name_f\""
		wget "$link" -O "$name_f"||(_msg_warning "错误！下载出错" && exit -1)
	} elif [[ $input_dir != "" ]] {
		local_sound
	}
}

download() {
	get_info
	_msg_info "音频链接: $link"
	# 下载音频
	name_f="${format}.${extension}"
	download_sound
	# 下载封面
	name_f="${format}.jpg"
	_msg_info "wget \"$img_link\" -O \"$name_f\""
	wget "$img_link" -O "$name_f"
}

m4a_to_mp3() {
	# 输入文件
	input_f="${format}.m4a"
	# 输出文件
	out_f="${format}.mp3"

	# 检查
	if [[ -e $out_f ]] {
		_msg_info "$input_f"
		_msg_info "输出文件已存在，跳过" "$F_yellow"
		echo "$F_line"
        	return 0
	}
	_msg_info "输入文件：'$input_f'"
	_msg_info "ffmpeg -i \"$input_f\" \"$out_f\""
	echo "$F_line"
	_msg_info "以下为程序输出："
	(ffmpeg -i "$input_f" "$out_f" && rm "$input_f" && _msg_info "'$input_f' 完成转换！" "$F_yellow") || (_msg_warning "'$input_f' 转换出现问题！" && exit -1)
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
	_msg_info "'$input_f'"
	_msg_info "$title"
	_msg_info "$artist"
	_msg_info "$icon"
	echo "$F_line"
	# 设置图标
	_msg_info "以下为程序输出："
	[[ $subtitle != "" ]] && title="${title}(${subtitle})"
	if [[ -f "$icon" ]] {
		(ffmpeg\
	            -i "$input_f"                                 \
	            -i "$icon" -map 0:0 -map 1:0 -id3v2_version 3 \
	            -c copy                                       \
	            -metadata album="$album"                      \
	            -metadata artist="$artist"                    \
	            -metadata title="$title"                      \
	            -metadata TIT3="$subtitle"                    \
	            -metadata:s:v title='Album cover'             \
	            -metadata:s:v comment='Cover (Front)'         \
	            "$out_f" &&
	            rm "$input_f" "$icon" &&
	            _msg_info "'$input_f' 完成转换！" "$F_yellow"
	        ) || _msg_warning "'$input_f' 转换出现问题！"
	} else {
		(ffmpeg\
		    -i "$input_f"                                 \
		    -c copy                                       \
		    -metadata album="$album"                      \
		    -metadata artist="$artist"                    \
		    -metadata title="$title"                      \
	            -metadata TIT3="$subtitle"                    \
		    -metadata:s:v title='Album cover'             \
		    -metadata:s:v comment='Cover (Front)'         \
		    "$out_f" &&
		    rm "$input_f" &&
		    _msg_info "'$input_f' 完成转换！" "$F_yellow"
		) || _msg_warning "'$input_f' 转换出现问题！"
	}
}

running() {
	for i ({1..100}) {
		echo $F_line
		download
		[[ $file_type != "mp3" ]] && echo "$F_line"
		[[ $file_type != "mp3" ]] && m4a_to_mp3
		echo "$F_line"
		add_info
	}
}

link=""
img_link=""
title=""
subtitle=""
artist=""
album=""

format=""
extension="m4a"

input_dir=""
flag_name="false"
flag_local="false"
while {getopts "i:nhl" OPTION} {
	case $OPTION {
		i) input_dir=$OPTARG ;;
		n) flag_name="true" ;;
		l) flag_local="true" ;;
		h|?) usage ;;
		*) usage -1 ;;
	}
}

running

