#!zsh


help() {
	echo "\
usage: $1 [options]
  options:
     -i    show more info in output file
     -h    show this message
     -o    output file
     -v    video output (3gp file)
     -V    video output (mp4 file)"
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

		local part=""
		part=$(cat "$line"|grep "part" >/dev/null && cat "$line"|sed -n "s/.*\"part\":\"\\([^\"]*\\)\".*/\\1/p")

		local subtitle=""
		subtitle=$(cat "$line"|grep "download_subtitle" >/dev/null && cat "$line"|sed -n "s/.*\"download_subtitle\":\"\\([^\"]*\\)\".*/\\1/p")

		local main_title=""
		main_title=$(cat "$line"|sed -n "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/p")

		title=$main_title
		[[ $subtitle != "" && $subtitle != $main_title ]] && title="$title - $subtitle"
		[[ $part != "" && $part != $main_title ]] && title="$title - $part"

		local ind_title=""
		local index=""
		ind_title=$(cat "$line"|grep "index_title" >/dev/null && cat "$line"|sed -n "s/.*\"index_title\":\"\\([^\"]*\\)\".*/\\1/p" || cat "$line"|sed -n "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/p")
		index=$(cat "$line"|grep "index" >/dev/null && cat "$line"|sed -n "s/.*\"index\":\"\\([^\"]*\\)\".*/\\1/p" || cat "$line"|sed -n "s/.*\"index\":\"\\([^\"]*\\)\".*/\\1/p")
		[[ $ind_title != "" && $index != "" ]] && title="${title}_No${index}_${ind_title}"

		if [[ $flag_info == "true" ]] {
			echo "# $title <Dir: $ind>" >> $f_o
		} else {
			echo "# $title" >> $f_o
		}
		if [[ $flag_type == "3gp" ]] {
			echo "ffmpeg -i \"`find $ind -name "video.m4s"`\" -i \"`find $ind -name "audio.m4s"`\" -r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000 \"$title.3gp\"" >> $f_o
		} elif [[ $flag_type == "mp4" ]] {
			echo "ffmpeg -i \"`find $ind -name "video.m4s"`\" -i \"`find $ind -name "audio.m4s"`\" -c copy \"$title.mp4\"" >> $f_o
		} else {
			echo "ffmpeg -i \"`find $ind -name "audio.m4s"`\" \"$title.mp3\"" >> $f_o
		}
	done
}

flag_type="mp3"
flag_info="false"
f_o="run.sh"
while {getopts "io:vVh" OPTION} {
	case $OPTION {
		i) flag_info="true" ;;
		o) f_o=$OPTARG ;;
		v) flag_type="3gp" ;;
		V) flag_type="mp4" ;;
		h|?) help $0 ;;
		*) help $0 -1 ;;
	}
}

running

