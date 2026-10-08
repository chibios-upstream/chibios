# Deterministic native tests of the generated driver's input/read code.
CC ?= cc
CFLAGS ?= -Og -g -Wall -Wextra -Wundef -Wstrict-prototypes -Werror
BUILDDIR := build
CHIBIOS := ../../../../../..
POSIXINC := $(CHIBIOS)/os/common/posix/include
POSIXHEADERS := $(POSIXINC)/sys/termios.h $(POSIXINC)/sys/ioctl.h

.PHONY: all check clean
all: $(BUILDDIR)/input $(BUILDDIR)/drain $(BUILDDIR)/drain-release

# Compile the production functions, not a second implementation. Only RT
# scheduling/time and the enclosing driver fixture are substituted by input.c.
$(BUILDDIR)/input_impl.inc: ../hal_posix_tty_sio.c Makefile
	@mkdir -p $(BUILDDIR)
	awk '/^static .*__ptty_(input_(init|used_i|advance|is_boundary_i|set_boundary_i|clear_boundary_i|wakeup_i|reset_i|append_i|commit_i|read_i)|attributes_(default|valid)|vtime_interval|read(_[[:alnum:]_]+)?|select_read|apply_attributes_i|tty_get_impl)\(/ {copy=1} copy {print} copy && (/^}/ || /^static .*;$$/) {copy=0}' $< > $@

$(BUILDDIR)/input: input.c $(BUILDDIR)/input_impl.inc $(POSIXHEADERS)
	$(CC) $(CFLAGS) -I$(BUILDDIR) -I$(POSIXINC) input.c $(LDFLAGS) -o $@

$(BUILDDIR)/drain_impl.inc: ../hal_posix_tty_sio.c Makefile
	@mkdir -p $(BUILDDIR)
	awk '/^static .*__ptty_(output_pending_i|update_tx_i|reset_drain_i|resume_drain_i|drain)\(/ {copy=1} copy {print} copy && (/^}/ || /^static .*;$$/) {copy=0}' $< > $@

$(BUILDDIR)/drain: drain.c $(BUILDDIR)/drain_impl.inc
	$(CC) $(CFLAGS) -pthread -I$(BUILDDIR) drain.c $(LDFLAGS) -o $@

$(BUILDDIR)/drain-release: drain.c $(BUILDDIR)/drain_impl.inc
	$(CC) $(CFLAGS) -pthread -DPTTY_TEST_RELEASE -I$(BUILDDIR) drain.c $(LDFLAGS) -o $@

.PHONY: intervals
intervals: $(BUILDDIR)/input_impl.inc
	@set -e; for bits in 16 32; do \
	  defs=; if test $$bits = 32; then defs=-DPTTY_TEST_TIME32; fi; \
	  for frequency in 10 1003 32768 1000000000; do \
	    $(CC) $(CFLAGS) -I$(BUILDDIR) -I$(POSIXINC) $$defs \
	      -DCH_CFG_ST_FREQUENCY=$$frequency input.c $(LDFLAGS) \
	      -o $(BUILDDIR)/intervals; \
	    $(BUILDDIR)/intervals --intervals; \
	  done; \
	done

check: all intervals
	$(BUILDDIR)/input
	$(BUILDDIR)/drain
	$(BUILDDIR)/drain-release

clean:
	rm -f $(BUILDDIR)/input $(BUILDDIR)/input_impl.inc $(BUILDDIR)/intervals
	rm -f $(BUILDDIR)/drain $(BUILDDIR)/drain-release $(BUILDDIR)/drain_impl.inc
	@-rmdir $(BUILDDIR) 2>/dev/null
