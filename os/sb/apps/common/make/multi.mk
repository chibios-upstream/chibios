##############################################################################
# Multi-profile application rules.
#

SUB_MAKES := $(sort $(wildcard make/*.make))
CHECK_MAKES := $(sort $(wildcard make/*-posix-*.make))

.PHONY: all check clean

all:
	@for m in $(SUB_MAKES); do \
		echo; \
		echo "=== Building $$m ==="; \
		$(MAKE) --no-print-directory -f $$m all || exit $$?; \
		echo; \
	done
	@echo

check:
	@for m in $(CHECK_MAKES); do \
		echo; \
		echo "=== Checking $$m ==="; \
		$(MAKE) --no-print-directory -f $$m check || exit $$?; \
		echo; \
	done
	@echo

clean:
	@for m in $(SUB_MAKES); do \
		echo "=== Cleaning $$m ==="; \
		$(MAKE) --no-print-directory -f $$m clean || exit $$?; \
		echo; \
	done

#
##############################################################################
