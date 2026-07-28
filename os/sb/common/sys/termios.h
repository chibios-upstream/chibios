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
 * @file    sb/common/sys/termios.h
 * @brief   Sandbox POSIX terminal definitions.
 *
 * @addtogroup ARM_SANDBOX_TERMIOS
 * @{
 */

#ifndef SYS_TERMIOS_H
#define SYS_TERMIOS_H

#include <sys/types.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Control character indexes
 * @{
 */
#define VINTR               0
#define VQUIT               1
#define VERASE              2
#define VKILL               3
#define VEOF                4
#define VTIME               5
#define VMIN                6
#define VSWTC               7
#define VSTART              8
#define VSTOP               9
#define VSUSP               10
#define VEOL                11
#define VREPRINT            12
#define VDISCARD            13
#define VWERASE             14
#define VLNEXT              15
#define NCCS                16
/** @} */

/**
 * @name    Input flags
 * @{
 */
#define IGNBRK              0x00000001U
#define BRKINT              0x00000002U
#define IGNPAR              0x00000004U
#define PARMRK              0x00000008U
#define INPCK               0x00000010U
#define ISTRIP              0x00000020U
#define INLCR               0x00000040U
#define IGNCR               0x00000080U
#define ICRNL               0x00000100U
#define IUCLC               0x00000200U
#define IXON                0x00000400U
#define IXANY               0x00000800U
#define IXOFF               0x00001000U
#define IMAXBEL             0x00002000U
#define IUTF8               0x00004000U
/** @} */

/**
 * @name    Output flags
 * @{
 */
#define OPOST               0x00000001U
#define OLCUC               0x00000002U
#define ONLCR               0x00000004U
#define OCRNL               0x00000008U
#define ONOCR               0x00000010U
#define ONLRET              0x00000020U
#define OFILL               0x00000040U
#define OFDEL               0x00000080U
/** @} */

/**
 * @name    Control flags
 * @{
 */
#define CSIZE               0x00000030U
#define CS5                 0x00000000U
#define CS6                 0x00000010U
#define CS7                 0x00000020U
#define CS8                 0x00000030U
#define CSTOPB              0x00000040U
#define CREAD               0x00000080U
#define PARENB              0x00000100U
#define PARODD              0x00000200U
#define HUPCL               0x00000400U
#define CLOCAL              0x00000800U
/** @} */

/**
 * @name    Local flags
 * @{
 */
#define ISIG                0x00000001U
#define ICANON              0x00000002U
#define XCASE               0x00000004U
#define ECHO                0x00000008U
#define ECHOE               0x00000010U
#define ECHOK               0x00000020U
#define ECHONL              0x00000040U
#define NOFLSH              0x00000080U
#define TOSTOP              0x00000100U
#define ECHOCTL             0x00000200U
#define ECHOPRT             0x00000400U
#define ECHOKE              0x00000800U
#define FLUSHO              0x00001000U
#define PENDIN              0x00004000U
#define IEXTEN              0x00008000U
/** @} */

/**
 * @name    Standard line speeds
 * @{
 */
#define B0                  0U
#define B50                 50U
#define B75                 75U
#define B110                110U
#define B134                134U
#define B150                150U
#define B200                200U
#define B300                300U
#define B600                600U
#define B1200               1200U
#define B1800               1800U
#define B2400               2400U
#define B4800               4800U
#define B9600               9600U
#define B19200              19200U
#define B38400              38400U
#define B57600              57600U
#define B115200             115200U
#define B230400             230400U
#define B460800             460800U
#define B500000             500000U
#define B576000             576000U
#define B921600             921600U
#define B1000000            1000000U
#define B1152000            1152000U
#define B1500000            1500000U
#define B2000000            2000000U
#define B2500000            2500000U
#define B3000000            3000000U
#define B3500000            3500000U
#define B4000000            4000000U
/** @} */

/**
 * @name    Attribute application actions
 * @{
 */
#define TCSANOW             0
#define TCSADRAIN           1
#define TCSAFLUSH           2
/** @} */

/**
 * @name    Queue selectors
 * @{
 */
#define TCIFLUSH            0
#define TCOFLUSH            1
#define TCIOFLUSH           2
/** @} */

/**
 * @name    Flow-control actions
 * @{
 */
#define TCOOFF              0
#define TCOON               1
#define TCIOFF              2
#define TCION               3
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

typedef unsigned char cc_t;
typedef unsigned int  speed_t;
typedef unsigned int  tcflag_t;

struct termios {
  tcflag_t          c_iflag;
  tcflag_t          c_oflag;
  tcflag_t          c_cflag;
  tcflag_t          c_lflag;
  cc_t              c_cc[NCCS];
  speed_t           c_ispeed;
  speed_t           c_ospeed;
};

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  speed_t cfgetispeed(const struct termios *termiosp);
  speed_t cfgetospeed(const struct termios *termiosp);
  int cfsetispeed(struct termios *termiosp, speed_t speed);
  int cfsetospeed(struct termios *termiosp, speed_t speed);
  int tcdrain(int fd);
  int tcflow(int fd, int action);
  int tcflush(int fd, int queue_selector);
  int tcgetattr(int fd, struct termios *termiosp);
  int tcsendbreak(int fd, int duration);
  int tcsetattr(int fd, int optional_actions,
                const struct termios *termiosp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* SYS_TERMIOS_H */

/** @} */
