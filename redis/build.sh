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


pushd ${build_dir}

    if ! [ -e "redis-3.2.0.tar.gz" ]
    then
        # download redis
        echo "########## to download ffmpeg ##########"
        wget wget http://125.39.35.134/files/20670000082F10B6/download.redis.io/releases/redis-3.2.0.tar.gz
        echo "########## redis has download over ##########"
    else
        echo "########## redis has download ##########"
    fi

    tar xf redis-3.2.0.tar.gz
    pushd redis-3.2.0

    cp -rf ${current_dir}/src/server.c      ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/server.h      ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/smem.c        ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/smem.h        ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/smem4redis.c  ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/Makefile      ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/Makefile.dep  ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/networking.c  ${build_dir}/redis-3.2.0/src/
    cp -rf ${current_dir}/src/redis-cli.c   ${build_dir}/redis-3.2.0/src/

    cp -rf ${current_dir}/deps/hiredis/async.c   ${build_dir}/redis-3.2.0/deps/hiredis


    make PREFIX=${release_dir}
    make PREFIX=${release_dir} install
    popd
popd


gcc -o ${release_dir}/bin/producer ${current_dir}/test/producer.c -I${build_dir}/redis-3.2.0/deps/hiredis  ${build_dir}/redis-3.2.0/deps/hiredis/libhiredis.a
gcc -o ${release_dir}/bin/consumer ${current_dir}/test/consumer.c -I${build_dir}/redis-3.2.0/deps/hiredis  ${build_dir}/redis-3.2.0/deps/hiredis/libhiredis.a -levent -lpthread


gcc -o ${release_dir}/bin/producer_smem ${current_dir}/test/producer_smem.c  ${current_dir}/test/smem_client.c -I${build_dir}/redis-3.2.0/deps/hiredis  ${build_dir}/redis-3.2.0/deps/hiredis/libhiredis.a -levent -lpthread
gcc -o ${release_dir}/bin/consumer_smem ${current_dir}/test/consumer_smem.c  ${current_dir}/test/smem_client.c -I${build_dir}/redis-3.2.0/deps/hiredis  ${build_dir}/redis-3.2.0/deps/hiredis/libhiredis.a -levent -lpthread


