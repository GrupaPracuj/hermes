#!/bin/bash

# User configuration
LIBRARY_NAME="openssl-1.1.0f"
ANDROID_NDK_ROOT="${ANDROID_HOME}/ndk-bundle"
ANDROID_API="android-21"

# Check settings
if [ -z "${ANDROID_NDK_ROOT}" ] || [ ! -d "${ANDROID_NDK_ROOT}" ]; then
    ANDROID_NDK_ROOT="${ANDROID_HOME}/ndk-bundle"

    if [ ! -d "${ANDROID_NDK_ROOT}" ]; then
        echo "Error: ${ANDROID_NDK_ROOT} is not a valid path."
        exit 1
    fi
fi

# Check software and detect CPU cores
CPU_CORE=1
PLATFORM_NAME=`uname`
if [[ "${PLATFORM_NAME}" == "Linux" ]]; then
   CPU_CORE=`cat /proc/cpuinfo | grep processor | wc -l`
elif [[ "${PLATFORM_NAME}" == "Darwin" ]]; then
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
TOOLCHAIN_DIR=${WORKING_DIR}/toolchain
TOOLCHAIN_ARCH=("arm" "arm" "arm64" "mips" "mips64" "x86" "x86_64")
TOOLCHAIN_NAME=("arm-linux-androideabi" "arm-linux-androideabi" "aarch64-linux-android" "mipsel-linux-android" "mips64el-linux-android" "i686-linux-android" "x86_64-linux-android")
ARCH=("android" "android-armeabi" "android64-aarch64" "android-mips" "linux64-mips64" "android-x86" "android64")
ARCH_FLAG=("-mthumb" "-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon" "" "" "" "-march=i686 -m32 -mtune=intel -msse3 -mfpmath=sse" "-march=x86-64 -m64 -mtune=intel -msse4.2 -mpopcnt")
ARCH_LINK=("" "-march=armv7-a -Wl,--fix-cortex-a8" "" "" "" "" "")
ARCH_NAME=("arm" "armv7" "arm64" "mips" "mips64" "x86" "x86_64")

# Prepare toolchains
for ((i=0; i<${#TOOLCHAIN_ARCH[@]}; i++)); do
	if [ ! -d ${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]} ]; then
		${ANDROID_NDK_ROOT}/build/tools/make-standalone-toolchain.sh --arch=${TOOLCHAIN_ARCH[$i]} --platform=${ANDROID_API} --stl=libc++ --install-dir=${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}
	fi
done

# Download and extract library
rm -rf ${LIBRARY_NAME}
[ -f ${LIBRARY_NAME}.tar.gz ] || wget --no-check-certificate https://www.openssl.org/source/${LIBRARY_NAME}.tar.gz;
tar xfz ${LIBRARY_NAME}.tar.gz

# Fix for clang
sed -i -e 's/-mandroid//g' ${LIBRARY_NAME}/Configurations/10-main.conf

# Clean include directory
rm ${WORKING_DIR}/include/openssl/*
rm ${WORKING_DIR}/include/openssl/android/*

# Build for all architectures and copy data
cd ${LIBRARY_NAME}

for ((i=0; i<${#ARCH[@]}; i++)); do
	TOOLCHAIN_BIN_PATH=${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}/bin/${TOOLCHAIN_NAME[$i]}
	export CC=${TOOLCHAIN_BIN_PATH}-clang
	export LD=${TOOLCHAIN_BIN_PATH}-ld
	export AR=${TOOLCHAIN_BIN_PATH}-ar
	export RANLIB=${TOOLCHAIN_BIN_PATH}-ranlib
	export STRIP=${TOOLCHAIN_BIN_PATH}-strip
	export SYSROOT=${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}/sysroot
	export CROSS_SYSROOT=${SYSROOT}
	export CFLAGS="${ARCH_FLAGS} -O2 -fPIC -fno-strict-aliasing -fstack-protector"
	export LDFLAGS="${ARCH_LINK}"

	rm -f ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/libcrypto.a
	rm -f ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/libssl.a
	rm -rf ${TMP_DIR}

	./Configure ${ARCH[$i]} --prefix=${TMP_DIR} \
		--openssldir=${TMP_DIR} \
        --with-zlib-include=${SYSROOT}/usr/include \
        --with-zlib-lib=${SYSROOT}/usr/lib \
        zlib \
        no-asm \
        no-shared \
        no-unit-test

	if make -j${CPU_CORE}; then
		make install

		cp ${TMP_DIR}/include/openssl/* ${WORKING_DIR}/include/openssl/
		cp ${WORKING_DIR}/include/openssl/opensslconf.h ${WORKING_DIR}/include/openssl/android/opensslconf-${ARCH_NAME[$i]}.h
        cp ${TMP_DIR}/lib/libcrypto.a ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/
        cp ${TMP_DIR}/lib/libssl.a ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/
	fi

	make clean
done

cp ${WORKING_DIR}/opensslconf_shared.h.in ${WORKING_DIR}/include/openssl/opensslconf.h
cd ..

# Cleanup
rm -rf ${TMP_DIR}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}.tar.gz
rm -rf ${TOOLCHAIN_DIR}
