#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2025 Chglish
#
#   文件名称：renames.sh
#   创 建 者：Chglish
#   创建日期：2025年03月14日
#   描    述：
#
#================================================================

app_name="${0##*/}"
usage() {
	echo "\
usage: $app_name [options]
  options:
     -i <dir>     输入文件或文件夹，默认./
     -p <prefix>  前缀
     -e <postfix> 后缀
     -m <mode>    模式选择(time|md5|auto)
     -f <format>  时间格式选择，默认'$time_fmt'
     -h           帮助信息"
	#echo "usage: timename_dir <dir> <prefix> <postfix>"
	exit $1
}

file_rename() {
	input="$1"
	#ext=$(echo $input|sed 's|.*\.||')
	ext=$(echo $input |sed -n "s/.*\\.\(.*\)/\1/p")
	mdfiytime=$(date -d "@$(stat $input -c "%Y")" +"$time_fmt")
	md5=""
	output=""

	output="${prefix}${mdfiytime}${postfix}.${ext}"
	if [[ "$mode" == "md5" ]] {
		md5="$(md5sum "$input"|awk '{print $1}')"
		output="${prefix}${md5}${postfix}.${ext}"
	}

	[[ "$input" -ef "$output" ]] && return 0

	if [[ -f "$output" && $mode != "time" && $mode != "md5" ]] {
		md5="$(md5sum "$input"|awk '{print $1}')"
		output="${prefix}${mdfiytime}_${md5}${postfix}.${ext}"
	}

	[[ "$input" -ef "$output" ]] && return 0

	if [[ -f "$output" ]] {
		echo "Error: 重命名'$input'时目标文件'$output'已存在"
		return -1
	}

	mv "$input" "$output"
	return $?
}

main() {
	if [[ -f "$input_arg" ]] {
		file_rename "$input_arg"
		return 0
	} elif [[ ! -d "$input_arg" ]] {
		echo "'$input_arg' 不存在"
		return -1
	}

	line=$(find "$input_arg" -maxdepth 1 -type f,l|sed "s|^\./||")
	echo $line |while {read input} {
		if [[ ! -f $input ]] {
			continue
		}
		file_rename "$input"
	}
}

input_arg="./"
prefix=""
postfix=""
time_fmt="%Y%m%d_%H%M%S"
mode="auto"

while {getopts "hi:p:e:m:f:" OPTION} {
	case $OPTION {
		i) input_arg="$OPTARG" ;;
		p) prefix="$OPTARG" ;;
		e) postfix="$OPTARG" ;;
		m) mode="$OPTARG" ;;
		f) time_fmt="$OPTARG" ;;
		h|?) usage ;;
		*) usage -1 ;;
	}
}

main

