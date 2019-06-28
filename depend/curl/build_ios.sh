#!/bin/bash

# User configuration
LIBRARY_NAME="curl-7.65.1"

# Check settings
if [ -z "${HERMES_IOS_DEPLOYMENT_TARGET}" ]; then
    IOS_DEPLOYMENT_TARGET="8.0"
else
    IOS_DEPLOYMENT_TARGET=${HERMES_IOS_DEPLOYMENT_TARGET}
fi

if [ -z "${XCODE_ROOT}" ] || [ ! -d "${XCODE_ROOT}" ]; then
    XCODE_ROOT=`xcode-select --print-path`

    if [ ! -d "${XCODE_ROOT}" ]; then
        echo "Error: ${XCODE_ROOT} is not a valid path."
        exit 1
    fi
fi

printf "iOS Deployment Target: \"${IOS_DEPLOYMENT_TARGET}\"\n"
printf "Xcode path: \"${XCODE_ROOT}\"\n\n"

# Check software and detect CPU cores
CPU_CORE=1
PLATFORM_NAME=`uname`
if [[ "${PLATFORM_NAME}" == "Darwin" ]]; then
   CPU_CORE=`sysctl -n hw.logicalcpu_max`
else
  echo "Error: ${PLATFORM_NAME} is not supported." >&2
  exit 1
fi

if [ -z `which wget` ]; then
  echo "Error: wget is not installed." >&2
  exit 1
fi

# Internal variables
WORKING_DIR=`pwd`
TMP_DIR=${WORKING_DIR}/tmp
TMP_LIB_DIR=${WORKING_DIR}/tmp_lib
TOOLCHAIN_DIR=${XCODE_ROOT}/Toolchains/XcodeDefault.xctoolchain
TOOLCHAIN_NAME=("armv7-apple-darwin" "armv7s-apple-darwin" "arm-apple-darwin" "i386-apple-darwin" "x86_64-apple-darwin")
ARCH=("armv7" "armv7s" "arm64" "i386" "x86_64")
ARCH_NAME=("armv7" "armv7s" "arm64" "x86" "x86_64")
TARGET=("iPhoneOS" "iPhoneOS" "iPhoneOS" "iPhoneSimulator" "iPhoneSimulator")

# Download and extract library
rm -rf ${LIBRARY_NAME}
[ -f ${LIBRARY_NAME}.tar.gz ] || wget --no-check-certificate https://curl.haxx.se/download/${LIBRARY_NAME}.tar.gz;
tar xfz ${LIBRARY_NAME}.tar.gz

# Remove include and clean lib directory
rm -rf ${WORKING_DIR}/include
rm -f ${WORKING_DIR}/../../lib/ios/libcurl.a

# Build for all architectures and copy data
mkdir ${TMP_LIB_DIR}
cd ${LIBRARY_NAME}
export CC=${TOOLCHAIN_DIR}/usr/bin/clang
export LD=${TOOLCHAIN_DIR}/usr/bin/ld
export AR=${TOOLCHAIN_DIR}/usr/bin/ar
export RANLIB=${TOOLCHAIN_DIR}/usr/bin/ranlib
export STRIP=${TOOLCHAIN_DIR}/usr/bin/strip

for ((i=0; i<${#ARCH[@]}; i++)); do
	export SYSROOT=${XCODE_ROOT}/Platforms/${TARGET[$i]}.platform/Developer/SDKs/${TARGET[$i]}.sdk
	export CROSS_SYSROOT=${SYSROOT}
	export CFLAGS="-arch ${ARCH[$i]} -isysroot ${SYSROOT} -O2 -fPIC -fno-strict-aliasing -fstack-protector -pipe -gdwarf-2 -miphoneos-version-min=${IOS_DEPLOYMENT_TARGET}"
	export LDFLAGS="-arch ${ARCH[$i]} -isysroot ${SYSROOT}"
	
	if [ "${TARGET[$i]}" = "iPhoneSimulator" ]; then
		export CFLAGS="${CFLAGS} -D__IPHONE_OS_VERSION_MIN_REQUIRED=${IOS_DEPLOYMENT_TARGET%%.*}0000"
	fi
	
	rm -rf ${TMP_DIR}

	./configure --prefix=${TMP_DIR} \
		--host=${TOOLCHAIN_NAME[$i]} \
        --with-darwinssl \
        --without-libidn2 \
        --enable-ipv6 \
        --enable-static \
        --enable-threaded-resolver \
        --disable-shared \
        --disable-dict \
        --disable-gopher \
        --disable-ldap \
        --disable-ldaps \
        --disable-manual \
        --disable-pop3 \
        --disable-smtp \
        --disable-imap \
        --disable-rtsp \
        --disable-smb \
        --disable-telnet \
        --disable-verbose

	if make -j${CPU_CORE}; then
		make install
		
		if [ ! -d ${WORKING_DIR}/include/curl ]; then
			mkdir -p ${WORKING_DIR}/include/curl
		fi

		cp ${TMP_DIR}/include/curl/* ${WORKING_DIR}/include/curl/
        cp ${TMP_DIR}/lib/libcurl.a ${TMP_LIB_DIR}/libcurl-${ARCH_NAME[$i]}.a
	fi

	make clean
done

cd ${TMP_LIB_DIR}
lipo -create -output "libcurl.a" "libcurl-armv7.a" "libcurl-armv7s.a" "libcurl-arm64.a" "libcurl-x86.a" "libcurl-x86_64.a"
cp ${TMP_LIB_DIR}/libcurl.a ${WORKING_DIR}/../../lib/ios/
cd ..

# Cleanup
rm -rf ${TMP_LIB_DIR}
rm -rf ${TMP_DIR}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}.tar.gz
