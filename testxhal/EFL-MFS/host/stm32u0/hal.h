/* Host kernel/register shim. All driver types and HLDs are the real ones. */
#ifndef TEST_EFL_U0_HAL_H
#define TEST_EFL_U0_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#if defined(STM32U031xx)
#include "stm32u031xx.h"
_Static_assert(FLASHSIZE_BASE == 0x1FFF3EA0U, "U031 size register");
#elif defined(STM32U073xx)
#include "stm32u073xx.h"
_Static_assert(FLASHSIZE_BASE == 0x1FFF6EA0U, "U073 size register");
#else
#include "stm32u083xx.h"
_Static_assert(FLASHSIZE_BASE == 0x1FFF6EA0U, "U083 size register");
#endif
#include "oop_base_object.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_EFL TRUE
#define HAL_USE_MUTUAL_EXCLUSION FALSE
#define HAL_USE_REGISTRY FALSE
#define HAL_RET_SUCCESS 0
#define HAL_RET_INV_STATE -1
#define HAL_RET_CONFIG_ERROR -2
typedef int msg_t;

extern FLASH_TypeDef test_flash;
extern uint16_t test_size_kb;
extern uint8_t test_memory[256U * 1024U];
#undef FLASH
#undef FLASH_BASE
#undef FLASHSIZE_BASE
#define FLASH (&test_flash)
#define FLASH_BASE ((uintptr_t)test_memory)
#define FLASHSIZE_BASE ((uintptr_t)&test_size_kb)

void test_check(bool condition);
#define chDbgCheck(c) test_check(c)
#define chDbgAssert(c, msg) test_check(c)
void chSysLock(void);
void chSysUnlock(void);
void chThdSleepMilliseconds(unsigned msec);
void chSysHalt(const char *reason);

/* Registry-disabled name accessors have an unused local in the current HLD. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "hal_base_driver.h"
#pragma GCC diagnostic pop
#include "hal_efl.h"

#endif
