All the code contained under ./os/common/ports are RTOS ports compatible
with both RT and NIL. The code is placed under ./os/common in order to
prevent code duplication and disalignments.

Note (2026-06): the port-dependent part of thread creation is now made
of two inline functions defined in each port's chcore.h, operating on a
pointer to the port-owned struct port_context:

- port_setup_context_base(ctxp): initializes only the context fields
  which must be valid also for thread objects representing
  already-running execution flows (the boot thread of each instance);
  empty in ports with no such fields. Invoked by the kernel on every
  thread object and by port_setup_context() itself.
- port_setup_context(ctxp, wbase, wtop, pf, arg): the full context
  initialization for created threads.

They replace the former PORT_SETUP_CONTEXT() macro, which is no longer
used by RT or NIL: out-of-tree ports must define both functions (see
./templates/chcore.h for the reference shapes).
