#!zsh


help() {
	echo "\
usage: $1 [options]
  options:
     -i    show more info in output file
     -h    show this message
     -o    output file
     -v    video output (3gp file)"
	exit $2
}

running() {
	list=$(find ./ -name "entry.json")

	rm -f $f_o
	echo $list|while read line
	do
		ind=$(echo $line |sed "s/\.\/\([^/]*\/[^/]*\).*/\1/")
		# echo "dir:$ind"
		local title=""
		title=$(cat "$line"|grep "download_subtitle" >/dev/null && cat "$line"|sed -n "s/.*\"download_subtitle\":\"\\([^\"]*\\)\".*/\\1/p" || cat "$line"|sed -n "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/p")
		[[ $title == "" ]] && title=$(cat "$line"|sed -n "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/p")
		if [[ $flag_info == "true" ]] {
			echo "# $title <Dir: $ind>" >> $f_o
		} else {
			echo "# $title" >> $f_o
		}
		if [[ $flag_3gp == "true" ]] {
			echo "ffmpeg -i \"`find $ind -name "video.m4s"`\" -i \"`find $ind -name "audio.m4s"`\" -r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000 \"$title.3gp\"" >> $f_o
		} else {
			echo "ffmpeg -i \"`find $ind -name "audio.m4s"`\" \"$title.mp3\"" >> $f_o
		}
	done
}

flag_3gp="false"
flag_info="false"
f_o="run.sh"
while {getopts "io:vh" OPTION} {
	case $OPTION {
		i) flag_info="true" ;;
		o) f_o=$OPTARG ;;
		v) flag_3gp="true" ;;
		h|?) help $0 ;;
		*) help $0 -1 ;;
	}
}

running

