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


gcc -o filter filtering_video.c -I${release_dir}/include -L${release_dir}/lib \
 -lavfilter  -lavformat  -lavdevice -lavcodec  -lswscale  -lswresample -lavutil   \
-lva -lva-x11 -lva -lxcb -lxcb-shm -lxcb -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lxcb-shape -lxcb -lX11 -lasound -lm -llzma -lbz2 -lz -pthread