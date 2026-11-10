# ELF32 loader subsystem build.

# Architecture-specific relocation backend, selectable before inclusion.
ELF32ARCH ?= arm32

# Required files.
ELF32SRC := $(CHIBIOS)/os/elf/elf32/src/elf32.c \
            $(CHIBIOS)/os/elf/elf32/arch/$(ELF32ARCH)/elf32arch.c

# Required include directories
ELF32INC := $(CHIBIOS)/os/common/utils/include \
            $(CHIBIOS)/os/elf/elf32/include \
            $(CHIBIOS)/os/elf/elf32/arch/$(ELF32ARCH)

# Shared variables
ALLCSRC += $(ELF32SRC)
ALLINC  += $(ELF32INC)
