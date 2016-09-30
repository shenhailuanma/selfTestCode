#!/bin/bash

set -e




function rand(){
    min=$1
    max=$(($2-$min+1))
    num=$(cat /dev/urandom | head -n 10 | cksum | awk -F ' ' '{print $1}')
    echo $(($num%$max+$min))
}






FFMPEG="/tmp/ffmpeg_qsv_9_29"
SWITCHS=""
DST="zxswitch"
VODS="/home/zgl_wl.flv /home/paiqiu.mp4 /home/lian.mp4"


while ((1));do
    echo "hello"

    for switch in $SWITCHS;do
        rnd=$(rand 1 30)
        echo $rnd        
        $FFMPEG -f smem -i $switch -t $rnd -f smem $DST
    done

    for vod in $VODS;do
        $FFMPEG -re -i $vod -ar 44100 -ac 2 -s 640x360 -r 25 -f smem $DST
    done

done