#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#   
#   文件名称：build.sh
#   创 建 者：youlanjie
#   创建日期：2023年08月07日
#   描    述：用于构建仓库的程序
#
#================================================================


check() {
	if [[ $(ls include/lib/) == "" ]] {
		echo 'WARN: dir`include/lib/`为空或不存在。请尝试执行以下命令'
		echo 'CMD> git submodule init'
		echo 'CMD> git submodule sync'
		exit -1
	}
	if {! stat bin >/dev/null 2>&1} {
		mkdir bin
	}
}

build_lib() {
	cd ./include/lib/
	gcc -c `find ./ -name "*.c"`
	ar -rv ./libtools.a ./*.o
	rm ./*.o
	cd ../../
}

build_file() {
	declare -A build_arg=(		\
		["RSA.c"]="-lm"		\
		["input.c"]="-lm"	\
		["build.c"]="#"		\
		["gtk.c"]="$(pkg-config --cflags --libs gtk+-3.0 2>/dev/null || echo "#")")
	if [[ $build_arg[$(echo $1 | sed 's/^.*\///')] == "#" ]] {
		return -1
	}
	if {! file $1|grep "C source" >/dev/null 2>&1} {
		echo "WARN: '$1'非C语言文件！Skip!"
		return -2
	}
	cmd=$(echo "gcc $1 -g -Wall -Linclude/lib -ltools -lncurses $build_arg[$(echo $1 | sed 's/^.*\///')] -o bin/$(echo $1 | sed 's/^.*\///'| sed 's/\.c$//')")
	if [[ $flag_cmd != "" || $flag_source != "" ]] {
		cmd=$(echo "gcc $1 -g -Wall `find ./include/lib -name "*.c"|sed ":a;N;s/\n/ /g;b a"` -lncurses $build_arg[$(echo $1 | sed 's/^.*\///')] -o bin/$(echo $1 | sed 's/^.*\///'| sed 's/\.c$//')")
	}
	echo "\033[0;1;32mCOMMAND> \033[0;1;33m$cmd\033[0m"
	if [[ $flag_cmd == "" ]] {
		zsh -c $cmd
	}
}

build_dir() {
	ls src/*/*.c |while {read line} {
		build_file $line
	}
}

app_name="${0##*/}"
usage() {
	usagetext="\
usage: $app_name [options]
  options:
     -c          只是获取编译命令
     -s          不使用lib文件编译
     -f filename 指定编译的C文件
     -n          不删除lib文件
     -h          帮助信息"
	echo $usagetext
	exit $1
}

obj_file=""
nc_lib=""
flag_cmd=""
flag_source=""
while {getopts 'csf:nh?' arg} {
	case $arg {
		c) flag_cmd="true" ;;
		s) flag_source="true" ;;
		f) obj_file=$OPTARG ;;
		n) nc_lib="true" ;;
		h|?) usage 0;;
		*) usage 1;;
	}
}

if [[ $flag_cmd == "" && $flag_source == "" ]] {
	check
	if [[ ! -f include/lib/libtools.a ]] {
		build_lib
	}
}
if [[ $obj_file == "" ]] {
	build_dir
} else {
	if {! stat $obj_file >/dev/null 2>&1} {
		echo "ERROR:文件不存在！" >&2
		return -3
	}
	build_file $obj_file
}
if [[ $nc_lib == "" ]] {
	rm -f include/lib/libtools.a
}

