#!zsh

if [[ $1 == "" ]] || (( $1 <= 0 )) {
	echo "\
Usage: bilibili_web_trans.sh <count>"
	return -1
}

mkdir backup

for i ({1..$1})
{
	printf "printf: %02d\n" $i
	audio=$(ls $(printf "%02d" $i)*.m4a)
	video=$(ls $(printf "%02d" $i)*.mp4)
	out=$(printf "%s_out.mp4" $video)
	echo "cmd: ffmpeg -i \"$audio\" -i \"$video\" -c copy \"$out\""
	ffmpeg -i "$audio" -i "$video" -c copy "$out"
	mv "$audio" "backup/$audio"
	mv "$video" "backup/$video"
	mv "$out" "$video"
}
