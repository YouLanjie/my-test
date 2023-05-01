#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#
#   文件名称：Type_conversion.sh
#   创 建 者：youlanjie
#   创建日期：2023年05月01日
#   描    述：翻译翻译m4a
#
#================================================================

source ./source.sh 

o_dir=$1
((! $+1)) && {
    o_dir="."
}
o_out="true"

run() {
    echo "$line"
    count=0
    for i ($o_dir/*.m4a) {
	count=$count+1
	out=$(echo $i |sed "s/.m4a$/.mp3/")
	logfile=$(printf "%s/Log%03d" "$o_dir" $count)

	cmd="ffmpeg -i '$i' '$out' >$logfile 2>&1"
	if [[ $o_out == "true" ]] {
	       if [[ -e "$o_dir/out" ]] {
		      if [[ ! -d "$o_dir/out" ]] {
			     Warn "存在输出目录，为防止出错，退出"
			     exit -1
			 }
		  }
	       if [[ ! -d "$o_dir/out" ]] {
		      Info "不存在输出目录，将创建"
		      mkdir "$o_dir/out" || (Warn "创建文件夹失败，退出" && exit -1)
		  }
	       out=$(echo $out |sed "s/$o_dir\//$o_dir\/out\//")
	       cmd="ffmpeg -i '$i' '$out' >$logfile 2>&1"
	   }

	# 检查
	if [[ -e $out ]] {
	       Info2 $i
	       Info "输出文件已存在，跳过"
	       echo "$line"
	       count=$count-1
	       continue
	   }

	if [[ -e $logfile ]] {
	       Info2 "'$logfile'"
	       Warn "日志文件已存在，将会覆盖文件"
	   }

	Info2 "'$i'"
	Info2 "$cmd"
	echo "$line"
	{(zsh -c "$cmd" && rm "$logfile" && Info "'$i' 完成转换！") || Warn "'$i' 转换出现问题！详见 '$logfile'" } &
    }
	Info "所有文件准备就绪，开始等待"
	wait
	Info "等待完毕！"
	echo "$line"
}

setting() {
    echo "$line"
    Info2 "这里有一些配置"
    Info2 "输出到单独的目录？"
    select o_out ("false" "true") {
	    break
	}
}

setting
run
