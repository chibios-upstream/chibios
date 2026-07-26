##############################################################################
# Aggregate sandbox userspace build.
#

APPS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

override BUILD_ROOT := $(APPS_ROOT)/build
SB_BUILD_ROOT       := $(BUILD_ROOT)/sb
POSIX_BUILD_ROOT    := $(BUILD_ROOT)/posix

include $(APPS_ROOT)/common/manifest.mk

.PHONY: all sb posix check stage clean

all: sb posix

sb: $(addprefix sb-,$(SBAPP_DEPLOY_APPS))

posix: $(addprefix posix-,$(SBAPP_POSIX_APPS))

check: $(addprefix check-,$(SBAPP_POSIX_APPS))

stage:
	+$(MAKE) -f $(APPS_ROOT)/common/stage.mk \
	         STAGE_ROOT="$(STAGE_ROOT)" \
	         STAGE_BUILD_ROOT="$(SB_BUILD_ROOT)" \
	         STAGE_DEP_ROOT="$(SB_BUILD_ROOT)/.dep" stage

clean:
	@if [ "$(BUILD_ROOT)" != "$(APPS_ROOT)/build" ]; then \
		echo "Refusing to remove unexpected BUILD_ROOT: $(BUILD_ROOT)"; \
		exit 2; \
	fi
	@rm -rf -- "$(BUILD_ROOT)"

define SBAPP_SB_RULE
.PHONY: sb-$(1)

sb-$(1):
	+$$(MAKE) -C $$(APPS_ROOT)/$(1) \
	          -f $$(SBAPP_DEPLOY_$(1)_MAKEFILE) \
	          BUILDDIR="$$(SB_BUILD_ROOT)/$(1)" \
	          DEPDIR="$$(SB_BUILD_ROOT)/.dep/$(1)" all
endef

define SBAPP_POSIX_RULE
.PHONY: posix-$(1) check-$(1)

posix-$(1):
	+$$(MAKE) -C $$(APPS_ROOT)/$(1) \
	          -f make/$(1)-posix-x86-debug.make \
	          BUILDDIR="$$(POSIX_BUILD_ROOT)/$(1)" \
	          DEPDIR="$$(POSIX_BUILD_ROOT)/.dep/$(1)" all

check-$(1):
	+$$(MAKE) -C $$(APPS_ROOT)/$(1) \
	          -f make/$(1)-posix-x86-debug.make \
	          BUILDDIR="$$(POSIX_BUILD_ROOT)/$(1)" \
	          DEPDIR="$$(POSIX_BUILD_ROOT)/.dep/$(1)" check
endef

$(foreach app,$(SBAPP_DEPLOY_APPS),$(eval $(call SBAPP_SB_RULE,$(app))))
$(foreach app,$(SBAPP_POSIX_APPS),$(eval $(call SBAPP_POSIX_RULE,$(app))))

#
##############################################################################
