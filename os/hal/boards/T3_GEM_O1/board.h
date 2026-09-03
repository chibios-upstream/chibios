/*
    T3 Gemstone - Copyright (C) 2026 T3 Foundation (https://t3vakfi.org).
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
 * @file    board.h
 * @brief   T3 Gemstone O1 (TI AM67A/J722S) Cortex-R5F board definitions.
 * @details Addresses taken from the TI J722S TRM, cross-checked against the
 *          NuttX am67 port and the TI MCU+ SDK (j722s).
 */

#ifndef BOARD_H
#define BOARD_H

/*
 * Board identifier.
 */
#define BOARD_T3_GEM_O1
#define BOARD_NAME              "T3 Gemstone O1 (AM67A/J722S) R5F"

/*
 * Memory map as seen by the MCU_R5FSS0 core 0, see
 * cslr_mcu_r5fss0_baseaddress.h in the TI MCU+ SDK.
 *
 * Only one TCM is usable on this core. The Linux device tree gives it
 * ti,loczrama = <0>, which puts the BTCM at address 0 and the ATCM at
 * 0x41010000, and then ti,atcm-enable = <0> leaves the ATCM switched off.
 * Address 0 is not negotiable: Cortex-R5 has no VBAR, so the vectors, the
 * pre-MPU boot code and the banked mode stacks all share this 32K block.
 * The Main domain core has the opposite layout (loczrama = 1, both TCMs
 * enabled), which is why the linker script differs from the Main build.
 */
#define AM67_TCM_BASE           0x00000000U
#define AM67_TCM_SIZE           (32U * 1024U)
#define AM67_MSRAM_BASE         0x60000000U
#define AM67_MSRAM_SIZE         (512U * 1024U)
#define AM67_DDR_BASE           0x80000000U

/*
 * DDR carveout assigned to this core by the Linux device tree, must match
 * the reserved-memory nodes used by the k3-r5 remoteproc driver. For the
 * MCU core these are mcu_r5fss0_core0_memory_region@a1100000 (15M), 16M
 * below the Main core's carveout so both cores can run at the same time.
 */
#define AM67_RSCTABLE_BASE      0xA1100000U
#define AM67_TRACEBUF_BASE      0xA1110000U
#define AM67_TRACEBUF_SIZE      (16U * 1024U)

/*
 * VIM interrupt controller (MCU_R5FSS0 VIC_CFG, in this core's own view).
 */
#define AM67_VIM_BASE           0x07FF0000U
#define AM67_VIM_NUM_IRQS       512U

/*
 * MCU_TIMER0 used as the OS tick source, IRQ number from
 * cslr_intr_mcu_r5fss0_core0.h.
 *
 * The device tree leaves this timer's clock mux at its reset default,
 * unlike main_timer0 which is explicitly reparented, so the 25 MHz below
 * is the expected HFOSC0 default rather than a verified value. A wrong
 * parent shows up as a tick rate off by a clean integer ratio, not as a
 * boot failure, so it is safe to confirm on hardware.
 */
#define AM67_TIMER0_BASE        0x04800000U
#define AM67_TIMER0_CLK_HZ      25000000U
#define AM67_TIMER0_IRQ         28U

/*
 * MAILBOX0 cluster 1 (MAILBOX0_REGS1), the A53 <-> MCU_R5F doorbell.
 *
 * This is the channel the Linux k3_r5_remoteproc driver uses to ask the R5F to
 * shut down: `mboxes = <0x11 0x12>` on the r5f@79000000 node resolves to
 * mailbox@29010000 / chan mbox-mcu-r5-0. Without a handler on this side,
 * `remoteproc stop` blocks ~25 s and fails with -EBUSY
 * ("k3_r5_rproc_stop: timeout waiting for rproc completion event"), which is
 * why loading new firmware needed a full power cycle.
 *
 * FIFO direction is stated from LINUX's point of view in the device tree, so it
 * inverts here: DT `ti,mbox-tx = <1 0 0>` means Linux transmits on FIFO 1, so
 * the R5F RECEIVES on FIFO 1; DT `ti,mbox-rx = <0 0 0>` means the R5F
 * TRANSMITS on FIFO 0. Getting this backwards yields a mailbox that never
 * interrupts, which looks exactly like having no driver at all.
 *
 * The user index is not discoverable from the device tree -- Linux only
 * declares its own (0). From the J722S TRM Table 4-138:
 *   ..._CLUSTER_1_mailbox_cluster_pend_0 -> GICSS0_spi_109              (Linux, user 0)
 *   ..._CLUSTER_1_mailbox_cluster_pend_2 -> MCU_R5FSS0_CORE0_cpu0_intr_241  (us, user 2)
 *   ..._CLUSTER_1_mailbox_cluster_pend_3 -> WKUP_R5FSS0_CORE0_intr_241
 * Cross-checked two ways: the DT parent interrupt <0 0x4d 4> is GIC SPI 77,
 * and 77 + 32 = INTID 109 = the TRM's GICSS0_spi_109, which confirms
 * pend_N <-> user N; and the TRM's MCU_TIMER0 -> cpu0_intr_28 matches
 * AM67_TIMER0_IRQ above, which is hardware-verified, confirming that
 * cpu0_intr_N is the VIM input number.
 */
#define AM67_MAILBOX_BASE       0x29010000U
#define AM67_MAILBOX_IRQ        241U
#define AM67_MAILBOX_USER       2U      /* Our interrupt user index          */
#define AM67_MAILBOX_RX_FIFO    1U      /* Linux writes here, we read it     */
#define AM67_MAILBOX_TX_FIFO    0U      /* We write here, Linux reads it     */

/*
 * EPWM0 (eHRPWM), first PWM output on EHRPWM0_A -> Gemstone 40-pin header
 * pin 29 (GPIO5 pad, muxed to EHRPWM0_A by the Linux DT overlay
 * k3-am67a-t3-gem-o1-pwm-epwm0-gpio5.dtbo). The DT owns pinmux + the module
 * clock/power; the firmware only drives EPWM registers.
 *
 * The EPWM counter input clock (SYSCLKOUT) is the clock BEFORE the TBCTL
 * HSPCLKDIV/CLKDIV prescale, i.e. the module "fck": confirmed 250 MHz on this
 * board via /sys/kernel/debug/clk/clk_summary and the Linux pwm-tiehrpwm
 * driver (it derives period/duty from clk_get_rate of "fck"). The separate
 * epwm_tbclk gate must be enabled for the counter to run but is NOT a
 * divider. The value itself lives in am67_registry.h as AM67_EPWM_CLOCK.
 */
#define AM67_EPWM0_BASE         0x23000000U

/*
 * EPWM1 (eHRPWM) second instance: EHRPWM1_A -> GPIO6 (pin 31), EHRPWM1_B ->
 * GPIO13 (pin 33). Same IP and 250 MHz fck as EPWM0; A and B share the time
 * base but have independent CMPA/CMPB compares.
 *
 * ECAP0/1/2 (eCAP in APWM mode) -> GPIO12 (pin 32) / GPIO16 (pin 36) /
 * GPIO18 (pin 12). Separate IP from EHRPWM. The APWM counter (TSCTR) is 32-bit
 * and clocked directly by the module fck, so no prescale is used.
 * That fck is 125 MHz on this board -- HALF the EPWM fck (a different clock
 * domain), verified via /sys/kernel/debug/clk/clk_summary
 * (23100000/23110000/23120000.pwm fck = 125000000) and confirmed by scope
 * (50 Hz with period = 2500000 ticks). All three ECAP instances share it.
 *
 * EHRPWM0_B -> GPIO14 (pin 8). Freed 2026-08-03: this pad was the UART1
 * console TX until MAVLink moved to the shared-memory rings (DR-016) and the
 * stock overlay k3-am67a-t3-gem-o1-pwm-epwm0-gpio5-gpio14.dtbo reconfigured
 * main_uart1 to an RX-only pin group (pad 0x01AC, pin 10) while taking pad
 * 0x01B0 for EHRPWM0_B. The four flight outputs are now EHRPWM0_A/B and
 * EHRPWM1_A/B; the ECAP bases below are retained but no longer driven by
 * AP_HAL_ChibiOS_K3::RCOutput.
 */
#define AM67_EPWM1_BASE         0x23010000U
#define AM67_ECAP0_BASE         0x23100000U
#define AM67_ECAP1_BASE         0x23110000U
#define AM67_ECAP2_BASE         0x23120000U

/*
 * Peripheral input clocks the XHAL drivers need from the board.
 *
 * These are board-level rather than SoC-level because the device tree owns
 * the clock muxes: the same peripheral on another carrier can be parented
 * differently. A wrong value shows up as a rate off by a clean integer
 * ratio, not as a boot failure.
 *
 * The eHRPWM and eCAP counter clocks are NOT repeated here: they are owned
 * by am67_registry.h (AM67_EPWM_CLOCK / AM67_ECAP_CLOCK), which is the single
 * source of truth for them and what the XHAL PWM driver actually reads.
 */
#define AM67_ST_TIMER_CLOCK     AM67_TIMER0_CLK_HZ
#define AM67_MAIN_UART1_CLOCK   48000000U

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */
