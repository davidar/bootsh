# *boot*sh

A comprehensive POSIX userspace, including a C compiler, in a single tiny (<1MB) executable.

- **Bootstrappable:** *boot*sh contains a variety of builtin commands (including `grep`, `diff`, `sed`, `xzcat`, `sha1sum` and many more) providing a solid POSIX base to bootstrap a full GNU/Linux distribution from source.

- **Self-hosting:** *boot*sh can compile itself from source, thanks to its builtin `cc` command which implements a full C99 compiler.

- **Tiny:** *boot*sh can be compiled into a single ~600KB binary executable in under 10 seconds. With [UPX](https://upx.github.io/) it can be reduced to half that size.

## Build

Just run `./configure && make -j$(nproc)`. The configure script will prompt you for any missing build dependencies.

## Usage

A [Docker image](https://hub.docker.com/r/davidar/bootsh/tags) is provided to demonstrate how *boot*sh can be used in a self-contained environment.

```sh
docker run --rm -it davidar/bootsh
```

This image contains four files:

- `/bin/sh`: the *boot*sh executable

- `/bin/boot.sh`: a [shell script](boot.sh) for bootstrapping the system by downloading and building [musl libc](https://musl.libc.org/) and GNU Make from source

- `/bin/awk`: a [C script](wak.c) (note the `#!/bin/cc -run` interpreter line) which is JIT compiled as needed

- `/tmp/Makefile`: which functions as a rudimentary package manager for downloading and compiling [further source packages](Makefile.packages)

Upon running the Docker image, it will bootstrap itself by constructing a minimal root filesystem and compiling additional build tools.
Run `make` once the bootstrap process has completed to list additional source packages available for installation.

## Credits

*boot*sh is based on several MIT/BSD licensed projects:

- The [Debian Almquist shell](http://gondor.apana.org.au/~herbert/dash/)
- Command line utilities from [toybox](http://landley.net/toybox/)
- The MIT-relicensed parts of the [Tiny C Compiler](https://bellard.org/tcc/)
- Version 8.0 of [compiler-rt](https://compiler-rt.llvm.org/) (before it was relicensed from MIT to Apache 2.0)
- `ar` from the [Cosmopolitan](https://github.com/jart/cosmopolitan) project
- [wak](https://github.com/raygard/wak), the one-file implementation of `awk`
- [musl libc](https://musl.libc.org/)
