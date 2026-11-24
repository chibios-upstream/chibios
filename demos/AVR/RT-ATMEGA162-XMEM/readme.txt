*****************************************************************************
** ChibiOS/RT external memory demo for Atmel AVR ATmega162.                **
*****************************************************************************

** TARGET **

The demo runs on a minimal ATmega162 board at 16 MHz (ATMEGA162_MINIMAL)
with a 32 KiB SRAM on the external memory interface:
- AD7:0 (PA7:0) to the SRAM data lines and to an address latch (74HC573)
  enabled by ALE (PE1), the latch outputs drive the SRAM A7:0.
- A14:8 (PC6:0) to the SRAM A14:8, PC7 is not used by the interface.
- WR (PD6) to the SRAM write enable, RD (PD7) to the SRAM output enable,
  the SRAM chip select tied active.

** The Demo **

The external memory interface is enabled at start, with one wait state, and
the JTAG interface is disabled (JTD) because it shares PC7:4. The external
memory is mapped after the internal SRAM. A15 is not connected, so the
addresses from 0x8000 to 0x84FF reach the first 1280 bytes of the SRAM and
the region from 0x0500 to 0x84FF is the whole 32 KiB. It is the kernel core
memory (CH_CFG_MEMCORE_SIZE is zero, the region is set by the __heap_base__
and __heap_end__ symbols in the Makefile), so the heap allocates from the
external SRAM.

The region is tested first, a pattern derived from each address then its
complement are written and read back. A failure is reported and the LED
then blinks quickly.

Then, in a loop, three worker threads are created from the heap, so their
stacks are in the external SRAM. Each one fills a buffer on its stack and
exits with its checksum, the main thread waits for them, which releases
their memory, and prints the checksums and the heap status. The LED on PB0
toggles every 500ms.

The output is on USART0, PD0 (RXD0) and PD1 (TXD0), at 115200 baud 8N1
using the SIO driver.

** Build Procedure **

The demo was built using the GCC AVR toolchain.

** Notes **

The wait states are set in xmem_init(), check the SRAM access time against
the external memory timing in the ATmega162 datasheet. With a smaller
SRAM, change __heap_end__ in the Makefile and the XMM2:0 setting.

The demo runs natively, the fuses must select the external 16 MHz crystal
(CKSEL and SUT, see the ATmega162 datasheet). The CKDIV8 fuse, programmed
when shipped, must be unprogrammed: it divides the clock by 8 and the
serial output is then unreadable. The JTAGEN fuse, also programmed when
shipped, can stay: the demo disables the JTAG interface, which then cannot
be used for debugging. The programmer settings are in the Makefile.
