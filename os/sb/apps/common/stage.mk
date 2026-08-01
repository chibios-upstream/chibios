# Build and stage the deployable sandbox command set.
#
# Invoke from os/sb/apps:
#   make -f common/stage.mk STAGE_ROOT=/path/to/sandbox-root stage

APPS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

include $(APPS_ROOT)/common/manifest.mk

STAGE_BUILD_ROOT ?= $(APPS_ROOT)/build/sb

.PHONY: stage build-sb check-stage-root

stage: check-stage-root $(addprefix stage-,$(SBAPP_DEPLOY_APPS))

build-sb:
	+$(MAKE) -C $(APPS_ROOT) sb

check-stage-root:
	@if [ -z "$(STAGE_ROOT)" ]; then \
		echo "STAGE_ROOT must name the sandbox VFS root"; \
		exit 2; \
	fi

define SBAPP_STAGE_RULE
.PHONY: stage-$(1)

stage-$(1): check-stage-root build-sb
	@mkdir -p "$$(STAGE_ROOT)/$$(dir $$(SBAPP_DEPLOY_$(1)_PATH))"
	@cp "$$(STAGE_BUILD_ROOT)/$$(SBAPP_DEPLOY_$(1)_ARTIFACT)" \
	    "$$(STAGE_ROOT)/$$(SBAPP_DEPLOY_$(1)_PATH)"
endef

$(foreach app,$(SBAPP_DEPLOY_APPS),$(eval $(call SBAPP_STAGE_RULE,$(app))))
