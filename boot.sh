#!/bin/sh

set -e

echo
echo "-----------------------------------"
echo "         Executing boot.sh         "
echo " https://github.com/davidar/bootsh "
echo "-----------------------------------"
echo

[ ! -L /usr ] && ln -s / /usr
mkdir -p /etc /local/bin /tmp
chmod 1777 /tmp
cd /tmp

export PATH=/local/bin:/bin

cat > /etc/passwd <<EOF
root:x:0:0:root:/root:/bin/sh
daemon:x:1:1:daemon:/dev/null:/bin/false
bin:x:2:2:bin:/bin:/dev/null:/bin/false
sys:x:3:3:sys:/dev:/dev/null:/bin/false
user:x:1000:1000:user:/home/user:/bin/sh
nobody:x:65534:65534:nobody:/nonexistent:/bin/false
EOF

cat > /etc/group <<EOF
root:x:0:
bin:x:1:daemon
sys:x:2:
users:x:100:
user:x:1000:
nogroup:x:65534:
EOF

[ ! -f /etc/resolv.conf ] && echo "nameserver 1.1.1.1" > /etc/resolv.conf

[ ! -e /proc/self ] && mkdir -p /proc && mount -t proc none /proc
[ ! -e /sys/kernel ] && mkdir -p /sys && mount -t sysfs none /sys

for cmd in $(sh --list-builtins); do
  printf '#!/bin/sh\ncommand %s "$@"' "$cmd" > /bin/$cmd;
done
chmod +x /bin/*

mkdir -p /src/tarballs /src/logs

if [ ! -f /src/tarballs/musl-1.2.5.tar.gz ]; then
  echo "Downloading musl-1.2.5 source code..."
  wget http://www.musl-libc.org/releases/musl-1.2.5.tar.gz -O /src/tarballs/musl-1.2.5.tar.gz
fi
tar -xf /src/tarballs/musl-1.2.5.tar.gz
cd musl-1.2.5
configure-musl.sh
ninja | while read line; do
  printf "\r%-80s" "$line"
done
echo
echo
cd ..

export USER=$(whoami)
export HOSTNAME=$(hostname)
export PS1='$USER@$HOSTNAME:$PWD\$ '

exec /bin/sh "$@"
