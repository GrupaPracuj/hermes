if (HMS_ARCH)
    set(ENV{_HMS_ARCH} "${HMS_ARCH}")
else ()
    set(HMS_ARCH "$ENV{_HMS_ARCH}")
endif ()

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_OSX_ARCHITECTURES "${HMS_ARCH}")
