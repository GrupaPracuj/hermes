#ifndef _OPENSSLCONF_SHARED_H_
#define _OPENSSLCONF_SHARED_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define NON_EMPTY_TRANSLATION_UNIT static void *dummy = &dummy;
#define DECLARE_DEPRECATED(f) f __attribute__ ((deprecated));

#define OPENSSL_FILE __FILE__
#define OPENSSL_LINE __LINE__

#define DEPRECATEDIN_1_1_0(f)   DECLARE_DEPRECATED(f)
#define DEPRECATEDIN_1_0_0(f)   DECLARE_DEPRECATED(f)
#define DEPRECATEDIN_0_9_8(f) DECLARE_DEPRECATED(f)

#undef I386_ONLY
#undef OPENSSL_UNISTD
#define OPENSSL_UNISTD <unistd.h>
#undef OPENSSL_EXPORT_VAR_AS_FUNCTION

#if __APPLE__
    #include <TargetConditionals.h>
    #error "Not supported Apple platform"
#elif defined(ANDROID) || defined(__ANDROID__)
    #if defined(__arm__)
        #define BN_LLONG
        #undef SIXTY_FOUR_BIT_LONG
        #undef SIXTY_FOUR_BIT
        #define THIRTY_TWO_BIT
        #define RC4_INT unsigned char
    #elif defined(__aarch64__)
        #undef BN_LLONG
        #define SIXTY_FOUR_BIT_LONG
        #undef SIXTY_FOUR_BIT
        #undef THIRTY_TWO_BIT
        #define RC4_INT unsigned char
    #elif defined(__i386__)
        #define BN_LLONG
        #undef SIXTY_FOUR_BIT_LONG
        #undef SIXTY_FOUR_BIT
        #define THIRTY_TWO_BIT
        #define RC4_INT unsigned int
    #elif defined(__x86_64__)
        #undef BN_LLONG
        #define SIXTY_FOUR_BIT_LONG
        #undef SIXTY_FOUR_BIT
        #undef THIRTY_TWO_BIT
        #define RC4_INT unsigned int
    #else
        #error "Not supported Android platform"
    #endif
#elif __linux__
    #if defined(__LP64__) && __LP64__
        #undef BN_LLONG
        #define SIXTY_FOUR_BIT_LONG
        #undef SIXTY_FOUR_BIT
        #undef THIRTY_TWO_BIT
        #define RC4_INT unsigned int
    #elif defined(__i386__)
        #define BN_LLONG
        #undef SIXTY_FOUR_BIT_LONG
        #undef SIXTY_FOUR_BIT
        #define THIRTY_TWO_BIT
        #define RC4_INT unsigned int
    #else
        #error "Not supported Linux platform"
    #endif
#else
    #error "Not supported platform"
#endif

#ifdef  __cplusplus
}
#endif

#endif
