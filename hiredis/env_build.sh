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

# download redis
pushd ${build_dir}
wget http://125.39.35.134/files/20670000082F10B6/download.redis.io/releases/redis-3.2.0.tar.gz

tar xf redis-3.2.0.tar.gz
pushd redis-3.2.0
make PREFIX=${release_dir}
make PREFIX=${release_dir} install
popd
popd

# download hiredis
pushd ${build_dir}
wget https://codeload.github.com/redis/hiredis/zip/v0.13.3 -O hiredis-0.13.3.zip

unzip hiredis-0.13.3.zip
pushd hiredis-0.13.3
make PREFIX=${release_dir}
make PREFIX=${release_dir} install
popd

popd