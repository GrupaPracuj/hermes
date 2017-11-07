#!/bin/bash

# User configuration
LIBRARY_NAME="curl-7.56.0"

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
TARGET_NAME=curl
TMP_DIR=${WORKING_DIR}/tmp
ARCH_FLAG=("-m32" "-m64")
ARCH_NAME=("x86" "x86_64")

# Download and extract library
rm -rf ${LIBRARY_NAME}
[ -f ${LIBRARY_NAME}.tar.gz ] || wget --no-check-certificate https://curl.haxx.se/download/${LIBRARY_NAME}.tar.gz;
tar xfz ${LIBRARY_NAME}.tar.gz

# Remove include directory
rm -rf ${WORKING_DIR}/include

# Build for all architectures and copy data
cd ${LIBRARY_NAME}

for ((i=0; i<${#ARCH_NAME[@]}; i++)); do
	export CFLAGS="${ARCH_FLAG[$i]} -pipe -fPIC -fno-strict-aliasing -fstack-protector"

	rm -rf ${TMP_DIR}
	
    if [ ! -d ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]} ]; then
		mkdir -p ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]}
	else
		rm -f ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]}/lib${TARGET_NAME}.a
	fi

	./configure --prefix=${TMP_DIR} \
		--host=${TOOLCHAIN_NAME[$i]} \
        --with-ssl \
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

		cp ${TMP_DIR}/include/${TARGET_NAME}/* ${WORKING_DIR}/include/${TARGET_NAME}/
        cp ${TMP_DIR}/lib/lib${TARGET_NAME}.a ${WORKING_DIR}/../../lib/linux/${ARCH_NAME[$i]}/lib${TARGET_NAME}.a
	fi

    make clean
done

# Cleanup
rm -rf ${TMP_DIR}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}
rm -rf ${WORKING_DIR}/${LIBRARY_NAME}.tar.gz
