FROM scratch AS base

ARG ROOTFS
COPY $ROOTFS/bin/sh /bin/sh

SHELL ["/bin/sh", "-c"]
CMD ["/bin/sh"]

COPY lib/musl /src/musl
WORKDIR /src/musl
RUN ./build.sh

RUN ln -s / /usr && ln -s /lib /lib/x86_64-linux-gnu && ln -s /lib /lib64 && mkdir /tmp

COPY test/test.c /src/test.c
WORKDIR /src
RUN cc -v && cc -run test.c foo bar baz

RUN for cmd in `sh -c toybox` cc ar; do \
        printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /bin/$cmd; \
        chmod +x /bin/$cmd; \
    done

COPY test/sbang.sh /bin/sbang
COPY test/wak.c /bin/awk
COPY test/muxzcat.c /bin/xzcat
COPY test/tar.c /bin/tar

COPY lib/toybox /src/toybox

ADD https://ftp.gnu.org/gnu/make/make-4.4.tar.gz /src/make-4.4.tar.gz
WORKDIR /src
RUN /bin/tar -xf make-4.4.tar.gz
WORKDIR /src/make-4.4
RUN ./configure --disable-dependency-tracking LD=cc && \
    ./build.sh && ./make && ./make install

ADD https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz /src/bash-5.2.21.tar.gz
WORKDIR /src
RUN /bin/tar -xf bash-5.2.21.tar.gz
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


FROM base AS gcc

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
RUN rm -rf /lib /include
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
