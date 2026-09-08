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
 * @file    rt/templates/chconf.h
 * @brief   Configuration file template.
 * @details A copy of this file must be placed in each project directory, it
 *          contains the application-specific kernel settings.
 *
 * @addtogroup config
 * @details Kernel-related settings and hooks.
 * @{
 */

#ifndef CHCONF_H
#define CHCONF_H

#define _CHIBIOS_RT_CONF_
#define _CHIBIOS_RT_CONF_VER_8_0_

/*===========================================================================*/
/**
 * @name System settings
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Handling of instances.
 * @note    If enabled then threads assigned to various instances can
 *          interact with each other using the same synchronization objects.
 *          If disabled then each OS instance is a separate world, no
 *          direct interactions are handled by the OS.
 */
#if !defined(CH_CFG_SMP_MODE)
#define CH_CFG_SMP_MODE                     FALSE
#endif

/**
 * @brief   Runtime Faults Collection Unit.
 * @details If enabled then detected runtime faults are collected and made
 *          available to the application.
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_RFCU)
#define CH_CFG_USE_RFCU                     TRUE
#endif

/**
 * @brief   Kernel hardening level.
 * @details The automatic hardening levels are cumulative:
 *          - 0: No automatic hardening checks or object clearing.
 *          - 1: Object integrity checks and clearing on disposal.
 *          - 2: Also checks backward links during insertion into ready lists
 *               and priority-ordered wait queues.
 *          - 3: Also validates forward-link pointers before dereferencing
 *               them during these insertions.
 *          .
 * @note    @p CH_DBG_ENABLE_ASSERTS also enables level-0/1/2 checks,
 *          including disposal list/queue checks, independently of this
 *          setting. It does not enable level-3 checks or object clearing
 *          at level 0.
 * @note    Explicit @p chSftIntegrityCheckI() scans are available at all
 *          levels. Their backward-link checks always execute; pointer
 *          validation requires level 2 or higher, or enabled debug assertions.
 */
#if !defined(CH_CFG_HARDENING_LEVEL)
#define CH_CFG_HARDENING_LEVEL              0
#endif

/** @} */

/*===========================================================================*/
/**
 * @name System timers settings
 * @{
 */
/*===========================================================================*/

/**
 * @brief   System time counter resolution.
 * @note    Allowed values are 16, 32 or 64 bits.
 * @note    In tick-less mode this value must match the physical system tick
 *          timer counter width.
 */
#if !defined(CH_CFG_ST_RESOLUTION)
#define CH_CFG_ST_RESOLUTION                32
#endif

/**
 * @brief   System tick frequency.
 * @details Frequency of the system timer that drives the system ticks. This
 *          setting also defines the system tick time unit.
 * @note    This must be a frequency that is obtainable from the system tick
 *          timer frequency.
 */
#if !defined(CH_CFG_ST_FREQUENCY)
#define CH_CFG_ST_FREQUENCY                 10000
#endif

/**
 * @brief   Time intervals data size.
 * @note    Allowed values are 16, 32 or 64 bits.
 */
#if !defined(CH_CFG_INTERVALS_SIZE)
#define CH_CFG_INTERVALS_SIZE               32
#endif

/**
 * @brief   Time types data size.
 * @note    Allowed values are 16 or 32 bits.
 */
#if !defined(CH_CFG_TIME_TYPES_SIZE)
#define CH_CFG_TIME_TYPES_SIZE              32
#endif

/**
 * @brief   Time delta constant for the tick-less mode.
 * @note    If this value is zero then the system uses the classic
 *          periodic tick. This value represents the minimum number
 *          of ticks that is safe to specify in a timeout directive.
 *          The value one is not valid, timeouts are rounded up to
 *          this value.
 */
#if !defined(CH_CFG_ST_TIMEDELTA)
#define CH_CFG_ST_TIMEDELTA                 2
#endif

/** @} */

/*===========================================================================*/
/**
 * @name Kernel parameters and options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Round robin interval.
 * @details This constant is the number of system ticks allowed for the
 *          threads before preemption occurs. Setting this value to zero
 *          disables the preemption for threads with equal priority and the
 *          round robin becomes cooperative. Note that higher priority
 *          threads can still preempt, the kernel is always preemptive.
 * @note    Disabling the round robin preemption makes the kernel more compact
 *          and generally faster.
 * @note    The round robin preemption is not supported in tickless mode and
 *          must be set to zero in that case.
 */
#if !defined(CH_CFG_TIME_QUANTUM)
#define CH_CFG_TIME_QUANTUM                 0
#endif

/**
 * @brief   Idle thread automatic spawn suppression.
 * @details When this option is activated the function @p chSysInit()
 *          does not spawn the idle thread. The application @p main()
 *          function becomes the idle thread and must implement an
 *          infinite loop.
 */
#if !defined(CH_CFG_NO_IDLE_THREAD)
#define CH_CFG_NO_IDLE_THREAD               FALSE
#endif

/** @} */

/*===========================================================================*/
/**
 * @name Performance options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   OS optimization.
 * @details If enabled then time efficient rather than space efficient code
 *          is used when two possible implementations exist.
 *
 * @note    This is not related to the compiler optimization options.
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_OPTIMIZE_SPEED)
#define CH_CFG_OPTIMIZE_SPEED               TRUE
#endif

/** @} */

/*===========================================================================*/
/**
 * @name Subsystem options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Time Measurement APIs.
 * @details If enabled then the time measurement APIs are included in
 *          the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_TM)
#define CH_CFG_USE_TM                       TRUE
#endif

/**
 * @brief   Time Stamps APIs.
 * @details If enabled then the time stamps APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_TIMESTAMP)
#define CH_CFG_USE_TIMESTAMP                FALSE
#endif

/**
 * @brief   Threads registry APIs.
 * @details If enabled then the registry APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_REGISTRY)
#define CH_CFG_USE_REGISTRY                 TRUE
#endif

/**
 * @brief   Threads synchronization APIs.
 * @details If enabled then the @p chThdWait() function is included in
 *          the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_WAITEXIT)
#define CH_CFG_USE_WAITEXIT                 TRUE
#endif

/**
 * @brief   Semaphores APIs.
 * @details If enabled then the Semaphores APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_SEMAPHORES)
#define CH_CFG_USE_SEMAPHORES               TRUE
#endif

/**
 * @brief   Semaphores queuing mode.
 * @details If enabled then the threads are enqueued on semaphores by
 *          priority rather than in FIFO order.
 *
 * @note    The default is @p FALSE. Enable this if you have special
 *          requirements.
 * @note    Requires @p CH_CFG_USE_SEMAPHORES.
 */
#if !defined(CH_CFG_USE_SEMAPHORES_PRIORITY)
#define CH_CFG_USE_SEMAPHORES_PRIORITY      FALSE
#endif

/**
 * @brief   Mutexes APIs.
 * @details If enabled then the mutexes APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_MUTEXES)
#define CH_CFG_USE_MUTEXES                  TRUE
#endif

/**
 * @brief   Enables recursive behavior on mutexes.
 * @note    Recursive mutexes are heavier and have an increased
 *          memory footprint.
 *
 * @note    The default is @p FALSE.
 * @note    Requires @p CH_CFG_USE_MUTEXES.
 */
#if !defined(CH_CFG_USE_MUTEXES_RECURSIVE)
#define CH_CFG_USE_MUTEXES_RECURSIVE        FALSE
#endif

/**
 * @brief   Conditional Variables APIs.
 * @details If enabled then the conditional variables APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_MUTEXES.
 */
#if !defined(CH_CFG_USE_CONDVARS)
#define CH_CFG_USE_CONDVARS                 TRUE
#endif

/**
 * @brief   Conditional Variables APIs with timeout.
 * @details If enabled then the conditional variables APIs with timeout
 *          specification are included in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_CONDVARS.
 */
#if !defined(CH_CFG_USE_CONDVARS_TIMEOUT)
#define CH_CFG_USE_CONDVARS_TIMEOUT         TRUE
#endif

/**
 * @brief   Events Flags APIs.
 * @details If enabled then the event flags APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_EVENTS)
#define CH_CFG_USE_EVENTS                   TRUE
#endif

/**
 * @brief   Events Flags APIs with timeout.
 * @details If enabled then the events APIs with timeout specification
 *          are included in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_EVENTS.
 */
#if !defined(CH_CFG_USE_EVENTS_TIMEOUT)
#define CH_CFG_USE_EVENTS_TIMEOUT           TRUE
#endif

/**
 * @brief   Synchronous Messages APIs.
 * @details If enabled then the synchronous messages APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_MESSAGES)
#define CH_CFG_USE_MESSAGES                 TRUE
#endif

/**
 * @brief   Synchronous Messages queuing mode.
 * @details If enabled then messages are served by priority rather than in
 *          FIFO order.
 *
 * @note    The default is @p FALSE. Enable this if you have special
 *          requirements.
 * @note    Requires @p CH_CFG_USE_MESSAGES.
 */
#if !defined(CH_CFG_USE_MESSAGES_PRIORITY)
#define CH_CFG_USE_MESSAGES_PRIORITY        FALSE
#endif

/**
 * @brief   Dynamic Threads APIs.
 * @details If enabled then the dynamic threads creation APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_WAITEXIT.
 * @note    Requires @p CH_CFG_USE_HEAP and/or @p CH_CFG_USE_MEMPOOLS.
 */
#if !defined(CH_CFG_USE_DYNAMIC)
#define CH_CFG_USE_DYNAMIC                  TRUE
#endif

/** @} */

/*===========================================================================*/
/**
 * @name OSLIB options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Mailboxes APIs.
 * @details If enabled then the asynchronous messages (mailboxes) APIs are
 *          included in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_SEMAPHORES.
 */
#if !defined(CH_CFG_USE_MAILBOXES)
#define CH_CFG_USE_MAILBOXES                TRUE
#endif

/**
 * @brief   Memory checks APIs.
 * @details If enabled then the memory checks APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_MEMCHECKS)
#define CH_CFG_USE_MEMCHECKS                TRUE
#endif

/**
 * @brief   Core Memory Manager APIs.
 * @details If enabled then the core memory manager APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_MEMCORE)
#define CH_CFG_USE_MEMCORE                  TRUE
#endif

/**
 * @brief   Managed RAM size.
 * @details Size of the RAM area to be managed by the OS. If set to zero
 *          then the whole available RAM is used. The core memory is made
 *          available to the heap allocator and/or can be used directly through
 *          the simplified core memory allocator.
 *
 * @note    In order to let the OS manage the whole RAM the linker script must
 *          provide the @p __heap_base__ and @p __heap_end__ symbols.
 * @note    Requires @p CH_CFG_USE_MEMCORE.
 */
#if !defined(CH_CFG_MEMCORE_SIZE)
#define CH_CFG_MEMCORE_SIZE                 0
#endif

/**
 * @brief   Heap Allocator APIs.
 * @details If enabled then the memory heap allocator APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_MEMCORE and either @p CH_CFG_USE_MUTEXES or
 *          @p CH_CFG_USE_SEMAPHORES.
 * @note    Mutexes are recommended.
 */
#if !defined(CH_CFG_USE_HEAP)
#define CH_CFG_USE_HEAP                     TRUE
#endif

/**
 * @brief   Memory Pools Allocator APIs.
 * @details If enabled then the memory pools allocator APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_MEMPOOLS)
#define CH_CFG_USE_MEMPOOLS                 TRUE
#endif

/**
 * @brief   Objects FIFOs APIs.
 * @details If enabled then the objects FIFOs APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_OBJ_FIFOS)
#define CH_CFG_USE_OBJ_FIFOS                TRUE
#endif

/**
 * @brief   Pipes APIs.
 * @details If enabled then the pipes APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_PIPES)
#define CH_CFG_USE_PIPES                    TRUE
#endif

/**
 * @brief   Objects Caches APIs.
 * @details If enabled then the objects caches APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_OBJ_CACHES)
#define CH_CFG_USE_OBJ_CACHES               TRUE
#endif

/**
 * @brief   Delegate threads APIs.
 * @details If enabled then the delegate threads APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_DELEGATES)
#define CH_CFG_USE_DELEGATES                TRUE
#endif

/**
 * @brief   Jobs Queues APIs.
 * @details If enabled then the jobs queues APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#if !defined(CH_CFG_USE_JOBS)
#define CH_CFG_USE_JOBS                     TRUE
#endif

/** @} */

/*===========================================================================*/
/**
 * @name Objects factory options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Objects Factory APIs.
 * @details If enabled then the objects factory APIs are included in the
 *          kernel.
 *
 * @note    The default is @p FALSE.
 */
#if !defined(CH_CFG_USE_FACTORY)
#define CH_CFG_USE_FACTORY                  TRUE
#endif

/**
 * @brief   Maximum length for object names.
 * @details If the specified length is zero then the name is stored by
 *          pointer but this could have unintended side effects.
 */
#if !defined(CH_CFG_FACTORY_MAX_NAMES_LENGTH)
#define CH_CFG_FACTORY_MAX_NAMES_LENGTH     8
#endif

/**
 * @brief   Enables the registry of generic objects.
 */
#if !defined(CH_CFG_FACTORY_OBJECTS_REGISTRY)
#define CH_CFG_FACTORY_OBJECTS_REGISTRY     TRUE
#endif

/**
 * @brief   Enables factory for generic buffers.
 */
#if !defined(CH_CFG_FACTORY_GENERIC_BUFFERS)
#define CH_CFG_FACTORY_GENERIC_BUFFERS      TRUE
#endif

/**
 * @brief   Enables factory for semaphores.
 */
#if !defined(CH_CFG_FACTORY_SEMAPHORES)
#define CH_CFG_FACTORY_SEMAPHORES           TRUE
#endif

/**
 * @brief   Enables factory for mailboxes.
 */
#if !defined(CH_CFG_FACTORY_MAILBOXES)
#define CH_CFG_FACTORY_MAILBOXES            TRUE
#endif

/**
 * @brief   Enables factory for objects FIFOs.
 */
#if !defined(CH_CFG_FACTORY_OBJ_FIFOS)
#define CH_CFG_FACTORY_OBJ_FIFOS            TRUE
#endif

/**
 * @brief   Enables factory for Pipes.
 */
#if !defined(CH_CFG_FACTORY_PIPES) || defined(__DOXYGEN__)
#define CH_CFG_FACTORY_PIPES                TRUE
#endif

/** @} */

/*===========================================================================*/
/**
 * @name Debug options
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Debug option, kernel statistics.
 *
 * @note    The default is @p FALSE.
 */
#if !defined(CH_DBG_STATISTICS)
#define CH_DBG_STATISTICS                   FALSE
#endif

/**
 * @brief   Debug option, system state check.
 * @details If enabled the correct call protocol for system APIs is checked
 *          at runtime.
 *
 * @note    The default is @p FALSE.
 */
#if !defined(CH_DBG_SYSTEM_STATE_CHECK)
#define CH_DBG_SYSTEM_STATE_CHECK           TRUE
#endif

/**
 * @brief   Debug option, parameters checks.
 * @details If enabled then the checks on the API functions input
 *          parameters are activated.
 *
 * @note    The default is @p FALSE.
 */
#if !defined(CH_DBG_ENABLE_CHECKS)
#define CH_DBG_ENABLE_CHECKS                TRUE
#endif

/**
 * @brief   Debug option, consistency checks.
 * @details If enabled then all the assertions in the kernel code are
 *          activated. This includes consistency checks inside the kernel,
 *          runtime anomalies and port-defined checks.
 *
 * @note    The default is @p FALSE.
 */
#if !defined(CH_DBG_ENABLE_ASSERTS)
#define CH_DBG_ENABLE_ASSERTS               TRUE
#endif

/**
 * @brief   Debug option, trace buffer.
 * @details If enabled then the trace buffer is activated.
 *
 * @note    The default is @p CH_DBG_TRACE_MASK_DISABLED.
 */
#if !defined(CH_DBG_TRACE_MASK)
#define CH_DBG_TRACE_MASK                   CH_DBG_TRACE_MASK_DISABLED
#endif

/**
 * @brief   Trace buffer entries.
 * @note    The trace buffer is only allocated if @p CH_DBG_TRACE_MASK is
 *          different from @p CH_DBG_TRACE_MASK_DISABLED.
 * @note    The allowed range is 1..65535 when the trace buffer is enabled.
 */
#if !defined(CH_DBG_TRACE_BUFFER_SIZE)
#define CH_DBG_TRACE_BUFFER_SIZE            128
#endif

/**
 * @brief   Debug option, stack checks.
 * @details If enabled then a runtime stack check is performed.
 *
 * @note    The default is @p FALSE.
 * @note    The stack check is performed in a architecture/port dependent way.
 *          It may not be implemented or some ports.
 * @note    The default failure mode is to halt the system with the global
 *          @p panic_msg variable set to @p NULL.
 */
#if !defined(CH_DBG_ENABLE_STACK_CHECK)
#define CH_DBG_ENABLE_STACK_CHECK           FALSE
#endif

/**
 * @brief   Debug option, stacks initialization.
 * @details If enabled then the threads working area is filled with a byte
 *          value when a thread is created. This can be useful for the
 *          runtime measurement of the used stack.
 *
 * @note    The default is @p FALSE.
 */
#if !defined(CH_DBG_FILL_THREADS)
#define CH_DBG_FILL_THREADS                 FALSE
#endif

/**
 * @brief   Debug option, threads profiling.
 * @details If enabled then a field is added to the @p thread_t structure that
 *          counts the system ticks that occurred while executing the thread.
 *
 * @note    The default is @p FALSE.
 * @note    This debug option is not currently compatible with the
 *          tickless mode.
 */
#if !defined(CH_DBG_THREADS_PROFILING)
#define CH_DBG_THREADS_PROFILING            FALSE
#endif

/** @} */

/*===========================================================================*/
/**
 * @name Kernel hooks
 * @{
 */
/*===========================================================================*/

/**
 * @brief   System structure extension.
 * @details User fields added to the end of the @p ch_system_t structure.
 */
#define CH_CFG_SYSTEM_EXTRA_FIELDS                                          \
  /* Add system custom fields here.*/

/**
 * @brief   System extra fields initialization hook.
 * @details Initializes the fields added to @p ch_system_t by
 *          @p CH_CFG_SYSTEM_EXTRA_FIELDS. Invoked by @p chSysInit() before
 *          the OS library and the default OS instance are initialized.
 * @note    Runs with interrupts disabled and no current OS instance. In SMP
 *          mode the shared kernel lock has not yet been acquired.
 * @note    Must preserve the interrupt and lock state and must not use
 *          services requiring an initialized OS instance or OS library.
 */
#define CH_CFG_SYSTEM_INIT_HOOK() do {                                      \
  /* Add system initialization code here.*/                                 \
} while (false)

/**
 * @brief   OS instance structure extension.
 * @details User fields added to the end of the @p os_instance_t structure.
 */
#define CH_CFG_OS_INSTANCE_EXTRA_FIELDS                                     \
  /* Add OS instance custom fields here.*/

/**
 * @brief   OS instance extra fields initialization hook.
 * @details Initializes the fields added to @p os_instance_t by
 *          @p CH_CFG_OS_INSTANCE_EXTRA_FIELDS. Invoked by
 *          @p chInstanceObjectInit() after the local kernel objects and the
 *          current thread are initialized, before creating a separate idle
 *          thread, if configured.
 * @note    Runs on the instance's core in the initial I-Lock state. Must
 *          preserve the interrupt and lock state and must not make threads
 *          ready or invoke the scheduler while initialization is incomplete.
 *
 * @param[in] oip       pointer to the @p os_instance_t structure
 */
#define CH_CFG_OS_INSTANCE_INIT_HOOK(oip) do {                              \
  /* Add OS instance initialization code here.*/                            \
} while (false)

/**
 * @brief   Threads descriptor structure extension.
 * @details User fields added to the end of the @p thread_t structure.
 */
#define CH_CFG_THREAD_EXTRA_FIELDS                                          \
  /* Add threads custom fields here.*/

/**
 * @brief   Thread extra fields initialization hook.
 * @details Initializes the fields added to @p thread_t by
 *          @p CH_CFG_THREAD_EXTRA_FIELDS. Invoked by @p chThdObjectInit(),
 *          including during system initialization and thread creation.
 * @note    Inherits the caller's context and lock state. During system
 *          initialization the current thread may not yet be established.
 * @note    Must preserve the interrupt and lock state and must not make
 *          threads ready or invoke the scheduler.
 *
 * @param[in] tp        pointer to the @p thread_t structure
 */
#define CH_CFG_THREAD_INIT_HOOK(tp) do {                                    \
  /* Add threads initialization code here.*/                                \
} while (false)

/**
 * @brief   Threads finalization hook.
 * @details User finalization code added to the @p chThdExit() API.
 *
 * @param[in] tp        pointer to the @p thread_t structure
 */
#define CH_CFG_THREAD_EXIT_HOOK(tp) do {                                    \
  /* Add threads finalization code here.*/                                  \
} while (false)

/**
 * @brief   Context switch hook.
 * @details This hook is invoked just before switching between threads.
 *          It is intended for bounded bookkeeping or instrumentation.
 * @note    Runs with the kernel locked, after the scheduler has selected
 *          the next thread. Must preserve the interrupt and lock state and
 *          must not change runnable state or invoke the scheduler.
 *
 * @param[in] ntp       thread being switched in
 * @param[in] otp       thread being switched out
 */
#define CH_CFG_CONTEXT_SWITCH_HOOK(ntp, otp) do {                           \
  /* Context switch code here.*/                                            \
} while (false)

/**
 * @brief   ISR enter hook.
 * @details Invoked after the ISR trace entry and debug state setup.
 * @note    This hook runs in ISR context outside the kernel lock. I-class
 *          APIs require a balanced @p chSysLockFromISR() and
 *          @p chSysUnlockFromISR() pair. The hook must return unlocked.
 */
#define CH_CFG_IRQ_PROLOGUE_HOOK() do {                                     \
  /* IRQ prologue code here.*/                                              \
} while (false)

/**
 * @brief   ISR exit hook.
 * @details Invoked before the debug state teardown and ISR trace exit.
 * @note    This hook runs in ISR context outside the kernel lock. I-class
 *          APIs require a balanced @p chSysLockFromISR() and
 *          @p chSysUnlockFromISR() pair. The hook must return unlocked.
 */
#define CH_CFG_IRQ_EPILOGUE_HOOK() do {                                     \
  /* IRQ epilogue code here.*/                                              \
} while (false)

/**
 * @brief   Idle thread enter hook.
 * @note    This hook is invoked within a critical zone, no OS functions
 *          should be invoked from here.
 * @note    This macro can be used to activate a power saving mode.
 */
#define CH_CFG_IDLE_ENTER_HOOK() do {                                       \
  /* Idle-enter code here.*/                                                \
} while (false)

/**
 * @brief   Idle thread leave hook.
 * @note    This hook is invoked within a critical zone, no OS functions
 *          should be invoked from here.
 * @note    This macro can be used to deactivate a power saving mode.
 */
#define CH_CFG_IDLE_LEAVE_HOOK() do {                                       \
  /* Idle-leave code here.*/                                                \
} while (false)

/**
 * @brief   Idle Loop hook.
 * @details This hook is continuously invoked by the idle thread loop.
 */
#define CH_CFG_IDLE_LOOP_HOOK() do {                                        \
  /* Idle loop code here.*/                                                 \
} while (false)

/**
 * @brief   System tick event hook.
 * @details This hook is invoked in the system tick handler immediately
 *          after processing the virtual timers queue.
 */
#define CH_CFG_SYSTEM_TICK_HOOK() do {                                      \
  /* System tick event code here.*/                                         \
} while (false)

/**
 * @brief   System halt hook.
 * @details This hook is invoked in case to a system halting error before
 *          the system is halted.
 */
#define CH_CFG_SYSTEM_HALT_HOOK(reason) do {                                \
  /* System halt code here.*/                                               \
} while (false)

/**
 * @brief   Trace hook.
 * @details Invoked synchronously on the reporting core after the record fields
 *          and timestamps are written, before the next-slot pointer advances.
 *          It can run in thread or ISR context, during initialization and on
 *          halt paths, including inside scheduler operations.
 * @note    This is not a general I-class callback. IRQ tracing holds the port
 *          lock but runs before ISR debug-state setup or after its teardown.
 *          Halt tracing disables local interrupts but need not hold the shared
 *          SMP kernel lock.
 * @note    The hook must be bounded and nonblocking, preserve interrupt and
 *          lock state, and must not acquire or release kernel locks, reschedule,
 *          wake threads, or modify kernel objects or the trace buffer.
 * @note    The record pointer is borrowed for this call. Copy needed fields to
 *          application-owned storage and defer processing to a suitable context;
 *          retaining the pointer does not preserve the record against overwrite.
 * @note    Hook-owned storage must be initialized before the first possible
 *          record. Application initialization and current-thread setup need not
 *          be complete then. Shared storage must account for concurrent SMP
 *          halt reporting without the shared kernel lock.
 * @warning Direct or indirect tracing from the hook invokes it recursively
 *          and must be avoided.
 *
 * @param[in] tep       pointer to the completed record, read-only for the hook
 */
#define CH_CFG_TRACE_HOOK(tep) do {                                         \
  /* Trace code here.*/                                                     \
} while (false)

/**
 * @brief   Runtime Faults Collection Unit hook.
 * @details Invoked synchronously after storing fault flags, on the reporting
 *          core with the caller's kernel lock still held. It can run in thread
 *          or ISR context, including during kernel updates that are not
 *          general callback boundaries. In SMP the common kernel lock is held.
 *          Every collection invokes the hook, including repeated flags and
 *          a zero mask; pending fault flags coalesce repeated occurrences.
 * @note    The hook must be bounded and nonblocking, preserve interrupt and
 *          lock state, and must not reschedule, wake threads, or modify timer
 *          lists or other kernel objects. Use application-owned storage to
 *          record information and defer processing to a suitable context.
 * @note    Hook-owned storage must be initialized before the first possible
 *          collection; application initialization need not be complete then.
 * @warning Collecting faults from the hook invokes it recursively and must
 *          be avoided.
 *
 * @param[in] mask      supplied fault flags, not just newly set bits
 */
#define CH_CFG_RUNTIME_FAULTS_HOOK(mask) do {                               \
  /* Faults handling code here.*/                                           \
} while (false)

/**
 * @brief   Safety checks hook.
 * @details Invoked synchronously in the detecting caller's context when an
 *          enabled @p chSftAssert() check fails. This hook supplies the entire
 *          failure action; the default calls @p chSysHalt().
 * @note    The hook can run in thread or ISR context, inside kernel updates
 *          or without a kernel lock, and during initialization. It is not a
 *          general I-class callback.
 * @note    Kernel objects may already be inconsistent. Diagnostic work before
 *          stopping must be bounded and nonblocking, and must not lock or
 *          unlock the kernel, reschedule, wake threads, or manipulate kernel
 *          objects. Avoid triggering further checks from the hook.
 * @note    Hook-owned diagnostic storage must be initialized before the first
 *          possible failure. Application initialization need not be complete
 *          then. In SMP, shared storage must not assume that the common
 *          kernel lock is held.
 * @note    The function name can identify a shared checking helper rather
 *          than its caller. The assertion's remark string is not passed.
 * @warning A replacement hook must stop execution without returning to the
 *          failed operation, for example by calling @p chSysHalt(). There is
 *          no implicit halt after the hook; continuing can dereference an
 *          invalid pointer or modify inconsistent data.
 *
 * @param[in] l         failed check level, not the configured hardening level
 * @param[in] f         detecting function's name, supplied through __func__
 */
#define CH_CFG_SAFETY_CHECK_HOOK(l, f) do {                                 \
  /* Safety handling code here.*/                                           \
  chSysHalt(f);                                                             \
} while (false)

/** @} */

/*===========================================================================*/
/* Port-specific settings (override port settings defaulted in chcore.h).    */
/*===========================================================================*/

#endif  /* CHCONF_H */

/** @} */
