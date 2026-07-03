# make rules for building an .uf2 image, suitable for use with the bootloader on the RP MCUs

# these make rules should be included _BEFORE_ $(RULESPATH)/rules.mk

# path to the picotool binary, can be overridden
# picotool is distributed as part of the Raspberry Pi Pico SDK
# https://github.com/raspberrypi/pico-sdk
PICOTOOL ?= picotool

# check if picotool binary is available & executable
$(if $(shell $(PICOTOOL)), , $(error picotool not found. Download it from https://github.com/raspberrypi/picotool\
 and either install it in $$PATH or set the PICOTOOL variable to the location))

# build a .uf2 file out of the .elf to use with the bootloader of the RP MCUs
$(BUILDDIR)/$(PROJECT).uf2: $(BUILDDIR)/$(PROJECT).elf
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(PICOTOOL) uf2 convert $(BUILDDIR)/$(PROJECT).elf $(BUILDDIR)/$(PROJECT).uf2
else
	@echo Creating $@
	@$(PICOTOOL) uf2 convert $(BUILDDIR)/$(PROJECT).elf $(BUILDDIR)/$(PROJECT).uf2
endif

# always build the .uf2 as part of the regular build target ("all")
ADDITIONAL_OUTFILES += $(BUILDDIR)/$(PROJECT).uf2
