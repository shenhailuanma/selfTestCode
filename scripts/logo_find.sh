#!/bin/bash

set -e








###### Functions ######

# ffmpeg从视频中截取一些小截图
# usage: crop_pics $video_type $video_path $pics_number $pics_dest
# @param: video_type 视频类型, 取值范围: 'youku', 'xigua', 'bilibili'
# @param: video_path 视频路径
# @param: pics_number 截图数量
# @param: pics_dest 输出存储路径
function crop_pics() {

	if [ $# -lt 4 ]; then 
		echo "Error: input params number is $#"
		echo "usage: crop_pics video_type video_path pics_number pics_dest"
		return 1
	fi

	video_type=$1
	video_path=$2
	pics_number=$3
	pics_dest=$4


	case $video_type in
	'youku')
		echo "video_type: youku, now to do it."
		crop_pics_youku $video_path $pics_number $pics_dest
		return $?
		;;
	'xigua')
		echo "video_type: xigua, now to do it."
		crop_pics_xigua $video_path $pics_number $pics_dest
		return $?
		;;
	'bilibili')
		echo "video_type: bilibili, now to do it."
		crop_pics_bilibili $video_path $pics_number $pics_dest
		return $?
		;;
	*)
		echo "video_type: $video_type not support, return"
		return 1
		;;
	esac


	return 0
}

function crop_pics_youku() {
	echo "crop_pics_youku begin."
	video_path=$1
	pics_number=$2
	pics_dest=$3

	for((i=0;i<$pics_number;i++))
    do
        echo $i
        ffmpeg -i $video_path -ss 5 -vframes 1 -vf crop=iw*0.115:ih*0.08:iw*0.865:ih*0.03 -f image2 -y $pics_dest/$(basename $video_path)_$i.jpg

    done


	echo "crop_pics_youku over."
	return 0
}

function crop_pics_xigua() {
	echo "crop_pics_xigua begin."
	video_path=$1
	pics_number=$2
	pics_dest=$3

	for((i=0;i<$pics_number;i++))
    do
        echo $i
        #ffmpeg -i $video_path -ss 5 -vframes 1 -f image2 -y $pics_dest/$(basename $video_path)_src_$i.jpg
        ffmpeg -i $video_path -ss 5 -vframes 1 -vf crop=iw*0.079:ih*0.15:iw*0.902:ih*0.04 -f image2 -y $pics_dest/$(basename $video_path)_$i.jpg
    done

	echo "crop_pics_xigua over."
	return 0
}

function crop_pics_bilibili() {
	echo "crop_pics_bilibili begin."
	video_path=$1
	pics_number=$2
	pics_dest=$3

	for((i=0;i<$pics_number;i++))
    do
        echo $i
        # 左上
        ffmpeg -i $video_path -ss 5 -vframes 1 -vf crop=iw*0.12:ih*0.045:iw*0.03:ih*0.04 -f image2 -y $pics_dest/$(basename $video_path)_${i}_LU.jpg

        # 左下
        ffmpeg -i $video_path -ss 5 -vframes 1 -vf crop=iw*0.12:ih*0.045:iw*0.03:ih*0.92 -f image2 -y $pics_dest/$(basename $video_path)_${i}_LB.jpg

        # 右上
        ffmpeg -i $video_path -ss 5 -vframes 1 -vf crop=iw*0.12:ih*0.045:iw*0.85:ih*0.04 -f image2 -y $pics_dest/$(basename $video_path)_${i}_RU.jpg

        # 右下
        ffmpeg -i $video_path -ss 5 -vframes 1 -vf crop=iw*0.12:ih*0.045:iw*0.85:ih*0.92 -f image2 -y $pics_dest/$(basename $video_path)_${i}_RB.jpg

    done

	echo "crop_pics_bilibili over."
	return 0
}


###### Functions over ######

video_source_path="/Users/zhangxu/video/video/"
out_path="/Users/zhangxu/video/pics"
xigua_files="xigua01.mp4 xigua02.mp4 xigua03.mp4 xigua04.mp4"
youku_files="youku01.mp4 youku02.mp4 youku03.mp4 youku04.mp4 youku05.mp4 youku06.mp4"
bilibili_files="bilibili01.mp4 bilibili02.mp4 bilibili03.flv bilibili04.flv bilibili05.flv bilibili06.mp4 bilibili07.mp4"

for one in $xigua_files;do
    echo $video_source_path$one
    #crop_pics 'xigua' $video_source_path$one 1 $out_path
done

for one in $youku_files;do
    echo $video_source_path$one
    #crop_pics 'youku' $video_source_path$one 1 $out_path
done

for one in $bilibili_files;do
    echo $video_source_path$one
    crop_pics 'bilibili' $video_source_path$one 1 $out_path
done










