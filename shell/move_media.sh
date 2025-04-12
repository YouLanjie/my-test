#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 Chglish
#
#   文件名称：move_video.sh
#   创 建 者：Chglish
#   创建日期：2023年04月02日
#   描    述：将媒体文件按照文件名的时间排列进入文件夹
#
#================================================================

usage() {
	usagetext="\
usage: ${0##*/} [options]
  options:
     -i <dirname> 指定输入文件夹
     -o <dirname> 指定输出根文件夹
     -m           使用mv命令（默认）
     -c           使用cp命令
     -f <format>  指定输出路径格式(\1/\1_\2/\1_\2_\3)
     -v           执行时显示更多输出
     -h           帮助信息"
	echo $usagetext
	exit $1
}

input="."
output="."
move_type=1
# 1: mv
# 2: cp
format="\1/\1_\2/\1_\2_\3"
verbose="false"

check() {
	if [[ ! -d $1 ]] {
		echo "无效的选项:$1 不是一个文件夹"
		usage -1
	}
}

quit() {
	echo "无效的sed格式:$format"
	exit -1
}

move() {
	#update_file index.org
	#echo $file_list
	file_list=$(cd "$input";find ./ -maxdepth 1 -iregex '.*\.\(mp4\|mkv\|png\|jpg\|dng\|jpeg\|gif\|3gp\|m4a\|webp\)'|sed 's|^./||')
	max_line=$(echo $file_list|wc -l)
	for (( i=1; i <= $max_line; i++)) {
		name=$(echo $file_list|sed -n "${i}p")
		ymd=$(echo $name|sed -n "s|[^0-9]*\([0-9]\{4\}\)[-_年]*\([0-9]\{2\}\)[-_月]*\([0-9]\{2\}\).*|$format|p" || quit)
		if [[ $ymd == "" ]] {
			continue
		}
		out_d=$(find "$output" -maxdepth 3 -type d -wholename "*$ymd*"|sed -n '1p')
		if [[ $out_d == "" ]] out_d="$output/$ymd/"
		out="$out_d/$name"
		if [[ ! -d $out_d ]] {
			[[ $verbose == "true" ]] && echo "mkdir $out_d"
			mkdir -p "$out_d"
		}
		if [[ -f $out ]] {
			if [[ $(md5sum $name|awk '{print $1}') == $(md5sum $out|awk '{print $1}') ]] {
				[[ $verbose == "true" ]] && echo "\033[1;30;40m相同的文件: $name\033[0m"
				if [[ ! -f "$name.bak" ]] mv "$name" "$name.bak"
				continue
			}
			echo "无法移动的重名文件: $name"
			continue
		}
		if (( $move_type == 1 )) {
			mv "$name" "$out"
		} else {
			cp "$name" "$out"
		}
	}
}

while {getopts 'i:o:mcf:vh?' arg} {
	case $arg {
		i) check $OPTARG && input=$OPTARG ;;
		o) check $OPTARG && output=$OPTARG ;;
		m) move_type=1 ;;
		c) move_type=2 ;;
		f) format=$OPTARG ;;
		v) verbose="true" ;;
		h|?) usage 0;;
		*) usage 1;;
	}
}

move

