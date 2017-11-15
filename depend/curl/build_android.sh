#!/bin/bash

# User configuration
LIBRARY_NAME="curl-7.56.1"

# Check settings
if [ -z "${HERMES_ANDROID_API}" ]; then
    ANDROID_API="21"
else
    ANDROID_API=${HERMES_ANDROID_API}
fi

if [ -z "${ANDROID_NDK_ROOT}" ] || [ ! -d "${ANDROID_NDK_ROOT}" ]; then
    ANDROID_NDK_ROOT="${ANDROID_HOME}/ndk-bundle"

    if [ ! -d "${ANDROID_NDK_ROOT}" ]; then
        echo "Error: ${ANDROID_NDK_ROOT} is not a valid path."
        exit 1
    fi
fi

printf "Android API: \"${ANDROID_API}\"\n"
printf "Android NDK path: \"${ANDROID_NDK_ROOT}\"\n\n"

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
TOOLCHAIN_ARCH=("arm" "arm64" "x86" "x86_64")
TOOLCHAIN_NAME=("arm-linux-androideabi" "aarch64-linux-android" "i686-linux-android" "x86_64-linux-android")
ARCH=("android-armeabi" "android64-aarch64" "android-x86" "android64")
ARCH_FLAG=("-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -mfpu=neon" "" "-march=i686 -m32 -mtune=intel -msse3 -mfpmath=sse" "-march=x86-64 -m64 -mtune=intel -msse4.2 -mpopcnt")
ARCH_NAME=("armv7" "arm64" "x86" "x86_64")

# Prepare toolchains
for ((i=0; i<${#TOOLCHAIN_ARCH[@]}; i++)); do
	if [ ! -d ${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]} ]; then
        python ${ANDROID_NDK_ROOT}/build/tools/make_standalone_toolchain.py --arch ${TOOLCHAIN_ARCH[$i]} --api ${ANDROID_API} --stl libc++ --install-dir ${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}
	fi
done

# Download and extract library
rm -rf ${LIBRARY_NAME}
[ -f ${LIBRARY_NAME}.tar.gz ] || wget --no-check-certificate https://curl.haxx.se/download/${LIBRARY_NAME}.tar.gz;
tar xfz ${LIBRARY_NAME}.tar.gz

# Remove include directory
rm -rf ${WORKING_DIR}/include

# Build for all architectures and copy data
cd ${LIBRARY_NAME}

for ((i=0; i<${#ARCH[@]}; i++)); do
    if [ ! -d ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]} ]; then
        mkdir -p ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}
    else
        rm -f ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/libcurl.a
    fi

    rm -rf ${TMP_DIR}

	TOOLCHAIN_BIN_PATH=${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}/bin/${TOOLCHAIN_NAME[$i]}
	CONFIGURE_FLAG_SSL=""
	export CC=${TOOLCHAIN_BIN_PATH}-clang
	export LD=${TOOLCHAIN_BIN_PATH}-ld
	export AR=${TOOLCHAIN_BIN_PATH}-ar
	export RANLIB=${TOOLCHAIN_BIN_PATH}-ranlib
	export STRIP=${TOOLCHAIN_BIN_PATH}-strip
	export SYSROOT=${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}/sysroot
	export CROSS_SYSROOT=${SYSROOT}
	export CFLAGS="${ARCH_FLAG[$i]} -O2 -pipe -fPIC -fno-strict-aliasing -fstack-protector"
	
	if [ -e ${CC} ]; then
        if [ -d ${WORKING_DIR}/../openssl/include/openssl/android ] && [ -e ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/libcrypto.a ]  && [ -e ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/libssl.a ]; then
            rm -rf ${WORKING_DIR}/${LIBRARY_NAME}/openssl
            mkdir -p ${WORKING_DIR}/${LIBRARY_NAME}/openssl/lib
            cp -R ${WORKING_DIR}/../openssl/include ${WORKING_DIR}/${LIBRARY_NAME}/openssl/
            cp ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/libssl.a ${WORKING_DIR}/${LIBRARY_NAME}/openssl/lib/
            cp ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/libcrypto.a ${WORKING_DIR}/${LIBRARY_NAME}/openssl/lib/

            CONFIGURE_FLAG_SSL="--with-ssl=${WORKING_DIR}/${LIBRARY_NAME}/openssl"
        fi

        ./configure --prefix=${TMP_DIR} \
            --host=${TOOLCHAIN_NAME[$i]} \
            ${CONFIGURE_FLAG_SSL} \
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
            cp ${TMP_DIR}/lib/libcurl.a ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}/
        fi

        make clean
    fi
done

cd ..

# Cleanup
rm -rf ${TMP_DIR}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}.tar.gz
rm -rf ${TOOLCHAIN_DIR}
