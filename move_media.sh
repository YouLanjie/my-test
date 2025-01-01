#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#
#   文件名称：move_video.sh
#   创 建 者：youlanjie
#   创建日期：2023年04月02日
#   描    述：将录屏文件(obs)按照时间排列进入文件夹
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
    printf "$F_B${F_red}[%s] ERROR:$F_I %s$F_C\n"  "${app_name}" "${_msg}" >&2
    if (( _error > 0 )); then
        exit "${_error}"
    fi
}

usage() {
	usagetext="\
usage: $app_name [options]
  options:
     -i [dirname] 指定输入文件夹
     -o [dirname] 指定输出根文件夹
     -m           使用mv命令（默认）
     -c           使用cp命令
     -h           帮助信息"
	echo $usagetext
	exit $1
}

input="."
output="."
move_type=1
# 1: mv
# 2: cp

check() {
	_msg_info "$1: $2"
	if [[ ! -d $2 ]] {
		_msg_warning "$2 不是一个文件夹"
		_msg_error "无效的选项 -- $2"
		usage -1
	}
	_msg_info "Check Pass..."
}

move() {
	#update_file index.org
	#echo $file_list
	file_list=$(cd "$input";find ./ -maxdepth 1 -regex '.*\.\(mp4\|mkv\|png\|jpg\|dng\|jpeg\|gif\)')
	max_line=$(echo $file_list|wc -l)
	for (( i=1; i <= $max_line; i++)) {
		name=$(echo $file_list|sed -n "${i}p")
		ymd=$(echo $name|sed -n "s|[^0-9]*\([0-9]\{4\}\)[-年]*\([0-9]\{2\}\)[-月]*\([0-9]\{2\}\).*|\1/\1_\2/\1_\2_\3|p")
		if [[ $ymd == "" ]] {
			continue
		}
		echo ""
		out_d=$(find "$output" -maxdepth 3 -type d -wholename "*$ymd*"|sed -n '1p')
		if [[ $out_d == "" ]] out_d="$output/$ymd/"
		out="$out_d/$name"
		if [[ ! -d $out_d ]] {
			_msg_warning "No Out Dir : $out_d"
			_msg_info "Make Dir : $out_d"
			mkdir -p "$out_d"
		}
		_msg_info "Source File: $name"
		_msg_info "Object File: $out"
		if [[ -f $out ]] {
			_msg_warning "Has same name file: $out"
			_msg_info "Checking now..."
			_msg_info "MD5 of  Input File: $(md5sum $name|awk '{print $1}')"
			_msg_info "MD5 of Output File: $(md5sum $out|awk '{print $1}')"
			if [[ $(md5sum $name|awk '{print $1}') == $(md5sum $out|awk '{print $1}') ]] {
				_msg_info "Same file data. Skipping..."
				continue
			}
			_msg_warning "Different file data. The object file will be overwritten..."
		}
		if (( $move_type == 1 )) {
			_msg_info "Moving!"
			mv "$name" "$out"
		} else {
			_msg_info "Copying!"
			cp "$name" "$out"
		}
	}
}

while {getopts 'i:o:mch?' arg} {
	case $arg {
		i) check "Input  Dir Set" $OPTARG && input=$OPTARG ;;
		o) check "Output Dir Set" $OPTARG && output=$OPTARG ;;
		m) move_type=1 ;;
		c) move_type=2 ;;
		h|?) usage 0;;
		*) usage 1;;
	}
}

_msg_info "Input  Dir: $input"
_msg_info "Output Dir: $output"
move

