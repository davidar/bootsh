#!/bin/sh

set -ex

BINDIR="$DESTDIR$PREFIX/bin"

install -D -m 755 wak.c "$BINDIR/awk"
install -D -m 755 boot.sh "$BINDIR/boot.sh"
install -D -m 755 configure-musl.sh "$BINDIR/configure-musl.sh"
install -D -m 755 build/bootsh "$BINDIR/sh"
