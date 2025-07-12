#!/bin/zsh

help() {
	echo "\
usage: $1 [options]
  options:
     -r         恢复窗口(用-N防误杀)
     -n         无输入
     -N         不创建xwinwrap窗口
     -h         显示帮助"
	exit $2
}

run_main() {
	#TARGET_WID=$1  # 传入窗口ID
	echo "请点击需要缩小化的窗口"
	TARGET_WID=$(xwininfo |grep "Window id" | awk '{print $4}')
	pid=$(xdotool getwindowpid $TARGET_WID)
	subpid=""

	if [[ $no_xwinwrap == "false" ]] {
		if [[ $(pgrep xwinwrap) == "" ]] {
			# 创建透明覆盖层
			if [[ $no_input == "true" ]] {
				xwinwrap -ni -ov -fs -o 0.9999999 -- zsh -c "while [[ \$(ps "$pid" >/dev/null && echo \$?) == 0 ]] {sleep 0.4}" &
			} else {
				xwinwrap -ov -fs -o 0.9999999 -- zsh -c "while [[ \$(ps "$pid" >/dev/null && echo \$?) == 0 ]] {sleep 0.4}" &
			}
			subpid=$(pgrep xwinwrap)
		} else {
			echo "WARN 疑似有正在运行的xwinwrap进程"
		}
	}
	echo "请点击需要作为父窗口的窗口"
	WRAPPER_WID=$(xwininfo |grep "Window id" | awk '{print $4}')

	# 将目标窗口挂载到覆盖层
	xdotool windowreparent $TARGET_WID $WRAPPER_WID
	xdotool windowsize $TARGET_WID 100% 100%

	# 调整窗口属性
	#xprop -id $TARGET_WID -format _NET_WM_WINDOW_TYPE 32a -set _NET_WM_WINDOW_TYPE _NET_WM_WINDOW_TYPE_DESKTOP
	#xprop -id $TARGET_WID -remove _NET_WM_STATE
}

run_main_recover() {
	echo "恢复模式，点击需要恢复的窗口（或父窗口）"
	ret=$(xwininfo -children)
	TARGET_WID=$(echo $ret |grep "Window id" | awk '{print $4}')
	ret2=$(echo $ret|grep -G " \+[0-9]\+ child:" -A 1)
	if [[ $ret2 != "" ]] {
		TARGET_WID=$(echo $ret2|sed -n "2p" | awk '{print $1}')
	}
	WRAPPER_WID=$(xwininfo -root |grep "Window id" | awk '{print $4}')
	xdotool windowreparent $TARGET_WID $WRAPPER_WID
	if [[ $no_xwinwrap == "false" ]] {
		killall xwinwrap
	}
}

recover="false"
no_xwinwrap="false"
no_input="false"

echo "本脚本具有一定的危险性，请谨慎使用"
while {getopts "rnNh" OPTION} {
	case $OPTION {
		r) recover="true" ;;
		n) no_input="true" ;;
		N) no_xwinwrap="true" ;;
		h|?) help $0 ;;
		*) help $0 -1 ;;
	}
}

if [[ $recover == "true" ]] {
	run_main_recover
	exit
}

run_main
