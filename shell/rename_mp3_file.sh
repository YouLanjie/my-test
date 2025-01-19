#!/usr/bin/zsh

# 扫描指定文件夹下的mp3文件并输出用于
# 使用其自带的歌手歌名等信息重命名它
# 的脚本于run.sh中

if [[ -f run.sh ]] {
	echo "Please make sure the file 'run.sh' is not nessary and remove it by yourself."
	exit 1
}

dir=$1
if [[ $1 == "" ]] {
	dir="~/Musics/"
}
list=$(ls $dir/*.mp3)

echo $list|while read line
do
	out=$(ffprobe "$line" 2>&1)
	title=$(echo "$out"|grep title|sed -n "1p"|sed "s/.*: //")
	artist=$(echo "$out"|grep artist|sed "s/.*: //")
	if [[ $artist == "" || $title == "" ]];then
		echo "WARN: file '$line' lost info:'$artist - $title'" >&2
		continue
	fi
        echo "# $artist - $title" >> run.sh
        echo "mv \"$line\" \"$artist - $title.mp3\"" >> run.sh
done

