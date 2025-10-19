#!/bin/bash
# 程序由ai生成
# 功能：将git仓库中历次提交中的所有二进制文件提取出来按提交分别安置

# 目标文件夹：存储所有提交的二进制文件
OUTPUT_DIR="./bcwt_$(date +"%Y%m%d_%H%M%S")"
mkdir -p "$OUTPUT_DIR"

# 获取所有提交的哈希值和提交时间（按时间正序，格式：哈希值 时间戳）
# 时间戳格式：%Y%m%d_%H%M%S（年-月-日_时-分-秒）
COMMITS=$(git log --reverse --pretty=format:"%H %ct" | while read -r commit epoch; do
	# 将 epoch 时间（秒级）转换为指定格式的时间戳
	timestamp=$(date -d "@$epoch" +"%Y%m%d_%H%M%S")
	echo "$commit $timestamp"
done)

# 上一次提交的哈希值（初始为空）
PREV_COMMIT=""

# 遍历每个提交（格式：哈希值 时间戳）
while read -r COMMIT TIMESTAMP; do
	echo "处理提交: $COMMIT（时间：$TIMESTAMP）"
	
	# 为当前提交创建文件夹，名称格式：时间戳_哈希值
	COMMIT_DIR="$OUTPUT_DIR/${TIMESTAMP}_${COMMIT}"
	mkdir -p "$COMMIT_DIR"
	COUNT="false"
	
	# 对比当前提交与上一次提交，找出新增/修改的文件
	if [ -z "$PREV_COMMIT" ]; then
		# 第一个提交：对比空树
		DIFF_FILES=$(git diff-tree --no-commit-id --name-only -r "$COMMIT")
	else
		# 非第一个提交：对比上一次提交
		DIFF_FILES=$(git diff --name-only "$PREV_COMMIT" "$COMMIT")
	fi
	
	# 筛选并复制二进制文件
	for FILE in $DIFF_FILES; do
		# 检查是否为二进制文件（Git 标记为 "binary file"）
		if git diff --numstat "$PREV_COMMIT" "$COMMIT" -- "$FILE" | grep -q '^-'; then
			if ! git show "$COMMIT:$FILE" > /dev/null; then
				continue
			fi
			echo "提取二进制文件: $FILE"
			# 保留原文件的目录结构
			mkdir -p "$COMMIT_DIR/$(dirname "$FILE")"
			# 从当前提交导出文件
			git show "$COMMIT:$FILE" > "$COMMIT_DIR/$FILE"
			COUNT="true"
		fi
	done
	if [[ $COUNT != "true" ]]; then
		rmdir "$COMMIT_DIR"
	fi
	
	# 更新上一次提交的哈希值
	PREV_COMMIT="$COMMIT"
done <<< "$COMMITS"

echo "所有二进制文件已提取至: $OUTPUT_DIR"
