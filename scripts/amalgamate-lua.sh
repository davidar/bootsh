#!/bin/sh
# https://old.reddit.com/r/C_Programming/comments/j7wzmy/large_single_compilationunit_c_programs/g889o8r/

tar -xvf tarballs/lua-5.4.0.tar.gz
cd lua-5.4.0/src
sed '/^#include "/d' \
    lprefix.h luaconf.h lua.h llimits.h lobject.h ltm.h lmem.h lzio.h \
    lstate.h lapi.h ldebug.h ldo.h lfunc.h lgc.h lstring.h ltable.h \
    lundump.h lvm.h lauxlib.h lualib.h llex.h lopcodes.h lparser.h \
    lcode.h lctype.h lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c \
    lctype.c ldblib.c ldebug.c ldo.c ldump.c lfunc.c lgc.c linit.c \
    liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c \
    loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c \
    ltm.c lua.c lundump.c lutf8lib.c lvm.c lzio.c \
    > ../../scripts/lua.c
cd ../..
rm -rf lua-5.4.0
