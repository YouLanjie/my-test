#!/usr/bin/bash

#==================================
# 文件名称：termux_check_mem.sh
# 创 建 者：u0_a221
# 创建日期：2026年06月20日
# 描    述：内存监控(ai生成)
#==================================

# 配置参数
THRESHOLD=80            # 报警阈值（百分比）
INTERVAL=1              # 检查间隔（秒）
NOTIFIED=false          # 当前是否已报警（防止重复）

while true; do
	# 读取内存信息（Mem: 行）
	mem_info=$(free | awk '/^Mem:/ {print $2, $7}')  # 总内存 可用内存
	total=$(echo "$mem_info" | cut -d' ' -f1)
	available=$(echo "$mem_info" | cut -d' ' -f2)

	# 计算使用百分比
	used=$((total - available))
	usage=$((used * 100 / total))

	if [ "$usage" -gt "$THRESHOLD" ]; then
		if [ "$NOTIFIED" = false ]; then
			top3=$(echo "";ps axww -o %mem,rss,comm --sort=-%mem --no-headers|head -n 3|\
				awk '{print $2/1024 "MB [" $1 "%] " $3}')
			nowtime=$(date +"%Y.%m.%d %H:%M:%S")
			# 发送高优先级通知
			termux-notification \
				--title "[WARN] 内存占用警告 (${usage}%)" \
				--content "[$nowtime]当前内存使用率 ${usage}% (阈值 ${THRESHOLD}%)$top3" \
				--priority max \
				--vibrate 500 \
				--sound  # 可选，带提示音
			NOTIFIED=true
		fi
	else
		# 内存正常，重置报警状态
		NOTIFIED=false
	fi

	sleep "$INTERVAL"
done
