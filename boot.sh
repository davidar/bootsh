#!/bin/sh

set -e

echo
echo "---------------------------------"
echo "        Executing boot.sh        "
echo "https://github.com/davidar/bootsh"
echo "---------------------------------"
echo

if which make >/dev/null && tty -s; then
    exec /bin/sh
fi

# Setup root filesystem

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

echo "Bootstrapping musl"

ARCH=$(uname -m | sed 's/i.86/i386/')

# Patch musl to be compatible with tcc

rm -rf src/complex src/math/$ARCH crt/$ARCH

sed -i s/@PLT//g src/signal/x86_64/sigsetjmp.s
sed -i 's/jecxz/test %ecx,%ecx; jz/' src/signal/i386/sigsetjmp.s

sed /_REDIR_TIME64/d -i arch/$ARCH/bits/alltypes.h.in

head -n3 arch/x86_64/syscall_arch.h > arch/x86_64/syscall_arch.h.new
tail -n8 arch/x86_64/syscall_arch.h >> arch/x86_64/syscall_arch.h.new
mv -f arch/x86_64/syscall_arch.h.new arch/x86_64/syscall_arch.h

head -n5 arch/i386/syscall_arch.h > arch/i386/syscall_arch.h.new
tail -n6 arch/i386/syscall_arch.h >> arch/i386/syscall_arch.h.new
mv -f arch/i386/syscall_arch.h.new arch/i386/syscall_arch.h

# These functions are provided by libtcc1.a
cat >> arch/$ARCH/syscall_arch.h <<EOF
long __syscall0(long);
long __syscall1(long, long);
long __syscall2(long, long, long);
long __syscall3(long, long, long, long);
long __syscall4(long, long, long, long, long);
long __syscall5(long, long, long, long, long, long);
long __syscall6(long, long, long, long, long, long, long);
EOF

# Build musl

CFLAGS="$CFLAGS -std=c99 -nostdinc -D_XOPEN_SOURCE=700"
CFLAGS="$CFLAGS -Iarch/$ARCH -Iarch/generic -Iobj/src/internal -Isrc/include -Isrc/internal -Iobj/include -Iinclude"

mkdir -p obj/include/bits
sed -f ./tools/mkalltypes.sed ./arch/$ARCH/bits/alltypes.h.in ./include/alltypes.h.in > obj/include/bits/alltypes.h
cp arch/$ARCH/bits/syscall.h.in obj/include/bits/syscall.h
sed -n -e s/__NR_/SYS_/p < arch/$ARCH/bits/syscall.h.in >> obj/include/bits/syscall.h

cp -r include $PREFIX/
cp -r arch/generic/bits $PREFIX/include/
cp -r arch/$ARCH/bits $PREFIX/include/
cp -r obj/include/bits $PREFIX/include/

mkdir -p obj/src/internal
printf '#define VERSION "%s"\n' "$(sh tools/version.sh)" > obj/src/internal/version.h

mkdir -p $PREFIX/lib
cc $CFLAGS -fno-stack-protector -DCRT -c -o $PREFIX/lib/crt1.o crt/crt1.c
cc $CFLAGS -fno-stack-protector -DCRT -c -o $PREFIX/lib/crti.o crt/crti.c
cc $CFLAGS -fno-stack-protector -DCRT -c -o $PREFIX/lib/crtn.o crt/crtn.c

ls src/*/*.c src/malloc/mallocng/*.c | sort > sources.txt

SOURCES=$(comm -23 sources.txt - <<EOF
src/env/__init_tls.c
src/env/__libc_start_main.c
src/env/__stack_chk_fail.c
src/fenv/fenv.c
src/ldso/dlsym.c
src/ldso/tlsdesc.c
src/process/vfork.c
src/setjmp/longjmp.c
src/setjmp/setjmp.c
src/signal/restore.c
src/signal/sigsetjmp.c
src/string/memcmp.c
src/string/memcpy.c
src/string/memmove.c
src/string/memset.c
src/thread/__set_thread_area.c
src/thread/__unmapself.c
src/thread/clone.c
src/thread/syscall_cp.c
EOF
)

for src in $SOURCES; do
    obj=obj/${src%.c}.o
    mkdir -p $(dirname $obj)
    cc $CFLAGS -c -o $obj $src
done

mkdir -p obj/src/env
mkdir -p obj/src/fenv/$ARCH
mkdir -p obj/src/ldso/$ARCH
mkdir -p obj/src/process/$ARCH
mkdir -p obj/src/setjmp/$ARCH
mkdir -p obj/src/signal/$ARCH
mkdir -p obj/src/string/$ARCH
mkdir -p obj/src/thread/$ARCH

cc $CFLAGS -c -o obj/src/fenv/$ARCH/fenv.o src/fenv/$ARCH/fenv.s
cc $CFLAGS -c -o obj/src/ldso/$ARCH/dlsym.o src/ldso/$ARCH/dlsym.s
cc $CFLAGS -c -o obj/src/ldso/$ARCH/tlsdesc.o src/ldso/$ARCH/tlsdesc.s
cc $CFLAGS -c -o obj/src/process/$ARCH/vfork.o src/process/$ARCH/vfork.s
cc $CFLAGS -c -o obj/src/setjmp/$ARCH/longjmp.o src/setjmp/$ARCH/longjmp.s
cc $CFLAGS -c -o obj/src/setjmp/$ARCH/setjmp.o src/setjmp/$ARCH/setjmp.s
cc $CFLAGS -c -o obj/src/signal/$ARCH/restore.o src/signal/$ARCH/restore.s
cc $CFLAGS -c -o obj/src/signal/$ARCH/sigsetjmp.o src/signal/$ARCH/sigsetjmp.s
cc $CFLAGS -c -o obj/src/thread/$ARCH/__unmapself.o src/thread/$ARCH/__unmapself.s
cc $CFLAGS -c -o obj/src/thread/$ARCH/clone.o src/thread/$ARCH/clone.s
cc $CFLAGS -c -o obj/src/thread/$ARCH/syscall_cp.o src/thread/$ARCH/syscall_cp.s

cc $CFLAGS -fno-stack-protector -c -o obj/src/env/__init_tls.o src/env/__init_tls.c
cc $CFLAGS -fno-stack-protector -c -o obj/src/env/__libc_start_main.o src/env/__libc_start_main.c
cc $CFLAGS -fno-stack-protector -c -o obj/src/env/__stack_chk_fail.o src/env/__stack_chk_fail.c
cc $CFLAGS -fno-stack-protector -c -o obj/src/thread/$ARCH/__set_thread_area.o src/thread/$ARCH/__set_thread_area.s
cc $CFLAGS -fno-tree-loop-distribute-patterns -c -o obj/src/string/memcmp.o src/string/memcmp.c
cc $CFLAGS -fno-tree-loop-distribute-patterns -c -o obj/src/string/$ARCH/memmove.o src/string/$ARCH/memmove.s
cc $CFLAGS -fno-tree-loop-distribute-patterns -fno-stack-protector -c -o obj/src/string/$ARCH/memcpy.o src/string/$ARCH/memcpy.s
cc $CFLAGS -fno-tree-loop-distribute-patterns -fno-stack-protector -c -o obj/src/string/$ARCH/memset.o src/string/$ARCH/memset.s

ar rcs $PREFIX/lib/libc.a `find obj/src -name '*.o'`
cd ..

if [ ! -f /src/tarballs/make-4.4.1.tar.gz ]; then
    echo "Downloading make-4.4.1 source code..."
    wget http://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz -O /src/tarballs/make-4.4.1.tar.gz
fi
tar -xf /src/tarballs/make-4.4.1.tar.gz
cd make-4.4.1
echo "Bootstrapping make -> /src/logs/boot_make.log"
(
./configure --disable-dependency-tracking LD=cc
./build.sh && ./make -s && ./make -s install
) > /src/logs/boot_make.log 2>&1
cd ..

if [ $# -gt 0 ]; then
    make -s $@
fi

if tty -s; then
    export USER=$(whoami)
    export HOSTNAME=$(hostname)
    export PS1='$USER@$HOSTNAME:$PWD\$ '
    exec /bin/sh
fi
