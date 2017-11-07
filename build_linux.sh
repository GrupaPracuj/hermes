#!/bin/bash

# Check software and detect CPU cores
CPU_CORE=1
PLATFORM_NAME=`uname`
if [[ "${PLATFORM_NAME}" == "Linux" ]]; then
   CPU_CORE=`nproc`
else
  echo "Error: ${PLATFORM_NAME} is not supported." >&2
  exit 1
fi

# Internal variables
WORKING_DIR=`pwd`
TARGET_NAME=hermes
ARCH_FLAG=("-m32" "-m64")
ARCH_NAME=("x86" "x86_64")

for ((i=0; i<${#ARCH_NAME[@]}; i++)); do
	export CXXFLAGS="${ARCH_FLAG[$i]}"
	
	if [ ! -d ${WORKING_DIR}/lib/linux/${ARCH_NAME[$i]} ]; then
		mkdir -p ${WORKING_DIR}/lib/linux/${ARCH_NAME[$i]}
	else
		rm -f ${WORKING_DIR}/lib/linux/${ARCH_NAME[$i]}/lib${TARGET_NAME}.a
	fi

	if make $1 -j${CPU_CORE}; then
        cp ${WORKING_DIR}/lib${TARGET_NAME}.a ${WORKING_DIR}/lib/linux/${ARCH_NAME[$i]}/lib${TARGET_NAME}.a
	fi

    make clean
done
