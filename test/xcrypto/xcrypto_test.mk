# List of all the ChibiOS/XHAL Crypto test files.
TESTSRC += ${CHIBIOS}/test/xcrypto/source/test/xcry_test_root.c \
           ${CHIBIOS}/test/xcrypto/source/testref/xcry_vectors.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_001.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_002.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_003.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_004.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_005.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_006.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_007.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_008.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_009.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_010.c \
           ${CHIBIOS}/test/xcrypto/source/test/xcry_test_sequence_011.c

# Required include directories
TESTINC += ${CHIBIOS}/test/xcrypto/source/test \
           ${CHIBIOS}/test/xcrypto/source/testref
