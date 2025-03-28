#!/usr/bin/zsh

if [[ $1 == "-h" ]] {
	echo "扫描指定文件夹下的mp3文件并输出用于"
	echo "使用其自带的歌手歌名等信息重命名它"
	echo "的脚本于run.sh中"
	exit
}

if [[ -f run.sh ]] {
	echo "Error: 输出文件'run.sh'已存在"
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

