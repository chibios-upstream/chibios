All the code contained under ./ext is not part of the ChibiOS project and
supplied as-is without any additional warranty by ChibiOS. For ownership and
copyright statements see the license details inside the code.

Some modules may contain changes from the ChibiOS team in order to increase
compatibility or usability with ChibiOS itself.

*** Contents ***

Each archive is unpacked under ./ext, its directory is the one used by the
ChibiOS makefiles.

Archive                     Directory        Version       License
--------------------------  ---------------  ------------  -----------------
fatfs-0.14b_patched.7z      fatfs            R0.14b        FatFs license
FreeRTOS-Kernel-11.3.1.7z   FreeRTOS-Kernel  V11.3.1       MIT
littlefs-2.10.1.7z          littlefs         2.10.1        BSD-3-Clause
lwip-2.1.2.7z               lwip             2.1.2         BSD-3-Clause
threadx-6.5.2.202603.7z     threadx          6.5.2.202603  MIT
wolfssl-4.0.0.7z            wolfssl          4.0.0         GPL-2.0-or-later

- FatFs, by ChaN, https://elm-chan.org/fsw/ff/
  The FatFs license is BSD-style with a single condition, see
  fatfs/LICENSE.txt. The archive contains patches applied by ChibiOS,
  details to be documented. Used through os/various/fatfs_bindings.
- FreeRTOS Kernel, https://github.com/FreeRTOS/FreeRTOS-Kernel
  Sandbox guest kernel, used through os/various/freertos_bindings.
- littlefs, https://github.com/littlefs-project/littlefs
  Used through os/various/littlefs_bindings and the VFS LittleFS driver.
- lwIP, https://savannah.nongnu.org/projects/lwip/
  Used through os/various/lwip_bindings.
- Eclipse ThreadX, https://github.com/eclipse-threadx/threadx
  The archive contains the common directory of the release, its license
  and change log. Sandbox guest kernel, used through
  os/various/threadx_bindings.
- wolfSSL, https://github.com/wolfSSL/wolfssl
  Licensed by wolfSSL Inc. under the GPLv2 or a commercial license, the
  source files allow any later GPL version. Used through
  os/various/wolfssl_bindings.

The Zephyr tree used by the Zephyr sandbox guest, os/various/zephyr_bindings,
is not distributed with ChibiOS, see zephyr_readme.md for how to fetch it.
