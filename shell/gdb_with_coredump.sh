#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2025 Chglish
#
#   文件名称：gdb_with_coredump.sh
#   创 建 者：Chglish
#   创建日期：2025年11月23日
#   描    述：为方便地使用gdb调试coredump使用
#
#================================================================

tmpfile="/tmp/corefile.uuid.`uuidgen`"
echo "文件列表："
ls --sort=time -r /var/lib/systemd/coredump|tail -n 5
echo "请输入pid(用于匹配程序)"
pid=$(head -n 1)
flist=$(find /var/lib/systemd/coredump -name "*$pid*"|head -n 1)
if [[ $flist == "" ]];then
	echo "未匹配到对应文件"
	exit
fi
zstdcat "$flist" >$tmpfile
echo "输入程序名"
gdb "$(head -n 1)" $tmpfile
rm $tmpfile
