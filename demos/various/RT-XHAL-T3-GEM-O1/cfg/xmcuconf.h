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
 * @file    xmcuconf.h
 * @brief   AM67 XHAL driver configuration.
 * @details The R5F runs as a RemoteProc slave: clocks, power domains and
 *          pin multiplexing belong to the Linux host, so there is no clock
 *          tree configuration here. Only peripheral assignment and
 *          interrupt priorities are the firmware's to choose.
 *
 * @addtogroup XHAL_CONF
 * @{
 */

#ifndef XMCUCONF_H
#define XMCUCONF_H

#define AM67_MCUCONF

/*
 * VIM priorities, 0 is the most urgent and 15 the least. Same-priority
 * lines cannot preempt each other, so peripherals share a level below the
 * system tick, which keeps its default of 0.
 */

/*
 * SIO driver system settings.
 */
#define AM67_SIO_USE_UART1                  TRUE
#define AM67_SIO_UART1_IRQ_PRIORITY         8U

#endif /* XMCUCONF_H */

/** @} */
