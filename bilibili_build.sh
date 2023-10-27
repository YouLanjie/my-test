#!zsh

list=$(find ./ -name "entry.json")
# cmd=$(echo $list| sed "s/^/cat /"| sed 's/$/| sed "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/"/')
#sed "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/"

# echo $list

rm -f run.sh
echo $list|while read line
do
	ind=$(echo $line |sed "s/\.\/\([^/]*\).*/\1/")
	# echo "dir:$ind"
	title=$(cat "$line"|sed "s/.*\"title\":\"\\([^\"]*\\)\".*/\\1/")
	echo "# $title" >> run.sh
	echo "ffmpeg -i \"`find $ind -name "audio.m4s"`\" \"$title.mp3\"" >> run.sh
done

# zsh -c "$cmd"
