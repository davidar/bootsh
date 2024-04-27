FROM scratch

COPY rootfs/seed/bin/sh /bin/sh
COPY rootfs/seed/usr/local/musl/include /include
COPY rootfs/seed/usr/local/musl/lib /lib
COPY rootfs/seed/usr/local/lib/tcc /local/lib/tcc

SHELL ["/bin/sh", "-c"]
CMD ["/bin/sh"]

RUN ln -s / /usr && ln -s /lib /lib/x86_64-linux-gnu

COPY test/test.c /src/test.c
WORKDIR /src
RUN cc -o test test.c && ./test && rm ./test

RUN mkdir -p /tmp /usr/local/bin; \
    for cmd in `sh -c toybox` cc; do \
        printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /usr/local/bin/$cmd; \
        chmod +x /usr/local/bin/$cmd; \
    done

ADD https://ftp.gnu.org/gnu/make/make-4.4.tar.gz /src/make-4.4.tar.gz
WORKDIR /src
RUN tar -xf make-4.4.tar.gz
WORKDIR /src/make-4.4
RUN ./configure --disable-dependency-tracking LD=cc AR="cc -ar" && \
    ./build.sh && ./make MAKEINFO=true && ./make MAKEINFO=true install

ADD https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz /src/bash-5.2.21.tar.gz
WORKDIR /src
RUN tar -xf bash-5.2.21.tar.gz
WORKDIR /src/bash-5.2.21
RUN ./configure --prefix=/ --without-bash-malloc LD=cc AR="cc -ar" && make -j$(nproc) && make install

COPY src /src/bootsh/src
COPY lib /src/bootsh/lib
COPY Makefile toybox.config config.h /src/bootsh/
WORKDIR /src/bootsh
RUN make bootstrap

# COPY ports/lang/perl5 /src/perl5
# COPY ports/lang/perl5.patch /tmp/perl5.patch
# WORKDIR /src/perl5
# RUN patch -p1 < /tmp/perl5.patch
# RUN ./Configure -des -Uusenm -Uusedl -DEBUGGING=both
# RUN make -j$(nproc) AR="cc -ar"
# # RUN make test
# RUN make install

# ADD https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.gz /src/m4-1.4.19.tar.gz
# WORKDIR /src
# RUN tar -xf m4-1.4.19.tar.gz
# WORKDIR /src/m4-1.4.19
# RUN ./configure LD=cc AR="cc -ar"
# RUN make
# RUN make install

# ADD https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz /src/autoconf-2.69.tar.gz
# WORKDIR /src
# RUN tar -xf autoconf-2.69.tar.gz
# WORKDIR /src/autoconf-2.69
# RUN ./configure
# RUN make
# RUN make install

# ADD https://ftp.gnu.org/gnu/automake/automake-1.16.5.tar.gz /src/automake-1.16.5.tar.gz
# WORKDIR /src
# RUN tar -xf automake-1.16.5.tar.gz
# WORKDIR /src/automake-1.16.5
# RUN ./configure
# RUN make
# RUN make install

# RUN for cmd in env; do \
#         printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /usr/local/bin/$cmd; \
#         chmod +x /usr/local/bin/$cmd; \
#     done

# RUN mkdir -p /usr/bin
# RUN ln -sv /usr/local/bin/env /usr/bin/env

# ADD https://ftp.gnu.org/gnu/libtool/libtool-2.4.7.tar.gz /src/libtool-2.4.7.tar.gz
# WORKDIR /src
# RUN tar -xf libtool-2.4.7.tar.gz
# WORKDIR /src/libtool-2.4.7
# RUN ./configure --disable-shared LD=cc AR="cc -ar"
# RUN make
# RUN make install

# ADD https://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.gz /src/gawk-5.3.0.tar.gz
# WORKDIR /src
# RUN tar -xf gawk-5.3.0.tar.gz
# WORKDIR /src/gawk-5.3.0
# RUN ./configure --disable-shared LD=cc AR="cc -ar"
# RUN make
# RUN make install

# ADD https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.gz /src/binutils-2.39.tar.gz
# WORKDIR /src
# RUN tar -xf binutils-2.39.tar.gz
# WORKDIR /src/binutils-2.39
# RUN ./configure --disable-gprofng CFLAGS='-O2 -D__LITTLE_ENDIAN__=1' AR="cc -ar"
# RUN make MAKEINFO=true
# RUN make MAKEINFO=true install

# RUN ln -s /usr/lib/x86_64-linux-gnu /usr/lib64

# RUN for cmd in bzcat; do \
#         printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /usr/local/bin/$cmd; \
#         chmod +x /usr/local/bin/$cmd; \
#     done

# ADD https://ftp.gnu.org/gnu/gcc/gcc-4.6.4/gcc-4.6.4.tar.gz /src/gcc-4.6.4.tar.gz
# ADD https://gcc.gnu.org/pub/gcc/infrastructure/mpfr-2.4.2.tar.bz2 /src/mpfr-2.4.2.tar.bz2
# ADD https://gcc.gnu.org/pub/gcc/infrastructure/gmp-4.3.2.tar.bz2 /src/gmp-4.3.2.tar.bz2
# ADD https://gcc.gnu.org/pub/gcc/infrastructure/mpc-0.8.1.tar.gz /src/mpc-0.8.1.tar.gz
# WORKDIR /src
# RUN tar -xf gcc-4.6.4.tar.gz
# WORKDIR /src/gcc-4.6.4
# RUN tar -xf ../mpfr-2.4.2.tar.bz2
# RUN tar -xf ../gmp-4.3.2.tar.bz2
# RUN tar -xf ../mpc-0.8.1.tar.gz
# RUN ln -sf mpfr-2.4.2 mpfr
# RUN ln -sf gmp-4.3.2 gmp
# RUN ln -sf mpc-0.8.1 mpc
# RUN ./configure CFLAGS=-O2 CFLAGS_FOR_TARGET=-O2 \
#     --enable-languages=c \
#     --disable-bootstrap \
#     --disable-libquadmath --disable-decimal-float --disable-fixed-point \
#     --disable-lto \
#     --disable-libgomp \
#     --disable-multilib \
#     --disable-multiarch \
#     --disable-libmudflap \
#     --disable-libssp \
#     --disable-nls \
#     --host x86_64-linux --build x86_64-linux
# # RUN ./configure --prefix=/usr/local/gcc-4.6.4 --enable-languages=c,c++ --disable-multilib
# RUN make
# RUN make install

# # COPY src/test.c /src/test.c
# # WORKDIR /src
# # RUN gcc -o test test.c -static
# # RUN ./test
