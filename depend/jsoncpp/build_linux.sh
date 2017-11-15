#!/bin/bash

# User configuration
LIBRARY_VERSION="1.8.3"

# Check software and detect CPU cores
CPU_CORE=1
PLATFORM_NAME=`uname`
if [[ "${PLATFORM_NAME}" == "Linux" ]]; then
   CPU_CORE=`nproc`
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
TMP_DIR=${WORKING_DIR}/tmp
ARCH_FLAG=("-m32" "-m64")
ARCH_NAME=("x86" "x86_64")

# Download and extract library
LIBRARY_NAME=jsoncpp-${LIBRARY_VERSION}
TMP_DIR=${WORKING_DIR}/${LIBRARY_NAME}/tmp

rm -rf ${LIBRARY_NAME}
[ -f ${LIBRARY_VERSION}.tar.gz ] || wget --no-check-certificate https://github.com/open-source-parsers/jsoncpp/archive/${LIBRARY_VERSION}.tar.gz;
tar xfz ${LIBRARY_VERSION}.tar.gz

# Remove include directory
rm -rf ${WORKING_DIR}/include

# Build for all architectures and copy data
cp ${WORKING_DIR}/Makefile.in ${WORKING_DIR}/${LIBRARY_NAME}/Makefile
cd ${WORKING_DIR}/${LIBRARY_NAME}

for ((i=0; i<${#ARCH_NAME[@]}; i++)); do
	export CXXFLAGS="${ARCH_FLAG[$i]}"

	rm -rf ${TMP_DIR}
	mkdir ${TMP_DIR}
	
	if [ ! -d ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]} ]; then
		mkdir -p ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]}
	else
		rm -f ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]}/lib${TARGET_NAME}.a
	fi

	if make $1 -j${CPU_CORE}; then
        cp ${TMP_DIR}/lib${TARGET_NAME}.a ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]}/lib${TARGET_NAME}.a
	fi

    make clean
done

mkdir -p ${WORKING_DIR}/include/json
cp ${WORKING_DIR}/${LIBRARY_NAME}/include/json/* ${WORKING_DIR}/include/json/
cd ${WORKING_DIR}

# Cleanup
rm -rf ${TMP_DIR}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}
rm -rf ${WORKING_DIR}/${LIBRARY_VERSION}.tar.gz
