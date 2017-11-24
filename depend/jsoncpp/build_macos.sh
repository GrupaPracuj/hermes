#!/bin/bash

# User configuration
LIBRARY_VERSION="1.8.3"

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
TARGET_NAME=jsoncpp
TMP_LIB_DIR=${WORKING_DIR}/tmp_lib
ARCH_FLAG=("-m32" "-m64")
ARCH_NAME=("x86" "x86_64")

# Download and extract library
LIBRARY_NAME=jsoncpp-${LIBRARY_VERSION}

rm -rf ${LIBRARY_NAME}
[ -f ${LIBRARY_VERSION}.tar.gz ] || wget --no-check-certificate https://github.com/open-source-parsers/jsoncpp/archive/${LIBRARY_VERSION}.tar.gz;
tar xfz ${LIBRARY_VERSION}.tar.gz

# Remove include and lib directory
rm -rf ${WORKING_DIR}/include
rm -f ${WORKING_DIR}/../../lib/macos/lib${TARGET_NAME}.a

# Build for all architectures and copy data
cp ${WORKING_DIR}/Makefile.in ${WORKING_DIR}/${LIBRARY_NAME}/Makefile
mkdir ${TMP_LIB_DIR}
cd ${WORKING_DIR}/${LIBRARY_NAME}

for ((i=0; i<${#ARCH_NAME[@]}; i++)); do
	export CXXFLAGS="${ARCH_FLAG[$i]} -gdwarf-2 -fembed-bitcode"

	if make $1 -j${CPU_CORE}; then
        cp lib${TARGET_NAME}.a ${TMP_LIB_DIR}/lib${TARGET_NAME}-${ARCH_NAME[$i]}.a
	fi

    make clean
done

cd ${TMP_LIB_DIR}
lipo -create -output "lib${TARGET_NAME}.a" "lib${TARGET_NAME}-${ARCH_NAME[0]}.a" "lib${TARGET_NAME}-${ARCH_NAME[1]}.a"
cp ${TMP_LIB_DIR}/lib${TARGET_NAME}.a ${WORKING_DIR}/../../lib/macos/
mkdir -p ${WORKING_DIR}/include/json
cp ${WORKING_DIR}/${LIBRARY_NAME}/include/json/* ${WORKING_DIR}/include/json/
cd ${WORKING_DIR}

# Cleanup
rm -rf ${TMP_LIB_DIR}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}
rm -rf ${WORKING_DIR}/${LIBRARY_VERSION}.tar.gz
