#!/bin/bash


# 将制定目录下的图片文件转为tiff格式
# usage:
# transcode_to_tiff  $src_dir  $dst_dir
function transcode_to_tiff()
{
    if [ $# -lt 2 ]; then
		echo "Error: input params number is $#"
		echo "usage: transcode_to_tiff  $src_dir  $dst_dir"
		return 1
	fi

    src_dir=$1
    dst_dir=$2

    echo $src_dir
    echo $dst_dir

    i=0
    for file in $src_dir/*
    do
        echo $file

        ffmpeg -i $file -y $dst_dir/${i}.tif
        i=$((i+1))


    done


}

# 准备N个样本图片，并全部转换成tif文件
transcode_to_tiff /Users/zhangxu/video/train1 /Users/zhangxu/video/train1tiff
