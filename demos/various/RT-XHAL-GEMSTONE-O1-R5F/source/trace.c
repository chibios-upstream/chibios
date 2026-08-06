/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    trace.c
 * @brief   RemoteProc trace buffer logging.
 * @details Append-only character buffer in a non-cacheable DDR window, the
 *          Linux remoteproc core exposes it through debugfs. Formatting is
 *          intentionally minimal: %s, %c, %d, %u, %x and %% only.
 */

#include <stdarg.h>
#include <stdint.h>

#include "board.h"
#include "trace.h"

/* Plain CPSR-based critical section, usable from any context including
   before kernel initialization, the ARMv7-R port has no recursive locks.*/
static inline uint32_t irq_save(void) {
  uint32_t cpsr;

  __asm volatile ("mrs %0, cpsr" : "=r" (cpsr));
  __asm volatile ("cpsid i" ::: "memory");
  return cpsr;
}

static inline void irq_restore(uint32_t cpsr) {

  if ((cpsr & 0x80U) == 0U) {
    __asm volatile ("cpsie i" ::: "memory");
  }
}

__attribute__((section(".trace"), used))
static char trace_buffer[AM67_TRACEBUF_SIZE];

/* The final 0x100 bytes hold the bring-up boot markers (see board.c), the
   log never writes into or clears that area.*/
#define TRACE_BOOTMARK_SIZE     0x100U
#define TRACE_LOG_SIZE          (AM67_TRACEBUF_SIZE - TRACE_BOOTMARK_SIZE)

static uint32_t trace_pos;

static void trace_putc(char c) {

  /* Last byte stays zero as terminator, logging stops when full.*/
  if (trace_pos < (TRACE_LOG_SIZE - 1U)) {
    trace_buffer[trace_pos++] = c;
  }
}

static void trace_puts(const char *s) {

  while (*s != '\0') {
    trace_putc(*s++);
  }
}

static void trace_putu(uint32_t value, uint32_t base) {
  char digits[11];
  uint32_t i = 0U;

  do {
    uint32_t d = value % base;
    digits[i++] = (d < 10U) ? (char)('0' + d) : (char)('a' + d - 10U);
    value /= base;
  } while (value != 0U);

  while (i > 0U) {
    trace_putc(digits[--i]);
  }
}

void trace_init(void) {
  uint32_t i;

  for (i = 0U; i < TRACE_LOG_SIZE; i++) {
    trace_buffer[i] = '\0';
  }
  trace_pos = 0U;
}

void trace_printf(const char *fmt, ...) {
  va_list ap;
  uint32_t sts;

  sts = irq_save();

  va_start(ap, fmt);
  while (*fmt != '\0') {
    if (*fmt != '%') {
      trace_putc(*fmt++);
      continue;
    }

    fmt++;
    switch (*fmt++) {
    case 's':
      trace_puts(va_arg(ap, const char *));
      break;
    case 'c':
      trace_putc((char)va_arg(ap, int));
      break;
    case 'u':
      trace_putu(va_arg(ap, uint32_t), 10U);
      break;
    case 'x':
      trace_putu(va_arg(ap, uint32_t), 16U);
      break;
    case 'd': {
      int32_t value = va_arg(ap, int32_t);
      if (value < 0) {
        trace_putc('-');
        value = -value;
      }
      trace_putu((uint32_t)value, 10U);
      break;
    }
    case '%':
      trace_putc('%');
      break;
    default:
      /* Unknown specifier, printed as-is to keep the log readable.*/
      trace_putc('%');
      trace_putc(*(fmt - 1));
      break;
    }
  }
  va_end(ap);

  irq_restore(sts);
}
