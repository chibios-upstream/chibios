/* Deterministic port boundary; all scheduler/wait code is production NIL. */
#ifndef CHCORE_H
#define CHCORE_H

typedef uint64_t stkalign_t;
struct port_context { void *sp; };
#define PORT_SUPPORTS_RT FALSE
#define PORT_NATURAL_ALIGN sizeof(void *)
#define PORT_STACK_ALIGN 8U
#define PORT_WORKING_AREA_ALIGN 8U
#define PORT_WA_SIZE(n) (n)

void test_unlock_isr(void);
void port_switch(thread_t *ntp, thread_t *otp);
systime_t test_time(void);
systime_t test_alarm(void);
void test_set_alarm(systime_t t);
void test_start_alarm(systime_t t);
void test_stop_alarm(void);

#define port_init(p) ((void)(p))
#define PORT_SETUP_CONTEXT(c, b, e, f, a) do {                              \
  (void)(c); (void)(b); (void)(e); (void)(f); (void)(a);                       \
} while (false)
#define port_disable() ((void)0)
#define port_enable() ((void)0)
#define port_suspend() ((void)0)
#define port_lock() ((void)0)
#define port_unlock() ((void)0)
#define port_lock_from_isr() ((void)0)
#define port_unlock_from_isr() test_unlock_isr()
#define port_timer_get_time() test_time()
#define port_timer_get_alarm() test_alarm()
#define port_timer_set_alarm(t) test_set_alarm(t)
#define port_timer_start_alarm(t) test_start_alarm(t)
#define port_timer_stop_alarm() test_stop_alarm()

#endif /* CHCORE_H */
