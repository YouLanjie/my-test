#!/usr/bin/zsh
# 用于生成一堆org文件然后导出成html（看漫画用的）

# {{{
app_name="${0##*/}"
F_B='\033[1m' ; F_I='\033[3m' ; F_U='\033[4m' ; F_S='\033[5m' ; F_R='\033[7m'
F_black='\033[30m' ; F_red='\033[31m' ; F_green='\033[32m' ; F_yellow='\033[33m'
F_blue='\033[34m' ; F_purple='\033[35m' ; F_cyan='\033[36m' ; F_white='\033[37m'
F_C='\033[0m'
F_line="$F_B$F_red-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*$F_C"
_msg_info() {
    local _msg="${1}"
    local _color="${2}"
    [[ "${quiet}" == "y" ]] || printf "$F_B${F_green}[%s] INFO:$F_cyan$F_I$_color %s$F_C\n" "${app_name}" "${_msg}"
}
_msg_warning() {
    local _msg="${1}"
    printf "$F_B${F_yellow}[%s] WARNING:$F_I %s$F_C\n" "${app_name}" "${_msg}" >&2
}
_msg_error() {
    local _msg="${1}"
    local _error=${2}
    printf "$F_B${F_red}[%s] ERROR:$F_I %s$F_C\n"  "${app_name}" "${_msg}" >&2
    if (( _error > 0 )); then
        exit "${_error}"
    fi
}
# }}}

# $1: content; $2: repleace format; $3: MainTitle; $4: SubTitle
get_output() {
	header=""
	tail=""
	if [[ $5 != "| | |" ]] {
		navigation="|上一页|本页文件总数|下一页|\n|-|-|\n${5}"
		header="* Header\n$navigation"
		tail="* Tail\n$navigation"
	}
	final_output="$(echo $1|sed "s|\(.*\)|$2|"|sed "s|//|/|g")"
	final_output="$(echo $final_output|sed "s/^- \[\[\(.*\.\($format2\)\)\]\]/#+begin_export html\n<video controls><source src=\"\1\"><\/video>\n#+end_export/")"
	final_output="\
#+title: $3
#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='$css_file'/>
#+HTML_LINK_UP: ../
#+HTML_LINK_HOME: ./
#+HTML_HEAD: <style>li{margin: 0px}</style>
$header
* $4
$final_output
$tail"
	echo $final_output
}

export_file() {
	emacsclient -e "(progn (find-file \"$1\")(org-mode)(org-html-export-as-html)(write-file \"$2\")
	(delete-window)(kill-buffer \"$3\")(kill-buffer))" 2>&1 >>/dev/null
}

gettitle() {
	[[ $title != "<NULL>" ]] && return 0
	pwd=$(pwd)
	cd $input_d
	title=$(basename "$(pwd)")
	cd "$pwd"
}

run() {
	[[ $(echo $input_d|sed -n "s|^\([.]\{0,2\}/\)|\1|p") == "" ]] && input_d="./$input_d"
	[[ ! -d $input_d ]] && _msg_error "不正确的输入目录:$input_d" 1
	gettitle
	# $input目录下文件夹列表
	dir_list=$(ls -F "$input_d"|sed -n 's|/$||p'|sort -n)
	max_line=$(echo $dir_list|wc -l)
	max_line_fail=0
	for (( i=1; i <= $max_line; i++ )) {
		dir="$(echo $dir_list|sed -n "$(($i-$max_line_fail))p")"
		if [[ $(find "$input_d/$dir" -maxdepth 1 -type f,l -iregex ".*\.\($format\|$format2$format3\)") == "" ]] {
			dir_list=$(echo $dir_list|sed "$((i-max_line_fail))d")
			((max_line_fail++))
		}
	}
	max_line=$((max_line-max_line_fail))
	[[ ! -d $output_d ]] && mkdir -p $output_d
	if (( $max_line >= 1 )) {
		_msg_info "创建Emacs服务"
		emacs --bg-daemon "ORG_TO_HTML_SERVER" -Q
		_msg_info "创建分页"
		final_output=$(get_output "$dir_list" "- [[$input_d/\1.html]]\n" "INDEX:$title" "$title" "| | |")
		echo $final_output >"$output_d/index.org"
		emacsclient -e "(progn (require 'package) (package-initialize)
(setq-default make-backup-files nil auto-save-default nil)
(setq-default org-src-fontify-natively t org-export-with-sub-superscripts '{} org-use-sub-superscripts '{})
(require 'monokai-theme) (load-theme 'monokai t) (require 'htmlize))" 2>&1 >>/dev/null
		(( $max_line > 1 )) && export_file "$output_d/index.org" "$output_d/index.html" "index.html"
	} else {
		_msg_warning "好像没有找到识别范围内文件呢喵"
	}
	for (( i=1; i <= $max_line; i++ )) {
		final_output=""

		dir="$(echo $dir_list|sed -n "${i}p")"
		[[ -f $dir ]] && continue
		file_list=$(cd "$input_d/$dir" && find ./ -maxdepth 1 -type f,l -iregex ".*\.\($format\|$format2$format3\)"|sed "s|^\./||"|sort -n|sed "s|^|$input_d/$dir/|")
		[[ $file_list == "" ]] && continue

		navigation="| |"
		(( i > 1 )) && navigation="|[[./$(echo $dir_list|sed -n "$(($i-1))p").html][$(echo $dir_list|sed -n "$(($i-1))p")]] |"
		(( $max_line > 1 )) && navigation="${navigation} $(echo $file_list|wc -l) |"
		(( i < $max_line )) && navigation="${navigation} [[./$(echo $dir_list|sed -n "$(($i+1))p").html][$(echo $dir_list|sed -n "$(($i+1))p")]]"
		navigation="${navigation} |"

		outp="$output_d/$dir.org"
		if [[ $max_line == 1 ]] {
			outp="$output_d/index.org"
			dir="$title"
			final_output=$(get_output "$file_list" "- [[\1]]\n" "$title" "$dir" "$navigation")
		} else {
			final_output=$(get_output "$file_list" "- [[\1]]\n" "$title" "$i / $dir" "$navigation")
		}

		[[ -f $outp ]] && _msg_warning "Overwrite output dir '$outp'"
		_msg_info "OUTP: $outp"
		echo $final_output >"$outp"
		export_file "$outp" "$(echo "$outp"|sed "s|org$|html|")" "$(echo $outp|sed "s|org$|html|"|sed "s|^$output_d/||")"
	}
	(( $max_line >= 1 )) && emacsclient -e "(kill-emacs)"
}

format='png\|PNG\|JPG\|jpg\|jpeg\|gif\|webp'
format2='mp4\|mkv\|3gp\|webm\|mov\|mpeg\|m4a\|mp3\|wav'
format3=''
input_d="./"
output_d="./"
title="<NULL>"
css_file="../main.css"

usage() {
	usagetext="\
usage: $app_name [options]
  options:
     -i <dirname> 设置输入目录(默认./)
     -o <dirname> 设置输出目录(不推荐)(默认./)
     -c <cssfile> 设置css文件链接(默认../main.css)
     -f <extern>  设置额外查找文件后缀
     -t <title>   设置标题
     -h           帮助信息

标准文件层级格式:
input_dir                       output_dir
├── expect_dir_1                ├── index.org
│   ├── expect_file_1.image     ├── expect_dir_1.org
│   ├── expect_file_2.image     ├── expect_dir_2.org
│   ├── expect_file_3.image     ├── index.html
│   └── expect_file_4.image     ├── expect_dir_1.html
└── expect_dir_2                └── expect_dir_2.html
    └── expect_file_1.image"
	echo $usagetext
	exit $1
}

while {getopts 'i:o:t:c:f:xh?' arg} {
	case $arg {
		i) input_d=$OPTARG ;;
		o) output_d=$OPTARG ;;
		t) title=$OPTARG ;;
		c) css_file=$OPTARG ;;
		f) format3="\\|$OPTARG" ;;
		x) set -x ;;
		h|?) usage 0;;
		*) usage 1;;
	}
}

run

