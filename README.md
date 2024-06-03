# *boot*sh

A comprehensive POSIX userspace, including a C compiler, in a single tiny (<1MB) executable.

- **Bootstrappable:** *boot*sh contains a variety of builtin commands (including `grep`, `diff`, `sed`, `xzcat`, `sha1sum`, `ninja` and many more) providing a solid POSIX base to bootstrap a full GNU/Linux distribution from source.

- **Self-hosting:** *boot*sh can compile itself from source, thanks to its builtin `cc` command which implements a full C99 compiler.

- **Tiny:** *boot*sh can be compiled into a single ~700KB binary executable in under 10 seconds. With [UPX](https://upx.github.io/) it can be reduced to half that size.

## Build

Just run `./configure && ninja`. The configure script will prompt you for any missing build dependencies. If you don't have `ninja`, you can run `make` to bootstrap with the builtin copy of [samurai](https://github.com/michaelforney/samurai).

## Usage

A [Docker image](https://hub.docker.com/r/davidar/bootsh/tags) is provided to demonstrate how *boot*sh can be used in a self-contained environment.

```sh
docker run --rm -it davidar/bootsh
```

This image contains four files:

- `/bin/sh`: the *boot*sh executable

- `/bin/boot.sh` and `/bin/configure-musl.sh`: shell scripts for bootstrapping the system and building [musl libc](https://musl.libc.org/)

- `/bin/awk`: a [C script](wak.c) (note the `#!/bin/cc -run` interpreter line) which is JIT compiled as needed

Upon running the Docker image, it will bootstrap itself by constructing a minimal root filesystem on top of this.

## POSIX Compliance

*boot*sh implements all of the [mandatory POSIX commands](https://en.wikipedia.org/wiki/List_of_POSIX_commands)
(and a number of optional commands) except for the following:

- system administration: `at`, `batch`, `crontab`, `df`, `logger`, `lp`, `man`, `newgrp`, `ps`
- text/locale processing: `csplit`, `gencat`, `iconv`, `locale`, `localedef`, `pr`, `split`
- interactive tools: `ed`, `mailx`, `mesg`, `unexpand`, `write`
- terminal control: `stty`, `tabs`, `tput`
- miscellaneous: `bc`, `join`, `m4`, `pathchk`, `pax`, `tsort`

## Credits

*boot*sh is based on several MIT/BSD licensed projects:

- The [Debian Almquist shell](http://gondor.apana.org.au/~herbert/dash/)
- Command line utilities from [toybox](http://landley.net/toybox/)
- The MIT-relicensed parts of the [Tiny C Compiler](https://bellard.org/tcc/)
- Version 8.0 of [compiler-rt](https://compiler-rt.llvm.org/) (before it was relicensed from MIT to Apache 2.0)
- `ar` from the [Cosmopolitan](https://github.com/jart/cosmopolitan) project
- [wak](https://github.com/raygard/wak), the one-file implementation of `awk`
- [samurai](https://github.com/michaelforney/samurai), a reimplementation of [Ninja](https://github.com/ninja-build/ninja)
- [musl libc](https://musl.libc.org/)
