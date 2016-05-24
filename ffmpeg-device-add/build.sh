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
    if ! [ -e "ffmpeg-3.0.2.tar.gz" ]
    then
        # download ffmpeg
        echo "########## to download ffmpeg ##########"
        wget https://ffmpeg.org/releases/ffmpeg-3.0.2.tar.gz
    fi

    tar xf ffmpeg-3.0.2.tar.gz

    # copy the files to ffmpeg 
    cp -rf ${current_dir}/src/ffmpeg-3.0.2/libavdevice/Makefile         ${build_dir}/ffmpeg-3.0.2/libavdevice
    cp -rf ${current_dir}/src/ffmpeg-3.0.2/libavdevice/alldevices.c     ${build_dir}/ffmpeg-3.0.2/libavdevice
    cp -rf ${current_dir}/src/ffmpeg-3.0.2/libavdevice/testdev.c        ${build_dir}/ffmpeg-3.0.2/libavdevice   

    pushd ffmpeg-3.0.2
    ./configure --prefix=${release_dir}
    make
    make install
    popd
    touch ffmpeg
    echo "########## ffmpeg ok ##########"

popd

