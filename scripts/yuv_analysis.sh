#!/bin/bash


# functions
function exit_if_error()
{
    if [ "$?" != "0" ]; then
        echo "error!!!!!  "
        echo "++++++++++stack top++++++++++"
        declare i
        declare level=${#FUNCNAME[@]}
        for((i=1;i<level;i++))
        do
            [ x"${FUNCNAME[$i]}" != x ] && \
                echo -e "in ${FUNCNAME[$i]}()   at ${BASH_SOURCE[$i]}: ${BASH_LINENO[((i-1))]}"
        done

        echo "----------stack buttom-------"
        exit 1
    fi

}


function do_one_analysis()
{
    echo "use encode_type:"$1 "bit_rate:"$2 "gop_size:"$3 "preset:"$4 "profile:"$5
    encode_type=$1
    bit_rate=$2
    gop_size=$3
    preset=$4
    profile=$5


    # to generate ffmpeg command, 
    #e.g: ./ffmpeg -s 1920x1080 -i haoshengyin_420.yuv  -vcodec h264_qsv -b 1000k -s 1280x720 -f mp4 -y qsv.mp4
    dst_path="$dst_dir/${encode_type}_${bit_rate}_gop${gop_size}_${preset}_${profile}.mp4"
    encod_cmd="${ffmpeg_path} -s ${src_width}x${src_height} -i $src_yuv -vcodec $encode_type -b $bit_rate -s ${src_width}x${src_height} -g ${gop_size} -preset $preset -profile $profile -f mp4 -y $dst_path" 
    echo "encod_cmd: $encod_cmd"

    # to encode
    ${ffmpeg_path} -s ${src_width}x${src_height} -i $src_yuv -vcodec $encode_type -b $bit_rate -s ${src_width}x${src_height} -g ${gop_size} -preset $preset -profile $profile -f mp4 -y $dst_path
    exit_if_error

    dst_yuv_path="${dst_path}.yuv"
    decode_com="${ffmpeg_path} -i ${dst_path} -pix_fmt yuv420p -y ${dst_yuv_path}"
    echo "decode_com: $decode_com"

    # to decode
    ${ffmpeg_path} -i ${dst_path} -pix_fmt yuv420p -y ${dst_yuv_path}
    exit_if_error

    # to generate analysis cmd
    # e.g: ./psnr 1920 1080 420 haoshengyin_420.yuv qsv-1080p.yuv ssim > qsv_ssim.csv

    analysis_cmd_psnr="${analysis_bin} ${src_width} ${src_height} 420 ${src_yuv} ${dst_yuv_path} > ${dst_path}_psnr.csv"
    echo "analysis_cmd_psnr:${analysis_cmd_psnr}"
    ${analysis_bin} ${src_width} ${src_height} 420 ${src_yuv} ${dst_yuv_path} > ${dst_path}_psnr.csv
    exit_if_error

    analysis_cmd_ssim="${analysis_bin} ${src_width} ${src_height} 420 ${src_yuv} ${dst_yuv_path} ssim > ${dst_path}_ssim.csv"
    echo "analysis_cmd_ssim:${analysis_cmd_ssim}"
    ${analysis_bin} ${src_width} ${src_height} 420 ${src_yuv} ${dst_yuv_path} ssim > ${dst_path}_ssim.csv
    exit_if_error

    # clean the no need yuv file
    rm -rf ${dst_yuv_path}
}

function Analysis()
{
    echo "Analysis begin"

    for rate in $g_bit_rate
    do
        #echo "use bit_rate:"$rate
        for gop in $g_gop_size
        do
            #echo "use gop_size:"$gop
            for preset in $g_presets
            do
                #echo "use preset:"$preset
                for profile in $g_profiles
                do
                    #echo "use profile:"$profile
                    for codec in $g_encode_type
                    do
                        #echo "use codec:"$codec
                        do_one_analysis $codec $rate $gop $preset $profile

                    done

                done

            done

        done

    done

}



# globle params
g_encode_type="libx264 h264_qsv"
g_bit_rate="1000k 1500k 2000k 2500k"
g_gop_size="25 50 100"
g_presets="veryfast medium veryslow"
g_profiles="baseline main high"

# the source yuv path
src_yuv="/home/haoshengyin_420_deinterlace.yuv"
src_width=1920
src_height=1080
dst_dir="/home/test"

ffmpeg_path="/home/ffmpeg"
analysis_bin="/home/psnr"

# to analysis
Analysis 