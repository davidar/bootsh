FROM scratch AS base

ARG ROOTFS
COPY $ROOTFS/bin/sh /bin/sh

SHELL ["/bin/sh", "-c"]
CMD ["/bin/sh"]

COPY lib/musl /src/musl
COPY build-musl.sh /src/musl/build.sh
WORKDIR /src/musl
RUN ./build.sh

RUN ln -s / /usr && ln -s /lib /lib/x86_64-linux-gnu && ln -s /lib /lib64 && mkdir /tmp

COPY test/ar.c /src/ar.c
COPY test/cosmopolitan /src/cosmopolitan

RUN LIBC=$(find /src/musl/obj/src -name \*.o) && \
    cc -I/src/cosmopolitan -c /src/ar.c -o /tmp/ar.o && \
    cc -o /tmp/ar -nostdinc -nostdlib /lib/crt1.o /lib/crti.o /tmp/ar.o $LIBC /lib/crtn.o && \
    /tmp/ar rcs /lib/libc.a $LIBC && \
    rm /tmp/ar /tmp/ar.o && \
    cp /src/ar.c /bin/ar

COPY test/test.c /src/test.c
WORKDIR /src
RUN cc -v && cc -run test.c foo bar baz

RUN for cmd in `sh -c toybox` cc; do \
        printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /bin/$cmd; \
        chmod +x /bin/$cmd; \
    done

COPY test/sbang.sh /bin/sbang
COPY test/wak.c /bin/awk
COPY test/muxzcat.c /bin/xzcat

COPY test/sed.c /bin/sed
COPY test/tar.c /bin/tar
COPY test/*.flags.h /bin/
COPY test/libtoys /lib/libtoys

ADD https://ftp.gnu.org/gnu/make/make-4.4.tar.gz /src/make-4.4.tar.gz
WORKDIR /src
RUN tar -xf make-4.4.tar.gz
WORKDIR /src/make-4.4
RUN ./configure --disable-dependency-tracking LD=cc && \
    ./build.sh && ./make && ./make install

ADD https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz /src/bash-5.2.21.tar.gz
WORKDIR /src
RUN tar -xf bash-5.2.21.tar.gz
WORKDIR /src/bash-5.2.21
RUN ./configure --prefix=/ --without-bash-malloc LD=cc && make && make install

COPY test/cc-tests /src/cc-tests
WORKDIR /src/cc-tests
RUN make


FROM base AS bootstrap

COPY src /src/bootsh/src
COPY lib /src/bootsh/lib
COPY Makefile toybox.config config.h /src/bootsh/
WORKDIR /src/bootsh
RUN make bootstrap

COPY test/passwd.txt /etc/passwd
COPY test/group.txt /etc/group
RUN chmod 777 /tmp && chown -R user:user /src/bootsh/lib/toybox
USER user
ENV USER=user
WORKDIR /src/bootsh/lib/toybox
RUN TEST_HOST=1 VERBOSE=all scripts/test.sh


FROM base AS sabotage-stage0

RUN mv /bin/sed /bin/sed.c && cc -I/lib -o /bin/sed /bin/sed.c

# ADD https://www.cpan.org/src/5.0/perl-5.38.2.tar.xz /src/perl-5.38.2.tar.xz
# WORKDIR /src
# RUN tar -xf perl-5.38.2.tar.xz
# WORKDIR /src/perl-5.38.2
# COPY test/perl5.patch /tmp/perl5.patch
# RUN patch -p1 < /tmp/perl5.patch && \
#     ./Configure -des -Uusenm -Uusedl -DEBUGGING=both && \
#     make -j$(nproc) && \
#     make install

ADD https://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.xz /src/gawk-5.3.0.tar.xz
WORKDIR /src
RUN tar -xf gawk-5.3.0.tar.xz
WORKDIR /src/gawk-5.3.0
RUN ./configure --disable-shared LD=cc && \
    make && \
    make install

ADD https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.xz /src/binutils-2.39.tar.xz
WORKDIR /src
RUN tar -xf binutils-2.39.tar.xz
WORKDIR /src/binutils-2.39
RUN ./configure --disable-gprofng CFLAGS='-O2 -D__LITTLE_ENDIAN__=1' && \
    make MAKEINFO=true && \
    make MAKEINFO=true install


## set path to builddir (where you want your chroot).
ENV SABOTAGE_BUILDDIR="/tmp/sabotage"

## set your arch, or try `uname -m`
#ENV A=i486
ENV A=x86_64
#ENV A=mips
#ENV A=powerpc
#ENV A=arm
# arch specific flags to pass to the gcc build
# your crosscompiler should be configured equivalently
# if not provided, will be derived from the compiler config
#ENV GCC_ARCH_CONFIG_FLAGS="--with-float=soft --with-fpu=vfp"

ENV ARCH=$A

## set the compiler environment
ENV CC=cc
ENV HOSTCC=cc

## set your core count for faster builds, this is passed as -jXXX to make
## stage0 builds in less than 2minutes using -j9 on an AMD 8core
ENV MAKE_THREADS=16

# SCP_MIRRORS is a newline-separated list where the default download template
# tries to download source tarballs from if set (before doing regular download).
# first %s passed is the tarball filename, second %s the local destination.
# it's very flexible, you can even call a custom script that does the download
# in a different way. if using scp, the used host key must allow login without
# interactive password.
# note that during stage1/2 no scp command will be available.
#ENV SCP_MIRRORS='
#scp -i ~/.ssh/host1_key user@host1:/src/tarballs/%s %s
#scp -P 2222 -i ~/.ssh/host2_key user@host2:/src/tarballs/%s %s
#'

# internal paths
# you should leave these all as-is, this is the intended way.
# K is the directory where patches and configs are kept
# C is the directory to place the downloaded tarballs
# S is the directory to extract tarballs and place sources for building stage0
# R is the directory underwhich the new filesystem will be created
# LOGPATH is the directory to place logfiles of builds

# needed to inherit $PWD from toplevel buildscript
ENV H=/src/sabotage

ENV K=$H/KEEP
ENV C=$H/tarballs
ENV R=$SABOTAGE_BUILDDIR
ENV S=$R/src
ENV LOGPATH=$S/logs

ENV BUTCH_BUILD_TEMPLATE=$K/butch_build_template.txt
ENV BUTCH_DOWNLOAD_TEMPLATE=$K/butch_download_template.txt

# this is needed so packages can detect from which stage they're called
ENV STAGE=0


ADD test/sabotage $H

# use: setup-rootfs.sh [no args, uses $H/config]
# used for cross-compilation
# this prepares a chroot-like directory containing the root file system

RUN mkdir -p "$K" "$C" "$S" "$R" "$LOGPATH"

WORKDIR $R
RUN mkdir -p \
        boot bin dev etc home lib mnt proc root share srv src sys tmp var \
        src/logs src/build src/tarballs src/pkg src/KEEP \
        var/log/sshd var/log/crond var/log/dmesg \
        var/empty var/service var/lib var/run var/spool/cron/crontabs

# usr and sbin are a mistake
RUN ln -sfn . usr && ln -sfn bin sbin && ln -sfn ../tmp var/tmp

RUN chmod 775 src/build && chmod 1777 tmp && chmod 700 root

RUN cp -r "$K"/etc/* "$R"/etc/ && cp -r "$K"/boot/* "$R"/boot/ && chmod 0600 "$R"/etc/shadow


RUN ln -s /proc/mounts "$R"/etc/mtab


# use: update-chroot.sh [no args, uses $H/config]
# updates the chroot /src with the contents of the current sabotage checkout

RUN cp "$H"/COOKBOOK.md "$H"/LICENSE "$H"/README.md "$R"/src/ && \
    cp "$H"/build-stage0 "$H"/enter-chroot "$R"/src/ && \
    cp -r "$H"/KEEP "$H"/pkg "$H"/utils "$R"/src/


ENV CONFIG=$H/config
ENV BUTCHDB=$R/var/lib/butch.db
ENV DEPS=build:stage0
# RUN "$K"/bin/butch install stage0

# RUN echo "export A=$A" > "$R"/src/config
# RUN cat "$K"/config.stage1 >> "$R"/src/config
# RUN [ -z "$MAKE_THREADS" ] || sed -i "s@MAKE_THREADS=1@MAKE_THREADS=$MAKE_THREADS@" "$R"/src/config



FROM base AS gcc

RUN mv /bin/sed /bin/sed.c && cc -I/lib -o /bin/sed /bin/sed.c

ADD https://www.cpan.org/src/5.0/perl-5.38.2.tar.xz /src/perl-5.38.2.tar.xz
WORKDIR /src
RUN tar -xf perl-5.38.2.tar.xz
WORKDIR /src/perl-5.38.2
COPY test/perl5.patch /tmp/perl5.patch
RUN patch -p1 < /tmp/perl5.patch && \
    ./Configure -des -Uusenm -Uusedl -DEBUGGING=both && \
    make -j$(nproc) && \
    make install

ADD https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.xz /src/m4-1.4.19.tar.xz
WORKDIR /src
RUN tar -xf m4-1.4.19.tar.xz
WORKDIR /src/m4-1.4.19
RUN ./configure LD=cc && \
    make && \
    make install

ADD https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.xz /src/autoconf-2.69.tar.xz
WORKDIR /src
RUN tar -xf autoconf-2.69.tar.xz
WORKDIR /src/autoconf-2.69
RUN ./configure && \
    make && \
    make install

ADD https://ftp.gnu.org/gnu/automake/automake-1.16.5.tar.xz /src/automake-1.16.5.tar.xz
WORKDIR /src
RUN tar -xf automake-1.16.5.tar.xz
WORKDIR /src/automake-1.16.5
RUN ./configure && \
    make && \
    make install

# ADD https://ftp.gnu.org/gnu/libtool/libtool-2.4.7.tar.gz /src/libtool-2.4.7.tar.gz
# WORKDIR /src
# RUN tar -xf libtool-2.4.7.tar.gz
# WORKDIR /src/libtool-2.4.7
# RUN ./configure --disable-shared LD=cc && \
#     make && \
#     make install

ADD https://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.xz /src/gawk-5.3.0.tar.xz
WORKDIR /src
RUN tar -xf gawk-5.3.0.tar.xz
WORKDIR /src/gawk-5.3.0
RUN ./configure --disable-shared LD=cc && \
    make && \
    make install

ADD https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.xz /src/binutils-2.39.tar.xz
WORKDIR /src
RUN tar -xf binutils-2.39.tar.xz
WORKDIR /src/binutils-2.39
RUN ./configure --disable-gprofng CFLAGS='-O2 -D__LITTLE_ENDIAN__=1' && \
    make MAKEINFO=true && \
    make MAKEINFO=true install

ADD https://ftp.gnu.org/gnu/gcc/gcc-4.6.4/gcc-4.6.4.tar.gz /src/gcc-4.6.4.tar.gz
ADD https://gcc.gnu.org/pub/gcc/infrastructure/mpfr-2.4.2.tar.bz2 /src/mpfr-2.4.2.tar.bz2
ADD https://gcc.gnu.org/pub/gcc/infrastructure/gmp-4.3.2.tar.bz2 /src/gmp-4.3.2.tar.bz2
ADD https://gcc.gnu.org/pub/gcc/infrastructure/mpc-0.8.1.tar.gz /src/mpc-0.8.1.tar.gz
WORKDIR /src
RUN tar -xf gcc-4.6.4.tar.gz
WORKDIR /src/gcc-4.6.4
RUN tar -xf ../mpfr-2.4.2.tar.bz2 && \
    tar -xf ../gmp-4.3.2.tar.bz2 && \
    tar -xf ../mpc-0.8.1.tar.gz && \
    ln -sf mpfr-2.4.2 mpfr && \
    ln -sf gmp-4.3.2 gmp && \
    ln -sf mpc-0.8.1 mpc && \
    ./configure \
        --disable-bootstrap \
        --disable-decimal-float \
        --disable-fixed-point \
        --disable-libgomp \
        --disable-libitm \
        --disable-libmudflap \
        --disable-libquadmath \
        --disable-libssp \
        --disable-lto \
        --disable-multiarch \
        --disable-multilib \
        --disable-nls \
        --enable-languages=c \
        --host x86_64-linux --build x86_64-linux && \
    make && make install

ADD https://musl.libc.org/releases/musl-1.2.5.tar.gz /src/musl-1.2.5.tar.gz
WORKDIR /src
RUN tar -xf musl-1.2.5.tar.gz
WORKDIR /src/musl-1.2.5
RUN ./configure --prefix=/ --enable-wrapper=all && make -j$(nproc) && make install
RUN ln -s /lib/ld-musl-x86_64.so.1 /bin/ldd
RUN ln -s /lib/ld-musl-x86_64.so.1 /lib/ld-linux-x86-64.so.2

WORKDIR /src
RUN musl-gcc -std=c99 -o test test.c && ldd ./test && ./test && rm ./test

WORKDIR /src/gcc-4.6.4
RUN make distclean && \
    sed -i 's|"os/gnu-linux"|"os/generic"|' libstdc++-v3/configure.host && \
    ./configure CC=musl-gcc CPP=/usr/local/bin/cpp AR=/usr/local/bin/ar LD=/usr/local/bin/ld \
        --disable-bootstrap \
        --disable-decimal-float \
        --disable-fixed-point \
        --disable-libgomp \
        --disable-libitm \
        --disable-libmudflap \
        --disable-libquadmath \
        --disable-libssp \
        --disable-lto \
        --disable-multiarch \
        --disable-multilib \
        --disable-nls \
        --enable-languages=c,c++ \
        --host x86_64-linux --build x86_64-linux && \
    make -j$(nproc) && make install

ENV LD_LIBRARY_PATH=/lib:/usr/local/lib:/usr/local/lib64

COPY test/test.cc /src/test.cc
WORKDIR /src
RUN g++ -o test test.cc && ldd ./test && ./test && rm ./test

COPY test/musl-cross-make /src/musl-cross-make
COPY test/musl-cross-make-config.mak /src/musl-cross-make/config.mak
WORKDIR /src/musl-cross-make
ADD https://ftp.gnu.org/gnu/binutils/binutils-2.33.1.tar.gz sources/binutils-2.33.1.tar.gz
ADD https://ftp.gnu.org/gnu/gcc/gcc-9.4.0/gcc-9.4.0.tar.gz sources/gcc-9.4.0.tar.gz
ADD https://ftp.gnu.org/gnu/gmp/gmp-6.1.2.tar.bz2 sources/gmp-6.1.2.tar.bz2
ADD https://ftp.gnu.org/gnu/mpc/mpc-1.1.0.tar.gz sources/mpc-1.1.0.tar.gz
ADD https://ftp.gnu.org/gnu/mpfr/mpfr-4.0.2.tar.bz2 sources/mpfr-4.0.2.tar.bz2
ADD https://musl.libc.org/releases/musl-1.2.5.tar.gz sources/musl-1.2.5.tar.gz
ADD https://github.com/sabotage-linux/kernel-headers/archive/refs/tags/v4.19.88-1.tar.gz sources/kernel-headers-4.19.88-1.tar.gz
ADD https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=3d5db9ebe860 sources/config.sub
RUN tar -xf sources/kernel-headers-4.19.88-1.tar.gz && mv -f kernel-headers-4.19.88-1 linux-headers-4.19.88-1.orig && \
    sed -i s/xvf/xf/g Makefile && touch sources/* *.orig && make -j$(nproc) && make install


FROM scratch AS llvm

ARG ROOTFS
COPY $ROOTFS/bin/sh /bin/sh

SHELL ["/bin/sh", "-c"]
CMD ["/bin/sh"]

COPY --from=gcc /opt/cross /opt/cross

ENV CC=/opt/cross/bin/x86_64-linux-musl-gcc
ENV CXX=/opt/cross/bin/x86_64-linux-musl-g++
ENV LD_LIBRARY_PATH=/opt/cross/x86_64-linux-musl/lib/

RUN mkdir -p /bin /lib /lib64 /tmp && \
    ln -s /opt/cross/x86_64-linux-musl/lib/libc.so /bin/ldd && \
    ln -s /opt/cross/x86_64-linux-musl/lib/libc.so /lib/ld-musl-x86_64.so.1 && \
    ln -s /opt/cross/x86_64-linux-musl/lib/libc.so /lib64/ld-linux-x86-64.so.2 && \
    rm /opt/cross/x86_64-linux-musl/lib/ld-musl-x86_64.so.1

RUN for cmd in `sh -c toybox` cc ar; do \
        printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /bin/$cmd; \
        chmod +x /bin/$cmd; \
    done

# RUN file /bin/ldd && /bin/ldd /opt/cross/bin/x86_64-linux-musl-gcc && /opt/cross/bin/x86_64-linux-musl-gcc -print-search-dirs

# COPY test/test.c /src/test.c
# COPY test/test.cc /src/test.cc
# WORKDIR /src
# RUN /opt/cross/bin/x86_64-linux-musl-gcc -o test test.c && ldd ./test && ./test && rm ./test
# RUN /opt/cross/bin/x86_64-linux-musl-g++ -o test test.cc && ldd ./test && ./test && rm ./test

ADD https://ftp.gnu.org/gnu/make/make-4.4.tar.gz /src/make-4.4.tar.gz
WORKDIR /src
RUN tar -xf make-4.4.tar.gz
WORKDIR /src/make-4.4
RUN ./configure --disable-dependency-tracking && \
    ./build.sh && ./make && ./make install

ADD https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2.tar.gz /src/cmake-3.29.2.tar.gz
WORKDIR /src
RUN tar -xf cmake-3.29.2.tar.gz
WORKDIR /src/cmake-3.29.2
RUN ./bootstrap --parallel=$(nproc) -- -DCMAKE_USE_OPENSSL=OFF && make -j$(nproc) && make install

RUN mkdir -p /usr/lib && ln -s /opt/cross/x86_64-linux-musl/lib /usr/lib/x86_64-linux-gnu

ADD https://raw.githubusercontent.com/pts/muxzcat/master/muxzcat.c /src/muxzcat.c
RUN cc -o /bin/xzcat /src/muxzcat.c

ADD http://zlib.net/zlib-1.3.1.tar.gz /src/zlib-1.3.1.tar.gz
WORKDIR /src
RUN tar -xf zlib-1.3.1.tar.gz
WORKDIR /src/zlib-1.3.1
RUN ./configure && make -j$(nproc) && make install

ADD https://www.python.org/ftp/python/3.12.3/Python-3.12.3.tgz /src/Python-3.12.3.tgz
WORKDIR /src
RUN tar -xf Python-3.12.3.tgz
WORKDIR /src/Python-3.12.3
RUN ./configure --without-ensurepip && make -j$(nproc) && make install

ADD https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.4/llvm-project-18.1.4.src.tar.xz /src/llvm-project-18.1.4.src.tar.xz
WORKDIR /src
RUN tar -xf llvm-project-18.1.4.src.tar.xz
WORKDIR /src/llvm-project-18.1.4.src
RUN mkdir build && cd build && \
    cmake -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=/opt/llvm \
        -DLLVM_ENABLE_PROJECTS="clang;lld" \
        -DLLVM_ENABLE_RUNTIMES="compiler-rt;libcxx;libcxxabi;libunwind" \
        ../llvm && \
    make -j$(nproc) && make install
