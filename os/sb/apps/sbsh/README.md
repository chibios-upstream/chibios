# sbsh

`sbsh` is the enhanced command shell for ChibiOS sandboxes. It is a separate
project from the intentionally minimal `msh`, so new shell features do not add
complexity or compatibility constraints to the original command loader.

The initial implementation focuses on predictable resource use and features
that work before separately spawned sandbox processes are available:

- interactive command entry and history;
- sequential command lists and script files;
- quoting, escaping, comments, and bounded glob expansion;
- input, output, and append redirection;
- non-streaming pipelines implemented with temporary files.

## Invocation

```text
sbsh
sbsh -c command
sbsh script
```

With no operands, `sbsh` displays a prompt and reads commands interactively.
The `PROMPT` environment variable overrides the default `sbsh> ` prompt.
Up-arrow and down-arrow recall the eight most recent non-empty lines,
Backspace edits the current line, and Ctrl-D on an empty line exits.

The `-c` form executes one command list without displaying the interactive
banner. The script form reads and executes one line at a time. Both LF and
CRLF script line endings are accepted.

Scripts continue after ordinary nonzero command statuses. A syntax error,
overlong line, or read error stops the script. The shell returns the status of
the last command that ran, or status 2 for a syntax or usage error.

## Command syntax

Commands can be separated with `;`:

```text
echo first; echo second
```

Single quotes preserve all enclosed characters. Double quotes and backslash
escapes prevent spaces and operators from separating an argument. Quoted or
escaped `*` and `?` characters remain literal, including when combined with
unquoted wildcards in the same word.

An unquoted `#` begins a comment when it occurs where a new token would start:

```text
echo value # this is a comment
echo value#this-is-not-a-comment
```

The supported redirections are:

```text
command <input
command >output
command >>output
```

Only one input and one output redirection are accepted per command. Input
redirection is allowed on the first pipeline stage, and output redirection is
allowed on the last stage.

## Globbing

Unquoted `*` and `?` are expanded against directory entries in the final path
component. A pattern that has no matches is passed to the command unchanged.
Expansion order is the directory enumeration order and is not sorted.

The initial implementation does not recursively expand wildcard directory
components. For example, `dir/*.txt` is supported, while `dir*/file.txt` is
treated as an unmatched literal pattern.

## Builtins

The initial builtins are:

- `cd <path>`
- `echo [argument ...]`
- `env`
- `exit [status]`, where status is from 0 through 255
- `help`
- `mkdir <directory>`
- `mv <old> <new>`
- `path`
- `pwd`
- `rm <file>`
- `rmdir <directory>`

`path` prints the current `PATH`. For sandbox execution, `PATH` entries must
be absolute. If `PATH` is unset, `/bin` is used. Commands found through
`PATH` receive the `.elf` extension automatically; command names containing
`/` are executed exactly as written.

## Temporary-file pipelines

The `|` operator is currently implemented as sequential execution through
temporary files:

```text
producer | filter | consumer
```

Each stage runs to completion, its output file is rewound, and the next stage
then consumes it. Files are created exclusively with mode 0600 under
`$TMPDIR`, or `/tmp` when `TMPDIR` is unset, and are removed as soon as they
are no longer needed.

These pipelines are deliberately not streaming. They are unsuitable for
unbounded producers, commands that require simultaneous interaction, or data
larger than the available temporary filesystem. `cd` and `exit` are rejected
inside pipelines because their stateful behavior would be misleading.

This executor can later be replaced with `posix_spawn()` and real pipes
without changing the parser or command syntax.

## Resource limits

The parser and executor use fixed-size tables:

- 127 input characters plus the terminating zero;
- 32 parsed words and 64 words after glob expansion;
- eight commands, eight command lists, and eight redirections per line;
- a 4 KiB bounded arena for copied glob matches.

On the sandbox target, directory enumeration uses a fixed aligned buffer and
does not allocate a `DIR` object. The glob arena aliases currently unused heap
space without advancing the shell break; external ELF loading starts
immediately after the live matches. The target image therefore does not link
`malloc()` or `free()`, avoiding persistent heap fragmentation between
commands.

## Current omissions

The initial shell intentionally has no variable expansion, command
substitution, environment assignments, `&&`, `||`, background jobs, job
control, or streamed pipes. Scripts are selected explicitly with
`sbsh script`; shebang dispatch and a `source` builtin are not yet provided.

## Building and testing

From this directory:

```sh
make
make check
make clean
```

`make` builds the native test executable plus ARM debug and deploy images.
The individual profiles are:

```sh
make -f make/sbsh-posix-x86-debug.make check
make -f make/sbsh-rambox-debug.make
make -f make/sbsh-rambox-deploy.make
```

From the parent `os/sb/apps` directory, the aggregate targets are:

```sh
make posix-sbsh
make check-sbsh
make sb-sbsh
```

The aggregate build copies only the final executable to `build/posix/sbsh`
or `build/sb/sbsh.elf`; each profile keeps its intermediate files inside the
`sbsh` project.
