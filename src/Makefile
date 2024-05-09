-include ../config.mak

SHSRCS=	bltin/printf.c bltin/test.c bltin/times.c \
	alias.c arith_yacc.c arith_yylex.c cd.c error.c eval.c \
	exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c output.c parser.c redir.c show.c \
	signames.c trap.c var.c \
	toybox.c tcc.c ar.c
GENSRCS= builtins.c nodes.c syntax.c init.c
GENHDRS= builtins.h nodes.h syntax.h token.h token_vars.h
SRCS= ${SHSRCS} ${GENSRCS} ${GENHDRS}
OBJS= ${SRCS:.c=.o}
STATIC_LIBS= ../lib/toybox/libtoybox.a ../lib/tcc/libtcc.a

CFLAGS+= -DHAVE_CONFIG_H -I. -I.. -include ../config.h -DBSD=1 -DSHELL -DDEBUG=1 -Wall
CFLAGS+= '-DCONFIG_TRIPLET="x86_64-linux-gnu"' -DTCC_TARGET_X86_64 -DONE_SOURCE=0
CFLAGS+= -DCONFIG_TCC_BCHECK=0 -DCONFIG_TCC_BACKTRACE=0

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
