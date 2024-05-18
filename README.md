# *boot*sh

A comprehensive POSIX userspace, including a C compiler, in a single tiny (<1MB) executable.

- **Bootstrappable:** *boot*sh contains a variety of builtin commands (including `grep`, `diff`, `sed`, `xzcat`, `sha1sum` and many more) providing a solid POSIX base to bootstrap a full GNU/Linux distribution from source.

- **Self-hosting:** *boot*sh can compile itself from source, thanks to its builtin `cc` command which implements a full C99 compiler.

- **Tiny:** *boot*sh can be compiled into a single ~600KB binary executable in under 10 seconds. With [UPX](https://upx.github.io/) it can be reduced to half that size.

## Build

Just run `./configure && make -j$(nproc)`

## Usage

A [Docker image](https://hub.docker.com/r/davidar/bootsh/tags) is provided to demonstrate how *boot*sh can be used in a self-contained environment.

```sh
docker run --rm -it davidar/bootsh
```

This image contains four files:

- `/bin/sh`: the *boot*sh executable

- `/bin/boot.sh`: a [shell script](boot.sh) for bootstrapping the system by downloading and building [musl libc](https://musl.libc.org/) and GNU Make from source

- `/tmp/Makefile`: which functions as a rudimentary package manager for downloading and compiling [further source packages](Makefile.packages)

- `/bin/awk`: a [C script](wak.c) (note the `#!/bin/cc -run` interpreter line) which is JIT compiled as needed
