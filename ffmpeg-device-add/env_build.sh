#!/bin/bash
set -e

echo ${PWD}

current_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Current dir: ${current_dir}"

build_dir="${current_dir}/_build"
release_dir="${current_dir}/_release"

# install tools
yum -y install wget git subversion net-tools bzip2 libevent libevent-devel

# install compile and build tools
yum -y install autoconf automake make libtool patch pkgconfig zip unzip gcc-c++ cmake


mkdir -p ${build_dir}
mkdir -p ${release_dir}

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${release_dir}/lib/pkgconfig
export PATH=${PATH}:${release_dir}/bin


# yasm (ffmpeg need)
pushd ${build_dir}
if ! [ -e "yasm" ]
then
    echo "########## yasm begin ##########"
    if ! [ -e "yasm" ]
    then
        # download yasm
        echo "########## to download yasm ##########"
        wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
    fi
    tar xf yasm-1.3.0.tar.gz
    pushd yasm-1.3.0
    ./configure --prefix=${release_dir}
    make
    make install
    popd
    touch yasm
    echo "########## yasm ok ##########"
else
    echo "########## yasm has been installed ##########"
fi
popd


# fdk-aac
pushd ${build_dir}
if ! [ -e "fdk-aac" ]
then
    echo "########## fdk-aac begin ##########"
    if ! [ -e "fdk-aac" ]
    then
        # download fdk-aac
        echo "########## to download fdk-aac ##########"
        wget https://codeload.github.com/mstorsjo/fdk-aac/zip/v0.1.4 -O fdk-aac-0.1.4.zip
    fi
    unzip fdk-aac-0.1.4.zip
    pushd fdk-aac-0.1.4
    ./autogen.sh 
    ./configure --prefix=${release_dir}
    make
    make install
    popd
    touch fdk-aac
    echo "########## fdk-aac ok ##########"
else
    echo "########## fdk-aac has been installed ##########"
fi
popd

# x264
pushd ${build_dir} 
if ! [ -e "x264-ok" ]
then
    echo "########## libx264 begin ##########"
    if ! [ -d "x264" ]
    then
        git clone --depth 1 git://git.videolan.org/x264
    fi
    pushd x264
    ./configure --prefix=${release_dir} --enable-static --disable-opencl
    make
    make install
    popd
    touch x264-ok
    echo "########## libx264 ok ##########"
else
    echo "########## libx264 has been installed ##########"
fi
popd

#  ffmpeg
pushd ${build_dir}
if ! [ -e "ffmpeg" ]
then
    echo "########## ffmpeg begin ##########"
    if ! [ -e "ffmpeg-3.0.2.tar.gz" ]
    then
        # download ffmpeg
        echo "########## to download ffmpeg ##########"
        wget https://ffmpeg.org/releases/ffmpeg-3.0.2.tar.gz
    fi

    rm -f ${release_dir}/lib/*.so*

    tar xf ffmpeg-3.0.2.tar.gz
    pushd ffmpeg-3.0.2
    ./configure --prefix=${release_dir}
    make
    make install
    popd
    touch ffmpeg
    echo "########## ffmpeg ok ##########"
else
    echo "########## ffmpeg has been installed ##########"
fi
popd



