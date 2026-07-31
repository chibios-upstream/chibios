# RT SMP simulator test

This project exercises the two-core POSIX simulator independently from the
single-core RT test application.

Build and run the complete SMP smoke test:

```sh
make
./build/ch-smp
```

Focused modes are also available:

```sh
./build/ch-smp --startup-only
./build/ch-smp --lock-stress-only
./build/ch-smp --ipi-stress-only
```

The test covers instance startup, shared kernel-lock contention, inter-core
thread wakeups, masked and coalesced IPIs, signal-frame unwinding, and remote
priority changes.
