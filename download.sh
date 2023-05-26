#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 Chglsih
#
#   文件名称：download.sh
#   创 建 者：水煮木头
#   创建日期：2023年05月01日
#   描    述：下载音乐
#
#================================================================

source ./source.sh 

for i ({1..100}) {
	clear

	echo $F_line
	_msg_info "音频链接" "$F_yellow"
	read link

	echo $F_line
	_msg_info "封面链接" "$F_yellow"
	read img_link

	echo $F_line
	_msg_info "名称" "$F_yellow"
	read name

	echo $F_line
	sec="yes"
	_msg_info "别称?" "$F_yellow"
	select sec ("yes" "no") {
		if [[ $sec == "yes" ]] {
			_msg_info "请输入:" "$F_yellow"
			read al
		}
		break
    	}

	echo $F_line
	_msg_info "艺术家:" "$F_yellow"
	read art

	echo $F_line
	_msg_info "专辑:" "$F_yellow"
	read alb
	
	# 下载音频
	echo $F_line
	cmd=$(printf "wget \"$link\" -O \"$art - %s.m4a\"\n" "$name")
	if [[ $sec == "yes" ]] {
		cmd=$(printf "wget \"$link\" -O \"$art - $name($al).m4a\"\n")
		echo "Has"
	}
	_msg_info $cmd
	zsh -c $cmd

	# 下载封面
	img_link=$(echo "$img_link" |sed 's/\(^.*\.jpg\).*/\1/')
	cmd=$(printf "wget \"$img_link\" -O \"$art - %s.jpg\"\n" "$name")
	if [[ $sec == "yes" ]] {
		cmd=$(printf "wget \"$img_link\" -O \"$art - $name($al).jpg\"\n")
	}
	_msg_info $cmd
	zsh -c $cmd

	# 设置专辑
	alb_f=$(printf "$art - %s.txt\n" "$name")
	if [[ $sec == "yes" ]] {
		alb_f=$(printf "$art - $name($al).txt\n")
	}
	_msg_info "echo \"$alb\" > \"$cmd\""
	echo "$alb" > "$alb_f"
}
