/* Standalone configuration for the deterministic NIL timeout tests. */
#ifndef CHCONF_H
#define CHCONF_H

#define _CHIBIOS_NIL_CONF_
#define _CHIBIOS_NIL_CONF_VER_4_0_
#define CH_CFG_MAX_THREADS                  4
#define CH_CFG_AUTOSTART_THREADS            TRUE
#define CH_CFG_ST_FREQUENCY                 1000
/* Resolution, time delta and assertion setting are supplied by the runner. */
#define CH_CFG_USE_WAITEXIT                 TRUE
#define CH_CFG_USE_SEMAPHORES               TRUE
#define CH_CFG_USE_MUTEXES                  FALSE
#define CH_CFG_USE_EVENTS                   TRUE
#define CH_CFG_USE_MESSAGES                 TRUE
#define CH_CFG_USE_MAILBOXES                FALSE
#define CH_CFG_USE_MEMCHECKS                FALSE
#define CH_CFG_USE_MEMCORE                  FALSE
#define CH_CFG_USE_HEAP                     FALSE
#define CH_CFG_USE_MEMPOOLS                 FALSE
#define CH_CFG_USE_OBJ_FIFOS                FALSE
#define CH_CFG_USE_PIPES                    FALSE
#define CH_CFG_USE_OBJ_CACHES               FALSE
#define CH_CFG_USE_DELEGATES                FALSE
#define CH_CFG_USE_JOBS                     FALSE
#define CH_CFG_USE_FACTORY                  FALSE
#define CH_CFG_FACTORY_MAX_NAMES_LENGTH     8
#define CH_CFG_FACTORY_OBJECTS_REGISTRY     FALSE
#define CH_CFG_FACTORY_GENERIC_BUFFERS      FALSE
#define CH_CFG_FACTORY_SEMAPHORES           FALSE
#define CH_CFG_FACTORY_MAILBOXES            FALSE
#define CH_CFG_FACTORY_OBJ_FIFOS            FALSE
#define CH_CFG_FACTORY_PIPES                FALSE
#define CH_DBG_STATISTICS                   FALSE
#define CH_DBG_SYSTEM_STATE_CHECK           TRUE
#define CH_DBG_ENABLE_CHECKS                TRUE
#define CH_DBG_ENABLE_STACK_CHECK           FALSE
#define CH_CFG_SYSTEM_INIT_HOOK()           do { } while (false)
#define CH_CFG_THREAD_EXT_FIELDS
#define CH_CFG_THREAD_EXT_INIT_HOOK(tp)     do { (void)(tp); } while (false)
#define CH_CFG_THREAD_EXIT_HOOK(tp)         do { (void)(tp); } while (false)
#define CH_CFG_IDLE_ENTER_HOOK()            do { } while (false)
#define CH_CFG_IDLE_LEAVE_HOOK()            do { } while (false)
#define CH_CFG_SYSTEM_HALT_HOOK(reason)     test_halt(reason)

void test_halt(const char *reason);

#endif /* CHCONF_H */
