This directory contains the ChibiOS bindings for the lwIP TCP/IP stack:
http://savannah.nongnu.org/projects/lwip

Layout
------

- `lwip_common.mk`
  XHAL binding source list and include paths.

- `lwip_xhal.mk`
  Binding for the XHAL ETH driver API.

- `lwipthread.[ch]`
  Wrapper thread used to bring up lwIP, manage link state, dispatch
  incoming frames and expose a uniform integration API to applications.

- `lwipthread_xhal.c`
  XHAL ETH low-level adapter.

- `arch/`
  ChibiOS-specific lwIP architecture layer.

Usage
-----

1. Place lwIP under `ext/lwip`.
2. Include the XHAL binding makefile:
   - `$(CHIBIOS)/os/xhal/lib/complex/lwip_bindings/lwip_xhal.mk`
3. Provide the usual application configuration files:
   - `lwipopts.h`
   - `chconf.h`
   - `xhalconf.h`
   - MCU/port-specific configuration headers as needed
4. Call `lwipInit()` from the application, then start higher-level lwIP
   services such as `httpd_init()`.

Notes
-----

- Ethernet driver configuration is owned by the application/driver, not by
  `lwipthread`. The effective MAC address is taken from the selected/applied
  driver configuration.

- The ETH driver must be enabled explicitly and currently requires:
  - `HAL_USE_ETH == TRUE`
  - `ETH_USE_SYNCHRONIZATION == TRUE`
  - `ETH_USE_EVENTS == TRUE`
  - `CH_CFG_USE_EVENTS == TRUE`
  - `CH_CFG_USE_SEMAPHORES == TRUE`
  - `CH_CFG_USE_MAILBOXES == TRUE`
  - `CH_CFG_USE_DYNAMIC == TRUE`
  - A heap, or memory pools with `CH_LWIP_USE_MEM_POOLS == TRUE`

- The XHAL backend currently supports the normal copy path. Zero-copy support
  is optional and depends on the selected ETH implementation.

- This implementation is independent from the classic HAL lwIP bindings under
  `os/various/lwip_bindings`.
