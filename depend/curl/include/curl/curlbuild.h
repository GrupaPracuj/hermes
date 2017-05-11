#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
#if defined(__LP64__) && __LP64__
#include "ios/curlbuild-x86_64.h"
#else
#include "ios/curlbuild-i386.h"
#endif
#elif TARGET_OS_IPHONE
#if defined(__LP64__) && __LP64__
#include "ios/curlbuild-arm64.h"
#else
#include "ios/curlbuild-armv7.h"
#endif
#else
# error "Not supported Apple platform"
#endif
#elif __linux__
#if defined(__LP64__) && __LP64__
#include "linux/curlbuild-x86_64.h"
#else
#include "linux/curlbuild-i386.h"
#endif
#else
# error "Not supported platform"
#endif
