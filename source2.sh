#!/usr/bin/zsh

#================================================================
#   Copyright (C) 2023 YouLanjie
#   
#   文件名称：source2.sh
#   创 建 者：youlanjie
#   创建日期：2023年05月21日
#   描    述：
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

# Show an INFO message
# $1: message string
_msg_info() {
    local _msg="${1}"
    [[ "${quiet}" == "y" ]] || printf "$F_B${F_green}[%s] INFO:$F_cyan$F_I %s$F_C\n" "${app_name}" "${_msg}"
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

