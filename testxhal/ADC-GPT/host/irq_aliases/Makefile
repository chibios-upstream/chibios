# Compile-time checks for platform-owned peripheral IRQ priority aliases.
CHIBIOS := ../../../..
STM32 := $(CHIBIOS)/os/xhal/ports/STM32
CPPFLAGS := -I$(STM32)
CFLAGS := -std=c11 -Wall -Wextra -Wundef -Werror
TESTS := c0 g031 g071 g0b0 g0b1 g4 g4_old u5 wl_m4 wl_m0 u0 h5

.PHONY: all clean $(TESTS)
all: $(TESTS)

c0: TEST_PLATFORM := 0
g031: TEST_PLATFORM := 1
g071: TEST_PLATFORM := 2
g0b0: TEST_PLATFORM := 3
g0b1: TEST_PLATFORM := 4
g4: TEST_PLATFORM := 5
g4_old: TEST_PLATFORM := 6
u5: TEST_PLATFORM := 7
wl_m4: TEST_PLATFORM := 8
wl_m0: TEST_PLATFORM := 9
u0: TEST_PLATFORM := 10
h5: TEST_PLATFORM := 11

$(TESTS):
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTEST_PLATFORM=$(TEST_PLATFORM) \
	      -fsyntax-only priority_aliases.c

# Syntax-only checks produce no build artifacts.
clean:
