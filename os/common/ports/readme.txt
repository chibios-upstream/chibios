All the code contained under ./os/common/ports are RTOS ports compatible
with both RT and NIL. The code is placed under ./os/common in order to
prevent code duplication and disalignments.

Note (2026-06): the port-dependent part of thread creation is now the
inline function port_setup_context(ctxp, wbase, wtop, pf, arg) defined
in each port's chcore.h, operating on a pointer to the port-owned
struct port_context. It replaces the former PORT_SETUP_CONTEXT() macro,
which is no longer used by RT or NIL: out-of-tree ports must define the
function instead (see ./templates/chcore.h for the reference shape).
