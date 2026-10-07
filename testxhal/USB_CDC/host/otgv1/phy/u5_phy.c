/* Actual U5 PHY helpers and safety wait, with deterministic register timing. */
#include "hal.h"
#include "hal_safety.c"
#include "phy_code.inc"

static uint32_t counter_value;
static unsigned counter_calls, ready_at, enables[2], disables, delays;
static bool preempted, mutate_other;

static uint32_t test_counter(void) {

  counter_calls++;
  counter_value += (preempted && counter_calls == 2U) ?
                   STM32_USB_BOOSTER_STARTUP_TIME : 10U;
  if (ready_at != 0U && counter_calls == ready_at) {
    PWR->VOSR |= PWR_VOSR_USBBOOSTRDY;
  }
  if (mutate_other && counter_calls == 2U) {
    PWR->VOSR |= PWR_VOSR_BOOSTEN;
  }
  return counter_value;
}

static void test_clock_enable(unsigned clock, bool low_power) {

  assert(low_power);
  assert((PWR->VOSR & PWR_VOSR_USBBOOSTRDY) != 0U);
  assert((PWR->VOSR & PWR_VOSR_VDD11USBDIS) == 0U);
  assert((PWR->VOSR & (PWR_VOSR_USBPWREN | PWR_VOSR_USBBOOSTEN)) ==
         (PWR_VOSR_USBPWREN | PWR_VOSR_USBBOOSTEN));
  enables[clock]++;
}

static void test_clock_disable(void) {

  assert(enables[1] == disables + 1U);
  disables++;
}

static void test_delay(uint32_t cycles) {

  assert(cycles == MS2RTC(STM32_HCLK, 2U));
  assert(enables[0] == 1U && enables[1] == 1U);
  assert((SYSCFG->OTGHSPHYCR & SYSCFG_OTGHSPHYCR_EN) != 0U);
  assert(((SYSCFG->OTGHSPHYCR & SYSCFG_OTGHSPHYCR_PDCTRL) != 0U) ==
         (delays == 1U));
  delays++;
}

static void init_case(uint32_t prior, bool wrap) {

  memset(&test_pwr, 0, sizeof test_pwr);
  memset(&test_syscfg, 0xa5, sizeof test_syscfg);
  PWR->VOSR = prior;
  counter_value = wrap ? UINT32_MAX - 500U : 0U;
  counter_calls = ready_at = enables[0] = enables[1] = disables = delays = 0U;
  preempted = mutate_other = false;
}

static void check_success(void) {
  uint32_t tune = SYSCFG->OTGHSPHYTUNER2;

  assert(!stm32_otg2_phy_start());
  assert(enables[0] == 1U && enables[1] == 1U && delays == 2U);
  assert(SYSCFG->OTGHSPHYCR == (STM32_OTGHS_PHY_CLKSEL |
                               SYSCFG_OTGHSPHYCR_EN |
                               SYSCFG_OTGHSPHYCR_PDCTRL));
  assert(SYSCFG->OTGHSPHYTUNER2 ==
         ((tune & ~(SYSCFG_OTGHSPHYTUNER2_COMPDISTUNE_Msk |
                    SYSCFG_OTGHSPHYTUNER2_SQRXTUNE_Msk)) |
          SYSCFG_OTGHSPHYTUNER2_COMPDISTUNE_1));
  stm32_otg2_phy_stop();
  assert(disables == 1U);
  assert((SYSCFG->OTGHSPHYCR & (SYSCFG_OTGHSPHYCR_EN |
                               SYSCFG_OTGHSPHYCR_PDCTRL)) == 0U);
  assert((PWR->VOSR & (PWR_VOSR_USBPWREN | PWR_VOSR_USBBOOSTEN)) == 0U);
}

int main(void) {
  SYSCFG_TypeDef before;
  uint32_t prior;

  for (unsigned wrap = 0U; wrap < 2U; wrap++) {
    /* Already ready, delayed readiness, and readiness while preempted
       beyond the deadline must all succeed. */
    for (unsigned mode = 0U; mode < 3U; mode++) {
      init_case(mode == 0U ? PWR_VOSR_USBBOOSTRDY : 0U, wrap);
      ready_at = mode == 1U ? 3U : 2U;
      preempted = mode == 2U;
      check_success();
    }
    /* Every combination of the three fields owned by startup is restored
       on timeout, while unrelated updates made during the wait survive. */
    for (unsigned bits = 0U; bits < 8U; bits++) {
      prior = PWR_VOSR_VOS_0 |
              ((bits & 1U) ? PWR_VOSR_VDD11USBDIS : 0U) |
              ((bits & 2U) ? PWR_VOSR_USBPWREN : 0U) |
              ((bits & 4U) ? PWR_VOSR_USBBOOSTEN : 0U);
      init_case(prior, wrap);
      memcpy(&before, &test_syscfg, sizeof before);
      mutate_other = true;
      assert(stm32_otg2_phy_start());
      assert(counter_calls >= 101U && counter_calls <= 103U);
      assert(PWR->VOSR == (prior | PWR_VOSR_BOOSTEN));
      assert(memcmp(&before, &test_syscfg, sizeof before) == 0);
      assert(enables[0] == 0U && enables[1] == 0U);
      assert(disables == 0U && delays == 0U);
      /* The same state can be retried as soon as the booster recovers. */
      PWR->VOSR |= PWR_VOSR_USBBOOSTRDY;
      check_success();
    }
  }
  puts("PASS: U5 PHY ready/delayed/preempted, timeout, rollback, clocks, retry and wrap");
  return 0;
}
