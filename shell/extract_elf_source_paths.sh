#!/bin/bash
# 程序由ai生成
# 功能：读取指定目录下所有elf文件，输出疑似目录的路径

# 目标目录：存放提取出的二进制文件（根据实际情况修改）
TARGET_DIR="$1"

if [ -z "$TARGET_DIR" ]; then
	echo "错误：目标目录为空！"
	echo "Usage: $0 xxx"
	exit 1
fi

# 检查目标目录是否存在
if [ ! -d "$TARGET_DIR" ]; then
	echo "错误：目标目录 $TARGET_DIR 不存在！"
	exit 1
fi

echo "开始遍历 $TARGET_DIR 中的ELF文件并提取源文件路径..."
echo "============================================"

# 递归查找所有文件，筛选ELF格式
find "$TARGET_DIR" -type f | while read -r file; do
	# 检查文件是否为ELF格式（32位或64位）
	if file -b "$file" | grep -q '^ELF'; then
		echo "发现ELF文件：$file"
		echo "  文件架构：$(readelf -h "$file"|grep Machine|sed 's/  Machine: *//')"
		if ! file -b "$file" | grep -q 'with debug_info'; then
			echo "  无debug信息"
			echo "--------------------------------------------"
			continue
		fi
		echo "  提取的源文件路径/目录结构："

		# 方法1：从字符串表中提取可能的源文件路径（包含.c/.h的路径）
		# 过滤掉过长或无意义的字符串，保留含目录分隔符和源文件后缀的路径
		# strings "$file" | grep -E '(/|\\)[a-zA-Z0-9_.-]+\.(c|h|cpp|cc)$' | sort -u | while read -r path; do
		#     echo "    - $path"
		# done
		# 这里可选旁边src_c编译的get_elf_str程序，支持联动中文
		(get_elf_str "$file" || strings "$file") | grep '^/' | grep -v '^/$' | grep -v '/usr/' | grep -v '^/lib' | sort -u | while read -r path; do
			echo "    - $path"
		done

		# 方法2：从调试信息中提取（需objdump支持，更精准但可能较慢）
		# 若存在调试信息，会显示编译时的源文件路径
		# if objdump -W "$file" 2>/dev/null | grep -q 'DW_AT_name.*\.c'; then
		#     echo "  调试信息中的源文件路径："
		#     objdump -W "$file" 2>/dev/null | grep -E 'DW_AT_name.*\.(c|h|cpp|cc)' | awk '{print $NF}' | sort -u | while read -r name; do
		#         echo "    - $name"
		#     done
		# fi
		
		echo "--------------------------------------------"
	fi
done

echo "处理完成！"
