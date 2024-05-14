# *boot*sh

A comprehensive POSIX userspace, including a C compiler, in a single tiny (<1MB) executable.

- **Bootstrappable:** *boot*sh contains a variety of builtin commands (including `grep`, `diff`, `sed`, `xzcat`, `sha1sum` and many more) providing a solid POSIX base to bootstrap a full GNU/Linux distribution from source.

- **Self-hosting:** *boot*sh can compile itself from source, thanks to its builtin `cc` command which implements a full C99 compiler.

- **Tiny:** *boot*sh can be compiled into a single 620KiB binary executable in less than 5 seconds.

  With [UPX](https://upx.github.io/) it can be compressed down to a 305KiB executable.

## Usage

A [Docker image](https://hub.docker.com/r/davidar/bootsh/tags) is provided to demonstrate how *boot*sh can be used.

```sh
docker run --rm -it davidar/bootsh
```

This image contains four files:

- `/bin/sh`: the *boot*sh executable

- `/root/boot.sh`: a shell script for bootstrapping the system by downloading and building [musl libc](https://musl.libc.org/) and GNU Make from source

- `/root/Makefile`: which functions as a rudimentary package manager for downloading and compiling further source packages

- `/root/wak.c`: a [single-file C implementation of AWK](https://github.com/raygard/wak), which is compiled during the system bootstrap as it's required to configure GNU Make
