#!/bin/sh

mkdir -p /etc /proc /sys /tmp
mount -t proc none /proc
mount -t sysfs none /sys
chmod 1777 /tmp
ln -s / /usr

for cmd in $(sh --list-builtins); do
  printf '#!/bin/sh\ncommand %s "$@"' "$cmd" > /bin/$cmd;
done
chmod +x /bin/*

ifconfig eth0 up
sdhcp -e env eth0

echo "Downloading musl-1.2.5 source code..."
cd /tmp
wget http://www.musl-libc.org/releases/musl-1.2.5.tar.gz
tar -xf musl-1.2.5.tar.gz
cd musl-1.2.5
configure-musl.sh
ninja | while read line; do
  printf "\r%-80s" "$line"
done
echo

cd /
sh

cc -run - <<EOF
#include <stdio.h>
#include <unistd.h>
#include <sys/reboot.h>
main() {
  sync();
  if (reboot(RB_POWER_OFF) < 0) {
    perror("reboot");
  }
}
EOF
