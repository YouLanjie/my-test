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
	file_list=$(ls -F "$input_d"|sed -n 's|/$||p'|sort -n)
	max_line=$(echo $file_list|wc -l)
	final_output="#+title: INDEX:$title\n#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='$css_file'/>\n* $title\n"
	final_output="$final_output$(echo $file_list|sed "s|\(.*\)|- [[$input_d/\1.html]]|"|sed "s|//|/|g")"
	#_msg_info "INDEX file:"
	[[ ! -d $output_d ]] && mkdir $output_d
	echo $final_output >"$output_d/index.org"
	tmpfile="$(date +"%Y.%m.%d %H:%M:%S"|md5sum|awk '{print $1}').el"
	echo $elisp_cmd >/tmp/$tmpfile
	emacs -Q -nw /tmp/$tmpfile "$output_d/index.org" --eval "(eval-buffer \"$tmpfile\")"
	for (( i=1; i <= $max_line; i++ )) {
		final_output=""
		file="$(echo $file_list|sed -n "${i}p")"
		outp="$output_d/$file.org"
		[[ -f $file ]] && continue
		[[ -f $outp ]] && _msg_warning "Overwrite output file '$outp'"
		final_output="#+title: $title / $i\n#+HTML_HEAD: <link rel='stylesheet' type='text/css' href='$css_file'/>\n* $file / $i\n"
		final_output="$final_output$(ls "$input_d/$file"|sort -n|sed "s|\(.*\)|[[$input_d/$file/\1]]|"|sed "s|//|/|g")"
		_msg_info "OUTP: $outp"
		echo $final_output >"$outp"
		emacs -Q -nw /tmp/$tmpfile "$outp" --eval "(eval-buffer \"$tmpfile\")"
	}
	rm /tmp/$tmpfile
}

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
     -i [dirname] 设置输入目录
     -o [dirname] 设置输出目录
     -t [title]   设置标题
     -c [cssfile] 设置css文件链接
     -h           帮助信息"
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

run

