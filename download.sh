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

	echo $line
	Info "音频链接"
	read link

	echo $line
	Info "封面链接"
	read img_link

	echo $line
	Info "名称"
	read name

	echo $line
	sec="yes"
	Info "别称?"
	select sec ("yes" "no") {
		if [[ $sec == "yes" ]] {
			Info "请输入:"
			read al
		}
		break
    	}

	echo $line
	Info "艺术家:"
	read art

	echo $line
	Info "专辑:"
	read alb
	
	# 下载音频
	echo $line
	cmd=$(printf "wget \"$link\" -O \"$art - %s.m4a\"\n" "$name")
	if [[ $sec == "yes" ]] {
		cmd=$(printf "wget \"$link\" -O \"$art - $name($al).m4a\"\n")
		echo "Has"
	}
	Info2 $cmd
	zsh -c $cmd

	# 下载封面
	img_link=$(echo "$img_link" |sed 's/\(^.*\.jpg\).*/\1/')
	cmd=$(printf "wget \"$img_link\" -O \"$art - %s.jpg\"\n" "$name")
	if [[ $sec == "yes" ]] {
		cmd=$(printf "wget \"$img_link\" -O \"$art - $name($al).jpg\"\n")
	}
	Info2 $cmd
	zsh -c $cmd

	# 设置专辑
	alb_f=$(printf "$art - %s.txt\n" "$name")
	if [[ $sec == "yes" ]] {
		alb_f=$(printf "$art - $name($al).txt\n")
	}
	Info2 "echo \"$alb\" > \"$cmd\""
	echo "$alb" > "$alb_f"
}
