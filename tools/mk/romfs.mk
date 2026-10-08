##############################################################################
# Build and embed the deployable sandbox applications as a /bin ROMFS.
#
# Include at the end of a project makefile defining CHIBIOS, then run mkfs
# before building the firmware. autobuild.mk picks up the generated files
# under ./source on the next make invocation. Normal builds and clean do not
# regenerate or remove the image.
#

ROMFSDIR ?= ./source
SBAPPS   ?= $(CHIBIOS)/os/sb/apps
MKROMFS  ?= $(CHIBIOS)/tools/mkromfs/mkromfs.sh

.PHONY: mkfs

# Always revisit the application makefiles to pick up source/header changes.
# Fresh staging prevents removed manifest entries from surviving in the image.
# Generate both files before installing them, preserving unchanged timestamps.
mkfs:
	@mkdir -p "$(ROMFSDIR)"
	+@stage_dir=$$(mktemp -d "$(abspath $(ROMFSDIR))/.romfs.XXXXXX") || exit $$?; \
	  trap 'rm -rf -- "$$stage_dir"' EXIT HUP INT TERM; \
	  $(MAKE) -C "$(SBAPPS)" -f common/stage.mk \
	          STAGE_ROOT="$$stage_dir" stage || exit $$?; \
	  sh "$(MKROMFS)" "$$stage_dir/bin" "$$stage_dir/generated" || exit $$?; \
	  for file in bin_romfs.h bin_romfs.c; do \
	    cmp -s "$$stage_dir/generated/$$file" "$(ROMFSDIR)/$$file" || \
	      mv -f -- "$$stage_dir/generated/$$file" "$(ROMFSDIR)/$$file" || exit $$?; \
	  done

#
##############################################################################
