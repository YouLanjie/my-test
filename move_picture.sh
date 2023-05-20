#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#
#   文件名称：move.sh
#   创 建 者：youlanjie
#   创建日期：2023年04月02日
#   描    述：构建博客
#
#================================================================

source ./source2.sh

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
	file_list=$(\
		find $input/ -maxdepth 1 -name '*.png' &&\
		find $input/ -maxdepth 1 -name '*.jpg')
	max_line=$(echo $file_list|wc -l)
	for (( i=1; i <= $max_line; i++)) {
		name=$(echo $file_list|sed -n "${i}p")
		out=$(echo $name|sed -n "s/$input\(.*\)\(Screenshot_\)\([0-9]\{4\}\)\([0-9]\{2\}\)\([0-9]\{2\}\)\(_.*\)/$output\1\3\/\3_\4\/\3_\4_\5\/\2\3\4\5\6/p")
		if [[ $out == "" ]] {
			continue
		}
		echo ""
		#out_d=$(echo $name|sed -n "s/\(Screenshot_\)\([0-9]\{4\}\)\([0-9]\{2\}\)\([0-9]\{2\}\)\(_.*\)/\2\/\2_\3\/\2_\3_\4/p")
		out_d=$(echo $name|sed -n "s/$input\(.*\)\(Screenshot_\)\([0-9]\{4\}\)\([0-9]\{2\}\)\([0-9]\{2\}\)\(_.*\)/$output\1\3\/\3_\4\/\3_\4_\5/p")
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

