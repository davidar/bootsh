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

printf '#!/bin/sh\nexit 0' > /bin/true
printf '#!/bin/sh\nexit 1' > /bin/false
for cmd in `sh --list-builtins`; do
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

echo "Bootstrapping musl -> /src/logs/boot_musl.log"

CFLAGS="$CFLAGS -std=c99 -nostdinc -D_XOPEN_SOURCE=700"
CFLAGS="$CFLAGS -Iarch/x86_64 -Iarch/generic -Iobj/src/internal -Isrc/include -Isrc/internal -Iobj/include -Iinclude"

rm -rf src/complex src/math/x86_64 crt/x86_64
sed -i s/@PLT//g src/signal/x86_64/sigsetjmp.s
cat > arch/x86_64/syscall_arch.h <<EOF
#define __SYSCALL_LL_E(x) (x)
#define __SYSCALL_LL_O(x) (x)

static __inline long __syscall0(long n);
asm (
	".type __syscall0, @function;"
	"__syscall0:;"
	"movq %rdi, %rax;"
	"syscall;"
	"ret"
);

static __inline long __syscall1(long n, long a1);
asm (
	".type __syscall1, @function;"
	"__syscall1:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"syscall;"
	"ret"
);

static __inline long __syscall2(long n, long a1, long a2);
asm (
	".type __syscall2, @function;"
	"__syscall2:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"syscall;"
	"ret"
);

static __inline long __syscall3(long n, long a1, long a2, long a3);
asm (
	".type __syscall3, @function;"
	"__syscall3:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"syscall;"
	"ret"
);

static __inline long __syscall4(long n, long a1, long a2, long a3, long a4);
asm (
	".type __syscall4, @function;"
	"__syscall4:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"movq %r8, %r10;"
	"syscall;"
	"ret"
);

static __inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5);
asm (
	".type __syscall5, @function;"
	"__syscall5:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"movq %r8, %r10;"
	"movq %r9, %r8;"
	"syscall;"
	"ret"
);

static __inline long __syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);
asm (
	".type __syscall6, @function;"
	"__syscall6:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"movq %r8, %r10;"
	"movq %r9, %r8;"
	"movq 8(%rsp), %r9;"
	"syscall;"
	"ret"
);

#define VDSO_USEFUL
#define VDSO_CGT_SYM "__vdso_clock_gettime"
#define VDSO_CGT_VER "LINUX_2.6"
#define VDSO_GETCPU_SYM "__vdso_getcpu"
#define VDSO_GETCPU_VER "LINUX_2.6"

#define IPC_64 0
EOF

mkdir -p obj/include/bits
sed -f ./tools/mkalltypes.sed ./arch/x86_64/bits/alltypes.h.in ./include/alltypes.h.in > obj/include/bits/alltypes.h
cp arch/x86_64/bits/syscall.h.in obj/include/bits/syscall.h
sed -n -e s/__NR_/SYS_/p < arch/x86_64/bits/syscall.h.in >> obj/include/bits/syscall.h

cp -r include $PREFIX/
cp -r arch/generic/bits $PREFIX/include/
cp -r arch/x86_64/bits $PREFIX/include/
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
    cc $CFLAGS -c -o $obj $src >> /src/logs/boot_musl.log 2>&1
done

mkdir -p obj/src/env
mkdir -p obj/src/fenv/x86_64
mkdir -p obj/src/ldso/x86_64
mkdir -p obj/src/process/x86_64
mkdir -p obj/src/setjmp/x86_64
mkdir -p obj/src/signal/x86_64
mkdir -p obj/src/string/x86_64
mkdir -p obj/src/thread/x86_64

cc $CFLAGS -c -o obj/src/fenv/x86_64/fenv.o src/fenv/x86_64/fenv.s
cc $CFLAGS -c -o obj/src/ldso/x86_64/dlsym.o src/ldso/x86_64/dlsym.s
cc $CFLAGS -c -o obj/src/ldso/x86_64/tlsdesc.o src/ldso/x86_64/tlsdesc.s
cc $CFLAGS -c -o obj/src/process/x86_64/vfork.o src/process/x86_64/vfork.s
cc $CFLAGS -c -o obj/src/setjmp/x86_64/longjmp.o src/setjmp/x86_64/longjmp.s
cc $CFLAGS -c -o obj/src/setjmp/x86_64/setjmp.o src/setjmp/x86_64/setjmp.s
cc $CFLAGS -c -o obj/src/signal/x86_64/restore.o src/signal/x86_64/restore.s
cc $CFLAGS -c -o obj/src/signal/x86_64/sigsetjmp.o src/signal/x86_64/sigsetjmp.s
cc $CFLAGS -c -o obj/src/thread/x86_64/__unmapself.o src/thread/x86_64/__unmapself.s
cc $CFLAGS -c -o obj/src/thread/x86_64/clone.o src/thread/x86_64/clone.s
cc $CFLAGS -c -o obj/src/thread/x86_64/syscall_cp.o src/thread/x86_64/syscall_cp.s

cc $CFLAGS -fno-stack-protector -c -o obj/src/env/__init_tls.o src/env/__init_tls.c
cc $CFLAGS -fno-stack-protector -c -o obj/src/env/__libc_start_main.o src/env/__libc_start_main.c
cc $CFLAGS -fno-stack-protector -c -o obj/src/env/__stack_chk_fail.o src/env/__stack_chk_fail.c
cc $CFLAGS -fno-stack-protector -c -o obj/src/thread/x86_64/__set_thread_area.o src/thread/x86_64/__set_thread_area.s
cc $CFLAGS -fno-tree-loop-distribute-patterns -c -o obj/src/string/memcmp.o src/string/memcmp.c
cc $CFLAGS -fno-tree-loop-distribute-patterns -c -o obj/src/string/x86_64/memmove.o src/string/x86_64/memmove.s
cc $CFLAGS -fno-tree-loop-distribute-patterns -fno-stack-protector -c -o obj/src/string/x86_64/memcpy.o src/string/x86_64/memcpy.s
cc $CFLAGS -fno-tree-loop-distribute-patterns -fno-stack-protector -c -o obj/src/string/x86_64/memset.o src/string/x86_64/memset.s

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
    exec /bin/sh
fi
