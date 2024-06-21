#!/bin/sh

set -e

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

# Generate header files

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

# Generate build.ninja

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

cat > build.ninja <<EOF
rule cc
    command = cc -std=c99 -nostdinc -D_XOPEN_SOURCE=700 -g \$cflags \$
        -Iarch/$ARCH -Iarch/generic -Iobj/src/internal -Isrc/include -Isrc/internal -Iobj/include -Iinclude \$
        -c -o \$out \$in
    description = Building \$in

rule ar
    command = ar rcs \$out \$in
    description = Building \$out

build $PREFIX/lib/crt1.o: cc crt/crt1.c
  cflags = -fno-stack-protector -DCRT
build $PREFIX/lib/crti.o: cc crt/crti.c
  cflags = -fno-stack-protector -DCRT
build $PREFIX/lib/crtn.o: cc crt/crtn.c
  cflags = -fno-stack-protector -DCRT

build obj/src/fenv/$ARCH/fenv.o: cc src/fenv/$ARCH/fenv.s
build obj/src/ldso/$ARCH/dlsym.o: cc src/ldso/$ARCH/dlsym.s
build obj/src/ldso/$ARCH/tlsdesc.o: cc src/ldso/$ARCH/tlsdesc.s
build obj/src/process/$ARCH/vfork.o: cc src/process/$ARCH/vfork.s
build obj/src/setjmp/$ARCH/longjmp.o: cc src/setjmp/$ARCH/longjmp.s
build obj/src/setjmp/$ARCH/setjmp.o: cc src/setjmp/$ARCH/setjmp.s
build obj/src/signal/$ARCH/restore.o: cc src/signal/$ARCH/restore.s
build obj/src/signal/$ARCH/sigsetjmp.o: cc src/signal/$ARCH/sigsetjmp.s
build obj/src/thread/$ARCH/__unmapself.o: cc src/thread/$ARCH/__unmapself.s
build obj/src/thread/$ARCH/clone.o: cc src/thread/$ARCH/clone.s
build obj/src/thread/$ARCH/syscall_cp.o: cc src/thread/$ARCH/syscall_cp.s

build obj/src/env/__init_tls.o: cc src/env/__init_tls.c
  cflags = -fno-stack-protector
build obj/src/env/__libc_start_main.o: cc src/env/__libc_start_main.c
  cflags = -fno-stack-protector
build obj/src/env/__stack_chk_fail.o: cc src/env/__stack_chk_fail.c
  cflags = -fno-stack-protector
build obj/src/thread/$ARCH/__set_thread_area.o: cc src/thread/$ARCH/__set_thread_area.s
  cflags = -fno-stack-protector
build obj/src/string/memcmp.o: cc src/string/memcmp.c
  cflags = -fno-tree-loop-distribute-patterns
build obj/src/string/$ARCH/memmove.o: cc src/string/$ARCH/memmove.s
  cflags = -fno-tree-loop-distribute-patterns
build obj/src/string/$ARCH/memcpy.o: cc src/string/$ARCH/memcpy.s
  cflags = -fno-tree-loop-distribute-patterns -fno-stack-protector
build obj/src/string/$ARCH/memset.o: cc src/string/$ARCH/memset.s
  cflags = -fno-tree-loop-distribute-patterns -fno-stack-protector
EOF

for src in $SOURCES; do
    echo build obj/${src%.c}.o: cc $src >> build.ninja
done

OBJS=$(grep '^build obj/' build.ninja | sed 's/^build \([^:]\+\):.*$/\1/')

echo build $PREFIX/lib/libc.a: ar $OBJS >> build.ninja
