SHSRCS=	bltin/printf.c bltin/test.c bltin/times.c \
	alias.c arith_yacc.c arith_yylex.c cd.c error.c eval.c \
	exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c output.c parser.c redir.c show.c \
	signames.c trap.c var.c toybox.c tcc.o
GENSRCS= builtins.c nodes.c syntax.c init.c
GENHDRS= builtins.h nodes.h syntax.h token.h token_vars.h
SRCS= ${SHSRCS} ${GENSRCS} ${GENHDRS}
OBJS= ${SRCS:.c=.o}

CFLAGS+= -DHAVE_CONFIG_H -I. -I.. -include ../config.h -DBSD=1 -DSHELL -DDEBUG=1 -Wall -g -O2
CFLAGS+= '-DCONFIG_TRIPLET="x86_64-linux-gnu"' -DTCC_TARGET_X86_64 -DONE_SOURCE=0
CFLAGS+= -DCONFIG_TCC_BCHECK=0 -DCONFIG_TCC_BACKTRACE=0

all: dash

dash: ${GENHDRS} ${OBJS}
	rm -f ../lib/toybox/generated/obj/main.o
	${CC} -o $@ ${OBJS} ../lib/toybox/generated/obj/*.o ../lib/tcc/libtcc.a ${LDFLAGS} ${LIBS} -g

CLEANFILES= ${GENSRCS} ${GENHDRS} ${OBJS} dash mknodes mksyntax mkinit

clean:
	rm -f ${CLEANFILES}

build-tools: mknodes mksyntax

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
