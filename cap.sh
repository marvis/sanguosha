win_id=39483
while [ 1 ];
do
	file=screen$[$(ls screen*.png | wc -l)+1].png
	screencapture -o -l$win_id $file
	./tools/image_recog $file
	sleep 2
done
