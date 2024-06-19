#!/bin/sh

INITPWD=$(pwd)

mkdir -p /etc /proc /sys /tmp
chmod 1777 /tmp

[ ! -e /usr ] && ln -s / /usr

if [ ! -e /proc/mounts ]; then
  mount -t proc none /proc
  mount -t sysfs none /sys
fi

[ ! -f /etc/mtab ] && ln -s /proc/mounts /etc/mtab
[ ! -f /etc/resolv.conf ] && [ -e /proc/net/pnp ] && ln -s /proc/net/pnp /etc/resolv.conf

[ ! -f /etc/passwd ] && cat > /etc/passwd <<EOF
root:x:0:0:root:/root:/bin/sh
daemon:x:1:1:daemon:/dev/null:/bin/false
bin:x:2:2:bin:/bin:/dev/null:/bin/false
sys:x:3:3:sys:/dev:/dev/null:/bin/false
user:x:1000:1000:user:/home/user:/bin/sh
nobody:x:65534:65534:nobody:/nonexistent:/bin/false
EOF

[ ! -f /etc/group ] && cat > /etc/group <<EOF
root:x:0:
bin:x:1:daemon
sys:x:2:
users:x:100:
user:x:1000:
nogroup:x:65534:
EOF

for cmd in $(sh --list-builtins); do
  printf '#!/bin/sh\ncommand %s "$@"' "$cmd" > /bin/$cmd;
done
chmod +x /bin/*

if [ ! -f /lib/libc.a ]; then
  cd /tmp
  if [ -f /src/tarballs/musl-1.2.5.tar.gz ]; then
    echo "Using cached musl-1.2.5 source code..."
    tar -xf /src/tarballs/musl-1.2.5.tar.gz
  else
    echo "Downloading musl-1.2.5 source code..."
    wget http://www.musl-libc.org/releases/musl-1.2.5.tar.gz
    tar -xf musl-1.2.5.tar.gz
  fi
  cd musl-1.2.5
  configure-musl.sh
  ninja | while read line; do
    printf "\r%-80s" "$line"
  done
  echo
fi

export USER=$(whoami)
export HOSTNAME=$(hostname)
export PS1='$USER:$PWD\$ '

cd "$INITPWD"

if [ -z "$1" ]; then
  /bin/sh
  poweroff -f
else
  exec "$@"
fi
