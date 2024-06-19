#!/bin/sh

set -ex

R="$DESTDIR$PREFIX"

install -D -m 755 wak.c "$R/bin/awk"
install -D -m 755 configure-musl.sh "$R/bin/configure-musl.sh"
install -D -m 755 build/bootsh "$R/bin/sh"
install -D -m 755 init.sh "$R/sbin/init"
