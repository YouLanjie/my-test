#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#
#   文件名称：m4a_to_mp3.sh
#   创 建 者：youlanjie
#   创建日期：2023年05月01日
#   描    述：将m4a文件转换为mp3文件
#
#================================================================

source ./source.sh 

usage() {
	usagetext="\
usage: $app_name [options] <input dir>
  options:
     -h           帮助信息"
	echo $usagetext
	exit $1
}

if [[ $1 == "-h" ]] {
	usage
}

o_dir=$1
((! $+1)) && {
	o_dir="."
}
_msg_info "输入目录: $o_dir"
o_out="true"

run() {
	echo "$F_line"
	count=0
	for i ($o_dir/*.m4a) {
		count=$count+1
		out=$(echo $i |sed "s/.m4a$/.mp3/")
		logfile=$(printf "%s/Log%03d" "$o_dir" $count)

		cmd="ffmpeg -i \"$i\" \"$out\" >$logfile 2>&1"
		if [[ $o_out == "true" ]] {
			if [[ -e "$o_dir/out" ]] {
				if [[ ! -d "$o_dir/out" ]] {
					_msg_warning "存在输出目录，为防止出错，退出"
					exit -1
				}
			}
			if [[ ! -d "$o_dir/out" ]] {
				_msg_info "不存在输出目录，将创建" "$F_yellow"
				mkdir "$o_dir/out" || (_msg_warning "创建文件夹失败，退出" && exit -1)
			}
			out=$(echo $out |sed "s/$o_dir\//$o_dir\/out\//")
			cmd="ffmpeg -i \"$i\" \"$out\" >$logfile 2>&1"
		}

		# 检查
		if [[ -e $out ]] {
			_msg_info $i
			_msg_info "输出文件已存在，跳过" "$F_yellow"
			echo "$F_line"
			count=$count-1
			continue
		}

		if [[ -e $logfile ]] {
			_msg_info "'$logfile'"
			_msg_warning "日志文件已存在，将会覆盖文件"
		}

		_msg_info "'$i'"
		_msg_info "$cmd"
		echo "$F_line"
		{(zsh -c "$cmd" && rm "$logfile" && _msg_info "'$i' 完成转换！" "$F_yellow") || _msg_warning "'$i' 转换出现问题！详见 '$logfile'" } &
	}
	_msg_info "所有文件准备就绪，开始等待" "$F_yellow"
	wait
	_msg_info "等待完毕！" "$F_yellow"
	echo "$F_line"
}

setting() {
	echo "$F_line"
	_msg_info "这里有一些配置"
	_msg_info "输出到单独的目录？"
	select o_out ("false" "true") {
		break
	}
}

setting
run
