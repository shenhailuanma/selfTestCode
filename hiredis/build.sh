#!/bin/bash
set -e

echo ${PWD}

current_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Current dir: ${current_dir}"

build_dir="${current_dir}/_build"
release_dir="${current_dir}/_release"


gcc -o test example.c -I${release_dir}/include/hiredis  ${release_dir}/lib/libhiredis.a

gcc -o producer producer.c -I${release_dir}/include/hiredis  ${release_dir}/lib/libhiredis.a

gcc -o consumer consumer.c -I${release_dir}/include/hiredis  ${release_dir}/lib/libhiredis.a -levent