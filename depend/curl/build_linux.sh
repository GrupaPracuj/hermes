#!/bin/sh

cd tmp

ARCH="x86"
rm ../../../lib/linux/${ARCH}/libcurl.a
export CFLAGS="-m32 -pipe -O2"
make distclean
./configure --disable-shared --enable-static --enable-threaded-resolver --with-ssl --enable-ipv6
make -j16
cp lib/.libs/libcurl.a "../../../lib/linux/${ARCH}/libcurl.a"
cp include/curl/curlbuild.h "../include/curl/linux/curlbuild-${ARCH}.h"

ARCH="x86_64"
rm ../../../lib/linux/${ARCH}/libcurl.a
export CFLAGS="-m64 -pipe -O2"
make distclean
./configure --disable-shared --enable-static --enable-threaded-resolver --with-ssl --enable-ipv6
make -j16
cp lib/.libs/libcurl.a "../../../lib/linux/${ARCH}/libcurl.a"
cp include/curl/curlbuild.h "../include/curl/linux/curlbuild-${ARCH}.h"

cd ..

rm include/curl/*
cp tmp/include/curl/*.h include/curl/
cp curlbuild_shared.h.in include/curl/curlbuild.h
