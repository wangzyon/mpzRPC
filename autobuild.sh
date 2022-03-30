#!/bin/bash

set -e

# 如果没有build目录，创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mpzrpc  .a静态库拷贝到 /usr/lib
if [ ! -d /usr/include/mpzrpc ]; then 
    mkdir /usr/include/mpzrpc
fi

for header in `ls include/*.h`
do
    cp $header /usr/include/mpzrpc
done

cp `pwd`/lib/libmpzrpc.a /usr/lib

ldconfig