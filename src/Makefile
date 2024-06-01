-include ../config.mak

ARCH=$(shell uname -m | sed 's/i.86/i386/')

SHSRCS=	bltin/printf.c bltin/test.c bltin/times.c \
	alias.c arith_yacc.c arith_yylex.c cd.c error.c eval.c \
	exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c output.c parser.c redir.c show.c \
	signames.c trap.c var.c \
	toybox.c tcc.c ar.c sbase-box.c

ifeq ($(ARCH),x86_64)
CFLAGS+= '-DCONFIG_TRIPLET="x86_64-linux-gnu"' -DTCC_TARGET_X86_64
else ifeq ($(ARCH),i386)
CFLAGS+= '-DCONFIG_TRIPLET="i386-linux-gnu"' -DTCC_TARGET_I386
else
$(error Unsupported architecture $(ARCH))
endif

SBASE_SRCS=\
	../lib/sbase/basename.c \
	../lib/sbase/cat.c \
	../lib/sbase/chgrp.c \
	../lib/sbase/chmod.c \
	../lib/sbase/chown.c \
	../lib/sbase/chroot.c \
	../lib/sbase/cksum.c \
	../lib/sbase/cmp.c \
	../lib/sbase/cols.c \
	../lib/sbase/comm.c \
	../lib/sbase/cp.c \
	../lib/sbase/date.c \
	../lib/sbase/dd.c \
	../lib/sbase/dirname.c \
	../lib/sbase/du.c \
	../lib/sbase/echo.c \
	../lib/sbase/env.c \
	../lib/sbase/expand.c \
	../lib/sbase/expr.c \
	../lib/sbase/false.c \
	../lib/sbase/find.c \
	../lib/sbase/fold.c \
	../lib/sbase/getconf.c \
	../lib/sbase/head.c \
	../lib/sbase/hostname.c \
	../lib/sbase/join.c \
	../lib/sbase/kill.c \
	../lib/sbase/link.c \
	../lib/sbase/ln.c \
	../lib/sbase/logger.c \
	../lib/sbase/ls.c \
	../lib/sbase/md5sum.c \
	../lib/sbase/mkdir.c \
	../lib/sbase/mkfifo.c \
	../lib/sbase/mknod.c \
	../lib/sbase/mktemp.c \
	../lib/sbase/mv.c \
	../lib/sbase/nice.c \
	../lib/sbase/nl.c \
	../lib/sbase/nohup.c \
	../lib/sbase/paste.c \
	../lib/sbase/pathchk.c \
	../lib/sbase/printenv.c \
	../lib/sbase/printf.c \
	../lib/sbase/pwd.c \
	../lib/sbase/readlink.c \
	../lib/sbase/renice.c \
	../lib/sbase/rev.c \
	../lib/sbase/rm.c \
	../lib/sbase/rmdir.c \
	../lib/sbase/seq.c \
	../lib/sbase/setsid.c \
	../lib/sbase/sha1sum.c \
	../lib/sbase/sha224sum.c \
	../lib/sbase/sha256sum.c \
	../lib/sbase/sha384sum.c \
	../lib/sbase/sha512sum.c \
	../lib/sbase/sha512-224sum.c \
	../lib/sbase/sha512-256sum.c \
	../lib/sbase/sleep.c \
	../lib/sbase/split.c \
	../lib/sbase/sponge.c \
	../lib/sbase/strings.c \
	../lib/sbase/sync.c \
	../lib/sbase/tail.c \
	../lib/sbase/tee.c \
	../lib/sbase/test.c \
	../lib/sbase/time.c \
	../lib/sbase/touch.c \
	../lib/sbase/tr.c \
	../lib/sbase/true.c \
	../lib/sbase/tsort.c \
	../lib/sbase/tty.c \
	../lib/sbase/unexpand.c \
	../lib/sbase/uniq.c \
	../lib/sbase/unlink.c \
	../lib/sbase/wc.c \
	../lib/sbase/which.c \
	../lib/sbase/whoami.c \
	../lib/sbase/xargs.c \
	../lib/sbase/xinstall.c \
	../lib/sbase/yes.c

GENSRCS= builtins.c nodes.c syntax.c init.c
GENHDRS= builtins.h nodes.h syntax.h token.h token_vars.h
SRCS= ${SHSRCS} ${GENSRCS} ${GENHDRS} ${SBASE_SRCS}
OBJS= ${SRCS:.c=.o}
STATIC_LIBS+= ../lib/toybox/libtoybox.a ../lib/tcc/libtcc.a
STATIC_LIBS+= ../lib/sbase/libutf.a ../lib/sbase/libutil.a

CFLAGS+= -DHAVE_CONFIG_H -I. -I.. -include config.h -DBSD=1 -DSHELL -DDEBUG=1 -Wall
CFLAGS+= -DCONFIG_TCC_BCHECK=0 -DCONFIG_TCC_BACKTRACE=0 -DONE_SOURCE=0
CFLAGS+= -D_LARGEFILE64_SOURCE
CFLAGS+= -I../lib/sbase

all: ash

ash: ${GENHDRS} libtcc1a.h ${OBJS} ${STATIC_LIBS}
	${CC} -o $@ ${OBJS} ${STATIC_LIBS} ${LDFLAGS} ${LIBS}

CLEANFILES= ${GENSRCS} ${GENHDRS} ${OBJS} ash mknodes mksyntax mkinit

clean:
	rm -f ${CLEANFILES}

builtins.c builtins.h: mkbuiltins builtins.def
	sh ./mkbuiltins builtins.def

mknodes: mknodes.c
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS} ${LIBS}

nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./mknodes nodetypes nodes.c.pat

mksyntax: mksyntax.c token.h
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS} ${LIBS}

syntax.c syntax.h: mksyntax
	./mksyntax

token.h token_vars.h: mktokens
	sh ./mktokens

mkinit: mkinit.c
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS} ${LIBS}

init.c: mkinit
	./mkinit ${SHSRCS}

toybox.o: ../lib/toybox/generated/newtoys.h

%.o: %.c ${GENHDRS}
	${CC} ${CFLAGS} -c $< -o $@
