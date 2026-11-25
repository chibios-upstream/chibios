#include <stdint.h>

/* Configuration */

#define WOLFSSL_GENERAL_ALIGNMENT 4
#define HAVE_TM_TYPE
#define WORD64_AVAILABLE


/* ChibiOS + Lwip */
#define WOLFSSL_USER_IO
#define WOLFSSL_NO_SOCK
#define WOLFSSL_CHIBIOS 

#define USER_TICKS 
#define WOLFSSL_USER_CURRTIME 
#define XMALLOC_OVERRIDE 
//#define USE_WOLF_TIME_T
#define XTIME(tl)       (LowResTimer())


/* ARM  */
		
#define RSA_LOW_MEM
#define NO_OLD_RNGNAME  
#define SMALL_SESSION_CACHE
#define WOLFSSL_SMALL_STACK

#define WOLFSSL_SP_MATH_ALL
#define WOLFSSL_SP_SMALL
#define SINGLE_THREADED
#define NO_SIG_WRAPPER
		
/* Cipher features */
//#define USE_FAST_MATH
//#define ALT_ECC_SIZE

#define HAVE_FFDHE_2048
#define HAVE_CHACHA 
#define HAVE_POLY1305 
#define HAVE_ECC 
#define HAVE_CURVE25519
#define CURVED25519_SMALL
#define HAVE_ONE_TIME_AUTH
#define WOLFSSL_DH_CONST

/* TLS 1.2 and 1.3 with the extensions expected by current clients */
#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_SERVER_RENEGOTIATION_INFO
#define HAVE_EXTENDED_MASTER
#define HAVE_ENCRYPT_THEN_MAC
#define HAVE_AESGCM
#define HAVE_HKDF
#define WC_RSA_PSS
		
/* HW RNG support */

unsigned int chibios_rand_generate(void);
int custom_rand_generate_block(unsigned char* output, unsigned int sz);

#define CUSTOM_RAND_GENERATE chibios_rand_generate
#define CUSTOM_RAND_TYPE uint32_t

#define HAVE_ED25519
#define HAVE_POLY1305
#define HAVE_SHA512
#define WOLFSSL_SHA512


/* Size/speed config */
#define USE_SLOW_SHA2
#define USE_SLOW_SHA512

/* Robustness */
#define TFM_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
#define WC_RSA_BLINDING

/* Remove Features */
#define NO_WRITEV
#define NO_DEV_RANDOM
#define NO_FILESYSTEM
#define NO_MAIN_DRIVER
#define NO_MD4
#define NO_RABBIT
#define NO_HC128
#define NO_DSA
#define NO_PWDBASED
#define NO_PSK
#define NO_DES3
#define NO_RC4


/* Realloc (to use without USE_FAST_MATH) */

void *chHeapRealloc (void *addr, uint32_t size);
#define XREALLOC(p, n, h, t) ((void)(h), (void)(t), chHeapRealloc((p), (n)))
