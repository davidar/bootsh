SHSRCS=	bltin/printf.c bltin/test.c bltin/times.c \
	alias.c arith_yacc.c arith_yylex.c cd.c error.c eval.c \
	exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c output.c parser.c redir.c show.c \
	signames.c trap.c var.c toybox.c
GENSRCS= builtins.c nodes.c syntax.c init.c
GENHDRS= builtins.h nodes.h syntax.h token.h
SRCS= ${SHSRCS} ${GENSRCS} ${GENHDRS}
OBJS= ${SRCS:.c=.o} ../../toybox/generated/obj/*.o

CFLAGS+= -DHAVE_CONFIG_H -I. -I.. -include ../config.h -DBSD=1 -DSHELL -DDEBUG=1 -Wall -g -O2

all: dash

dash: ${GENHDRS} ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS} ${LIBS}

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

token.h: mktokens
	sh ./mktokens

mkinit: mkinit.c
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS} ${LIBS}

init.c: mkinit
	./mkinit ${SHSRCS}

toybox.o: ../../toybox/generated/newtoys.h
