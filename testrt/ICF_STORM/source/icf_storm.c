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
 * @file    icf_storm.c
 * @brief   ICF Storm test code.
 * @details Nested interrupts served by byte-identical OS-aware handlers.
 *          Two handlers with identical bodies are candidates for identical
 *          code folding (-fipa-icf, enabled at -O2 and -Os, also under
 *          LTO), which can turn one of them into a thunk calling the other.
 *          A port deriving the outermost-ISR condition from a value the
 *          compiler can disturb, such as the LR read in the handler body,
 *          takes the wrong decision when the thunk is the nested handler:
 *          the nested exit pushes a second return context on the thread
 *          stack and returns to the outer handler with interrupts masked.
 *          After the outermost exit has performed the reschedule, that stale
 *          context runs the preemption code again in thread mode, with
 *          interrupts enabled, and switches to a thread of lower priority
 *          than the preempted one.
 *          The trigger thread pends the lower priority IRQ, whose handler
 *          pends the higher priority one, which preempts it. Each handler
 *          execution signals a semaphore waited by a thread with priority
 *          above the trigger thread. The trigger thread then pends a third
 *          IRQ served by a fast handler. Each phase checks that:
 *          - the nesting happened (maximum depth 2),
 *          - interrupts were enabled after every nested exit,
 *          - the outermost exit switched to the waiter before the trigger
 *            thread resumed,
 *          - no thread with priority below the trigger thread ran while it
 *            was ready,
 *          - every fast handler exit returned to the trigger thread with
 *            interrupts enabled.
 *          The IRQ priorities are swapped on every phase, so each handler
 *          is the nested one in half of the phases, whichever of the two is
 *          folded.
 * @note    The handlers are defined in this module, with the board vector
 *          names and IRQ numbers from @p portab.h, so that their common
 *          body is expanded in both before the compiler compares them.
 *          Folding is decided before cross-module inlining, a body in a
 *          separate module would leave two small call wrappers, which the
 *          compiler does not fold at @p -O2.
 * @note    The test is meaningful only when the handlers are folded, which
 *          happens in optimized builds (@p -O2, @p -Os) while their bodies
 *          stay identical. With @p CORTEX_ALTERNATE_SWITCH the assertion in
 *          @p PORT_CHECK_IRQ_PRIORITY() embeds @p __func__, so assertions
 *          must stay disabled. Check the image: one handler (on the standard
 *          port one @p __port_irq_body_ function) is an 8-byte thunk calling
 *          the other, see @p arm-none-eabi-nm @p -S.
 *
 * @addtogroup ICF_STORM
 * @{
 */

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "icf_storm.h"

#include "portab.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static const icf_storm_config_t *config;

/*
 * Signaled by each handler execution, waited by the waiter thread.
 */
static semaphore_t sem;

/*
 * Pending mask written to the NVIC by the next handler execution, the
 * handler clears it before writing it.
 */
static volatile uint32_t chain;

/*
 * Current and maximum nesting depth of the two handlers.
 */
static volatile uint32_t depth;
static volatile uint32_t max_depth;

/*
 * Handler executions that found interrupts masked after the nested
 * handler returned.
 */
static volatile uint32_t masked;

/*
 * Semaphore signals consumed by the waiter thread.
 */
static volatile uint32_t served;

/*
 * Fast handler executions.
 */
static volatile uint32_t fast_served;

/*
 * Incremented by a thread with priority below the trigger thread.
 */
static volatile uint32_t canary;

/*
 * Results of the trigger thread in the current phase.
 */
static volatile uint32_t pairs;
static volatile uint32_t late;
static volatile uint32_t inversions;
static volatile uint32_t fast_errors;

/*
 * Pending masks of the IRQ pended by the trigger thread and of the IRQ
 * nested into its handler.
 */
static uint32_t outer_mask;
static uint32_t inner_mask;

static unsigned pass_count;
static unsigned fail_count;

static THD_WORKING_AREA(waiter_wa, ICF_STORM_CFG_STACK_SIZE);
static THD_WORKING_AREA(trigger_wa, ICF_STORM_CFG_STACK_SIZE);
static THD_WORKING_AREA(canary_wa, ICF_STORM_CFG_STACK_SIZE);

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*
 * Common body, the two OS-aware handlers have the same code. Forced inline
 * so that both handler bodies are complete when the compiler compares them,
 * whatever the debug options and the inlining budget.
 */
CC_FORCE_INLINE
static inline void serve(void) {
  uint32_t mask;

  depth++;
  if (depth > max_depth) {
    max_depth = depth;
  }

  /* The first execution pends the other IRQ, which has the higher priority
     and preempts this handler before the next instruction.*/
  mask = chain;
  chain = 0U;
  if (mask != 0U) {
    NVIC->ISPR[0U] = mask;
    __DSB();
    __ISB();

    /* The nested handler returned, its exit must not leave interrupts
       masked.*/
    if (__get_PRIMASK() != 0U) {
      masked++;
    }
  }

  chSysLockFromISR();
  chSemSignalI(&sem);
  chSysUnlockFromISR();

  depth--;
}

/*
 * Woken by the handlers, its priority is above the trigger thread.
 */
static THD_FUNCTION(waiter_thread, arg) {

  (void)arg;
  chRegSetThreadName("waiter");

  while (true) {
    (void)chSemWait(&sem);
    served++;
  }
}

/*
 * Runs only while all the other test threads are blocked.
 */
static THD_FUNCTION(canary_thread, arg) {

  (void)arg;
  chRegSetThreadName("canary");

  while (true) {
    canary++;
  }
}

/*
 * Triggers nested pairs back to back until terminated.
 */
static THD_FUNCTION(trigger_thread, arg) {

  (void)arg;
  chRegSetThreadName("trigger");

  while (!chThdShouldTerminateX()) {
    uint32_t s, c, f;

    s = served;
    c = canary;

    /* Both handlers run before the next instruction, the outermost exit
       switches to the waiter, which consumes both signals before this
       thread resumes.*/
    chain = inner_mask;
    NVIC->ISPR[0U] = outer_mask;
    __DSB();
    __ISB();

    if (served != s + 2U) {
      late++;
    }
    if (canary != c) {
      inversions++;
    }

    /* The fast handler runs before the next instruction, its exit returns
       here with interrupts enabled.*/
    f = fast_served;
    NVIC->ISPR[0U] = 1U << PORTAB_IRQ_FAST_NUMBER;
    __DSB();
    __ISB();

    if ((fast_served != f + 1U) || (__get_PRIMASK() != 0U)) {
      fast_errors++;
    }
    pairs++;
  }
}

static void report(const char *name, unsigned phase, bool ok) {

  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
  chprintf(config->out, "  [%s] phase %u: %s\r\n",
           ok ? "PASS" : "FAIL", phase, name);
}

static void execute_phase(unsigned phase) {
  uint32_t outer, inner;
  thread_t *tp;
  unsigned i;

  /* The IRQ pended by the trigger thread has the lower priority, the roles
     are swapped on each phase.*/
  if ((phase & 1U) != 0U) {
    outer = PORTAB_IRQ_A_NUMBER;
    inner = PORTAB_IRQ_B_NUMBER;
  }
  else {
    outer = PORTAB_IRQ_B_NUMBER;
    inner = PORTAB_IRQ_A_NUMBER;
  }
  nvicEnableVector(outer, PORTAB_IRQ_LOW_PRIORITY);
  nvicEnableVector(inner, PORTAB_IRQ_HIGH_PRIORITY);
  outer_mask = 1U << outer;
  inner_mask = 1U << inner;

  max_depth   = 0U;
  masked      = 0U;
  pairs       = 0U;
  late        = 0U;
  inversions  = 0U;
  fast_errors = 0U;

  tp = chThdCreateStatic(trigger_wa, sizeof trigger_wa, NORMALPRIO - 2,
                         trigger_thread, NULL);

  /* Waking up every millisecond, the system tick interrupt lands at any
     point of the handlers and of the context switches, also nested into
     the outer handler.*/
  for (i = 0U; i < (unsigned)ICF_STORM_CFG_PHASE_TIME; i++) {
    chThdSleepMilliseconds(1);
  }
  chThdTerminate(tp);
  (void)chThdWait(tp);
  palToggleLine(config->line);

  chprintf(config->out, "Phase %u, IRQ %u nested into IRQ %u: %u pairs\r\n",
           phase, (unsigned)inner, (unsigned)outer, (unsigned)pairs);
  report("nested handler executed", phase, max_depth == 2U);
  report("interrupts enabled after nested exits", phase, masked == 0U);
  report("outermost exit rescheduled", phase, late == 0U);
  report("no lower priority thread ran", phase, inversions == 0U);
  report("fast handler exits returned to the thread", phase,
         (pairs > 0U) && (fast_errors == 0U));
}

/*===========================================================================*/
/* Module interrupt handlers.                                                */
/*===========================================================================*/

/*
 * First OS-aware handler.
 */
CH_IRQ_HANDLER(PORTAB_IRQ_A_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve();

  CH_IRQ_EPILOGUE();
}

/*
 * Second OS-aware handler, identical to the first one.
 */
CH_IRQ_HANDLER(PORTAB_IRQ_B_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve();

  CH_IRQ_EPILOGUE();
}

/*
 * Fast handler.
 */
CH_FAST_IRQ_HANDLER(PORTAB_IRQ_FAST_HANDLER) {

  fast_served++;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   ICF storm execution.
 *
 * @param[in] cfg       pointer to the test configuration structure
 *
 * @api
 */
void icf_storm_execute(const icf_storm_config_t *cfg) {
  unsigned i;

  config = cfg;

  chSemObjectInit(&sem, (cnt_t)0);
  (void)chThdCreateStatic(waiter_wa, sizeof waiter_wa, NORMALPRIO - 1,
                          waiter_thread, NULL);
  (void)chThdCreateStatic(canary_wa, sizeof canary_wa, NORMALPRIO - 3,
                          canary_thread, NULL);

  /* Printing environment information.*/
  chprintf(cfg->out, "\r\n*** ChibiOS/RT ICF-STORM nested identical handlers"
                     " test\r\n***\r\n");
  chprintf(cfg->out, "*** Kernel:       %s\r\n", CH_KERNEL_VERSION);
  chprintf(cfg->out, "*** Compiled:     %s\r\n", __DATE__ " - " __TIME__);
#ifdef PORT_COMPILER_NAME
  chprintf(cfg->out, "*** Compiler:     %s\r\n", PORT_COMPILER_NAME);
#endif
  chprintf(cfg->out, "*** Architecture: %s\r\n", PORT_ARCHITECTURE_NAME);
#ifdef PORT_CORE_VARIANT_NAME
  chprintf(cfg->out, "*** Core Variant: %s\r\n", PORT_CORE_VARIANT_NAME);
#endif
  chprintf(cfg->out, "*** System Clock: %d\r\n", cfg->sysclk);
#ifdef PORT_INFO
  chprintf(cfg->out, "*** Port Info:    %s\r\n", PORT_INFO);
#endif
#ifdef PLATFORM_NAME
  chprintf(cfg->out, "*** Platform:     %s\r\n", PLATFORM_NAME);
#endif
#ifdef BOARD_NAME
  chprintf(cfg->out, "*** Test Board:   %s\r\n", BOARD_NAME);
#endif
  chprintf(cfg->out, "***\r\n");
  chprintf(cfg->out, "*** Phases:       %d\r\n", ICF_STORM_CFG_PHASES);
  chprintf(cfg->out, "*** Phase time:   %d mS\r\n\r\n",
           ICF_STORM_CFG_PHASE_TIME);

  nvicEnableVector(PORTAB_IRQ_FAST_NUMBER, PORTAB_IRQ_LOW_PRIORITY);

  for (i = 1U; i <= (unsigned)ICF_STORM_CFG_PHASES; i++) {
    execute_phase(i);
  }
  nvicDisableVector(PORTAB_IRQ_A_NUMBER);
  nvicDisableVector(PORTAB_IRQ_B_NUMBER);
  nvicDisableVector(PORTAB_IRQ_FAST_NUMBER);

  chprintf(cfg->out, "\r\n  Results: %u pass, %u fail\r\n",
           pass_count, fail_count);
  if (fail_count == 0U) {
    chprintf(cfg->out, "  ALL TESTS PASSED\r\n");
  }
  else {
    chprintf(cfg->out, "  *** FAILURES DETECTED ***\r\n");
  }
}

/** @} */
