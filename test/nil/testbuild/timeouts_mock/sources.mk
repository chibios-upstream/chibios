# Parse the real NIL source list using this fixture's enabled modules.
include $(CHIBIOS)/os/nil/nil.mk

ifneq ($(sort $(notdir $(KERNSRC))),ch.c chevt.c chmsg.c chsem.c)
$(error Unexpected NIL kernel source list: $(KERNSRC))
endif

.PHONY: check
check:
	@:
