FROM scratch AS base

ARG ROOTFS
COPY $ROOTFS /

SHELL ["/bin/sh", "-c"]
CMD ["/bin/sh"]

RUN ln -s / /usr && ln -s /lib /lib/x86_64-linux-gnu && ln -s /lib /lib64 && mkdir /tmp

COPY test/test.c /src/test.c
WORKDIR /src
RUN cc -print-search-dirs && cc -o test test.c && ./test && rm ./test

RUN for cmd in `sh -c toybox` cc ar; do \
        printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /bin/$cmd; \
        chmod +x /bin/$cmd; \
    done

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


FROM base AS gcc

ADD https://www.cpan.org/src/5.0/perl-5.38.2.tar.gz /src/perl-5.38.2.tar.gz
WORKDIR /src
RUN tar -xf perl-5.38.2.tar.gz
WORKDIR /src/perl-5.38.2
COPY test/perl5.patch /tmp/perl5.patch
RUN patch -p1 < /tmp/perl5.patch && \
    ./Configure -des -Uusenm -Uusedl -DEBUGGING=both && \
    make -j$(nproc) && \
    make install

ADD https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.gz /src/m4-1.4.19.tar.gz
WORKDIR /src
RUN tar -xf m4-1.4.19.tar.gz
WORKDIR /src/m4-1.4.19
RUN ./configure LD=cc && \
    make && \
    make install

ADD https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz /src/autoconf-2.69.tar.gz
WORKDIR /src
RUN tar -xf autoconf-2.69.tar.gz
WORKDIR /src/autoconf-2.69
RUN ./configure && \
    make && \
    make install

ADD https://ftp.gnu.org/gnu/automake/automake-1.16.5.tar.gz /src/automake-1.16.5.tar.gz
WORKDIR /src
RUN tar -xf automake-1.16.5.tar.gz
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

ADD https://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.gz /src/gawk-5.3.0.tar.gz
WORKDIR /src
RUN tar -xf gawk-5.3.0.tar.gz
WORKDIR /src/gawk-5.3.0
RUN ./configure --disable-shared LD=cc && \
    make && \
    make install

ADD https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.gz /src/binutils-2.39.tar.gz
WORKDIR /src
RUN tar -xf binutils-2.39.tar.gz
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
    ./configure CFLAGS=-O2 CFLAGS_FOR_TARGET=-O2 \
        --enable-languages=c \
        --disable-bootstrap \
        --disable-libquadmath --disable-decimal-float --disable-fixed-point \
        --disable-lto \
        --disable-libgomp \
        --disable-multilib \
        --disable-multiarch \
        --disable-libmudflap \
        --disable-libssp \
        --disable-nls \
        --host x86_64-linux --build x86_64-linux && \
    make && \
    make install

WORKDIR /src
RUN gcc -o test test.c -static && ./test && rm ./test
