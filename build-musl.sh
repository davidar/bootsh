#!/bin/sh

set -e

CFLAGS="-std=c99 -nostdinc -ffreestanding -fexcess-precision=standard -frounding-math -fno-strict-aliasing -Wa,--noexecstack -D_XOPEN_SOURCE=700 -I./arch/x86_64 -I./arch/generic -Iobj/src/internal -I./src/include -I./src/internal -Iobj/include -I./include  -O2 -fno-align-jumps -fno-align-functions -fno-align-loops -fno-align-labels -fira-region=one -fira-hoist-pressure -freorder-blocks-algorithm=simple -fno-prefetch-loop-arrays -fno-tree-ch -pipe -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Wno-pointer-to-int-cast -Werror=implicit-function-declaration -Werror=implicit-int -Werror=pointer-sign -Werror=pointer-arith -Werror=int-conversion -Werror=incompatible-pointer-types -Werror=discarded-qualifiers -Werror=discarded-array-qualifiers -Waddress -Warray-bounds -Wchar-subscripts -Wduplicate-decl-specifier -Winit-self -Wreturn-type -Wsequence-point -Wstrict-aliasing -Wunused-function -Wunused-label -Wunused-variable"

# sed -f ./tools/mkalltypes.sed ./arch/x86_64/bits/alltypes.h.in ./include/alltypes.h.in > obj/include/bits/alltypes.h
# cp arch/x86_64/bits/syscall.h.in obj/include/bits/syscall.h
# sed -n -e s/__NR_/SYS_/p < arch/x86_64/bits/syscall.h.in >> obj/include/bits/syscall.h

cp -r include $PREFIX/
cp -r arch/generic/bits $PREFIX/include/
cp -r arch/x86_64/bits $PREFIX/include/
cp -r obj/include/bits $PREFIX/include/

printf '#define VERSION "%s"\n' "$(cd .; sh tools/version.sh)" > obj/src/internal/version.h

mkdir -p $PREFIX/lib
cc $CFLAGS -fno-stack-protector -DCRT -c -o $PREFIX/lib/crt1.o crt/crt1.c
cc $CFLAGS -fno-stack-protector -DCRT -c -o $PREFIX/lib/crti.o crt/crti.c
cc $CFLAGS -fno-stack-protector -DCRT -c -o $PREFIX/lib/crtn.o crt/crtn.c

for src in `cat sources.txt`; do
    obj=obj/${src%.c}.o
    mkdir -p $(dirname $obj)
    cc $CFLAGS -c -o $obj $src
done

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

# ar rcs $PREFIX/lib/libc.a `find obj/src -name '*.o'`