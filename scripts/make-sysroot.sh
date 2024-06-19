#!/bin/sh

set -e

MUSL=musl-1.2.5
PREFIX="$PWD/sysroot"

mkdir -p tarballs
wget -c https://musl.libc.org/releases/$MUSL.tar.gz -O tarballs/$MUSL.tar.gz
tar -xf tarballs/$MUSL.tar.gz
cd $MUSL
./configure --prefix="$PREFIX"
make -j$(nproc) CFLAGS="-Os"
make install
