#!/bin/bash

# Check software and detect CPU cores
CPU_CORE=1
PLATFORM_NAME=`uname`
if [[ "${PLATFORM_NAME}" == "Darwin" ]]; then
   CPU_CORE=`sysctl -n hw.logicalcpu_max`
else
  echo "Error: ${PLATFORM_NAME} is not supported." >&2
  exit 1
fi

# Internal variables
WORKING_DIR=`pwd`
TARGET_NAME=zlib
TMP_LIB_DIR=${WORKING_DIR}/tmp_lib
ARCH_FLAG=("-m32" "-m64")
ARCH_NAME=("x86" "x86_64")

# Clean include and lib directory
rm -f ${WORKING_DIR}/../../lib/macos/lib${TARGET_NAME}.a

# Build for all architectures and copy data
mkdir ${TMP_LIB_DIR}

for ((i=0; i<${#ARCH_NAME[@]}; i++)); do
	export CFLAGS="${ARCH_FLAG[$i]} -gdwarf-2 -fembed-bitcode"

	if make $1 -j${CPU_CORE}; then
        cp lib${TARGET_NAME}.a ${TMP_LIB_DIR}/lib${TARGET_NAME}-${ARCH_NAME[$i]}.a
	fi

    make clean
done

cd ${TMP_LIB_DIR}
lipo -create -output "lib${TARGET_NAME}.a" "lib${TARGET_NAME}-${ARCH_NAME[0]}.a" "lib${TARGET_NAME}-${ARCH_NAME[1]}.a"
cp ${TMP_LIB_DIR}/lib${TARGET_NAME}.a ${WORKING_DIR}/../../lib/macos/

# Cleanup
rm -rf ${TMP_LIB_DIR}
