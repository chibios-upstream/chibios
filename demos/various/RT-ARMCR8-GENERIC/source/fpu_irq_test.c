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
 * @file    fpu_irq_test.c
 * @brief   FPU registers across interrupts test.
 * @details A thread loads known values in all the FPU registers and in the
 *          FPSCR status bits, raises an SGI to itself and waits for the
 *          interrupt, then checks that the values are unchanged.
 *          - The interrupt handler overwrites the caller-saved registers
 *            d0-d7 and the FPSCR status bits, as any compiled C code is
 *            allowed to do.
 *          - On odd iterations the handler also resumes a higher priority
 *            thread, so the interrupt exits through a context switch. That
 *            thread overwrites all the FPU registers, including the
 *            callee-saved d8-d15, before suspending again.
 */

#include <stddef.h>

#include "ch.h"
#include "arm_gicv2.h"

#include "fpu_irq_test.h"

#if (CORTEX_USE_FPU == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Local definitions.                                                        */
/*===========================================================================*/

#define FIT_ITERATIONS          1000U

#define FIT_SGI                 1U
#define FIT_SGI_PRIORITY        0x80U

#define FIT_DREGS               16U

/* GICD_SGIR, the target list filter selects the requesting CPU.*/
#define FIT_GICD_SGIR           (GICV2_DISTRIBUTOR_BASE + 0xF00U)
#define FIT_SGIR_SELF           (2U << 24)

/* FPSCR status bits: N, Z, C, V and the cumulative exception flags.*/
#define FIT_FPSCR_STATUS_MASK   0xF000009FU
#define FIT_FPSCR_PROBE         0x60000000U
#define FIT_FPSCR_IRQ           0x9000009FU
#define FIT_FPSCR_THREAD        0x30000011U

#define FIT_CLOBBERS_D0_D7                                                  \
  "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"
#define FIT_CLOBBERS_D8_D15                                                 \
  "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15"

/*===========================================================================*/
/* Local types.                                                              */
/*===========================================================================*/

/**
 * @brief   Probe parameters and results.
 * @note    Accessed by offset from the probe asm code.
 */
typedef struct {
  uint64_t                  in[FIT_DREGS];
  uint64_t                  out[FIT_DREGS];
  uint32_t                  fpscr_in;
  uint32_t                  fpscr_out;
  uint32_t                  sgi_value;
  uint32_t                  sgi_reg;
  volatile uint32_t         *countp;
} fit_probe_t;

/*===========================================================================*/
/* Local variables.                                                          */
/*===========================================================================*/

static fit_probe_t fit_probe;
static uint64_t fit_irq_values[FIT_DREGS];
static uint64_t fit_thread_values[FIT_DREGS];
static volatile uint32_t fit_irq_count;
static volatile bool fit_irq_resume;
static thread_reference_t fit_trp;
static THD_WORKING_AREA(fit_wa, 512);

/*===========================================================================*/
/* Local functions.                                                          */
/*===========================================================================*/

/*
 * Overwrites d0-d7 and the FPSCR status bits, the FPSCR control bits are
 * preserved as required by the AAPCS.
 */
static void fit_clobber_volatile(const uint64_t *p, uint32_t status) {
  uint32_t tmp;

  __asm volatile (
    "vldmia  %[p0], {d0-d7}                 \n"
    "vmrs    %[tmp], fpscr                  \n"
    "bic     %[tmp], %[tmp], #0xF0000000    \n"
    "bic     %[tmp], %[tmp], #0x9F          \n"
    "orr     %[tmp], %[tmp], %[st]          \n"
    "vmsr    fpscr, %[tmp]                  \n"
    : [tmp] "=&r" (tmp)
    : [p0] "r" (p), [st] "r" (status)
    : "memory", FIT_CLOBBERS_D0_D7);
}

/*
 * Loads the input values, raises the SGI and waits for its handler, then
 * stores the register values seen after the interrupt.
 */
static void fit_run_probe(fit_probe_t *p) {

  __asm volatile (
    "vmrs    r4, fpscr                      \n"
    "ldr     r5, [%[p], %[o_countp]]        \n"
    "ldr     r6, [r5]                       \n"
    "add     r1, %[p], %[o_in]              \n"
    "vldmia  r1, {d0-d15}                   \n"
    "ldr     r1, [%[p], %[o_fpscr_in]]      \n"
    "vmsr    fpscr, r1                      \n"
    "ldr     r2, [%[p], %[o_sgi_value]]     \n"
    "ldr     r3, [%[p], %[o_sgi_reg]]       \n"
    "str     r2, [r3]                       \n" /* GICD_SGIR.*/
    "dsb                                    \n"
    "mov     r12, #0x100000                 \n"
    "1:                                     \n"
    "ldr     r1, [r5]                       \n"
    "cmp     r1, r6                         \n"
    "bne     2f                             \n"
    "subs    r12, r12, #1                   \n"
    "bne     1b                             \n"
    "2:                                     \n"
    "add     r1, %[p], %[o_out]             \n"
    "vstmia  r1, {d0-d15}                   \n"
    "vmrs    r1, fpscr                      \n"
    "str     r1, [%[p], %[o_fpscr_out]]     \n"
    "vmsr    fpscr, r4                      \n"
    :
    : [p]           "r" (p),
      [o_in]        "i" (offsetof(fit_probe_t, in)),
      [o_out]       "i" (offsetof(fit_probe_t, out)),
      [o_fpscr_in]  "i" (offsetof(fit_probe_t, fpscr_in)),
      [o_fpscr_out] "i" (offsetof(fit_probe_t, fpscr_out)),
      [o_sgi_value] "i" (offsetof(fit_probe_t, sgi_value)),
      [o_sgi_reg]   "i" (offsetof(fit_probe_t, sgi_reg)),
      [o_countp]    "i" (offsetof(fit_probe_t, countp))
    : "r1", "r2", "r3", "r4", "r5", "r6", "r12", "cc", "memory",
      FIT_CLOBBERS_D0_D7, FIT_CLOBBERS_D8_D15);
}

/*
 * SGI handler.
 */
static bool fit_irq_handler(uint32_t intid) {
  bool preemption_required;

  (void)intid;

  fit_clobber_volatile(fit_irq_values, FIT_FPSCR_IRQ);

  preemption_required = false;
  if (fit_irq_resume) {
    chSysLockFromISR();
    chThdResumeI(&fit_trp, MSG_OK);
    preemption_required = chSchIsPreemptionRequired();
    chSysUnlockFromISR();
  }

  fit_irq_count++;

  return preemption_required;
}

/*
 * Higher priority thread, it overwrites all the FPU registers each time it
 * is resumed. The asm statement is in the loop body so d8-d15 still hold
 * the thread values when it suspends again.
 */
static THD_FUNCTION(fit_thread, arg) {
  uint32_t tmp;

  (void)arg;

  chRegSetThreadName("fpu clobber");

  while (true) {
    msg_t msg;

    chSysLock();
    msg = chThdSuspendS(&fit_trp);
    chSysUnlock();

    if (msg != MSG_OK) {
      break;
    }

    __asm volatile (
      "vldmia  %[p0], {d0-d15}              \n"
      "vmrs    %[tmp], fpscr                \n"
      "bic     %[tmp], %[tmp], #0xF0000000  \n"
      "bic     %[tmp], %[tmp], #0x9F        \n"
      "orr     %[tmp], %[tmp], %[st]        \n"
      "vmsr    fpscr, %[tmp]                \n"
      : [tmp] "=&r" (tmp)
      : [p0] "r" (fit_thread_values), [st] "r" (FIT_FPSCR_THREAD)
      : "memory", FIT_CLOBBERS_D0_D7, FIT_CLOBBERS_D8_D15);
  }
}

static uint64_t fit_value(uint32_t tag, uint32_t i, uint32_t r) {

  return ((uint64_t)(tag | (r << 8) | (i & 0xFFU)) << 32) |
         (uint64_t)((i * 0x01010101U) ^ (r * 0x10204081U));
}

/*===========================================================================*/
/* Exported functions.                                                       */
/*===========================================================================*/

/**
 * @brief   Executes the test.
 *
 * @param[out] rp       pointer to the results structure
 * @return              The test result.
 * @retval true         if the test succeeded.
 * @retval false        if the test failed.
 */
bool fpu_irq_test_execute(fpu_irq_test_result_t *rp) {
  thread_t *tp;
  uint32_t i, r;

  rp->iterations           = 0U;
  rp->missed               = 0U;
  rp->errors_no_preemption = 0U;
  rp->errors_preemption    = 0U;

  for (r = 0U; r < FIT_DREGS; r++) {
    fit_irq_values[r]    = fit_value(0xD1000000U, 0U, r);
    fit_thread_values[r] = fit_value(0xD7000000U, 0U, r);
  }

  fit_probe.sgi_value = FIT_SGIR_SELF | FIT_SGI;
  fit_probe.sgi_reg   = FIT_GICD_SGIR;
  fit_probe.countp    = &fit_irq_count;

  (void) gicv2RegisterHandler(FIT_SGI, fit_irq_handler);
  gicv2EnableInterrupt(FIT_SGI, FIT_SGI_PRIORITY, GICV2_CONFIG_EDGE,
                       GICV2_CPU_TARGET(0U));

  tp = chThdCreateStatic(fit_wa, sizeof (fit_wa),
                         chThdGetPriorityX() + 1U, fit_thread, NULL);

  for (i = 0U; i < FIT_ITERATIONS; i++) {
    uint32_t count, errors;
    bool resume;

    resume = (i & 1U) != 0U;
    for (r = 0U; r < FIT_DREGS; r++) {
      fit_probe.in[r]  = fit_value(0xA5000000U, i, r);
      fit_probe.out[r] = 0U;
    }
    fit_probe.fpscr_in = FIT_FPSCR_PROBE;
    fit_irq_resume = resume;

    count = fit_irq_count;
    fit_run_probe(&fit_probe);
    rp->iterations++;
    if (fit_irq_count == count) {
      rp->missed++;
      break;
    }

    errors = 0U;
    for (r = 0U; r < FIT_DREGS; r++) {
      if (fit_probe.out[r] != fit_probe.in[r]) {
        errors++;
      }
    }
    if ((fit_probe.fpscr_out & FIT_FPSCR_STATUS_MASK) != FIT_FPSCR_PROBE) {
      errors++;
    }

    if (resume) {
      rp->errors_preemption += errors;
    }
    else {
      rp->errors_no_preemption += errors;
    }
  }

  chThdResume(&fit_trp, MSG_RESET);
  (void) chThdWait(tp);
  gicv2DisableInterrupt(FIT_SGI);
  gicv2UnregisterHandler(FIT_SGI);

  return (rp->missed == 0U) && (rp->errors_no_preemption == 0U) &&
         (rp->errors_preemption == 0U);
}

#endif /* CORTEX_USE_FPU == TRUE */
