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

# COPY src/tcc /src/tcc
# COPY src/tcc-boot.sh /src/tcc/boot.sh
# WORKDIR /src/tcc
# RUN ./boot.sh

# COPY sbase /src/sbase
# WORKDIR /src/sbase
# RUN bmake
# RUN bmake install PREFIX=/usr/local/sbase
# RUN ln -sv /usr/local/sbase/bin/expr /usr/bin/expr
# RUN ln -sv /usr/local/sbase/bin/tr /usr/bin/tr

# COPY awk /src/awk
# WORKDIR /src/awk
# RUN bmake YACC="yacc -d -b awkgram"
# RUN cp a.out /usr/bin/awk

RUN mkdir -p /tmp /usr/local/bin; \
    for cmd in cc grep sed awk rm mkdir cp echo true chmod ls; do \
        printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /usr/local/bin/$cmd; \
        chmod +x /usr/local/bin/$cmd; \
    done

ADD https://ftp.gnu.org/gnu/make/make-4.4.tar.gz /src/make-4.4.tar.gz
WORKDIR /src
RUN tar -xf make-4.4.tar.gz
WORKDIR /src/make-4.4
RUN ./configure --disable-dependency-tracking LD=cc AR="cc -ar" && \
    ./build.sh && ./make MAKEINFO=true && ./make MAKEINFO=true install

# COPY src/tcc /src/tcc
# WORKDIR /src/tcc
# RUN make clean
# RUN ./configure --cc=cc --config-ldl=no --config-debug=yes --config-bcheck=no
# RUN make -j$(nproc)
# RUN make install

# COPY src/test.c /src/test.c
# WORKDIR /src
# RUN /usr/local/bin/tcc -o test test.c
# RUN ./test
# RUN rm ./test

# RUN rm /usr/local/bin/tcc

# COPY src/musl /src/musl
# WORKDIR /src/musl
# RUN rm -rf /usr/local/musl
# RUN ./configure CC=cc AR="cc -ar" RANLIB=echo
# RUN make -j$(nproc) CFLAGS=-g
# RUN make install

ADD https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz /src/bash-5.2.21.tar.gz
WORKDIR /src
RUN tar -xf bash-5.2.21.tar.gz
WORKDIR /src/bash-5.2.21
RUN ./configure --prefix=/ --without-bash-malloc LD=cc AR="cc -ar" && make -j$(nproc) && make install

# RUN for cmd in sort xargs readlink tr uname cmp dirname basename head wc cat egrep fold tee gzip od ln; do \
#         printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /usr/local/bin/$cmd; \
#         chmod +x /usr/local/bin/$cmd; \
#     done

# COPY src/toybox /src/toybox
# COPY src/toybox.config /src/toybox/.config
# WORKDIR /src/toybox
# RUN make clean
# RUN make -j$(nproc)
# # RUN PREFIX=/usr/local/toybox/bin make install_flat
# # RUN cp -f /usr/local/toybox/bin/toybox /usr/local/bin/toybox
# RUN rm generated/obj/main.o

# COPY src/dash /src/dash
# WORKDIR /src/dash/src
# RUN make
# RUN cp -f dash /bin/sh

# # WORKDIR /src/oksh
# # RUN ./configure
# # RUN make -j$(nproc)
# # RUN make install

# WORKDIR /src
# RUN cc -o test test.c
# RUN ./test
# RUN rm ./test

# RUN for cmd in comm expr touch uniq mv; do \
#         printf '#!/bin/sh\nexec %s "$@"' "$cmd" > /usr/local/bin/$cmd; \
#         chmod +x /usr/local/bin/$cmd; \
#     done

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
