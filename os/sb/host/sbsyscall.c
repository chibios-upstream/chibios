/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    sb/host/sbhost.c
 * @brief   ARM SandBox syscall dispatcher code.
 *
 * @addtogroup ARM_SANDBOX_HOSTAPI
 * @{
 */

#include "ch.h"
#include "sb.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* The tables below list the codes from 0 to 255.*/
__CH_STATIC_ASSERT(sb_sysc_base, SB_SYSC_BASE == 128);

/* Weak default of a table entry, an alias of the not implemented handler,
   a handler bound by SB_SVC_HANDLER() replaces it.*/
#define SB_SVC_WEAK(n)                                                      \
  void __sb_svc##n(sb_class_t *sbp, struct port_extctx *ectxp)              \
    __attribute__((weak, alias("sb_undef_handler")))

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

static void sb_undef_handler(sb_class_t *sbp, struct port_extctx *ectxp);

SB_SVC_WEAK(0);   SB_SVC_WEAK(1);   SB_SVC_WEAK(2);   SB_SVC_WEAK(3);
SB_SVC_WEAK(4);   SB_SVC_WEAK(5);   SB_SVC_WEAK(6);   SB_SVC_WEAK(7);
SB_SVC_WEAK(8);   SB_SVC_WEAK(9);   SB_SVC_WEAK(10);  SB_SVC_WEAK(11);
SB_SVC_WEAK(12);  SB_SVC_WEAK(13);  SB_SVC_WEAK(14);  SB_SVC_WEAK(15);
SB_SVC_WEAK(16);  SB_SVC_WEAK(17);  SB_SVC_WEAK(18);  SB_SVC_WEAK(19);
SB_SVC_WEAK(20);  SB_SVC_WEAK(21);  SB_SVC_WEAK(22);  SB_SVC_WEAK(23);
SB_SVC_WEAK(24);  SB_SVC_WEAK(25);  SB_SVC_WEAK(26);  SB_SVC_WEAK(27);
SB_SVC_WEAK(28);  SB_SVC_WEAK(29);  SB_SVC_WEAK(30);  SB_SVC_WEAK(31);
SB_SVC_WEAK(32);  SB_SVC_WEAK(33);  SB_SVC_WEAK(34);  SB_SVC_WEAK(35);
SB_SVC_WEAK(36);  SB_SVC_WEAK(37);  SB_SVC_WEAK(38);  SB_SVC_WEAK(39);
SB_SVC_WEAK(40);  SB_SVC_WEAK(41);  SB_SVC_WEAK(42);  SB_SVC_WEAK(43);
SB_SVC_WEAK(44);  SB_SVC_WEAK(45);  SB_SVC_WEAK(46);  SB_SVC_WEAK(47);
SB_SVC_WEAK(48);  SB_SVC_WEAK(49);  SB_SVC_WEAK(50);  SB_SVC_WEAK(51);
SB_SVC_WEAK(52);  SB_SVC_WEAK(53);  SB_SVC_WEAK(54);  SB_SVC_WEAK(55);
SB_SVC_WEAK(56);  SB_SVC_WEAK(57);  SB_SVC_WEAK(58);  SB_SVC_WEAK(59);
SB_SVC_WEAK(60);  SB_SVC_WEAK(61);  SB_SVC_WEAK(62);  SB_SVC_WEAK(63);
SB_SVC_WEAK(64);  SB_SVC_WEAK(65);  SB_SVC_WEAK(66);  SB_SVC_WEAK(67);
SB_SVC_WEAK(68);  SB_SVC_WEAK(69);  SB_SVC_WEAK(70);  SB_SVC_WEAK(71);
SB_SVC_WEAK(72);  SB_SVC_WEAK(73);  SB_SVC_WEAK(74);  SB_SVC_WEAK(75);
SB_SVC_WEAK(76);  SB_SVC_WEAK(77);  SB_SVC_WEAK(78);  SB_SVC_WEAK(79);
SB_SVC_WEAK(80);  SB_SVC_WEAK(81);  SB_SVC_WEAK(82);  SB_SVC_WEAK(83);
SB_SVC_WEAK(84);  SB_SVC_WEAK(85);  SB_SVC_WEAK(86);  SB_SVC_WEAK(87);
SB_SVC_WEAK(88);  SB_SVC_WEAK(89);  SB_SVC_WEAK(90);  SB_SVC_WEAK(91);
SB_SVC_WEAK(92);  SB_SVC_WEAK(93);  SB_SVC_WEAK(94);  SB_SVC_WEAK(95);
SB_SVC_WEAK(96);  SB_SVC_WEAK(97);  SB_SVC_WEAK(98);  SB_SVC_WEAK(99);
SB_SVC_WEAK(100); SB_SVC_WEAK(101); SB_SVC_WEAK(102); SB_SVC_WEAK(103);
SB_SVC_WEAK(104); SB_SVC_WEAK(105); SB_SVC_WEAK(106); SB_SVC_WEAK(107);
SB_SVC_WEAK(108); SB_SVC_WEAK(109); SB_SVC_WEAK(110); SB_SVC_WEAK(111);
SB_SVC_WEAK(112); SB_SVC_WEAK(113); SB_SVC_WEAK(114); SB_SVC_WEAK(115);
SB_SVC_WEAK(116); SB_SVC_WEAK(117); SB_SVC_WEAK(118); SB_SVC_WEAK(119);
SB_SVC_WEAK(120); SB_SVC_WEAK(121); SB_SVC_WEAK(122); SB_SVC_WEAK(123);
SB_SVC_WEAK(124); SB_SVC_WEAK(125); SB_SVC_WEAK(126); SB_SVC_WEAK(127);
SB_SVC_WEAK(128); SB_SVC_WEAK(129); SB_SVC_WEAK(130); SB_SVC_WEAK(131);
SB_SVC_WEAK(132); SB_SVC_WEAK(133); SB_SVC_WEAK(134); SB_SVC_WEAK(135);
SB_SVC_WEAK(136); SB_SVC_WEAK(137); SB_SVC_WEAK(138); SB_SVC_WEAK(139);
SB_SVC_WEAK(140); SB_SVC_WEAK(141); SB_SVC_WEAK(142); SB_SVC_WEAK(143);
SB_SVC_WEAK(144); SB_SVC_WEAK(145); SB_SVC_WEAK(146); SB_SVC_WEAK(147);
SB_SVC_WEAK(148); SB_SVC_WEAK(149); SB_SVC_WEAK(150); SB_SVC_WEAK(151);
SB_SVC_WEAK(152); SB_SVC_WEAK(153); SB_SVC_WEAK(154); SB_SVC_WEAK(155);
SB_SVC_WEAK(156); SB_SVC_WEAK(157); SB_SVC_WEAK(158); SB_SVC_WEAK(159);
SB_SVC_WEAK(160); SB_SVC_WEAK(161); SB_SVC_WEAK(162); SB_SVC_WEAK(163);
SB_SVC_WEAK(164); SB_SVC_WEAK(165); SB_SVC_WEAK(166); SB_SVC_WEAK(167);
SB_SVC_WEAK(168); SB_SVC_WEAK(169); SB_SVC_WEAK(170); SB_SVC_WEAK(171);
SB_SVC_WEAK(172); SB_SVC_WEAK(173); SB_SVC_WEAK(174); SB_SVC_WEAK(175);
SB_SVC_WEAK(176); SB_SVC_WEAK(177); SB_SVC_WEAK(178); SB_SVC_WEAK(179);
SB_SVC_WEAK(180); SB_SVC_WEAK(181); SB_SVC_WEAK(182); SB_SVC_WEAK(183);
SB_SVC_WEAK(184); SB_SVC_WEAK(185); SB_SVC_WEAK(186); SB_SVC_WEAK(187);
SB_SVC_WEAK(188); SB_SVC_WEAK(189); SB_SVC_WEAK(190); SB_SVC_WEAK(191);
SB_SVC_WEAK(192); SB_SVC_WEAK(193); SB_SVC_WEAK(194); SB_SVC_WEAK(195);
SB_SVC_WEAK(196); SB_SVC_WEAK(197); SB_SVC_WEAK(198); SB_SVC_WEAK(199);
SB_SVC_WEAK(200); SB_SVC_WEAK(201); SB_SVC_WEAK(202); SB_SVC_WEAK(203);
SB_SVC_WEAK(204); SB_SVC_WEAK(205); SB_SVC_WEAK(206); SB_SVC_WEAK(207);
SB_SVC_WEAK(208); SB_SVC_WEAK(209); SB_SVC_WEAK(210); SB_SVC_WEAK(211);
SB_SVC_WEAK(212); SB_SVC_WEAK(213); SB_SVC_WEAK(214); SB_SVC_WEAK(215);
SB_SVC_WEAK(216); SB_SVC_WEAK(217); SB_SVC_WEAK(218); SB_SVC_WEAK(219);
SB_SVC_WEAK(220); SB_SVC_WEAK(221); SB_SVC_WEAK(222); SB_SVC_WEAK(223);
SB_SVC_WEAK(224); SB_SVC_WEAK(225); SB_SVC_WEAK(226); SB_SVC_WEAK(227);
SB_SVC_WEAK(228); SB_SVC_WEAK(229); SB_SVC_WEAK(230); SB_SVC_WEAK(231);
SB_SVC_WEAK(232); SB_SVC_WEAK(233); SB_SVC_WEAK(234); SB_SVC_WEAK(235);
SB_SVC_WEAK(236); SB_SVC_WEAK(237); SB_SVC_WEAK(238); SB_SVC_WEAK(239);
SB_SVC_WEAK(240); SB_SVC_WEAK(241); SB_SVC_WEAK(242); SB_SVC_WEAK(243);
SB_SVC_WEAK(244); SB_SVC_WEAK(245); SB_SVC_WEAK(246); SB_SVC_WEAK(247);
SB_SVC_WEAK(248); SB_SVC_WEAK(249); SB_SVC_WEAK(250); SB_SVC_WEAK(251);
SB_SVC_WEAK(252); SB_SVC_WEAK(253); SB_SVC_WEAK(254); SB_SVC_WEAK(255);

/**
 * @brief   Fastcalls table.
 * @note    Entries are indexed by the fastcall codes, the entries without
 *          a handler bound by @p SB_SVC_HANDLER() are served as not
 *          implemented.
 */
const port_syscall_t sb_fastcalls[SB_SYSC_BASE] = {
  __sb_svc0,   __sb_svc1,   __sb_svc2,   __sb_svc3,   __sb_svc4,   __sb_svc5,
  __sb_svc6,   __sb_svc7,   __sb_svc8,   __sb_svc9,   __sb_svc10,  __sb_svc11,
  __sb_svc12,  __sb_svc13,  __sb_svc14,  __sb_svc15,  __sb_svc16,  __sb_svc17,
  __sb_svc18,  __sb_svc19,  __sb_svc20,  __sb_svc21,  __sb_svc22,  __sb_svc23,
  __sb_svc24,  __sb_svc25,  __sb_svc26,  __sb_svc27,  __sb_svc28,  __sb_svc29,
  __sb_svc30,  __sb_svc31,  __sb_svc32,  __sb_svc33,  __sb_svc34,  __sb_svc35,
  __sb_svc36,  __sb_svc37,  __sb_svc38,  __sb_svc39,  __sb_svc40,  __sb_svc41,
  __sb_svc42,  __sb_svc43,  __sb_svc44,  __sb_svc45,  __sb_svc46,  __sb_svc47,
  __sb_svc48,  __sb_svc49,  __sb_svc50,  __sb_svc51,  __sb_svc52,  __sb_svc53,
  __sb_svc54,  __sb_svc55,  __sb_svc56,  __sb_svc57,  __sb_svc58,  __sb_svc59,
  __sb_svc60,  __sb_svc61,  __sb_svc62,  __sb_svc63,  __sb_svc64,  __sb_svc65,
  __sb_svc66,  __sb_svc67,  __sb_svc68,  __sb_svc69,  __sb_svc70,  __sb_svc71,
  __sb_svc72,  __sb_svc73,  __sb_svc74,  __sb_svc75,  __sb_svc76,  __sb_svc77,
  __sb_svc78,  __sb_svc79,  __sb_svc80,  __sb_svc81,  __sb_svc82,  __sb_svc83,
  __sb_svc84,  __sb_svc85,  __sb_svc86,  __sb_svc87,  __sb_svc88,  __sb_svc89,
  __sb_svc90,  __sb_svc91,  __sb_svc92,  __sb_svc93,  __sb_svc94,  __sb_svc95,
  __sb_svc96,  __sb_svc97,  __sb_svc98,  __sb_svc99,  __sb_svc100, __sb_svc101,
  __sb_svc102, __sb_svc103, __sb_svc104, __sb_svc105, __sb_svc106, __sb_svc107,
  __sb_svc108, __sb_svc109, __sb_svc110, __sb_svc111, __sb_svc112, __sb_svc113,
  __sb_svc114, __sb_svc115, __sb_svc116, __sb_svc117, __sb_svc118, __sb_svc119,
  __sb_svc120, __sb_svc121, __sb_svc122, __sb_svc123, __sb_svc124, __sb_svc125,
  __sb_svc126, __sb_svc127,
};

/**
 * @brief   Syscalls table.
 * @note    Entries are indexed by the syscall codes minus
 *          @p SB_SYSC_BASE, the entries without a handler bound by
 *          @p SB_SVC_HANDLER() are served as not implemented.
 */
const port_syscall_t sb_syscalls[256 - SB_SYSC_BASE] = {
  __sb_svc128, __sb_svc129, __sb_svc130, __sb_svc131, __sb_svc132, __sb_svc133,
  __sb_svc134, __sb_svc135, __sb_svc136, __sb_svc137, __sb_svc138, __sb_svc139,
  __sb_svc140, __sb_svc141, __sb_svc142, __sb_svc143, __sb_svc144, __sb_svc145,
  __sb_svc146, __sb_svc147, __sb_svc148, __sb_svc149, __sb_svc150, __sb_svc151,
  __sb_svc152, __sb_svc153, __sb_svc154, __sb_svc155, __sb_svc156, __sb_svc157,
  __sb_svc158, __sb_svc159, __sb_svc160, __sb_svc161, __sb_svc162, __sb_svc163,
  __sb_svc164, __sb_svc165, __sb_svc166, __sb_svc167, __sb_svc168, __sb_svc169,
  __sb_svc170, __sb_svc171, __sb_svc172, __sb_svc173, __sb_svc174, __sb_svc175,
  __sb_svc176, __sb_svc177, __sb_svc178, __sb_svc179, __sb_svc180, __sb_svc181,
  __sb_svc182, __sb_svc183, __sb_svc184, __sb_svc185, __sb_svc186, __sb_svc187,
  __sb_svc188, __sb_svc189, __sb_svc190, __sb_svc191, __sb_svc192, __sb_svc193,
  __sb_svc194, __sb_svc195, __sb_svc196, __sb_svc197, __sb_svc198, __sb_svc199,
  __sb_svc200, __sb_svc201, __sb_svc202, __sb_svc203, __sb_svc204, __sb_svc205,
  __sb_svc206, __sb_svc207, __sb_svc208, __sb_svc209, __sb_svc210, __sb_svc211,
  __sb_svc212, __sb_svc213, __sb_svc214, __sb_svc215, __sb_svc216, __sb_svc217,
  __sb_svc218, __sb_svc219, __sb_svc220, __sb_svc221, __sb_svc222, __sb_svc223,
  __sb_svc224, __sb_svc225, __sb_svc226, __sb_svc227, __sb_svc228, __sb_svc229,
  __sb_svc230, __sb_svc231, __sb_svc232, __sb_svc233, __sb_svc234, __sb_svc235,
  __sb_svc236, __sb_svc237, __sb_svc238, __sb_svc239, __sb_svc240, __sb_svc241,
  __sb_svc242, __sb_svc243, __sb_svc244, __sb_svc245, __sb_svc246, __sb_svc247,
  __sb_svc248, __sb_svc249, __sb_svc250, __sb_svc251, __sb_svc252, __sb_svc253,
  __sb_svc254, __sb_svc255,
};

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static void sb_undef_handler(sb_class_t *sbp, struct port_extctx *ectxp) {

  (void)sbp;

  ectxp->r0 = CH_RET_ENOSYS;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void __sb_abort(msg_t msg) {
  sb_class_t *sbp = (sb_class_t *)chThdGetSelfX()->object;

  chDbgAssert(sbp->state == SB_STATE_RUNNING, "invalid lifecycle state");
  sbp->state = SB_STATE_STOPPING;

#if SB_CFG_ENABLE_VRQ == TRUE
  chVTResetI(&sbp->vrq.alarm_vt);
#endif

#if (SB_CFG_ENABLE_VFS == TRUE) || (SB_CFG_ENABLE_VIO == TRUE) ||          \
    (SB_CFG_ENABLE_VSRV == TRUE)
  /* Modules cleanup, in a single unlocked window because it can block.*/
  chSysUnlock();
#if SB_CFG_ENABLE_VFS == TRUE
  __sb_io_cleanup(sbp);
#endif
#if SB_CFG_ENABLE_VIO == TRUE
  __sb_vio_cleanup(sbp);
#endif
#if SB_CFG_ENABLE_VSRV == TRUE
  __sb_vsrv_cleanup(sbp);
#endif
  chSysLock();
#endif

#if CH_CFG_USE_EVENTS == TRUE
  chEvtBroadcastI(&sb.termination_es);
#endif
#if CH_CFG_USE_MESSAGES == TRUE
  sbp->base.msg_tp = NULL;
  chMsgReleaseAllI();
#endif
  chThdExitS(msg);
  chSysHalt("zombies");
}

/**
 * @brief   Redefined syscall entry vector.
 * @note    When FPU is enabled there is the assumption that the syscall
 *          code does not change the FPCA state or __port_do_syscall_return()
 *          would try to return using the wrong frame type.
 */
void __port_do_syscall_entry(uint32_t n, struct port_extctx *ectxp) {
  extern void __sb_dispatch_syscall(sb_class_t *sbp,
                                    struct port_extctx *ectxp,
                                    uint32_t n);
  thread_t *tp = __sch_get_currthread();
  sb_class_t *sbp = (sb_class_t *)tp->object;
  struct port_extctx *newctxp;

#if (CORTEX_USE_FPU == TRUE) && (PORT_USE_FPU_FAST_SWITCHING > 0)
  /* Completing any lazy FP save to the caller frame before switching
     to the privileged stack.*/
  (void)__get_FPSCR();
  asm volatile ("" : : : "memory");
#endif

  /* Saving caller context in unprivileged memory.*/
  sbp->u_psp = (uint32_t)ectxp;

  /* Return context for change in privileged mode, it is created on the
     privileged PSP so no need to check for overflows.*/
  newctxp = (struct port_extctx *)(void *)((uint8_t *)tp->waend -
                                           sizeof (struct port_extctx));

  /* Creating context for return in privileged mode.*/
  newctxp->r0   = (uint32_t)sbp;
  newctxp->r1   = (uint32_t)ectxp;
  newctxp->r2   = n;
  newctxp->pc   = (uint32_t)__sb_dispatch_syscall;
  newctxp->xpsr = (uint32_t)0x01000000;
#if CORTEX_USE_FPU == TRUE
  newctxp->fpscr = FPU->FPDSCR;
#endif

  /* Switching PSP to the privileged mode PSP.*/
  __set_PSP((uint32_t)newctxp);
#if defined(PORT_ARCHITECTURE_ARM_V8M_MAINLINE)
  __set_PSPLIM((uint32_t)sbp->thread.wabase);
#endif
}

/**
 * @brief   Redefined syscall return vector with VRQ handling.
 * @note    When FPU is enabled there is the assumption that the syscall
 *          code does not change the FPCA state or this code would try to
 *          return using the wrong frame type.
 */
void __port_do_syscall_return(void) {
  thread_t *tp;
  sb_class_t *sbp;
  struct port_extctx *ectxp;

  tp = __sch_get_currthread();
  sbp = (sb_class_t *)tp->object;
  ectxp = (struct port_extctx *)sbp->u_psp;

#if CORTEX_USE_FPU == TRUE
  /* Discarding the privileged lazy FP save and forcing FP unstacking
     from the sandbox frame, including on the initial trampoline return.*/
  FPU->FPCCR &= ~FPU_FPCCR_LSPACT_Msk;
#endif

#if SB_CFG_ENABLE_VRQ == TRUE
  __sb_vrq_check_pending(sbp, ectxp);
#else
  __set_PSP((uint32_t)ectxp);
#if defined(PORT_ARCHITECTURE_ARM_V8M_MAINLINE)
  __set_PSPLIM((uint32_t)sbp->u_data->base);
#endif
#endif
}

void __port_do_fastcall_entry(uint32_t n, struct port_extctx *ectxp) {
  thread_t *tp = __sch_get_currthread();
  sb_class_t *sbp = (sb_class_t *)tp->object;

  sb_fastcalls[n](sbp, ectxp);
}

/** @} */
