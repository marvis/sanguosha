while [ 1 ];
do
	file=screen$[$(ls screen*.png | wc -l)+1].png
	screencapture -o -l33253 $file
	./tools/image_recog $file
	sleep 2
done
