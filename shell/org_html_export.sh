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

run() {
	[[ $(echo $input_d|sed -n "s|^\([.]\{0,2\}/\)|\1|p") == "" ]] && input_d="./$input_d"
	# $input目录下文件夹列表
	dir_list=$(ls -F "$input_d"|sed -n 's|/$||p'|sort -n)
	max_line=$(echo $dir_list|wc -l)
	max_line_fail=0
	for (( i=1; i <= $max_line; i++ )) {
		dir="$(echo $dir_list|sed -n "$(($i-$max_line_fail))p")"
		if [[ $(find "$input_d/$dir" -maxdepth 1 -type f,l -iregex ".*\.\($format\)") == "" ]] {
			dir_list=$(echo $dir_list|sed "$((i-max_line_fail))d")
			((max_line_fail++))
		}
	}
	max_line=$((max_line-max_line_fail))
	final_output="$(echo $dir_list|sed "s|\(.*\)|- [[$input_d/\1.html]]|"|sed "s|//|/|g")"
	final_output="#+title: INDEX:$title\n#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='$css_file'/>\n* $title\n$final_output"
	#_msg_info "INDEX file:"
	[[ ! -d $output_d ]] && mkdir $output_d
	echo $final_output >"$output_d/index.org"
	tmpfile="$(date +"%Y.%m.%d %H:%M:%S"|md5sum|awk '{print $1}').el"
	echo $elisp_cmd >/tmp/$tmpfile
	[[ $max_line > 1 ]] && emacs -Q -nw /tmp/$tmpfile "$output_d/index.org" --eval "(eval-buffer \"$tmpfile\")"
	for (( i=1; i <= $max_line; i++ )) {
		final_output=""

		dir="$(echo $dir_list|sed -n "${i}p")"
		[[ -f $dir ]] && continue
		file_list=$(cd "$input_d/$dir" && find ./ -maxdepth 1 -type f,l -iregex ".*\.\($format\)"|sed "s|^\./||"|sort -n|sed "s|^|$input_d/$dir/|")
		[[ $file_list == "" ]] && continue

		outp="$output_d/$dir.org"
		if [[ $max_line == 1 ]] {
			outp="$output_d/index.org"
			dir="$title"
		}
		final_output="$(echo "$file_list"|sed "s|\(.*\)|[[\1]]|"|sed "s|//|/|g")"
		final_output="#+title: $title / $i\n#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='$css_file'/>\n* $dir / $i\n$final_output"

		[[ -f $outp ]] && _msg_warning "Overwrite output dir '$outp'"
		_msg_info "OUTP: $outp"
		echo $final_output >"$outp"
		emacs -Q -nw /tmp/$tmpfile "$outp" --eval "(eval-buffer \"$tmpfile\")"
	}
	rm /tmp/$tmpfile
}

format='png\|jpg\|jpeg\|gif\|webp'
input_d="./"
output_d="./"
title="<NULL>"
css_file="../main.css"
elisp_cmd="\
(require 'package)
(package-initialize)
(setq-default make-backup-files nil auto-save-default nil)
(require 'monokai-theme)
(load-theme 'monokai t)
(require 'htmlize)
(setq-default org-src-fontify-natively t org-export-with-sub-superscripts '{} org-use-sub-superscripts '{})

(org-html-export-to-html)
(kill-emacs)
"

usage() {
	usagetext="\
usage: $app_name [options]
  options:
     -i <dirname> 设置输入目录(默认./)
     -o <dirname> 设置输出目录(不推荐)(默认./)
     -c <cssfile> 设置css文件链接(默认../main.css)
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

while {getopts 'i:o:t:c:h?' arg} {
	case $arg {
		i) input_d=$OPTARG ;;
		o) output_d=$OPTARG ;;
		t) title=$OPTARG ;;
		c) css_file=$OPTARG ;;
		h|?) usage 0;;
		*) usage 1;;
	}
}

#set -x
run

