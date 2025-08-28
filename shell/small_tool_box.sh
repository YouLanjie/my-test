#!/usr/bin/bash
# 存放一些又臭又长的命令

choices=$(grep -G '[a-Z0-9 ]\+() {' $0|sed 's/() {//')

quit() {
	exit
}

help() {
	echo "func list:"
	echo "$choices"|awk '{printf "%s) %s\n",NR,$1}'
}

get_mem_by_grep() {
	tag=$1
	if [[ $1 == "" ]];then
		echo "请输入匹配的tag（默认为msedge）"
		read tag
	fi
	if [[ $tag == "" ]];then
		tag="msedge"
	fi
	echo "TAG:     '$tag'"
	for i in $(ps aux|grep "$tag"|awk '{print $2}')
	do
		if [[ ! -f "/proc/$i/status" ]];then continue;fi
		cat /proc/$i/status|grep 'Swap\|RSS'
	done|awk '{type[$1]+=$2;type["Total:"]+=$2}END{for (t in type) printf "%-8s %s MB\n",t,type[t]/1024}'
}

get_mem_by_grep_and_save() {
	tag=$1
	if [[ $tag == "" ]];then
		echo "请输入匹配的tag（默认为msedge）"
		read tag
	fi
	if [[ $tag == "" ]];then
		tag="msedge"
	fi
	outputf=$2
	if [[ $outputf == "" ]];then
		echo "请输入保存的文件名(csv)"
		read outputf
	fi
	if [[ $outputf == "" ]];then
		exit 1
	fi
	echo "使用Ctrl-c退出"
	echo "TAG: '$tag'"
	echo "Time,VmRSS,VmSwap,Total" >>"$outputf"
	while echo "-----";do
		result=$(get_mem_by_grep "$tag"|sed '1d')
		echo "$result"
		if [[ $result == "" ]];then
			echo "$(date -Iseconds),,," >>"$outputf"
		else
			echo "$(date -Iseconds),$(echo "$result"|awk '{print $2}'|xargs|sed 's/ /,/g')" >>"$outputf"
		fi
		sleep 1
	done
}

get_mem_split_by_user() {
	ps aux | awk '{users[$1] += $6} END {for (u in users) printf "%-10s %s MB\n",u,users[u]/1024}'
}

get_pacman_pkg_size_list() {
	#pacman -Qi|grep '名字\|安装后大小'|sed 's/.* : //'|sed 'N;s/\n/ /'|\
		#awk '{if ($3 == "MiB"){printf "%15.3f %s\n",$2*1024,$1}else{printf "%15.3f %s\n",$2,$1}}'|sort -n
	pacman -Qi|grep '名字\|安装后大小'|sed 's/.* : //'|sed 'N;s/\n/ /'|\
		awk '{if ($3 == "MiB"){printf "%15.3f %s\n",$2,$1}else{printf "%15.3f %s\n",$2/1024,$1}}'|sort -n
}

pacman_auto_remove() {
	pacman -Qdtq |sudo pacman -Rsun -
}

debug_coredump() {
	tmpfile="/tmp/corefile.uuid.`uuidgen`"
	echo "请输入pid并回车:"
	zstdcat `find /var/lib/systemd/coredump -name "*$(head -n 1)*"` >$tmpfile
	echo "请输入程序名并回车:"
	gdb "$(head -n 1)" $tmpfile
	rm $tmpfile
}

if [[ $1 != "" ]];then
	if echo "$choices"|grep -q "^$1\$";then
		args=("$@")
		$1 ${args[@]:1}
	fi
	exit
fi

select choice in $choices; do
	if [[ $choice == "" ]];then exit;fi
	#echo "$choice"
	echo "-----------"
	$choice
	echo "-----------"
done
