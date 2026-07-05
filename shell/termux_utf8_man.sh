#!/usr/bin/bash
# 借用groff实现在termux下显示中文man手册（虽然没有中文手册）

ORIGION="$PREFIX/bin/mandoc"

# 如果是搜索模式，交给真正的 mandoc
if [[ "$*" =~ -[kfQ] ]] || [[ "$1" == "-w" || "$@" == "" ]]; then
	exec -a "$0" "$ORIGION" "$@"
fi

# 获取文件列表
if [[ "$1" == "-l" && "$2" != "" ]];then
	FILELIST="$2"
else
	FILELIST=$(exec -a "$0" "$ORIGION" -w "$@")
fi
RET=$?
if [[ $RET != 0 ]];then
	exit $RET
fi
if [[ $FILELIST == "" ]];then
	exit -1
fi
FILE="$(echo "$FILELIST"|sort|head -n 1)"
if [[ ! -f "$FILE" ]];then
	exit -2
fi

# 解压/渲染
TEMPFILE=$(mktemp -t "man.XXXXXXXXXX")
trap 'rm -f "$TEMPFILE"' EXIT
if [[ "${FILE##*.}" == "gz" ]];then
	# 经测试，man只按照后缀名判断解压
	gzip -d -c "$FILE" >"$TEMPFILE"
	CODING=$(file -bi "$TEMPFILE")
	if [[ "$CODING" =~ "ascii" ]];then
		exec -a "$0" "$ORIGION" "$@"
		exit $?
	fi
	gzip -d -c "$FILE"|groff -mandoc -Kutf8 -Tutf8 2>/dev/null >"$TEMPFILE"
else
	groff -mandoc -Kutf8 -Tutf8 "$FILE" 2>/dev/null >"$TEMPFILE"
fi

# 输出
if [ -t 1 ];then
	"${MANPAGER:-less}" -R "$TEMPFILE"
else
	cat "$TEMPFILE"
fi
