# Deterministic native tests of the generated driver's input/read code.
CC ?= cc
CFLAGS ?= -Og -g -Wall -Wextra -Wundef -Wstrict-prototypes -Werror
BUILDDIR := build

.PHONY: all check clean
all: $(BUILDDIR)/input

# Compile the production functions, not a second implementation. Only RT
# scheduling/time and the enclosing driver fixture are substituted by input.c.
$(BUILDDIR)/input_impl.inc: ../hal_posix_tty_sio.c Makefile
	@mkdir -p $(BUILDDIR)
	awk '/^static .*__ptty_(input_(init|used_i|advance|is_boundary_i|set_boundary_i|clear_boundary_i|wakeup_i|reset_i|append_i|commit_i)|attributes_valid|read|apply_attributes_i|tty_get_impl)\(/ {copy=1} copy {print} copy && /^}/ {copy=0}' $< > $@

$(BUILDDIR)/input: input.c $(BUILDDIR)/input_impl.inc
	$(CC) $(CFLAGS) -I$(BUILDDIR) input.c $(LDFLAGS) -o $@

check: all
	$(BUILDDIR)/input

clean:
	rm -f $(BUILDDIR)/input $(BUILDDIR)/input_impl.inc
	@-rmdir $(BUILDDIR) 2>/dev/null
