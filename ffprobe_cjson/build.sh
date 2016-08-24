#!/bin/bash

set -e

echo ${PWD}

current_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Current dir: ${current_dir}"

build_dir="${current_dir}/_build"
release_dir="${current_dir}/_release"
source_dir="${current_dir}/src"


mkdir -p ${build_dir}
mkdir -p ${release_dir}
mkdir -p ${release_dir}/bin
mkdir -p ${release_dir}/include
mkdir -p ${release_dir}/lib

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${release_dir}/lib/pkgconfig
export PATH=${PATH}:${release_dir}/bin

# install base tools
pushd ${build_dir}
if ! [ -e "base" ]
then
    yum -y install wget autoconf automake make libtool patch pkgconfig
    touch base
fi
popd

# get and install json-c
pushd ${build_dir}
if ! [ -e "json-c" ]
then
    echo "########## json-c begin ##########"
    if ! [ -d "json-c-0.12" ]
    then
        # download yasm
        echo "########## download ##########"
        wget https://s3.amazonaws.com/json-c_releases/releases/json-c-0.12.tar.gz
    fi
    tar xf json-c-0.12.tar.gz
    pushd json-c-0.12



    export CFLAGS+="-Werror=unused-but-set-variable -Wno-error=unused-but-set-variable" 
    echo ${CFLAGS}
    ./autogen.sh
    ./configure --prefix=${release_dir} 
    make
    make install
    popd
    touch json-c
    echo "########## json-c ok ##########"
else
    echo "########## json-c has been installed ##########"
fi
popd


# build 
rm -rf libjson-c.so*
gcc -o ${release_dir}/bin/load_json ${source_dir}/load_json.c -I${release_dir}/include -L${release_dir}/lib -ljson-c