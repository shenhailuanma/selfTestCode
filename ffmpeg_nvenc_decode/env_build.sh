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