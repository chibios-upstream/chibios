# Native tests of the actual OOP stream implementations.
CC ?= cc
CFLAGS ?= -Og -g -Wall -Wextra -Wundef -Wstrict-prototypes
CPPFLAGS += -DOOP_USE_NOTHING -I../include -I../../portability/GCC \
            -include ccportab.h
BUILDDIR := build
SOURCES := nullstreams.c ../src/oop_nullstreams.c ../src/oop_base_object.c
HEADERS := ../include/oop_nullstreams.h ../include/oop_base_object.h \
           ../include/oop_base_interface.h ../include/oop_sequential_stream.h \
           ../../portability/GCC/ccportab.h

.PHONY: all check clean
all: $(BUILDDIR)/nullstreams

$(BUILDDIR)/nullstreams: $(SOURCES) $(HEADERS) Makefile
	@mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

check: all
	$(BUILDDIR)/nullstreams

clean:
	rm -f $(BUILDDIR)/nullstreams
	@-rmdir $(BUILDDIR) 2>/dev/null
