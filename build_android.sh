#!/bin/bash

# Check settings
if [ -z "${HERMES_ANDROID_API}" ]; then
    ANDROID_API="android-21"
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

# Build for all architectures and copy data
for ((i=0; i<${#ARCH[@]}; i++)); do
	TOOLCHAIN_BIN_PATH=${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}/bin/${TOOLCHAIN_NAME[$i]}
	export CXX=${TOOLCHAIN_BIN_PATH}-clang++
	export LD=${TOOLCHAIN_BIN_PATH}-ld
	export AR=${TOOLCHAIN_BIN_PATH}-ar
	export RANLIB=${TOOLCHAIN_BIN_PATH}-ranlib
	export STRIP=${TOOLCHAIN_BIN_PATH}-strip
	export SYSROOT=${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCH[$i]}/sysroot
	export CXXFLAGS="${ARCH_FLAGS} --sysroot=${SYSROOT}"
	export LDFLAGS="${ARCH_LINK} --sysroot=${SYSROOT}"

    if [ "${ARCH[$i]}" = "linux64-mips64" ]; then
		export CXXFLAGS="${CXXFLAGS} -fintegrated-as"
	fi
	
	if [ ! -d ${WORKING_DIR}/lib/android/${ARCH_NAME[$i]} ]; then
		mkdir -p ${WORKING_DIR}/../../lib/android/${ARCH_NAME[$i]}
	else
		rm -f ${WORKING_DIR}/lib/android/${ARCH_NAME[$i]}/libhermes.a
	fi

	rm -rf ${TMP_DIR}
    mkdir ${TMP_DIR}

	if make $1 -j${CPU_CORE}; then
        cp ${TMP_DIR}/libhermes.a ${WORKING_DIR}/lib/android/${ARCH_NAME[$i]}/
	fi

	make clean
done

# Cleanup
rm -rf ${TMP_DIR}
rm -rf ${TOOLCHAIN_DIR}

