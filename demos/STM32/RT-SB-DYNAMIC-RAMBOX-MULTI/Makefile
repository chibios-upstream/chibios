##############################################################################
# Multi-project makefile rules
#

.PHONY: all clean mkfs

SUB_MAKES := $(sort $(wildcard make/*.make))

all:
	@for m in $(SUB_MAKES); do \
		echo; \
		echo "=== Building $$m ==="; \
		$(MAKE) --no-print-directory -f $$m all || exit $$?; \
		echo; \
	done
	@echo

# All targets share the same generated ROMFS source, so generate it once.
mkfs:
	+$(MAKE) --no-print-directory -f $(firstword $(SUB_MAKES)) mkfs

clean:
	@for m in $(SUB_MAKES); do \
		echo "=== Cleaning $$m ==="; \
		$(MAKE) --no-print-directory -f $$m clean || exit $$?; \
		echo; \
	done
#
##############################################################################
