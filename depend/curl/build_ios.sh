#!/bin/sh

rm ../../lib/ios/libcurl.a
cd tmp

XCODE_PATH="/Applications/Xcode.app/Contents/Developer"

export IPHONEOS_DEPLOYMENT_TARGET="8.0"
export CC="${XCODE_PATH}/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang"

ARCH=(armv7 armv7s arm64 i386 x86_64)
HOST=(armv7 armv7s arm i386 x86_64)
TARGET=(iPhoneOS iPhoneOS iPhoneOS iPhoneSimulator iPhoneSimulator)

for ((i=0; i<${#ARCH[@]}; i++)); do
	export CFLAGS="-arch ${ARCH[$i]} -pipe -Os -gdwarf-2 -isysroot ${XCODE_PATH}/Platforms/${TARGET[$i]}.platform/Developer/SDKs/${TARGET[$i]}.sdk -miphoneos-version-min=${IPHONEOS_DEPLOYMENT_TARGET} -fembed-bitcode"
	export LDFLAGS="-arch ${ARCH[$i]} -isysroot ${XCODE_PATH}/Platforms/${TARGET[$i]}.platform/Developer/SDKs/${TARGET[$i]}.sdk"
	if [ "${TARGET[$i]}" = "iPhoneSimulator" ]; then
		export CPPFLAGS="-D__IPHONE_OS_VERSION_MIN_REQUIRED=${IPHONEOS_DEPLOYMENT_TARGET%%.*}0000"
	fi
	make distclean
	./configure  -host="${HOST[$i]}-apple-darwin" --disable-shared --enable-static --with-darwinssl --enable-ipv6 --enable-threaded-resolver --disable-verbose
	make -j $(sysctl -n hw.logicalcpu_max)
	cp lib/.libs/libcurl.a "../../../lib/ios/libcurl-${ARCH[$i]}.a"
	cp include/curl/curlbuild.h "../include/curl/ios/curlbuild-${ARCH[$i]}.h"
done

cd ..

rm include/curl/*
cp tmp/include/curl/*.h include/curl/
cp curlbuild_shared.h.in include/curl/curlbuild.h

cd ../../lib/ios
lipo -create -output "libcurl.a" "libcurl-armv7.a" "libcurl-armv7s.a" "libcurl-arm64.a" "libcurl-i386.a" "libcurl-x86_64.a"
rm libcurl-armv7.a libcurl-armv7s.a libcurl-arm64.a libcurl-i386.a libcurl-x86_64.a
