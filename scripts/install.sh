#!/bin/sh

set -ex

R="$DESTDIR$PREFIX"

install -D -m 755 build/bootsh "$R/bin/sh"
install -D -m 755 scripts/configure-musl.sh "$R/bin/configure-musl.sh"
install -D -m 755 scripts/init.sh "$R/sbin/init"
install -D -m 755 scripts/wak.c "$R/bin/awk"
