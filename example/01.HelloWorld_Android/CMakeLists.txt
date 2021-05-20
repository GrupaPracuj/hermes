cmake_minimum_required(VERSION 3.4.1)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG=1 -DDEBUG=1")
SET(CMAKE_CXX_FLAGS_RELEASE "-g -O2 -DNDEBUG=1")

set(HERMES_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../../lib/android/${ANDROID_ABI})

add_library(libaes STATIC IMPORTED )
set_target_properties(libaes PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libaes.a)

add_library(libcrypto STATIC IMPORTED )
set_target_properties(libcrypto PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libcrypto.a)

add_library(libcurl STATIC IMPORTED )
set_target_properties(libcurl PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libcurl.a)

add_library(libhermes STATIC IMPORTED )
set_target_properties(libhermes PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libhermes.a)

add_library(libhmsextjni STATIC IMPORTED )
set_target_properties(libhmsextjni PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libhmsextjni.a)

add_library(libhmsextserializer STATIC IMPORTED )
set_target_properties(libhmsextserializer PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libhmsextserializer.a)

add_library(libjsoncpp STATIC IMPORTED )
set_target_properties(libjsoncpp PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libjsoncpp.a)

add_library(libssl STATIC IMPORTED )
set_target_properties(libssl PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libssl.a)

add_library(libzlib STATIC IMPORTED )
set_target_properties(libzlib PROPERTIES IMPORTED_LOCATION ${HERMES_LIBRARY_PATH}/libzlib.a)

include_directories(${PROJECT_SOURCE_DIR}/../01.HelloWorld)

file(GLOB SOURCE_LIST
    ${PROJECT_SOURCE_DIR}/../01.HelloWorld/*.cpp
    ${PROJECT_SOURCE_DIR}/../01.HelloWorld/*.hpp
    src/main/cpp/*.cpp
    src/main/cpp/*.hpp
)

add_library(helloworld SHARED ${SOURCE_LIST})

target_link_libraries(helloworld libhmsextserializer libhmsextjni libhermes libaes libcurl libjsoncpp libzlib libssl libcrypto android log)
