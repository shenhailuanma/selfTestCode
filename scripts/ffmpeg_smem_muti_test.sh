#!/bin/bash

set -e

#  create_channel()
#  usage:  create_channel ffmpeg input number gpu_number command
function create_channel()
{
    echo "create_channel"
    echo $0 $1 $2 $3 $4 

    ffmpeg=$1
    src=$2
    number=$3
    gpu=$4


    yuv_channel="yuv"${number}"gpu"${gpu}
    es_channel="es"${number}"gpu"${gpu}
    rtmp_out="rtmp://10.33.1.48/"${number}"gpu"${gpu}"/"${number}"gpu"${gpu}
    echo $yuv_channel
    echo $es_channel
    echo $rtmp_out

    # decode and output to smem
    $ffmpeg -re -vcodec nvenc_mpeg2 -gpu $gpu -i $src -f smem $yuv_channel &

    # get data form smem and encode, the encoded data send to smem
    $ffmpeg -f smem -timeout 60 -i $yuv_channel -vcodec nvenc_h264 -gpu $gpu -acodec aac -r 25 -b 2000k -ar 44100 -ab 64k -ac 2 -f smem $es_channel &

    # get encoded data from smem and packetd output
    $ffmpeg -f smem -timeout 60 -i $es_channel -vcodec copy -acodec copy -f flv $rtmp_out &

    
}

#  create_channel()
#  usage:  create_channel ffmpeg input number gpu_number command
function create_channel2()
{
    echo "create_channel"
    echo $0 $1 $2 $3 $4 

    ffmpeg=$1
    src=$2
    number=$3
    gpu=$4


    yuv_channel="yuv"${number}"gpu"${gpu}
    es_channel="es"${number}"gpu"${gpu}
    rtmp_out="rtmp://10.33.1.48/"${number}"gpu"${gpu}"/"${number}"gpu"${gpu}
    echo $yuv_channel
    echo $es_channel
    echo $rtmp_out

    # decode and output to smem
    $ffmpeg -re -vcodec nvenc_mpeg2 -gpu $gpu -i $src -f smem $yuv_channel &

    # get data form smem and encode, and packetd output
    $ffmpeg -f smem -timeout 60 -i $yuv_channel -vcodec nvenc_h264 -gpu $gpu -acodec aac -r 25 -b 2000k -ar 44100 -ab 64k -ac 2 -f flv $rtmp_out &
    
}

#  create_channel()
#  usage:  create_channel ffmpeg input number gpu_number command
function create_channel3()
{
    echo "create_channel"
    echo $0 $1 $2 $3 $4 

    ffmpeg=$1
    src=$2
    number=$3
    gpu=$4


    yuv_channel="yuv"${number}"gpu"${gpu}
    es_channel="es"${number}"gpu"${gpu}

    echo $yuv_channel
    echo $es_channel

    # decode and output to smem
    $ffmpeg -re -vcodec nvenc_mpeg2 -gpu $gpu -i $src -f smem $yuv_channel &

    # get data form smem and encode, and packetd output
    $ffmpeg -f smem -timeout 60 -i $yuv_channel -vcodec nvenc_h264 -gpu $gpu -acodec aac -r 25 -b 2000k -ar 44100 -ab 64k -ac 2 -f flv /dev/null &
    
}

function create_channel4()
{
    echo "create_channel"
    echo $0 $1 $2 $3 $4 

    ffmpeg=$1
    src=$2
    number=$3
    gpu=$4


    yuv_channel="yuv"${number}"gpu"${gpu}
    es_channel="es"${number}"gpu"${gpu}

    echo $yuv_channel
    echo $es_channel

    
    $ffmpeg -re -vcodec nvenc_mpeg2 -gpu $gpu -i $src -vcodec nvenc_h264 -gpu $gpu -acodec aac -r 25 -b 2000k -ar 44100 -ab 64k -ac 2 -f smem $es_channel &
    
}

function create_channel5()
{
    echo "create_channel"
    echo $0 $1 $2 $3 $4 

    ffmpeg=$1
    src=$2
    number=$3
    gpu=$4


    yuv_channel="yuv"${number}"gpu"${gpu}
    es_channel="es"${number}"gpu"${gpu}
    rtmp_out="rtmp://10.33.1.48/"${number}"gpu"${gpu}"/"${number}"gpu"${gpu}

    echo $yuv_channel
    echo $es_channel

    
    $ffmpeg -re -vcodec nvenc_mpeg2 -gpu $gpu -i $src -vcodec nvenc_h264 -gpu $gpu -acodec aac -r 25 -b 2000k -ar 44100 -ab 64k -ac 2 -f flv $rtmp_out &
    
}


FFMPEG="/tmp/ffmpeg3"
#SRC="/home/HongGaoLiang01.mp4"
SRC="/tmp/save_cctv15.ts"
#COUNT="0 1 2 3 4 5 6 7 8 9 10 11 12 13 14"
COUNT="0 1 2 3 4 5 6 7 8 9 10 11"
GPUCOUNT="0 1 2 3"


for number in $COUNT;do
    #echo $number
    for gpu in $GPUCOUNT;do
        create_channel5 $FFMPEG $SRC $number $gpu 
    done 
done
