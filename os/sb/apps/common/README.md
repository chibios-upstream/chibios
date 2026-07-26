# Sandbox command applications

The applications under `os/sb/apps` are small user-space commands for the
ChibiOS sandbox environment. They are distinct from the complete sandbox
firmware examples under `demos/various/SB-CLIENT-*`.

The maintained command set currently includes:

- `cat`, `chedit`, `cmp`, `cp`, `head`, `hexdump`, `ls`, `stat`, and `wc`
  for files.
- `sleep` and `systime` for sandbox timing.
- `msh` as the minimal command loader and interactive shell.
- `sbsh` as the enhanced shell with scripts, quoting, redirection, and
  serialized pipelines.

## Command execution model

A relocatable command is linked at address zero using `ram_sandbox.ld`. The
sandbox ELF loader relocates it into the memory supplied by its caller. The
startup code passes `argc`, `argv`, and `envp` to:

```c
int main(int argc, char *argv[], char *envp[]);
```

`msh` and `sbsh` load external commands into unused memory and call them
through the support code in `elfexec/`. The loaded command remains in the same
sandbox: it shares the sandbox VFS root, descriptors, current directory, and
environment. It is not a separately isolated process.

`sbsh` keeps parsing and glob expansion in fixed-size storage. Glob matches
are copied into a bounded scratch arena directly below the loaded command,
without advancing the shell heap break. This avoids accumulating heap
fragmentation or permanently reducing the space available to later commands.

## `sbsh` syntax

`sbsh` can run interactively, execute one command list, or read a script:

```sh
sbsh
sbsh -c 'echo hello >message'
sbsh startup.sbsh
```

Scripts execute one bounded line at a time and accept LF or CRLF endings.
Commands on a line can be separated with `;`. Single quotes, double quotes,
backslash escapes, comments beginning at a token boundary, `<`, `>`, `>>`,
and `|` are supported.

Until separately spawned sandbox processes and real pipes are available,
`sbsh` implements `|` by running each stage sequentially through temporary
files. A pipeline is therefore non-streaming and should be kept small.
Temporary files are created with exclusive access under `$TMPDIR` (or `/tmp`)
and removed as each stage completes. Stateful builtins such as `cd` and
`exit` are rejected in pipelines.

Commands use the newlib adapters from `os/sb/user` for the supported POSIX
operations. Host-side POSIX calls are currently synchronous, so a blocking
VFS operation blocks the whole sandbox.

## Shared make fragments

- `make/app-arm.mk`: common ARMv7-M sandbox application build.
- `make/app-posix-x86.mk`: native 32-bit POSIX build used for command testing.
- `make/cmdutil.mk`: common error, numeric parsing, and reliable-write helpers.
- `make/elfexec.mk`: adds the relocatable ELF call/return support.
- `make/multi.mk`: front-end for application directories with multiple build
  profiles under `make/`.
- `manifest.mk`: deployable command images and their sandbox paths.
- `stage.mk`: builds and copies the manifest into a sandbox VFS root.

## Aggregate builds

Run the top-level makefile from `os/sb/apps`:

```sh
make            # Build both target families.
make sb         # Copy relocatable sandbox images to build/sb.
make posix      # Copy native test programs to build/posix.
make check      # Build and run the native behavioral tests.
make clean      # Clean applications and remove the aggregate build directory.
```

The `msh` application is sandbox-only and is not part of the POSIX target.
`sbsh` has a native profile for parser and executor tests; native external
commands are spawned only by that test build.
Each application keeps its object and dependency trees locally under its own
`build/` and `.dep/` directories. The aggregate `build/sb` and `build/posix`
directories contain only the final executables for easy deployment.

Each target makefile defines its identity and profile before including a
shared fragment. The ARM fragment accepts these application variables:

- `PROJECT`, `CHIBIOS`, `CONFDIR`, `BUILDDIR`, and `DEPDIR`.
- `SBAPP_OPT`, `SBAPP_LDOPT`, and `SBAPP_LDSCRIPT`.
- `SBAPP_PROCESS_STACKSIZE` and `SBAPP_EXCEPTIONS_STACKSIZE`.
- `SBAPP_CSRC`, `SBAPP_CPPSRC`, `SBAPP_ASMSRC`, and `SBAPP_ASMXSRC`.
- `SBAPP_UDEFS`, `SBAPP_UADEFS`, `SBAPP_UINCDIR`, `SBAPP_ULIBDIR`, and
  `SBAPP_ULIBS`.

Normal make command-line overrides such as `USE_OPT=...` and
`BUILDDIR=...` remain supported.

## Adding a command

1. Add a directory containing `main.c`.
2. Add `make/<command>-rambox-deploy.make` based on an existing command.
3. Include `../common/make/app-arm.mk` from the target makefile.
4. Include `../common/make/multi.mk` from the application `Makefile`.
5. Add the deployable image and destination to `manifest.mk`.
6. Add a native POSIX profile when the command does not require sandbox-only
   APIs.

Native profiles can set `SBAPP_TESTS` to a list of shell tests. Each test
receives the built executable path:

```sh
make -f make/ls-posix-x86-debug.make check
```

Relocatable commands installed for `msh` use the `.elf` suffix. The default
shell path is `/bin`.

From `os/sb/apps`, build and stage the maintained command set with:

```sh
make stage STAGE_ROOT=/path/to/sandbox-root
```

`STAGE_ROOT` is mandatory; the staging makefile never defaults to the host
root directory. Direct use of `common/stage.mk` remains supported.
