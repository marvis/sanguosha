win_id=43049
pre_text=""
while [ 1 ];
do
	file=screen$[$(ls screen*.png | wc -l)+1].png
	screencapture -o -l$win_id $file
	text=`./tools/image_recog $file`
	if [ "$text" != "$pre_text" ]; then
		pre_text=$text
		echo $text
	else
		rm $file
	fi
	sleep 2
done
