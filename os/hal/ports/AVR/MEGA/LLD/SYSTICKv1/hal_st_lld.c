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
 * @file    TIMv1/hal_st_lld.c
 * @brief   AVR/MEGA ST subsystem low level driver source.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

#if (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

/*==========================================================================*/
/* Driver local definitions.                                                */
/*==========================================================================*/

/**
 * @brief  Timer maximum value
 */
#define AVR_TIMER_COUNTER_MAX 255

/*==========================================================================*/
/* Driver exported variables.                                               */
/*==========================================================================*/

/*==========================================================================*/
/* Driver local types.                                                      */
/*==========================================================================*/

/*==========================================================================*/
/* Driver local variables and types.                                        */
/*==========================================================================*/

#if (OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC) || defined(__DOXYGEN__)

/* Periodic mode uses Timer 0, or Timer 2 where Timer 0 has no compare unit
   (ATmega8). */
#if defined(OCR0A) || defined(OCR0)
  #define AVR_TIMER_USE_TIM2 FALSE
#elif defined(OCR2)
  #define AVR_TIMER_USE_TIM2 TRUE
#else
  #error "Cannot find an 8 bit timer with a compare unit"
#endif

/* Asynchronous timers (Timer 0 on ATmega64/128, Timer 2 on ATmega8) have
   extra CLK/32 and CLK/128 steps, which move the clock select codes from
   CLK/64 upward. */
#if (AVR_TIMER_USE_TIM2 && defined(AS2)) || \
    (!AVR_TIMER_USE_TIM2 && defined(AS0))
  #define AVR_TIMER_ASYNC TRUE
#else
  #define AVR_TIMER_ASYNC FALSE
#endif

/* Work out what the timer interrupt is called on this MCU. */
#if AVR_TIMER_USE_TIM2
  #define AVR_TIMER_VECT TIMER2_COMP_vect
#elif defined(TIMER0_COMPA_vect)
  #define AVR_TIMER_VECT TIMER0_COMPA_vect
#elif defined(TIMER_COMPA_vect)
  #define AVR_TIMER_VECT TIMER_COMPA_vect
#elif defined(TIMER0_COMP_vect)
  #define AVR_TIMER_VECT TIMER0_COMP_vect
#else
  #error "Cannot find interrupt vector name for timer"
#endif

/* Find the most suitable prescaler setting for the desired OSAL_ST_FREQUENCY.
   The clock select field is bits 2:0 of the control register on every timer
   used here. */
#if ((F_CPU / OSAL_ST_FREQUENCY) <= AVR_TIMER_COUNTER_MAX)

  #define AVR_TIMER_PRESCALER 1
  #define AVR_TIMER_PRESCALER_BITS 1                        /* CLK      */

#elif ((F_CPU / OSAL_ST_FREQUENCY / 8) <= AVR_TIMER_COUNTER_MAX)

  #define AVR_TIMER_PRESCALER 8
  #define AVR_TIMER_PRESCALER_BITS 2                        /* CLK/8    */

#elif ((F_CPU / OSAL_ST_FREQUENCY / 64) <= AVR_TIMER_COUNTER_MAX)

  #define AVR_TIMER_PRESCALER 64

  #if AVR_TIMER_ASYNC
    #define AVR_TIMER_PRESCALER_BITS 4                      /* CLK/64   */
  #else
    #define AVR_TIMER_PRESCALER_BITS 3                      /* CLK/64   */
  #endif

#elif ((F_CPU / OSAL_ST_FREQUENCY / 256) <= AVR_TIMER_COUNTER_MAX)

  #define AVR_TIMER_PRESCALER 256

  #if AVR_TIMER_ASYNC
    #define AVR_TIMER_PRESCALER_BITS 6                      /* CLK/256  */
  #else
    #define AVR_TIMER_PRESCALER_BITS 4                      /* CLK/256  */
  #endif

#elif ((F_CPU / OSAL_ST_FREQUENCY / 1024) <= AVR_TIMER_COUNTER_MAX)

  #define AVR_TIMER_PRESCALER 1024

  #if AVR_TIMER_ASYNC
    #define AVR_TIMER_PRESCALER_BITS 7                      /* CLK/1024 */
  #else
    #define AVR_TIMER_PRESCALER_BITS 5                      /* CLK/1024 */
  #endif

#else
  #error "Frequency too low for timer, please set OSAL_ST_FREQUENCY to a higher value"
#endif

#define AVR_TIMER_COUNTER (F_CPU / OSAL_ST_FREQUENCY / AVR_TIMER_PRESCALER)

/* Test if OSAL_ST_FREQUENCY can be matched exactly using this timer. */
#define F_CPU_ (AVR_TIMER_COUNTER * AVR_TIMER_PRESCALER * OSAL_ST_FREQUENCY)
#if (F_CPU_ != F_CPU)
  #warning "OSAL_ST_FREQUENCY cannot be generated exactly using timer"
#endif
#undef F_CPU_

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC */

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

/* FIXME: Prescaler is now fixed in 1024.
 *        Should add support for calculating best value according to
 *        user requested configuration.
 */
#define PRESCALER (_BV(CS12) | _BV(CS10))

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

/*==========================================================================*/
/* Driver local functions.                                                  */
/*==========================================================================*/

/*==========================================================================*/
/* Driver interrupt handlers.                                               */
/*==========================================================================*/

#if (OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC) || defined(__DOXYGEN__)

/**
 * @brief Timer handler for periodic mode.
 */
OSAL_IRQ_HANDLER(AVR_TIMER_VECT) {

  OSAL_IRQ_PROLOGUE();

  osalSysLockFromISR();
  osalOsTimerHandlerI();
  osalSysUnlockFromISR();

  OSAL_IRQ_EPILOGUE();
}

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC */

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

/**
 * @brief Timer handler for free running mode.
 */
OSAL_IRQ_HANDLER(TIMER1_COMPA_vect) {

  OSAL_IRQ_PROLOGUE();

  /* TODO: reset status if required. */

  osalSysLockFromISR();
  osalOsTimerHandlerI();
  osalSysUnlockFromISR();

  OSAL_IRQ_EPILOGUE();
}

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

/*==========================================================================*/
/* Driver exported functions.                                               */
/*==========================================================================*/

/**
 * @brief   Low level ST driver initialization.
 *
 * @notapi
 */
void st_lld_init(void) {

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

  /*
   * Free running mode uses Timer 1 (16 bit).
   */

  /* CTC mode, no clock source. */
  TCCR1A     = 0;
  TCCR1B     = _BV(WGM12);

  /* Start disabled. */
#if defined(TCCR1C)
  TCCR1C     = 0;
#endif
  OCR1A      = 0;
  TCNT1      = 0;
  TIFR_REG   = _BV(OCF1A);                              /* Reset pending.   */
  TIMSK_REG &= (uint8_t)~_BV(OCIE1A);
  TCCR1B     = PRESCALER;

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#if (OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC) || defined(__DOXYGEN__)

  /*
   * Periodic mode uses Timer 0 (8 bit), or Timer 2 (8 bit) on the ATmega8.
   */
#if AVR_TIMER_USE_TIM2 /* Timer 0 has no output comparator.                 */
  TCCR2   = (1 << WGM21) | (0 << WGM20) |                /* CTC mode.       */
            (0 << COM21) | (0 << COM20) |                /* OC2 disabled.   */
            AVR_TIMER_PRESCALER_BITS;
  OCR2    = AVR_TIMER_COUNTER - 1;
  TCNT2   = 0;                                           /* Reset counter.  */
  TIFR    = (1 << OCF2);                                 /* Reset pending.  */
  TIMSK  |= (1 << OCIE2);                                /* IRQ on compare. */

#elif defined(TCCR0B) /* Timer has multiple output comparators.             */
  TCCR0A  = (1 << WGM01) | (0 << WGM00) |                /* CTC mode.       */
            (0 << COM0A1) | (0 << COM0A0) |              /* OC0A disabled.  */
            (0 << COM0B1) | (0 << COM0B0);               /* OC0B disabled.  */
  TCCR0B  = (0 << WGM02) | AVR_TIMER_PRESCALER_BITS;     /* CTC mode.       */
  OCR0A   = AVR_TIMER_COUNTER - 1;
  TCNT0   = 0;                                           /* Reset counter.  */
  TIFR0   = (1 << OCF0A);                                /* Reset pending.  */
  TIMSK0  = (1 << OCIE0A);                               /* IRQ on compare. */

#elif defined(TCCR0A) /* AT90CAN doesn't have TCCR0B and slightly different */
                      /* TCCR0A.                                            */
  TCCR0A  = (1 << WGM01) | (0 << WGM00) |                /* CTC mode.       */
            (0 << COM0A1) | (0 << COM0A0);               /* OC0A disabled.  */
  OCR0A   = AVR_TIMER_COUNTER - 1;
  TCNT0   = 0;                                           /* Reset counter.  */
  TIFR0   = (1 << OCF0A);                                /* Reset pending.  */
  TIMSK0  = (1 << OCIE0A);                               /* IRQ on compare. */

#elif defined(TCCR0) /* Timer has single output comparator                  */
  TCCR0   = (1 << WGM01) | (0 << WGM00) |                /* CTC mode.       */
            (0 << COM01) | (0 << COM00) |                /* OC0A disabled.  */
            AVR_TIMER_PRESCALER_BITS;
  OCR0    = AVR_TIMER_COUNTER - 1;
  TCNT0   = 0;                                           /* Reset counter.  */
  TIFR    = (1 << OCF0);                                 /* Reset pending.  */
  TIMSK  |= (1 << OCIE0);                                /* IRQ on compare. */
#else
  #error "Neither TCCR0A nor TCCR0 registers are defined"
#endif

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC */
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
