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

CFLAGS="$CFLAGS -std=c99 -nostdinc -D_XOPEN_SOURCE=700"
CFLAGS="$CFLAGS -Iarch/$ARCH -Iarch/generic -Iobj/src/internal -Isrc/include -Isrc/internal -Iobj/include -Iinclude"

# Patch musl to be compatible with tcc

rm -rf src/complex src/math/$ARCH crt/$ARCH

sed -i s/@PLT//g src/signal/x86_64/sigsetjmp.s
sed -i 's/jecxz/test %ecx,%ecx; jz/' src/signal/i386/sigsetjmp.s

sed /_REDIR_TIME64/d -i arch/$ARCH/bits/alltypes.h.in


cat > arch/x86_64/syscall_arch.h <<EOF
#define __SYSCALL_LL_E(x) (x)
#define __SYSCALL_LL_O(x) (x)

static __inline long __syscall0(long n);
asm (
"__syscall0:\n"
    "movq %rdi, %rax\n"
    "syscall\n"
    "ret"
);

static __inline long __syscall1(long n, long a1);
asm (
"__syscall1:\n"
    "movq %rdi, %rax\n"
    "movq %rsi, %rdi\n"
    "syscall\n"
    "ret"
);

static __inline long __syscall2(long n, long a1, long a2);
asm (
"__syscall2:\n"
    "movq %rdi, %rax\n"
    "movq %rsi, %rdi\n"
    "movq %rdx, %rsi\n"
    "syscall\n"
    "ret"
);

static __inline long __syscall3(long n, long a1, long a2, long a3);
asm (
"__syscall3:\n"
    "movq %rdi, %rax\n"
    "movq %rsi, %rdi\n"
    "movq %rdx, %rsi\n"
    "movq %rcx, %rdx\n"
    "syscall\n"
    "ret"
);

static __inline long __syscall4(long n, long a1, long a2, long a3, long a4);
asm (
"__syscall4:\n"
    "movq %rdi, %rax\n"
    "movq %rsi, %rdi\n"
    "movq %rdx, %rsi\n"
    "movq %rcx, %rdx\n"
    "movq %r8, %r10\n"
    "syscall\n"
    "ret"
);

static __inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5);
asm (
"__syscall5:\n"
    "movq %rdi, %rax\n"
    "movq %rsi, %rdi\n"
    "movq %rdx, %rsi\n"
    "movq %rcx, %rdx\n"
    "movq %r8, %r10\n"
    "movq %r9, %r8\n"
    "syscall\n"
    "ret"
);

static __inline long __syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);
asm (
"__syscall6:\n"
    "movq %rdi, %rax\n"
    "movq %rsi, %rdi\n"
    "movq %rdx, %rsi\n"
    "movq %rcx, %rdx\n"
    "movq %r8, %r10\n"
    "movq %r9, %r8\n"
    "movq 8(%rsp), %r9\n"
    "syscall\n"
    "ret"
);

#define VDSO_USEFUL
#define VDSO_CGT_SYM "__vdso_clock_gettime"
#define VDSO_CGT_VER "LINUX_2.6"
#define VDSO_GETCPU_SYM "__vdso_getcpu"
#define VDSO_GETCPU_VER "LINUX_2.6"

#define IPC_64 0
EOF


cat > arch/i386/syscall_arch.h <<EOF
#define __SYSCALL_LL_E(x) \\
((union { long long ll; long l[2]; }){ .ll = x }).l[0], \\
((union { long long ll; long l[2]; }){ .ll = x }).l[1]
#define __SYSCALL_LL_O(x) __SYSCALL_LL_E((x))

#define SYSCALL_NO_TLS 1

#if SYSCALL_NO_TLS
#define SYSCALL_INSNS "int \$128"
#else
#define SYSCALL_INSNS "call *%gs:16"
#endif

#define SYSCALL_INSNS_12 "xchg %ebx,%edx ; " SYSCALL_INSNS " ; xchg %ebx,%edx"
#define SYSCALL_INSNS_34 "xchg %ebx,%edi ; " SYSCALL_INSNS " ; xchg %ebx,%edi"

static inline long __syscall0(long n);
asm(
"__syscall0:\n"
"    movl   4(%esp),%eax\n"
     SYSCALL_INSNS "\n"
"    ret"
);

static inline long __syscall1(long n, long a1);
asm(
"__syscall1:\n"
"    movl   4(%esp),%eax\n"
"    movl   8(%esp),%edx\n"
     SYSCALL_INSNS_12 "\n"
"    ret"
);

static inline long __syscall2(long n, long a1, long a2);
asm(
"__syscall2:\n"
"    movl   4(%esp),%eax\n"
"    movl   8(%esp),%edx\n"
"    movl  12(%esp),%ecx\n"
     SYSCALL_INSNS_12 "\n"
"    ret"
);

static inline long __syscall3(long n, long a1, long a2, long a3);
asm(
"__syscall3:\n"
"    pushl %ebx\n"
"    movl   8(%esp),%eax\n"
"    movl  12(%esp),%ebx\n"
"    movl  16(%esp),%ecx\n"
"    movl  20(%esp),%edx\n"
     SYSCALL_INSNS "\n"
"    popl  %ebx\n"
"    ret"
);

static inline long __syscall4(long n, long a1, long a2, long a3, long a4);
asm(
"__syscall4:\n"
"    pushl %esi\n"
"    pushl %ebx\n"
"    movl  12(%esp),%eax\n"
"    movl  16(%esp),%ebx\n"
"    movl  20(%esp),%ecx\n"
"    movl  24(%esp),%edx\n"
"    movl  28(%esp),%esi\n"
     SYSCALL_INSNS "\n"
"    popl  %ebx\n"
"    popl  %esi\n"
"    ret"
);

static inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5);
asm(
"__syscall5:\n"
"    pushl %edi\n"
"    pushl %esi\n"
"    pushl %ebx\n"
"    movl  16(%esp),%eax\n"
"    movl  20(%esp),%ebx\n"
"    movl  24(%esp),%ecx\n"
"    movl  28(%esp),%edx\n"
"    movl  32(%esp),%esi\n"
"    movl  36(%esp),%edi\n"
     SYSCALL_INSNS "\n"
"    popl  %ebx\n"
"    popl  %esi\n"
"    popl  %edi\n"
"    ret"
);

static inline long __syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);
asm(
"__syscall6:\n"
"    pushl %ebp\n"
"    pushl %edi\n"
"    pushl %esi\n"
"    pushl %ebx\n"
"    movl  20(%esp),%eax\n"
"    movl  24(%esp),%ebx\n"
"    movl  28(%esp),%ecx\n"
"    movl  32(%esp),%edx\n"
"    movl  36(%esp),%esi\n"
"    movl  40(%esp),%edi\n"
"    movl  44(%esp),%ebp\n"
     SYSCALL_INSNS "\n"
"    popl  %ebx\n"
"    popl  %esi\n"
"    popl  %edi\n"
"    popl  %ebp\n"
"    ret"
);

#define VDSO_USEFUL
#define VDSO_CGT32_SYM "__vdso_clock_gettime"
#define VDSO_CGT32_VER "LINUX_2.6"
#define VDSO_CGT_SYM "__vdso_clock_gettime64"
#define VDSO_CGT_VER "LINUX_2.6"
EOF


# Build musl

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
