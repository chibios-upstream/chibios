# Native regression tests for the actual msh terminal code.
CHIBIOS := ../../../../..
BUILDDIR := build
CC ?= cc
CFLAGS ?= -Og -g -Wall -Wextra -Wundef -Wstrict-prototypes
CPPFLAGS += -Istubs -I$(CHIBIOS)/os/common/utils/include \
            -I../../common/elfexec
SOURCES := terminal.c $(CHIBIOS)/os/common/utils/src/paths.c \
           $(CHIBIOS)/os/common/utils/src/sglob.c
HEADERS := stubs/sbuser.h ../../common/elfexec/elfexec.h \
           $(CHIBIOS)/os/common/utils/include/paths.h \
           $(CHIBIOS)/os/common/utils/include/sglob.h

.PHONY: all check clean
all: $(BUILDDIR)/terminal $(BUILDDIR)/terminal-bounded

# Linux canonical input can hold 4096 bytes including the newline.
$(BUILDDIR)/terminal: CPPFLAGS += -DSHELL_MAX_CANONICAL_LENGTH=4096

$(BUILDDIR)/terminal $(BUILDDIR)/terminal-bounded: $(SOURCES) $(HEADERS) ../main.c Makefile
	@mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

check: all
	$(BUILDDIR)/terminal
	$(BUILDDIR)/terminal-bounded
	python3 terminal.py $(BUILDDIR)/terminal

clean:
	rm -f $(BUILDDIR)/terminal $(BUILDDIR)/terminal-bounded
	@-rmdir $(BUILDDIR) 2>/dev/null
