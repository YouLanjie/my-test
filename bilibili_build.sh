#!zsh

list=$(find ./ -name "entry.json")
# cmd=$(echo $list| sed "s/^/cat /"| sed 's/$/| sed "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/"/')
#sed "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/"

# echo $list

rm -f run.sh
echo $list|while read line
do
	ind=$(echo $line |sed "s/\.\/\([^/]*\/[^/]*\).*/\1/")
	# echo "dir:$ind"
	local title=""
	title=$(cat "$line"|grep "download_subtitle" >/dev/null && cat "$line"|sed -n "s/.*\"download_subtitle\":\"\\([^\"]*\\)\".*/\\1/p" || cat "$line"|sed -n "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/p")
	echo "# $title" >> run.sh
	echo "ffmpeg -i \"`find $ind -name "audio.m4s"`\" \"$title.mp3\"" >> run.sh
done

# zsh -c "$cmd"
