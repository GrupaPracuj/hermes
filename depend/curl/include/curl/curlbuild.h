#ifndef _CURLBUILD_SHARED_H_
#define _CURLBUILD_SHARED_H_

#if __APPLE__
	#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR
		#if defined(__LP64__) && __LP64__
			#include "ios/curlbuild-x86_64.h"
		#else
			#include "ios/curlbuild-x86.h"
		#endif
	#elif TARGET_OS_IPHONE
		#if defined(__LP64__) && __LP64__
			#include "ios/curlbuild-arm64.h"
		#else
			#include "ios/curlbuild-armv7.h"
		#endif
	#else
		#error "Not supported Apple platform"
	#endif
#elif defined(ANDROID) || defined(__ANDROID__)
	#if defined(__arm__)
		#if defined(__ARM_ARCH_7A__)
			#include "android/curlbuild-armv7.h"
		#else
			#include "android/curlbuild-arm.h"
		#endif
	#elif defined(__aarch64__)
		#include "android/curlbuild-arm64.h"
	#elif defined(__i386__)
		#include "android/curlbuild-x86.h"
	#elif defined(__x86_64__)
		#include "android/curlbuild-x86_64.h"
	#elif defined(__mips64)
		#include "android/curlbuild-mips64.h"
	#elif defined(__mips__)
		#include "android/curlbuild-mips.h"
	#else
		#error "Not supported Android platform"
	#endif
#elif __linux__
	#if defined(__LP64__) && __LP64__
		#include "linux/curlbuild-x86_64.h"
	#else
		#include "linux/curlbuild-x86.h"
	#endif
#else
	#error "Not supported platform"
#endif

#endif
