#!/bin/bash

# User configuration
LIBRARY_VERSION="1.8.4"
IOS_DEPLOYMENT_TARGET="8.0"

# Check settings
if [ -z "${XCODE_ROOT}" ] || [ ! -d "${XCODE_ROOT}" ]; then
    XCODE_ROOT=`xcode-select --print-path`

    if [ ! -d "${XCODE_ROOT}" ]; then
        echo "Error: ${XCODE_ROOT} is not a valid path."
        exit 1
    fi
fi

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
TMP_LIB_DIR=${WORKING_DIR}/tmp_lib
TOOLCHAIN_DIR=${XCODE_ROOT}/Toolchains/XcodeDefault.xctoolchain
TOOLCHAIN_NAME=("armv7-apple-darwin" "armv7s-apple-darwin" "arm-apple-darwin" "i386-apple-darwin" "x86_64-apple-darwin")
ARCH=("armv7" "armv7s" "arm64" "i386" "x86_64")
ARCH_NAME=("armv7" "armv7s" "arm64" "x86" "x86_64")
TARGET=("iPhoneOS" "iPhoneOS" "iPhoneOS" "iPhoneSimulator" "iPhoneSimulator")

# Download and extract library
LIBRARY_NAME=jsoncpp-${LIBRARY_VERSION}

rm -rf ${LIBRARY_NAME}
[ -f ${LIBRARY_VERSION}.tar.gz ] || wget --no-check-certificate https://github.com/open-source-parsers/jsoncpp/archive/${LIBRARY_VERSION}.tar.gz;
tar xfz ${LIBRARY_VERSION}.tar.gz

# Remove include and lib directory
rm -rf ${WORKING_DIR}/include
rm -f ${WORKING_DIR}/../../lib/ios/libjsoncpp.a

# Build for all architectures and copy data
cp ${WORKING_DIR}/Makefile.in ${WORKING_DIR}/${LIBRARY_NAME}/Makefile
mkdir ${TMP_LIB_DIR}
cd ${WORKING_DIR}/${LIBRARY_NAME}
export CXX=${TOOLCHAIN_DIR}/usr/bin/clang++
export LD=${TOOLCHAIN_DIR}/usr/bin/ld
export AR=${TOOLCHAIN_DIR}/usr/bin/ar
export RANLIB=${TOOLCHAIN_DIR}/usr/bin/ranlib
export STRIP=${TOOLCHAIN_DIR}/usr/bin/strip

for ((i=0; i<${#ARCH[@]}; i++)); do
	export SYSROOT=${XCODE_ROOT}/Platforms/${TARGET[$i]}.platform/Developer/SDKs/${TARGET[$i]}.sdk
	export CROSS_SYSROOT=${SYSROOT}
	export CXXFLAGS="-arch ${ARCH[$i]} -isysroot ${SYSROOT} -O2 -fPIC -fno-strict-aliasing -fstack-protector -pipe -gdwarf-2 -miphoneos-version-min=${IOS_DEPLOYMENT_TARGET}"
	export LDFLAGS="-arch ${ARCH[$i]} -isysroot ${SYSROOT}"
	
	if [ "${TARGET[$i]}" = "iPhoneSimulator" ]; then
		export CXXFLAGS="${CXXFLAGS} -D__IPHONE_OS_VERSION_MIN_REQUIRED=${IOS_DEPLOYMENT_TARGET%%.*}0000"
	fi

	if make $1 -j${CPU_CORE}; then
        cp libjsoncpp.a ${TMP_LIB_DIR}/libjsoncpp-${ARCH_NAME[$i]}.a
	fi

    make clean
done

cd ${TMP_LIB_DIR}
lipo -create -output "libjsoncpp.a" "libjsoncpp-armv7.a" "libjsoncpp-armv7s.a" "libjsoncpp-arm64.a" "libjsoncpp-x86.a" "libjsoncpp-x86_64.a"
cp ${TMP_LIB_DIR}/libjsoncpp.a ${WORKING_DIR}/../../lib/ios/
mkdir -p ${WORKING_DIR}/include/json
cp ${WORKING_DIR}/${LIBRARY_NAME}/include/json/* ${WORKING_DIR}/include/json/
cd ${WORKING_DIR}

# Cleanup
rm -rf ${TMP_LIB_DIR}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}
rm -rf ${WORKING_DIR}/${LIBRARY_VERSION}.tar.gz
