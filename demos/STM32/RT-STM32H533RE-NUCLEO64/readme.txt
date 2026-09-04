This ST-LINK VCP (USART2, 115200 8N1) and the RT/oslib test suites, the latter
triggered by pressing the blue USER button.

The board is an ST NUCLEO-H533RE (STM32H533RET6, Cortex-M33 @250MHz).

To build:

    make

Flash the resulting build/ch.elf with STM32CubeProgrammer or pyOCD, then open
the ST-LINK VCP serial port at 115200 8N1 for the test output.
