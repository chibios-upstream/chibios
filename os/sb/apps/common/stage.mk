# Build and stage the deployable sandbox command set.
#
# Invoke from os/sb/apps:
#   make -f common/stage.mk STAGE_ROOT=/path/to/sandbox-root stage

APPS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

include $(APPS_ROOT)/common/manifest.mk

STAGE_BUILD_ROOT ?= $(APPS_ROOT)/build/stage
STAGE_DEP_ROOT   ?= $(APPS_ROOT)/.dep/stage

.PHONY: stage check-stage-root

stage: check-stage-root $(addprefix stage-,$(SBAPP_DEPLOY_APPS))

check-stage-root:
	@if [ -z "$(STAGE_ROOT)" ]; then \
		echo "STAGE_ROOT must name the sandbox VFS root"; \
		exit 2; \
	fi

define SBAPP_STAGE_RULE
.PHONY: stage-$(1)

stage-$(1): check-stage-root
	+$$(MAKE) -C $$(APPS_ROOT)/$(1) \
	          -f $$(SBAPP_DEPLOY_$(1)_MAKEFILE) \
	          BUILDDIR="$$(STAGE_BUILD_ROOT)/$(1)" \
	          DEPDIR="$$(STAGE_DEP_ROOT)/$(1)" all
	@mkdir -p "$$(STAGE_ROOT)/$$(dir $$(SBAPP_DEPLOY_$(1)_PATH))"
	@cp "$$(STAGE_BUILD_ROOT)/$(1)/$$(SBAPP_DEPLOY_$(1)_ARTIFACT)" \
	    "$$(STAGE_ROOT)/$$(SBAPP_DEPLOY_$(1)_PATH)"
endef

$(foreach app,$(SBAPP_DEPLOY_APPS),$(eval $(call SBAPP_STAGE_RULE,$(app))))
