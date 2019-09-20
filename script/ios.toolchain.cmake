if (HMS_XCODE_PATH)
    set(ENV{_HMS_XCODE_PATH} "${HMS_XCODE_PATH}")
else ()
    set(HMS_XCODE_PATH "$ENV{_HMS_XCODE_PATH}")
endif ()

if (HMS_TARGET)
    set(ENV{_HMS_TARGET} "${HMS_TARGET}")
else ()
    set(HMS_TARGET "$ENV{_HMS_TARGET}")
endif ()

set(CMAKE_SYSTEM_NAME "Darwin")

set(CMAKE_C_COMPILER "${HMS_XCODE_PATH}/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang")
set(CMAKE_CXX_COMPILER "${HMS_XCODE_PATH}/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++")
set(CMAKE_LD "${HMS_XCODE_PATH}/Toolchains/XcodeDefault.xctoolchain/usr/bin/ld")
set(CMAKE_AR "${HMS_XCODE_PATH}/Toolchains/XcodeDefault.xctoolchain/usr/bin/ar")
set(CMAKE_RANLIB "${HMS_XCODE_PATH}/Toolchains/XcodeDefault.xctoolchain/usr/bin/ranlib")
set(CMAKE_STRIP "${HMS_XCODE_PATH}/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip")

set(CMAKE_SYSROOT "${HMS_XCODE_PATH}/Platforms/${HMS_TARGET}.platform/Developer/SDKs/${HMS_TARGET}.sdk")
