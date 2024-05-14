# *boot*sh

A comprehensive POSIX userspace, including a C compiler, in a single tiny (<1MB) executable.

- **Bootstrappable:** *boot*sh contains a variety of builtin commands (including `grep`, `diff`, `sed`, `xzcat`, `sha1sum` and many more) providing a solid POSIX base to bootstrap a full GNU/Linux distribution from source.

- **Self-hosting:** *boot*sh can compile itself from source, thanks to its builtin `cc` command which implements a full C99 compiler.

- **Tiny:** *boot*sh can be compiled into a single 620KiB binary executable in less than 5 seconds.
