#!/bin/bash
set -e

echo ${PWD}

current_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Current dir: ${current_dir}"

build_dir="${current_dir}/_build"
release_dir="${current_dir}/_release"


mkdir -p ${build_dir}
mkdir -p ${release_dir}

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${release_dir}/lib/pkgconfig
export PATH=${PATH}:${release_dir}/bin


#  ffmpeg
pushd ${build_dir}

    echo "########## ffmpeg begin ##########"
    if ! [ -e "ffmpeg-3.0.1.tar.gz" ]
    then
        # download ffmpeg
        echo "########## to download ffmpeg ##########"
        wget https://ffmpeg.org/releases/ffmpeg-3.0.1.tar.gz
    fi

    tar xf ffmpeg-3.0.1.tar.gz

    # copy the files to ffmpeg 
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/configure                    ${build_dir}/ffmpeg-3.0.1
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/libavdevice/Makefile         ${build_dir}/ffmpeg-3.0.1/libavdevice
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/libavdevice/alldevices.c     ${build_dir}/ffmpeg-3.0.1/libavdevice
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/libavdevice/smem_dec.c       ${build_dir}/ffmpeg-3.0.1/libavdevice   
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/libavdevice/smem_enc.c       ${build_dir}/ffmpeg-3.0.1/libavdevice   
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/libavdevice/smem.h           ${build_dir}/ffmpeg-3.0.1/libavdevice
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/libavdevice/smem_com.c       ${build_dir}/ffmpeg-3.0.1/libavdevice
    cp -rf ${current_dir}/src/ffmpeg-3.0.1/libavdevice/smem_com.h       ${build_dir}/ffmpeg-3.0.1/libavdevice
    cp -rf ${current_dir}/../redis/test/smem_client.c                   ${build_dir}/ffmpeg-3.0.1/libavdevice
    cp -rf ${current_dir}/../redis/test/smem_client.h                   ${build_dir}/ffmpeg-3.0.1/libavdevice

    rm -rf ${current_dir}/../redis/_release/lib/*.so.*

    pushd ffmpeg-3.0.1
    ./configure --prefix=${release_dir} --enable-gpl --enable-static --enable-nonfree --enable-version3 \
--extra-cflags="-I${current_dir}/../redis/_release/include/hiredis -I${current_dir}/../redis/_release/include -I${release_dir}/include" \
--extra-ldflags="-L${current_dir}/../redis/_release/lib -L${release_dir}/lib  -levent -lm -lpthread -lrt -ldl " --extra-libs=-lhiredis --enable-libx264
    make
    make install
    popd
    touch ffmpeg
    echo "########## ffmpeg ok ##########"

popd

