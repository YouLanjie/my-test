#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#   
#   文件名称：run.sh
#   创 建 者：youlanjie
#   创建日期：2023年06月09日
#   描    述：下载网易云音乐（一站式解决）
#
#================================================================

app_name="${0##*/}"

F_B='\033[1m'
F_I='\033[3m'
F_U='\033[4m'
F_S='\033[5m'
F_R='\033[7m'

F_black='\033[30m'
F_red='\033[31m'
F_green='\033[32m'
F_yellow='\033[33m'
F_blue='\033[34m'
F_purple='\033[35m'
F_cyan='\033[36m'
F_white='\033[37m'

F_C='\033[0m'

F_line="$F_B$F_red-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*$F_C"

# Show an INFO message
# $1: message string
# $2: style string
_msg_info() {
    local _msg="${1}"
    local _color="${2}"
    [[ "${quiet}" == "y" ]] || printf "$F_B${F_green}[%s] INFO:$F_cyan$F_I$_color %s$F_C\n" "${app_name}" "${_msg}"
}

# Show a WARNING message
# $1: message string
_msg_warning() {
    local _msg="${1}"
    printf "$F_B${F_yellow}[%s] WARNING:$F_I %s$F_C\n" "${app_name}" "${_msg}" >&2
}

# Show an ERROR message then exit with status
# $1: message string
# $2: exit code number (with 0 does not exit)
_msg_error() {
    local _msg="${1}"
    local _error=${2}
    printf "$F_B${F_red}[%s] ERROR:$F_I %s$F_C\n"  "${app_name}" "${_msg}" >&2
    if (( _error > 0 )); then
        exit "${_error}"
    fi
}

file_type="m4a"
album="NULL"
artist="NULL"
file_name_format="NULL"
vip="false"

usage() {
    usagetext="\
usage: $app_name [options] <input dir>
  options:
     -v           下载VIP歌曲
     -h           帮助信息"
    echo $usagetext
    exit $1
}

download() {
    # 获取信息
    _msg_info "音频链接"
    read link

    _msg_info "封面链接"
    read img_link

    _msg_info "名称"
    read name

    sec="yes"
    _msg_info "别称?" "$F_yellow"
    select sec ("yes" "no") {
        if [[ $sec == "yes" ]] {
            _msg_info "请输入:"
            read alias_name
        }
        break
        }

    _msg_info "艺术家:"
    read artist

    _msg_info "专辑:"
    read album

    clear

    # 获取没有后缀的下载链接
    _msg_info "链接1: $link"
    link=$(echo $link|sed "s/?authSecret=.*//")
    # _msg_info "链接2: $link"

    # 获取文件后缀名
    extension=$(echo $link|sed "s/.*\.//")
    file_type=$extension
    # _msg_info "扩展名: $extension"

    # 设置文件名（没有扩展名）
    file_name_format=$(printf "${artist} - ${name}")
    if [[ $sec == "yes" ]] {
        file_name_format=$(printf "${artist} - ${name}(${alias_name})")
        # _msg_info "有别名"
    }

    if [[ $vip != "true" && $link != "NULL" ]] {
        # 下载音频
        echo $F_line
        file_name=$(echo $file_name_format|sed "s/$/.${extension}/")
        _msg_info "wget \"$link\" -O \"$file_name\""
        wget "$link" -O "$file_name"||(_msg_warning "错误！下载出错" && exit -1)
    }

    # 下载封面
    # 获取高清的封面链接
    img_link=$(echo "$img_link" |sed 's/\(^.*\.jpg\).*/\1/')
    file_name=$(echo $file_name_format|sed "s/$/.jpg/")
    _msg_info "wget \"$img_link\" -O \"$file_name\""
    wget "$img_link" -O "$file_name"

    # 设置专辑
    # file_name=$(echo $file_name_format|sed "s/$/.txt/")
    # _msg_info "echo \"$album\" > \"$file_name\""
    # echo "$album" > "$file_name"
}

m4a_to_mp3() {
    # 输入文件
    input_f=$(printf "./%s.m4a" $file_name_format)
    # 输出文件
    out_f=$(printf "./%s.mp3" $file_name_format)

    # 检查
    if [[ -e $out_f ]] {
        _msg_info "$input_f"
        _msg_info "输出文件已存在，跳过" "$F_yellow"
        echo "$F_line"
        return 0
    }

    _msg_info "输入文件：'$input_f'"
    _msg_info "ffmpeg -i \"$input_f\" \"$out_f\""
    echo "$F_line"
    _msg_info "以下为程序输出："
    (ffmpeg -i "$input_f" "$out_f" && rm "$input_f" && _msg_info "'$input_f' 完成转换！" "$F_yellow") || (_msg_warning "'$input_f' 转换出现问题！" && exit -1)
}

add_info() {
    # 输入文件
    input_f=$(printf "./%s.mp3" $file_name_format)
    # 输出文件
    out_f=$(printf "./out/%s.mp3" $file_name_format)

    icon=$(printf "./%s.jpg" $file_name_format)
    title=$(echo $file_name_format |sed 's/^.* - \(.*\)/\1/')
    # album

    if [[ $album == "NULL" ]] {
        album=$title
    }

    # 设置输出文件
    if [[ -e "./out" ]] {
        if [[ ! -d "./out" ]] {
            _msg_warning "存在输出文件，为防止出错，退出"
            exit -1
        }
    }
    if [[ ! -d "./out" ]] {
        _msg_info "不存在输出目录，将创建" "$F_yellow"
        mkdir "./out" || (_msg_warning "创建文件夹失败，退出" && exit -1)
    }

    cmd="ffmpeg -i \"$i\" $arg2 $arg \"$out\" >$logfile 2>&1"

    # 检查
    if [[ -e $out ]] {
        _msg_info $i
        _msg_info "输出文件已存在，跳过" "$F_yellow"
        echo "$F_line"
        return 0
    }

    echo "$F_line"
    _msg_info "'$input_f'"
    _msg_info "$title"
    _msg_info "$artist"
    _msg_info "$icon"
    _msg_info "$cmd"
    echo "$F_line"
    # 设置图标
    _msg_info "以下为程序输出："
    if [[ -f "$icon" ]] {
        (ffmpeg\
            -i "$input_f"                                 \
            -i "$icon" -map 0:0 -map 1:0 -id3v2_version 3 \
            -c copy                                       \
            -metadata album="$album"                      \
            -metadata artist="$artist"                    \
            -metadata title="$title"                      \
            -metadata:s:v title='Album cover'             \
            -metadata:s:v comment='Cover (Front)'         \
            "$out_f" &&
            rm "$input_f" "$icon" &&
            _msg_info "'$input_f' 完成转换！" "$F_yellow"
        ) || _msg_warning "'$input_f' 转换出现问题！"
    } else {
        (ffmpeg\
            -i "$input_f"                                 \
            -c copy                                       \
            -metadata album="$album"                      \
            -metadata artist="$artist"                    \
            -metadata title="$title"                      \
            -metadata:s:v title='Album cover'             \
            -metadata:s:v comment='Cover (Front)'         \
            "$out_f" &&
            rm "$input_f" &&
            _msg_info "'$input_f' 完成转换！" "$F_yellow"
        ) || _msg_warning "'$input_f' 转换出现问题！"
    }
}

if [[ $1 == "-h" ]] {
    usage
} elif [[ $1 == "-v" ]] {
    vip="true"
}

for i ({1..100}) {
    # clear
    echo $F_line
    download
    # clear
    if [[ $file_type != "mp3" ]] {
        echo "$F_line"
        m4a_to_mp3
        # clear
    }
    echo "$F_line"
    add_info
}

