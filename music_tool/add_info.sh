#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#
#   文件名称：add_info.sh
#   创 建 者：youlanjie
#   创建日期：2023年05月01日
#   描    述：为mp3文件增加歌手、专辑、标题等信息
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
	for i ($o_dir/*.mp3) {
		count=$count+1
		out=$i
		logfile=$(printf "%s/Log%03d" "$o_dir" $count)
		icon=$(echo $i |sed 's/\.mp3/\.jpg/')
		artist=$(echo $i |sed 's/^.*\/\(.*\) - .*/\1/')
		title=$(echo $i |sed 's/^.* - \(.*\)\.mp3/\1/')
		msg=$(echo $i|sed 's/.mp3$/.txt/')
		if [[ -f "$icon" ]] {
			arg2="-i \"$icon\" -map 0:0 -map 1:0 -id3v2_version 3"
		} else {
			arg2=" "
		}
		if [[ -f "$msg" ]] {
			album=$(cat $msg)
		} else {
			album=$title
		}
		arg="-c copy -metadata album=\"$album\" -metadata artist=\"$artist\" -metadata title=\"$title\" -metadata:s:v title='Album cover' -metadata:s:v comment='Cover (Front)'"

		if [[ -e "$o_dir/out" ]] {
			if [[ ! -d "$o_dir/out" ]] {
				_msg_warning "存在输出文件，为防止出错，退出"
				exit -1
			}
		}
		if [[ ! -d "$o_dir/out" ]] {
			_msg_info "不存在输出目录，将创建" "$F_yellow"
			mkdir "$o_dir/out" || (_msg_warning "创建文件夹失败，退出" && exit -1)
		}
		out=$(echo $out |sed "s/$o_dir\//$o_dir\/out\//")
		cmd="ffmpeg -i \"$i\" $arg2 $arg \"$out\" >$logfile 2>&1"

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
		_msg_info "$title"
		_msg_info "$artist"
		_msg_info "$icon"
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

o_out="true"
#setting
run
