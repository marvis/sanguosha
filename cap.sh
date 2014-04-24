win_id=43049
i=1
while [ 1 ];
do
	screencapture -o -l$win_id screen.png
	./tools/image_recog screen.png 1> /dev/null 2>out; 
	err=`cat out`; 
	if [ "$err" != "" ]; then
		cp screen.png screen$i.png
		i=$[i+1];
	fi
	sleep 1
done
