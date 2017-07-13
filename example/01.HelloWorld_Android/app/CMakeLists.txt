cmake_minimum_required(VERSION 3.4.1)

if(${ANDROID_ABI} STREQUAL "armeabi")
    set(HERMES_LIBRARY_PREFIX arm)
elseif(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(HERMES_LIBRARY_PREFIX armv7)
elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(HERMES_LIBRARY_PREFIX arm64)
elseif(${ANDROID_ABI} STREQUAL "mips")
    set(HERMES_LIBRARY_PREFIX mips)
elseif(${ANDROID_ABI} STREQUAL "mips64")
    set(HERMES_LIBRARY_PREFIX mips64)
elseif(${ANDROID_ABI} STREQUAL "x86")
    set(HERMES_LIBRARY_PREFIX x86)
elseif(${ANDROID_ABI} STREQUAL "x86_64")
    set(HERMES_LIBRARY_PREFIX x86_64)
endif()

set(HERMES_LIBRARY_PATH $ENV{HERMES_HOME}/lib/android/${HERMES_LIBRARY_PREFIX})

add_library(libssl STATIC IMPORTED )
set_target_properties(libssl PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libssl.a)

add_library(libcrypto STATIC IMPORTED )
set_target_properties(libcrypto PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libcrypto.a)

add_library(libcurl STATIC IMPORTED )
set_target_properties(libcurl PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libcurl.a)

add_library(libjsoncpp STATIC IMPORTED )
set_target_properties(libjsoncpp PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libjsoncpp.a)

add_library(libhermes STATIC IMPORTED )
set_target_properties(libhermes PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libhermes.a)

add_library(main SHARED src/main/cpp/main.cpp)

target_link_libraries(main libhermes libjsoncpp libcurl libssl libcrypto android z log)
