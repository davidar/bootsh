#!/bin/cc -run
/*
 * mLite Language Interpreter
 * By Nils M Holm, 2014
 * Placed in the public domain.
 */

/*
 * Use -DNO_SIGNALS to disable POSIX signal handlers.
 * Use -DBITS_PER_WORD_64 on 64-bit systems.
 */

#define VERSION "2014-12-29"

/*
 * Ugly prelude to figure out if
 * we are compiling on a Un*x system.
 */

#ifdef __unix
 #ifndef unix
  #define unix
 #endif
#endif

#ifdef __NetBSD__
 #ifndef unix
  #define unix
 #endif
#endif

#ifdef __linux
 #ifndef unix
  #define unix
 #endif
#endif

#ifdef __GNUC__
 #ifndef unix
  #define unix
 #endif
#endif

#ifndef unix
 #ifndef plan9
  #error "Either 'unix' or 'plan9' must be #defined."
 #endif
#endif

#ifdef plan9
 #include <u.h>
 #include <libc.h>
 #include <stdio.h>
 #include <ctype.h>
 #define NO_SIGNALS
 #define signal(sig, fn)
 #define exit(x) exits((x)? "error": NULL)
 #define ptrdiff_t int
#endif

#ifdef unix
 #include <stdlib.h>
 #include <stddef.h>
 #include <stdio.h>
 #include <string.h>
 #include <ctype.h>
 #ifdef NO_SIGNALS
  #define signal(sig, fn)
 #else
  #include <signal.h>
  #ifndef SIGQUIT
   /* MinGW does not define SIGQUIT */
   #define SIGQUIT SIGINT
  #endif
 #endif
#endif

/*
 * Tell later MSC compilers to let us use the standard CLIB API.
 * Blake McBride < b l a k e @ m c b r i d e . n a m e >
 */

#ifdef _MSC_VER
 #if _MSC_VER > 1200
  #ifndef _CRT_SECURE_NO_DEPRECATE
   #define _CRT_SECURE_NO_DEPRECATE
  #endif
 #endif
 #ifndef _POSIX_
  #define _POSIX_
 #endif
#endif

/*
 * Why, designers of C? Why?
 */
#define set(a, b)	{ new = b; a = new; }

#ifndef INITIAL_SEGMENT_SIZE
 #define INITIAL_SEGMENT_SIZE	32768
#endif

#define TOKEN_LENGTH		1024
#define MAX_STREAMS		32
#define MAX_IO_DEPTH		65536	/* Reduce on 16-bit systems! */
#define HASH_THRESHOLD		5
#define MAX_CALL_TRACE		100

/* Default memory limit in K nodes, 0 = none */
#define DEFAULT_LIMIT_KN	12392

/* A "cell" must be large enough to hold a pointer */
#define cell	ptrdiff_t

/* Pick one ... */
/* #define BITS_PER_WORD_64 */
/* #define BITS_PER_WORD_32 */
/* #define BITS_PER_WORD_16 */

/* ... or assume a reasonable default */
#ifndef BITS_PER_WORD_16
 #ifndef BITS_PER_WORD_32
  #ifndef BITS_PER_WORD_64
   #define BITS_PER_WORD_32
  #endif
 #endif
#endif

/*
 * N-bit arithmetics require sizeof(cell) >= N/8.
 */

#ifdef BITS_PER_WORD_64
 #define DIGITS_PER_WORD	18
 #define INT_SEG_LIMIT		1000000000000000000L
 #define MANTISSA_SEGMENTS	2
 #define ENDIANNESS		0x3132333435363738L
#else
 #ifdef BITS_PER_WORD_32
  #define DIGITS_PER_WORD	9
  #define INT_SEG_LIMIT		1000000000L
  #define MANTISSA_SEGMENTS	4
  #define ENDIANNESS		0x31323334L
 #else
  #ifdef BITS_PER_WORD_16
   #define DIGITS_PER_WORD	4
   #define INT_SEG_LIMIT	10000
   #define MANTISSA_SEGMENTS	3
   #define ENDIANNESS		0x3132
  #else
   #error "BITS_PER_WORD_* undefined (this should not happen)"
  #endif
 #endif
#endif

/* Mantissa sizes differ among systems */
#define MANTISSA_SIZE   (MANTISSA_SEGMENTS * DIGITS_PER_WORD)

/*
 * Node tags
 */

#define ATOM_TAG	0x01	/* Atom, Car = type, CDR = next */
#define MARK_TAG	0x02	/* Mark */
#define STATE_TAG	0x04	/* State */
#define LIST_TAG	0x08	/* List, like tuple, but consable */
#define VECTOR_TAG	0x10	/* Vector, Car = type, CDR = content */
#define STREAM_TAG	0x20	/* Atom is an I/O stream (with ATOM_TAG) */
#define USED_TAG	0x40	/* Stream: used flag */
#define LOCK_TAG	0x80	/* Stream: locked (do not close) */

/*
 * Evaluator states
 */

enum EVAL_STATES {
	EV_ATOM,	/* Evaluating atom */
	EV_ARGS,	/* Evaluating argument list */
	EV_BETA,	/* Evaluating procedure body */
	EV_GUARD,	/* Evaluating guard expression */
	EV_IF_PRED,	/* Evaluating predicate of IF */
	EV_SET_VAL,	/* Evaluating value of SET! and DEFINE */
	EV_MACRO,	/* Evaluating value of DEFINE_SYNTAX */
	EV_BEGIN,	/* Evaluating expressions of BEGIN */
	EV_AND,		/* Evaluating arguments of ALSO */
	EV_OR,		/* Evaluating arguments of OR */
	EV_INPUT,	/* Evaluating with input redirection */
	EV_OUTPUT,	/* Evaluating with output redirection */
	EV_RAISE	/* Evaluating RAISE */
};

/*
 * Binding structure
 */

#define make_binding(v, a)	(node((v), (a)))
#define binding_box(x)		(x)
#define binding_value(x)	(cdr(x))
#define box_value(x)		(cdr(x))

/*
 * Special objects
 */

#define special_value_p(x)	((x) < 0)
#define NIL			(-1)
#define TRUE			(-2)
#define FALSE			(-3)
#define END_OF_FILE		(-4)
#define UNDEFINED		(-5)
#define UNIT			(-6)
#define NAN			(-7)
#define NOEXPR			(-8)
#define GUARD			(-9)

/*
 * Types
 */

#define T_NONE			(-20)
#define T_BOOLEAN		(-21)
#define T_CHAR			(-22)
#define T_INSTREAM		(-23)
#define T_INTEGER		(-24)
#define T_OUTSTREAM		(-25)
#define T_TUPLE			(-26)
#define T_UNIT 			(-27)
#define T_LIST         		(-28)
#define T_PRIMITIVE		(-29)
#define T_FUNCTION		(-30)
#define T_REAL			(-31)
#define T_STREAM		(-32)
#define T_STRING		(-33)
#define T_SYMBOL		(-34)
#define T_SYNTAX		(-35)
#define T_VECTOR		(-36)
#define T_CONTINUATION		(-37)

/*
 * Tokens
 */

#define TOK_COMMA	(-100)
#define TOK_SEMI	(-101)
#define TOK_PIPE	(-102)
#define TOK_CONS	(-103)
#define TOK_LPAREN	(-104)
#define TOK_RPAREN	(-105)
#define TOK_LBRACK	(-106)
#define TOK_RBRACK	(-107)
#define TOK_FROM	(-108)
#define TOK_TO		(-109)

#define TOK_ALSO	(-200)
#define TOK_AND		(-201)
#define TOK_APPLY	(-202)
#define TOK_CASE	(-203)
#define TOK_COMPILE	(-204)
#define TOK_ELSE	(-205)
#define TOK_END		(-206)
#define TOK_EVAL	(-207)
#define TOK_EXCEPTION	(-208)
#define TOK_FN		(-209)
#define TOK_FUN		(-210)
#define TOK_HANDLE	(-211)
#define TOK_IF		(-212)
#define TOK_IN		(-213)
#define TOK_INFIX	(-214)
#define TOK_INFIXR	(-215)
#define TOK_LET		(-216)
#define TOK_LOCAL	(-217)
#define TOK_NONFIX	(-218)
#define TOK_OF		(-219)
#define TOK_OP		(-220)
#define TOK_OR		(-221)
#define TOK_RAISE	(-222)
#define TOK_REF		(-223)
#define TOK_THEN	(-224)
#define TOK_TYPE	(-225)
#define TOK_VAL		(-226)
#define TOK_WHERE	(-227)

#define TOK_NUM		(-300)
#define TOK_STR		(-301)
#define TOK_CHR		(-302)
#define TOK_ID		(-303)
#define TOK_BOOL	(-304)

/*
 * Short cuts for primitive procedure definitions
 * Yes, ___ violates the C standard, but it's too tempting
 */

#define BOL T_BOOLEAN
#define CHR T_CHAR
#define INS T_INSTREAM
#define INT T_INTEGER
#define LST T_LIST
#define OUS T_OUTSTREAM
#define TUP T_TUPLE
#define UNI T_UNIT
#define FUN T_FUNCTION
#define REA T_REAL
#define IOS T_STREAM
#define STR T_STRING
#define SYM T_SYMBOL
#define VEC T_VECTOR
#define ___ T_NONE

struct Primitive_procedure {
	char	*name;
	cell	(*handler)(cell expr);
	int	min_args;
	int	max_args;	/* -1 = variadic */
	int	arg_types[3];
};

#define PRIM    struct Primitive_procedure

#define PRIM_SEG_SIZE	256

/*
 * Globals
 */

int	Cons_segment_size,
	Vec_segment_size;
int	Cons_pool_size,
	Vec_pool_size;

cell	*Car,
	*Cdr;
char	*Tag;

cell	*Vectors;

cell	Free_list;
cell	Free_vecs;

PRIM	*Primitives;
int	Last_prim, Max_prims;

cell	Stack,
	Stack_bottom;
cell	State_stack;
cell	Tmp_car,
	Tmp_cdr,
	Tmp;
cell	Symbols;
cell	Program;
cell	Environment;
cell	Acc;
cell	Callcc_magic;

cell	Ops;
int	Token;
cell	Attr;
cell	Exceptions;
cell	Exnstack;

cell	Guard_ops;
int	Guard_lev;

int	Level;
int	Load_level;
int	Displaying;

cell    Called_procedures[MAX_CALL_TRACE];
int     Proc_ptr, Proc_max;
cell    File_list;
int     Line_no;
int	Opening_line;
int     Printer_count, Printer_limit;

FILE		*Streams[MAX_STREAMS];
unsigned char	Stream_flags[MAX_STREAMS];
int		Instream,
		Outstream,
		Errstream;

char	**Command_line;
long	Memory_limit_kn;
int	Quiet_mode;

volatile int     Error_flag;

/* mLite operators */

struct _operator {
	char	*name;
	int	prec;
	int	assoc;
};

#define LEFT	1
#define RIGHT	2
#define NONE	0

#define operator struct _operator

operator Op_init[] = {
	{ "",   -1, NONE  },
	{ "<",   1, LEFT  },
	{ "<=",  1, LEFT  },
	{ "<>",  1, LEFT  },
	{ "=",   1, LEFT  },
	{ ">",   1, LEFT  },
	{ ">=",  1, LEFT  },
	{ "~<",  1, LEFT  },
	{ "~<=", 1, LEFT  },
	{ "~<>", 1, LEFT  },
	{ "~=",  1, LEFT  },
	{ "~>",  1, LEFT  },
	{ "~>=", 1, LEFT  },
	{ "::",  2, RIGHT },
	{ "@",   2, RIGHT },
	{ "+",   3, LEFT  },
	{ "-",   3, LEFT  },
	{ "*",   4, LEFT  },
	{ "div", 4, LEFT  },
	{ "rem", 4, LEFT  },
	{ "/",   4, LEFT  },
	{ "",    0, NONE  }
};

/* Short cuts for accessing predefined symbols */
cell	S_callcc, S_cons, S_else, S_equal, S_extensions,
	S_greater, S_ignore, S_it, S_less,
	S_list, S_loading, S_primlist, S_raise, S_ref, S_register,
	S_quasiquote, S_quote, S_unquote, S_unquote_splicing,
	S_unregister, S_tuple, S_typecheck;
cell	S_also, S_begin, S_define, S_define_syntax,
	S_define_type, S_fn, S_from, S_if, S_letrec, S_or,
	S_setb, S_to;

/*
 * I/O
 */

#define nl()            pr("\n")
#define reject(c)       ungetc(c, Streams[Instream])
#define read_c()        getc(Streams[Instream])

/*
 * Access to fields of atoms
 */

#define string(n)       ((char *) &Vectors[Cdr[n]])
#define string_len(n)   (Vectors[Cdr[n] - 1])
#define symbol_name(n)  (string(n))
#define symbol_len(n)   (string_len(n))
#define vector(n)       (&Vectors[Cdr[n]])
#define vector_link(n)  (Vectors[Cdr[n] - 3])
#define vector_index(n) (Vectors[Cdr[n] - 2])
#define vector_size(k)  (((k) + sizeof(cell)-1) / sizeof(cell) + 3)
#define vector_len(n)   (vector_size(string_len(n)) - 3)
#define stream_id(n)    (cadr(n))
#define char_value(n)   (cadr(n))

/*
 * Internal vector representation
 */

#define RAW_VECTOR_LINK         0
#define RAW_VECTOR_INDEX        1
#define RAW_VECTOR_SIZE         2
#define RAW_VECTOR_DATA         3

/*
 * Flags and structure of real numbers
 */

#define x_real_flags(x)          (cadr(x))
#define x_real_exponent(x)       (caddr(x))
#define x_real_mantissa(x)       (cdddr(x))

#define REAL_NEGATIVE   0x01

#define x_real_negative_flag(x)  (x_real_flags(x) & REAL_NEGATIVE)

/*
 * Nested lists
 */

#define car(x)          (Car[x])
#define cdr(x)          (Cdr[x])
#define caar(x)         (Car[Car[x]])
#define cadr(x)         (Car[Cdr[x]])
#define cdar(x)         (Cdr[Car[x]])
#define cddr(x)         (Cdr[Cdr[x]])
#define caaar(x)        (Car[Car[Car[x]]])
#define caadr(x)        (Car[Car[Cdr[x]]])
#define cadar(x)        (Car[Cdr[Car[x]]])
#define caddr(x)        (Car[Cdr[Cdr[x]]])
#define cdaar(x)        (Cdr[Car[Car[x]]])
#define cdadr(x)        (Cdr[Car[Cdr[x]]])
#define cddar(x)        (Cdr[Cdr[Car[x]]])
#define cdddr(x)        (Cdr[Cdr[Cdr[x]]])
#define caaadr(x)       (Car[Car[Car[Cdr[x]]]])
#define caaddr(x)       (Car[Car[Cdr[Cdr[x]]]])
#define caddar(x)       (Car[Cdr[Cdr[Car[x]]]])
#define cadadr(x)       (Car[Cdr[Car[Cdr[x]]]])
#define cadddr(x)       (Car[Cdr[Cdr[Cdr[x]]]])
#define cdadar(x)       (Cdr[Car[Cdr[Car[x]]]])
#define cddaar(x)       (Cdr[Cdr[Car[Car[x]]]])
#define cddadr(x)       (Cdr[Cdr[Car[Cdr[x]]]])
#define cdddar(x)       (Cdr[Cdr[Cdr[Car[x]]]])
#define cddddr(x)       (Cdr[Cdr[Cdr[Cdr[x]]]])

/*
 * Type predicates
 */

#define virtual_p(n)	((n) < 0)

#define eof_p(n)	((n) == END_OF_FILE)
#define undefined_p(n)	((n) == UNDEFINED)
#define unit_p(n)	((n) == UNIT)

#define boolean_p(n)	((n) == TRUE || (n) == FALSE)

#define integer_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && Car[n] == T_INTEGER)
#define number_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && \
		(Car[n] == T_REAL || Car[n] == T_INTEGER))
#define primitive_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && Car[n] == T_PRIMITIVE)
#define procedure_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && Car[n] == T_FUNCTION)
#define continuation_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && \
		Car[n] == T_CONTINUATION)
#define real_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && Car[n] == T_REAL)
#define special_p(n)	((n) == S_quote	      || \
			 (n) == S_begin	      || \
			 (n) == S_if	      || \
			 (n) == S_also        || \
			 (n) == S_or          || \
			 (n) == S_fn          || \
			 (n) == S_letrec      || \
			 (n) == S_setb	      || \
			 (n) == S_raise       || \
			 (n) == S_define      || \
			 (n) == S_define_type || \
			 (n) == S_from        || \
			 (n) == S_to          || \
			 (n) == S_define_syntax)
#define char_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && Car[n] == T_CHAR)
#define syntax_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && Car[n] == T_SYNTAX)
#define instream_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && (Tag[n] & STREAM_TAG) \
	 && Car[n] == T_INSTREAM)
#define outstream_p(n) \
	(!special_value_p(n) && (Tag[n] & ATOM_TAG) && (Tag[n] & STREAM_TAG) \
	 && Car[n] == T_OUTSTREAM)

#define symbol_p(n) \
	(!special_value_p(n) && (Tag[n] & VECTOR_TAG) && Car[n] == T_SYMBOL)
#define vector_p(n) \
	(!special_value_p(n) && (Tag[n] & VECTOR_TAG) && Car[n] == T_VECTOR)
#define string_p(n) \
	(!special_value_p(n) && (Tag[n] & VECTOR_TAG) && Car[n] == T_STRING)

#define atom_p(n) \
	(special_value_p(n) || (Tag[n] & ATOM_TAG) || (Tag[n] & VECTOR_TAG))

#define consname_p(n) \
	(symbol_name(n)[0] == ':')

#define auto_quoting_p(n) atom_p(n)

#define cons_p(x)  (!atom_p(x))
#define tuple_p(x) (cons_p(x) && !(Tag[x] & LIST_TAG))
#define list_p(x)  ((cons_p(x) && (Tag[x] & LIST_TAG)) || (x) == NIL)

/*
 * Rib structure
 */

#define rib_args(x)     (car(x))
#define rib_append(x)   (cadr(x))
#define rib_result(x)   (caddr(x))
#define rib_source(x)   (cdddr(x))

/*
 * Allocators
 */

#define node(pa, pd)            node3((pa), (pd), 0)
#define cons(pa, pd)		node3((pa), (pd), LIST_TAG)
#define new_atom(pa, pd)        node3((pa), (pd), ATOM_TAG)

#define save(n)         (Stack = node((n), Stack))
#define save_state(v)   (State_stack = node3((v), State_stack, ATOM_TAG))

/*
 * Bignum arithmetics
 */

#define x_bignum_negative_p(a)	((cadr(a)) < 0)
#define x_bignum_zero_p(a)	((cadr(a) == 0) && (cddr(a)) == NIL)
#define x_bignum_positive_p(a) \
		(!x_bignum_negative_p(a) && !x_bignum_zero_p(a))

/*
 * Real-number arithmetics
 */

#define x_real_zero_p(x) \
	(car(x_real_mantissa(x)) == 0 && cdr(x_real_mantissa(x)) == NIL)

#define x_real_negative_p(x) \
	(x_real_negative_flag(x) && !x_real_zero_p(x))

#define x_real_positive_p(x) \
	(!x_real_negative_flag(x) && !x_real_zero_p(x))

#define x_real_negate(a) \
	make_real(x_real_flags(a) & REAL_NEGATIVE?	\
			x_real_flags(a) & ~REAL_NEGATIVE: \
			x_real_flags(a) |  REAL_NEGATIVE, \
		x_real_exponent(a), x_real_mantissa(a))

/*
 * Prototypes
 */

void	add_primitives(char *name, PRIM *p);
cell	symbol_ref(char *s);
cell	node3(cell pcar, cell pcdr, int ptag);
int	new_stream(void);
char	*copy_string(char *s);
cell	error(char *msg, cell expr);
void	fatal(char *msg);
int	length(cell x);
cell	make_char(int c);
cell	make_integer(cell i);
cell	make_stream(int sid, cell type);
cell	make_string(char *s, int k);
cell	unsave(int k);

int	Verbose_GC = 0;

cell	*GC_root[] = { &Program, &Symbols, &Environment, &Tmp,
			&Tmp_car, &Tmp_cdr, &Stack, &Stack_bottom,
			&State_stack, &Acc, &File_list, &Ops, &Attr,
			&Exceptions, &Exnstack, NULL };

/*
 * Counting
 */

int	Run_stats, Cons_stats;

struct counter {
	int	n, n1k, n1m, n1g, n1t;
};

struct counter  Reductions,
		Conses,
		Nodes,
		Collections;

void reset_counter(struct counter *c) {
	c->n = 0;
	c->n1k = 0;
	c->n1m = 0;
	c->n1g = 0;
	c->n1t = 0;
}

void count(struct counter *c) {
	char	msg[] = "statistics counter overflow";
	c->n++;
	if (c->n >= 1000) {
		c->n -= 1000;
		c->n1k++;
		if (c->n1k >= 1000) {
			c->n1k -= 1000;
			c->n1m++;
			if (c->n1m >= 1000) {
				c->n1m -= 1000;
				c->n1g++;
				if (c->n1g >= 1000) {
					c->n1t -= 1000;
					c->n1t++;
					if (c->n1t >= 1000) {
						error(msg, NOEXPR);
					}
				}
			}
		}
	}
}

cell counter_to_list(struct counter *c) {
	cell	n, m;

	n = make_integer(c->n);
	n = node(n, NIL);
	save(n);
	m = make_integer(c->n1k);
	n = node(m, n);
	car(Stack) = n;
	m = make_integer(c->n1m);
	n = node(m, n);
	car(Stack) = n;
	m = make_integer(c->n1g);
	n = node(m, n);
	car(Stack) = n;
	m = make_integer(c->n1t);
	n = node(m, n);
	unsave(1);
	return n;
}

cell error(char *msg, cell expr);

void flush(void) {
	fflush(Streams[Outstream]);
}

void pr_raw(char *s, int k) {
	if (Printer_limit && Printer_count > Printer_limit) {
		if (Printer_limit > 0)
			fwrite("...", 1, 3, Streams[Outstream]);
		Printer_limit = -1;
		return;
	}
	fwrite(s, 1, k, Streams[Outstream]);
	if (Outstream == 1 && s[k-1] == '\n')
		flush();
	Printer_count += k;
}

void pr(char *s) {
	if (Streams[Outstream] == NULL)
		error("outstream is not open", NOEXPR);
	else
		pr_raw(s, strlen(s));
}

/*
 * Error Handling
 */

void bye(int n) {
	exit(n);
}

void print_form(cell n);

void print_error_form(cell n) {
	Printer_limit = 50;
	Printer_count = 0;
	print_form(n);
	Printer_limit = 0;
}

void print_calltrace(void) {
	int	i, j;

	for (i=0; i<Proc_max; i++)
		if (Called_procedures[i] != NIL)
			break;
	if (i == Proc_max)
		return;
	pr("call trace:");
	i = Proc_ptr;
	for (j=0; j<Proc_max; j++) {
		if (i >= Proc_max)
			i = 0;
		if (Called_procedures[i] != NIL) {
			pr(" ");
			print_form(Called_procedures[i]);
		}
		i++;
	}
	nl();
}

cell error(char *msg, cell expr) {
	int	os;
	char	buf[100];

	if (Error_flag)
		return UNDEFINED;
	os = Outstream;
	Outstream = Quiet_mode? 2: 1;
	Error_flag = 1;
	pr("error: ");
	if (box_value(S_loading) == TRUE) {
		if (File_list != NIL) {
			print_form(car(File_list));
			pr(": ");
		}
		sprintf(buf, "%d: ", Line_no);
		pr(buf);
	}
	pr(msg);
	if (expr != NOEXPR) {
		pr(": ");
		Error_flag = 0;
		print_error_form(expr);
		Error_flag = 1;
	}
	nl();
	print_calltrace();
	Outstream = os;
	if (Quiet_mode)
		bye(1);
	return UNDEFINED;
}

void fatal(char *msg) {
	Outstream = Quiet_mode? 2: 1;
	pr("fatal ");
	Error_flag = 0;
	error(msg, NOEXPR);
	bye(2);
}

/*
 * Memory Management
 */

void new_cons_segment(void) {
	Car = realloc(Car, sizeof(cell) * (Cons_pool_size+Cons_segment_size));
	Cdr = realloc(Cdr, sizeof(cell) * (Cons_pool_size+Cons_segment_size));
	Tag = realloc(Tag, Cons_pool_size + Cons_segment_size);
	if (Car == NULL || Cdr == NULL || Tag == NULL)
		fatal("new_cons_segment: out of physical memory");
	memset(&car(Cons_pool_size), 0, Cons_segment_size * sizeof(cell));
	memset(&cdr(Cons_pool_size), 0, Cons_segment_size * sizeof(cell));
	memset(&Tag[Cons_pool_size], 0, Cons_segment_size);
	Cons_pool_size += Cons_segment_size;
	Cons_segment_size = Cons_segment_size * 3 / 2;
}

void new_vec_segment(void) {
	Vectors = realloc(Vectors, sizeof(cell) *
			(Vec_pool_size + Vec_segment_size));
	if (Vectors == NULL)
		fatal("out of physical memory");
	memset(&Vectors[Vec_pool_size], 0, Vec_segment_size * sizeof(cell));
	Vec_pool_size += Vec_segment_size;
	Vec_segment_size = Vec_segment_size * 3 / 2;
}

/*
 * Mark nodes which can be accessed through N.
 * Using the Deutsch/Schorr/Waite pointer reversal algorithm.
 * S0: M==0, S==0, unvisited, process CAR (vectors: process 1st slot);
 * S1: M==1, S==1, CAR visited, process CDR (vectors: process next slot);
 * S2: M==1, S==0, completely visited, return to parent.
 */

void mark(cell n) {
	cell	p, parent, *v;
	int	i;

	parent = NIL;	/* Initially, there is no parent node */
	while (1) {
		if (special_value_p(n) || Tag[n] & MARK_TAG) {
			if (parent == NIL)
				break;
			if (Tag[parent] & VECTOR_TAG) {	/* S1 --> S1|done */
				i = vector_index(parent);
				v = vector(parent);
				if (Tag[parent] & STATE_TAG &&
				    i+1 < vector_len(parent)
				) {			/* S1 --> S1 */
					p = v[i+1];
					v[i+1] = v[i];
					v[i] = n;
					n = p;
					vector_index(parent) = i+1;
				}
				else {			/* S1 --> done */
					p = parent;
					parent = v[i];
					v[i] = n;
					n = p;
				}
			}
			else if (Tag[parent] & STATE_TAG) {	/* S1 --> S2 */
				p = cdr(parent);
				cdr(parent) = car(parent);
				car(parent) = n;
				Tag[parent] &= ~STATE_TAG;
				Tag[parent] |=  MARK_TAG;
				n = p;
			}
			else {				/* S2 --> done */
				p = parent;
				parent = cdr(p);
				cdr(p) = n;
				n = p;
			}
		}
		else {
			if (Tag[n] & VECTOR_TAG) {	/* S0 --> S1|S2 */
				Tag[n] |= MARK_TAG;
				/* Tag[n] &= ~STATE_TAG; */
				vector_link(n) = n;
				if (car(n) == T_VECTOR && vector_len(n) != 0) {
					Tag[n] |= STATE_TAG;
					vector_index(n) = 0;
					v = vector(n);
					p = v[0];
					v[0] = parent;
					parent = n;
					n = p;
				}
			}
			else if (Tag[n] & ATOM_TAG) {	/* S0 --> S2 */
				if (instream_p(n) || outstream_p(n))
					Stream_flags[stream_id(n)] |= USED_TAG;
				p = cdr(n);
				cdr(n) = parent;
				/*Tag[n] &= ~STATE_TAG;*/
				parent = n;
				n = p;
				Tag[parent] |= MARK_TAG;
			}
			else {				/* S0 --> S1 */
				p = car(n);
				car(n) = parent;
				Tag[n] |= MARK_TAG;
				parent = n;
				n = p;
				Tag[parent] |= STATE_TAG;
			}
		}
	}
}

/* Mark and sweep GC. */
int gc(void) {
	int	i, k;
	char	buf[100];

	if (Run_stats)
		count(&Collections);
	for (i=0; i<MAX_STREAMS; i++)
		if (Stream_flags[i] & LOCK_TAG)
			Stream_flags[i] |= USED_TAG;
		else
			Stream_flags[i] &= ~USED_TAG;
	for (i=0; GC_root[i] != NULL; i++)
		mark(GC_root[i][0]);
	k = 0;
	Free_list = NIL;
	for (i=0; i<Cons_pool_size; i++) {
		if (!(Tag[i] & MARK_TAG)) {
			cdr(i) = Free_list;
			Free_list = i;
			k++;
		}
		else {
			Tag[i] &= ~MARK_TAG;
		}
	}
	for (i=0; i<MAX_STREAMS; i++) {
		if (!(Stream_flags[i] & USED_TAG) && Streams[i] != NULL) {
			fclose(Streams[i]);
			Streams[i] = NULL;
		}
	}
	if (Verbose_GC > 1) {
		sprintf(buf, "GC: %d nodes reclaimed", k);
		pr(buf); nl();
	}
	return k;
}

/* Allocate a fresh node and initialize with PCAR,PCDR,PTAG. */
cell node3(cell pcar, cell pcdr, int ptag) {
	cell	n;
	int	k;
	char	buf[100];

	if (Run_stats) {
		count(&Nodes);
		if (Cons_stats)
			count(&Conses);
	}
	if (Free_list == NIL) {
		if (ptag == 0)
			Tmp_car = pcar;
		if (ptag != VECTOR_TAG && ptag != STREAM_TAG)
			Tmp_cdr = pcdr;
		k = gc();
		/*
		 * Performance increases dramatically if we
		 * do not wait for the pool to run dry.
		 * In fact, don't even let it come close to that.
		 */
		if (k < Cons_pool_size / 2) {
			if (	Memory_limit_kn &&
				Cons_pool_size + Cons_segment_size
					> Memory_limit_kn
			) {
				error("hit memory limit", NOEXPR);
			}
			else {
				new_cons_segment();
				if (Verbose_GC) {
					sprintf(buf,
						"GC: new segment,"
						 " nodes = %d,"
						 " next segment = %d",
						Cons_pool_size,
						Cons_segment_size);
					pr(buf); nl();
				}
				gc();
			}
		}
		Tmp_car = Tmp_cdr = NIL;
	}
	if (Free_list == NIL)
		fatal("cons3: failed to recover from low memory condition");
	n = Free_list;
	Free_list = cdr(Free_list);
	car(n) = pcar;
	cdr(n) = pcdr;
	Tag[n] = ptag;
	return n;
}

/* Mark all vectors unused */
void unmark_vectors(void) {
	int	p, k, link;

	p = 0;
	while (p < Free_vecs) {
		link = p;
		k = Vectors[p + RAW_VECTOR_SIZE];
		p += vector_size(k);
		Vectors[link] = NIL;
	}
}

/* In situ vector pool garbage collection and compaction */
int gcv(void) {
	int	v, k, to, from;
	char	buf[100];

	unmark_vectors();
	gc();		/* re-mark live vectors */
	to = from = 0;
	while (from < Free_vecs) {
		v = Vectors[from + RAW_VECTOR_SIZE];
		k = vector_size(v);
		if (Vectors[from + RAW_VECTOR_LINK] != NIL) {
			if (to != from) {
				memmove(&Vectors[to], &Vectors[from],
					k * sizeof(cell));
				cdr(Vectors[to + RAW_VECTOR_LINK]) =
					to + RAW_VECTOR_DATA;
			}
			to += k;
		}
		from += k;
	}
	k = Free_vecs - to;
	if (Verbose_GC > 1) {
		sprintf(buf, "GC: gcv: %d cells reclaimed", k);
		pr(buf); nl();
	}
	Free_vecs = to;
	return k;
}

/* Allocate vector from pool */
cell new_vec(cell type, int size) {
	cell	n;
	int	v, wsize;
	char	buf[100];

	wsize = vector_size(size);
	if (Free_vecs + wsize >= Vec_pool_size) {
		gcv();
		while (	Free_vecs + wsize >=
			Vec_pool_size - Vec_pool_size / 2
		) {
			if (	Memory_limit_kn &&
				Vec_pool_size + Vec_segment_size
					> Memory_limit_kn
			) {
				error("hit memory limit", NOEXPR);
				break;
			}
			else {
				new_vec_segment();
				gcv();
				if (Verbose_GC) {
					sprintf(buf,
						"GC: new_vec: new segment,"
						 " cells = %d",
						Vec_pool_size);
					pr(buf); nl();
				}
			}
		}
	}
	if (Free_vecs + wsize >= Vec_pool_size)
		fatal("new_vec: failed to recover from low memory condition");
	v = Free_vecs;
	Free_vecs += wsize;
	n = node3(type, v + RAW_VECTOR_DATA, VECTOR_TAG);
	Vectors[v + RAW_VECTOR_LINK] = n;
	Vectors[v + RAW_VECTOR_INDEX] = 0;
	Vectors[v + RAW_VECTOR_SIZE] = size;
	return n;
}

/* Pop K nodes off the Stack, return last one. */
cell unsave(int k) {
	cell	n = NIL; /*LINT*/

	while (k) {
		if (Stack == NIL)
			fatal("unsave: stack underflow");
		n = car(Stack);
		Stack = cdr(Stack);
		k--;
	}
	return n;
}

/*
 * Integer Arithmetics (Bignums)
 */

cell make_integer(cell i) {
	cell	n;

	n = new_atom(i, NIL);
	return new_atom(T_INTEGER, n);
}

cell integer_value(char *src, cell x) {
	char	msg[100];

	if (cddr(x) != NIL) {
		sprintf(msg, "%s: integer argument too big", src);
		error(msg, x);
		return 0;
	}
	return cadr(x);
}

cell bignum_abs(cell a) {
	cell	n;

	n = new_atom(labs(cadr(a)), cddr(a));
	return new_atom(T_INTEGER, n);
}

cell bignum_negate(cell a) {
	cell	n;

	n = new_atom(-cadr(a), cddr(a));
	n = new_atom(T_INTEGER, n);
	return n;
}

cell reverse_segments(cell n) {
	cell	m;

	m = NIL;
	while (n != NIL) {
		m = new_atom(car(n), m);
		n = cdr(n);
	}
	return m;
}

cell bignum_add(cell a, cell b);
cell bignum_subtract(cell a, cell b);

cell x_bignum_add(cell a, cell b) {
	cell	fa, fb, result, r;
	int	carry;

	if (x_bignum_negative_p(a)) {
		if (x_bignum_negative_p(b)) {
			/* -A+-B --> -(|A|+|B|) */
			a = bignum_abs(a);
			save(a);
			a = bignum_add(a, bignum_abs(b));
			unsave(1);
			return bignum_negate(a);
		}
		else {
			/* -A+B --> B-|A| */
			return bignum_subtract(b, bignum_abs(a));
		}
	}
	else if (x_bignum_negative_p(b)) {
		/* A+-B --> A-|B| */
		return bignum_subtract(a, bignum_abs(b));
	}
	/* A+B */
	a = reverse_segments(cdr(a));
	save(a);
	b = reverse_segments(cdr(b));
	save(b);
	carry = 0;
	result = NIL;
	save(result);
	while (a != NIL || b != NIL || carry) {
		fa = a == NIL? 0: car(a);
		fb = b == NIL? 0: car(b);
		r = fa + fb + carry;
		carry = 0;
		if (r >= INT_SEG_LIMIT) {
			r -= INT_SEG_LIMIT;
			carry = 1;
		}
		result = new_atom(r, result);
		car(Stack) = result;
		if (a != NIL) a = cdr(a);
		if (b != NIL) b = cdr(b);
	}
	unsave(3);
	return new_atom(T_INTEGER, result);
}

cell bignum_add(cell a, cell b) {
	Tmp = b;
	save(a);
	save(b);
	Tmp = NIL;
	a = x_bignum_add(a, b);
	unsave(2);
	return a;
}

int bignum_less_p(cell a, cell b) {
	int	ka, kb, neg_a, neg_b;

	neg_a = x_bignum_negative_p(a);
	neg_b = x_bignum_negative_p(b);
	if (neg_a && !neg_b) return 1;
	if (!neg_a && neg_b) return 0;
	ka = length(a);
	kb = length(b);
	if (ka < kb) return neg_a? 0: 1;
	if (ka > kb) return neg_a? 1: 0;
	Tmp = b;
	a = bignum_abs(a);
	save(a);
	b = bignum_abs(b);
	unsave(1);
	Tmp = NIL;
	a = cdr(a);
	b = cdr(b);
	while (a != NIL) {
		if (car(a) < car(b)) return neg_a? 0: 1;
		if (car(a) > car(b)) return neg_a? 1: 0;
		a = cdr(a);
		b = cdr(b);
	}
	return 0;
}

int bignum_equal_p(cell a, cell b) {
	a = cdr(a);
	b = cdr(b);
	while (a != NIL && b != NIL) {
		if (car(a) != car(b))
			return 0;
		a = cdr(a);
		b = cdr(b);
	}
	return a == NIL && b == NIL;
}

cell x_bignum_subtract(cell a, cell b) {
	cell	fa, fb, result, r;
	int	borrow;

	if (x_bignum_negative_p(a)) {
		if (x_bignum_negative_p(b)) {
			/* -A--B --> -A+|B| --> |B|-|A| */
			a = bignum_abs(a);
			save(a);
			a = bignum_subtract(bignum_abs(b), a);
			unsave(1);
			return a;
		}
		else {
			/* -A-B --> -(|A|+B) */
			return bignum_negate(bignum_add(bignum_abs(a), b));
		}
	}
	else if (x_bignum_negative_p(b)) {
		/* A--B --> A+|B| */
		return bignum_add(a, bignum_abs(b));
	}
	/* A-B, A<B --> -(B-A) */
	if (bignum_less_p(a, b))
		return bignum_negate(bignum_subtract(b, a));
	/* A-B, A>=B */
	a = reverse_segments(cdr(a));
	save(a);
	b = reverse_segments(cdr(b));
	save(b);
	borrow = 0;
	result = NIL;
	save(result);
	while (a != NIL || b != NIL || borrow) {
		fa = a == NIL? 0: car(a);
		fb = b == NIL? 0: car(b);
		r = fa - fb - borrow;
		borrow = 0;
		if (r < 0) {
			r += INT_SEG_LIMIT;
			borrow = 1;
		}
		result = new_atom(r, result);
		car(Stack) = result;
		if (a != NIL) a = cdr(a);
		if (b != NIL) b = cdr(b);
	}
	unsave(3);
	while (car(result) == 0 && cdr(result) != NIL)
		result = cdr(result);
	return new_atom(T_INTEGER, result);
}

cell bignum_subtract(cell a, cell b) {
	Tmp = b;
	save(a);
	save(b);
	Tmp = NIL;
	a = x_bignum_subtract(a, b);
	unsave(2);
	return a;
}

cell bignum_shift_left(cell a, int fill) {
	cell	r, c, result;
	int	carry;

	a = reverse_segments(cdr(a));
	save(a);
	carry = fill;
	result = NIL;
	save(result);
	while (a != NIL) {
		if (car(a) >= INT_SEG_LIMIT/10) {
			c = car(a) / (INT_SEG_LIMIT/10);
			r = car(a) % (INT_SEG_LIMIT/10) * 10;
			r += carry;
			carry = c;
		}
		else {
			r = car(a) * 10 + carry;
			carry = 0;
		}
		result = new_atom(r, result);
		car(Stack) = result;
		a = cdr(a);
	}
	if (carry)
		result = new_atom(carry, result);
	unsave(2);
	return new_atom(T_INTEGER, result);
}

/* Result: (a/10 . a%10) */
cell bignum_shift_right(cell a) {
	cell	r, c, result;
	int	carry;

	a = cdr(a);
	save(a);
	carry = 0;
	result = NIL;
	save(result);
	while (a != NIL) {
		c = car(a) % 10;
		r = car(a) / 10;
		r += carry * (INT_SEG_LIMIT/10);
		carry = c;
		result = new_atom(r, result);
		car(Stack) = result;
		a = cdr(a);
	}
	result = reverse_segments(result);
	if (car(result) == 0 && cdr(result) != NIL)
		result = cdr(result);
	result = new_atom(T_INTEGER, result);
	car(Stack) = result;
	carry = make_integer(carry);
	unsave(2);
	return node(result, carry);
}

cell bignum_multiply(cell a, cell b) {
	int	neg;
	cell	r, i, result;

	neg = x_bignum_negative_p(a) != x_bignum_negative_p(b);
	a = bignum_abs(a);
	save(a);
	b = bignum_abs(b);
	save(b);
	result = make_integer(0);
	save(result);
	while (!x_bignum_zero_p(a)) {
		if (Error_flag)
			break;
		r = bignum_shift_right(a);
		i = caddr(r);
		a = car(r);
		caddr(Stack) = a;
		while (i) {
			result = bignum_add(result, b);
			car(Stack) = result;
			i--;
		}
		b = bignum_shift_left(b, 0);
		cadr(Stack) = b;
	}
	if (neg)
		result = bignum_negate(result);
	unsave(3);
	return result;
}

/*
 * Equalize A and B, e.g.:
 * A=123, B=12345 ---> 12300, 100
 * Return (scaled-a . scaling-factor)
 */
cell bignum_equalize(cell a, cell b) {
	cell	r, f, r0, f0;

	r0 = a;
	save(r0);
	f0 = make_integer(1);
	save(f0);
	r = r0;
	save(r);
	f = f0;
	save(f);
	while (bignum_less_p(r, b)) {
		cadddr(Stack) = r0 = r;
		caddr(Stack) = f0 = f;
		r = bignum_shift_left(r, 0);
		cadr(Stack) = r;
		f = bignum_shift_left(f, 0);
		car(Stack) = f;
	}
	unsave(4);
	return node(r0, f0);
}

/* Result: (a/b . a%b) */
cell x_bignum_divide(cell a, cell b) {
	int	neg, neg_a;
	cell	result, f;
	int	i;
	cell	c, c0;

	neg_a = x_bignum_negative_p(a);
	neg = neg_a != x_bignum_negative_p(b);
	a = bignum_abs(a);
	save(a);
	b = bignum_abs(b);
	save(b);
	if (bignum_less_p(a, b)) {
		if (neg_a)
			a = bignum_negate(a);
		Tmp = a;
		f = make_integer(0);
		Tmp = NIL;
		unsave(2);
		return node(f, a);
	}
	b = bignum_equalize(b, a);
	cadr(Stack) = b; /* cadr+cddddr */
	car(Stack) = a;	/* car+cddddr */
	c = NIL;
	save(c);	/* cadddr */
	c0 = NIL;
	save(c0);	/* caddr */
	f = cdr(b);
	b = car(b);
	cadddr(Stack) = b;
	save(f);	/* cadr */
	result = make_integer(0);
	save(result);	/* car */
	while (!x_bignum_zero_p(f)) {
		if (Error_flag)
			break;
		c = make_integer(0);
		cadddr(Stack) = c;
		caddr(Stack) = c0 = c;
		i = 0;
		while (!bignum_less_p(a, c)) {
			if (Error_flag)
				break;
			caddr(Stack) = c0 = c;
			c = bignum_add(c, b);
			cadddr(Stack) = c;
			i++;
		}
		result = bignum_shift_left(result, i-1);
		car(Stack) = result;
		a = bignum_subtract(a, c0);
		car(cddddr(Stack)) = a;
		f = bignum_shift_right(f);
		f = car(f);
		cadr(Stack) = f;
		b = bignum_shift_right(b);
		b = car(b);
		cadr(cddddr(Stack)) = b;
	}
	if (neg)
		result = bignum_negate(result);
	car(Stack) = result;
	if (neg_a)
		a = bignum_negate(a);
	unsave(6);
	return node(result, a);
}

cell bignum_divide(cell x, cell a, cell b) {
	if (x_bignum_zero_p(b))
		return error("divide by zero", x);
	Tmp = b;
	save(a);
	save(b);
	Tmp = NIL;
	a = x_bignum_divide(a, b);
	unsave(2);
	return a;
}

/*
 * Real Number Arithmetics
 */

/*
 * Functions and macros with a "x_real_" prefix expect data
 * objects of the primitive "real" type. Passing bignums to
 * them will result in mayhem.
 *
 * Functions with a "real_" prefix will delegate integer
 * operations to the corresponding bignum functions and
 * convert mixed arguments to real.
 */

cell make_real(int flags, cell exp, cell mant) {
	cell	n;

	n = new_atom(exp, mant);
	n = new_atom(flags, n);
	return new_atom(T_REAL, n);
}

cell count_digits(cell m) {
	int	k = 0;
	cell	x;

	x = car(m);
	k = 0;
	while (x != 0) {
		x /= 10;
		k++;
	}
	k = k==0? 1: k;
	m = cdr(m);
	while (m != NIL) {
		k += DIGITS_PER_WORD;
		m = cdr(m);
	}
	return k;
}

/*
 * Remove trailing zeros and move the decimal
 * point to the END of the mantissa, e.g.:
 * real_normalize(1.234e0) --> 1234e-3
 *
 * Limit the mantissa to MANTISSA_SEGMENTS
 * machine words. This may cause a loss of
 * precision.
 *
 * Also handle numeric overflow/underflow.
 */

cell real_normalize(cell x, char *who) {
	cell	m, e, r;
	int	dgs;
	char	buf[50];

	save(x);
	e = x_real_exponent(x);
	m = new_atom(T_INTEGER, x_real_mantissa(x));
	save(m);
	dgs = count_digits(cdr(m));
	while (dgs > MANTISSA_SIZE) {
		r = bignum_shift_right(m);
		m = car(r);
		car(Stack) = m;
		dgs--;
		e++;
	}
	while (!x_bignum_zero_p(m)) {
		r = bignum_shift_right(m);
		if (!x_bignum_zero_p(cdr(r)))
			break;
		m = car(r);
		car(Stack) = m;
		e++;
	}
	if (x_bignum_zero_p(m))
		e = 0;
	r = new_atom(e, NIL);
	unsave(2);
	if (count_digits(r) > DIGITS_PER_WORD) {
		sprintf(buf, "%s: real number overflow",
			who? who: "internal");
		error(buf, NOEXPR);
	}
	return make_real(x_real_flags(x), e, cdr(m));
}

cell flat_copy(cell n) {
	cell	a, m, new;

	if (n == NIL)
		return NIL;
	m = node3(NIL, NIL, Tag[n]);
	save(m);
	a = m;
	while (n != NIL) {
		car(a) = car(n);
		n = cdr(n);
		if (n != NIL) {
			set(cdr(a), node3(NIL, NIL, Tag[n]))
			a = cdr(a);
		}
	}
	unsave(1);
	return m;
}

cell bignum_to_real(cell a) {
	int	e, flags, d;
	cell	m, n;

	m = flat_copy(a);
	cadr(m) = labs(cadr(m));
	e = 0;
	if (length(cdr(m)) > MANTISSA_SEGMENTS) {
		d = count_digits(cdr(m));
		while (d > MANTISSA_SIZE) {
			m = bignum_shift_right(m);
			m = car(m);
			e++;
			d--;
		}
	}
	flags = x_bignum_negative_p(a)? REAL_NEGATIVE: 0;
	n = make_real(flags, e, cdr(m));
	return real_normalize(n, NULL);
}

cell real_negate(cell a) {
	if (integer_p(a))
		return bignum_negate(a);
	return x_real_negate(a);
}

cell real_negative_p(cell a) {
	if (integer_p(a))
		return x_bignum_negative_p(a);
	return x_real_negative_p(a);
}

cell real_positive_p(cell a) {
	if (integer_p(a))
		return x_bignum_positive_p(a);
	return x_real_positive_p(a);
}

cell real_zero_p(cell a) {
	if (integer_p(a))
		return x_bignum_zero_p(a);
	return x_real_zero_p(a);
}

cell bignum_abs(cell a);

cell real_abs(cell a) {
	if (integer_p(a))
		return bignum_abs(a);
	if (x_real_negative_p(a))
		return x_real_negate(a);
	return a;
}

cell shift_mantissa(cell m) {
	m = new_atom(T_INTEGER, m);
	save(m);
	m = bignum_shift_right(m);
	m = car(m);
	unsave(1);
	return cdr(m);
}

/*
 * Scale the number R so that it gets exponent DESIRED_E
 * without changing its value. When there is not enough
 * room for scaling the mantissa of R, return NIL.
 * E.g.: scale_mantissa(1.0e0, -2, 0) --> 100.0e-2
 *
 * Allow the mantissa to grow to MAX_SIZE segments.
 */

cell scale_mantissa(cell r, cell desired_e, int max_size) {
	int	dgs;
	cell	n, e;

	dgs = count_digits(x_real_mantissa(r));
	if (max_size && (max_size - dgs < x_real_exponent(r) - desired_e))
		return NIL;
	n = new_atom(T_INTEGER, flat_copy(x_real_mantissa(r)));
	save(n);
	e = x_real_exponent(r);
	while (e > desired_e) {
		n = bignum_shift_left(n, 0);
		car(Stack) = n;
		e--;
	}
	unsave(1);
	return make_real(x_real_flags(r), e, cdr(n));
}

void autoscale(cell *pa, cell *pb) {
	if (x_real_exponent(*pa) < x_real_exponent(*pb)) {
		*pb = scale_mantissa(*pb, x_real_exponent(*pa),
					MANTISSA_SIZE*2);
		return;
	}
	if (x_real_exponent(*pa) > x_real_exponent(*pb)) {
		*pa = scale_mantissa(*pa, x_real_exponent(*pb),
					MANTISSA_SIZE*2);
	}
}

int real_equal(cell a, cell b, int approx) {
	cell	ma, mb;

	if (integer_p(a) && integer_p(b))
		return bignum_equal_p(a, b);
	if (integer_p(a))
		a = bignum_to_real(a);
	if (integer_p(b)) {
		save(a);
		b = bignum_to_real(b);
		unsave(1);
	}
	if (x_real_negative_p(a) != x_real_negative_p(b))
		return 0;
	if (!approx) {
		if (x_real_exponent(a) != x_real_exponent(b))
			return 0;
		if (x_real_zero_p(a) && x_real_zero_p(b))
			return 1;
		ma = x_real_mantissa(a);
		mb = x_real_mantissa(b);
	}
	if (approx) {
		Tmp = b;
		save(a);
		save(b);
		Tmp = NIL;
		autoscale(&a, &b);
		if (a == NIL || b == NIL) {
			unsave(2);
			return 0;
		}
		ma = x_real_mantissa(a);
		mb = x_real_mantissa(b);
		ma = shift_mantissa(ma);
		mb = shift_mantissa(mb);
		unsave(2);
	}
	while (ma != NIL && mb != NIL) {
		if (car(ma) != car(mb))
			return 0;
		ma = cdr(ma);
		mb = cdr(mb);
	}
	if (ma != mb)
		return 0;
	return 1;
}

int real_equal_p(cell a, cell b) {
	return real_equal(a, b, 0);
}

int real_approx_p(cell a, cell b) {
	return real_equal(a, b, 1);
}

int real_less_p(cell a, cell b) {
	cell	ma, mb;
	int	ka, kb, neg;
	int	dpa, dpb;

	if (integer_p(a) && integer_p(b))
		return bignum_less_p(a, b);
	if (integer_p(a))
		a = bignum_to_real(a);
	if (integer_p(b)) {
		save(a);
		b = bignum_to_real(b);
		unsave(1);
	}
	if (x_real_negative_p(a) && !x_real_negative_p(b)) return 1;
	if (x_real_negative_p(b) && !x_real_negative_p(a)) return 0;
	if (x_real_zero_p(a) && x_real_positive_p(b)) return 1;
	if (x_real_zero_p(b) && x_real_positive_p(a)) return 0;
	neg = x_real_negative_p(a);
	dpa = count_digits(x_real_mantissa(a)) + x_real_exponent(a);
	dpb = count_digits(x_real_mantissa(b)) + x_real_exponent(b);
	if (dpa < dpb) return neg? 0: 1;
	if (dpa > dpb) return neg? 1: 0;
	Tmp = b;
	save(a);
	save(b);
	Tmp = NIL;
	autoscale(&a, &b);
	unsave(2);
	if (a == NIL) return neg? 1: 0;
	if (b == NIL) return neg? 0: 1;
	ma = x_real_mantissa(a);
	mb = x_real_mantissa(b);
	ka = length(ma);
	kb = length(mb);
	if (ka < kb) return 1;
	if (ka > kb) return 0;
	while (ma != NIL) {
		if (car(ma) < car(mb)) return neg? 0: 1;
		if (car(ma) > car(mb)) return neg? 1: 0;
		ma = cdr(ma);
		mb = cdr(mb);
	}
	return 0;
}

cell real_add(cell a, cell b) {
	cell	r, m, e, aa, ab;
	int	flags, nega, negb;

	if (integer_p(a) && integer_p(b))
		return bignum_add(a, b);
	if (integer_p(a))
		a = bignum_to_real(a);
	save(a);
	if (integer_p(b))
		b = bignum_to_real(b);
	save(b);
	if (x_real_zero_p(a)) {
		unsave(2);
		return b;
	}
	if (x_real_zero_p(b)) {
		unsave(2);
		return a;
	}
	autoscale(&a, &b);
	if (a == NIL || b == NIL) {
		ab = real_abs(car(Stack));
		save(ab);
		aa = real_abs(caddr(Stack));
		unsave(1);
		b = unsave(1);
		a = unsave(1);
		return real_less_p(aa, ab)? b: a;
	}
	cadr(Stack) = a;
	car(Stack) = b;
	e = x_real_exponent(a);
	nega = x_real_negative_p(a);
	negb = x_real_negative_p(b);
	a = new_atom(T_INTEGER, x_real_mantissa(a));
	if (nega)
		a = bignum_negate(a);
	cadr(Stack) = a;
	b = new_atom(T_INTEGER, x_real_mantissa(b));
	if (negb)
		b = bignum_negate(b);
	car(Stack) = b;
	m = bignum_add(a, b);
	unsave(2);
	flags = x_bignum_negative_p(m)? REAL_NEGATIVE: 0;
	r = bignum_abs(m);
	r = make_real(flags, e, cdr(r));
	return real_normalize(r, "+");
}

cell real_subtract(cell a, cell b) {
	cell	r;

	if (integer_p(b))
		b = bignum_negate(b);
	else
		b = x_real_negate(b);
	save(b);
	r = real_add(a, b);
	unsave(1);
	return r;
}

cell real_multiply(cell a, cell b) {
	cell	r, m, e, ma, mb, ea, eb, neg;

	if (integer_p(a) && integer_p(b))
		return bignum_multiply(a, b);
	if (integer_p(a))
		a = bignum_to_real(a);
	save(a);
	if (integer_p(b))
		b = bignum_to_real(b);
	save(b);
	neg = x_real_negative_flag(a) != x_real_negative_flag(b);
	ea = x_real_exponent(a);
	eb = x_real_exponent(b);
	ma = new_atom(T_INTEGER, x_real_mantissa(a));
	cadr(Stack) = ma;
	mb = new_atom(T_INTEGER, x_real_mantissa(b));
	car(Stack) = mb;
	e = ea + eb;
	m = bignum_multiply(ma, mb);
	unsave(2);
	r = make_real(neg? REAL_NEGATIVE: 0, e, cdr(m));
	return real_normalize(r, "*");
}

cell real_divide(cell x, cell a, cell b) {
	cell	r, m, e, ma, mb, ea, eb, neg;
	int	nd, dd;

	if (integer_p(a))
		a = bignum_to_real(a);
	if (x_real_zero_p(a)) {
		r = make_integer(0);
		return make_real(0, 0, cdr(r));
	}
	save(a);
	if (integer_p(b))
		b = bignum_to_real(b);
	save(b);
	neg = x_real_negative_flag(a) != x_real_negative_flag(b);
	ea = x_real_exponent(a);
	eb = x_real_exponent(b);
	ma = new_atom(T_INTEGER, x_real_mantissa(a));
	cadr(Stack) = ma;
	mb = new_atom(T_INTEGER, x_real_mantissa(b));
	car(Stack) = mb;
	if (x_bignum_zero_p(mb)) {
		unsave(2);
		return NAN;
	}
	nd = count_digits(cdr(ma));
	dd = MANTISSA_SIZE + count_digits(cdr(mb));
	while (nd < dd) {
		ma = bignum_shift_left(ma, 0);
		cadr(Stack) = ma;
		nd++;
		ea--;
	}
	e = ea - eb;
	m = bignum_divide(NOEXPR, ma, mb);
	unsave(2);
	r = make_real(neg? REAL_NEGATIVE: 0, e, cdar(m));
	return real_normalize(r, "/");
}

cell real_to_bignum(cell r) {
	cell	n;
	int	neg;

	if (x_real_exponent(r) >= 0) {
		neg = x_real_negative_p(r);
		n = scale_mantissa(r, 0, 0);
		if (n == NIL)
			return NIL;
		n = new_atom(T_INTEGER, x_real_mantissa(n));
		if (neg)
			n = bignum_negate(n);
		return n;
	}
	return NIL;
}

cell real_integer_p(cell x) {
	if (integer_p(x))
		return 1;
	if (real_p(x) && real_to_bignum(x) != NIL)
		return 1;
	return 0;
}

/*
 * Reader
 */

cell find_symbol(char *s) {
	cell	y;

	y = Symbols;
	while (y != NIL) {
		if (!strcmp(symbol_name(car(y)), s))
			return car(y);
		y = cdr(y);
	}
	return NIL;
}

cell make_ident(char *s, int k) {
	cell	n;

	n = new_vec(T_SYMBOL, k+1);
	strcpy(symbol_name(n), s);
	return n;
}

cell symbol_ref(char *s) {
	cell	y, new;

	y = find_symbol(s);
	if (y != NIL)
		return y;
	new = make_ident(s, (int) strlen(s));
	Symbols = node(new, Symbols);
	return car(Symbols);
}

cell read_form(void);

cell read_list(int lst, int delim) {
	cell	n,	/* Node read */
		m,	/* List */
		a;	/* Used to append nodes to m */
	cell	new;
	int	c;	/* Member counter */
	char	badcons[] = "malformed list constructor (::)";
	char	msg[80];

	if (!Level)
		Opening_line = Line_no;
	if (++Level > MAX_IO_DEPTH) {
		error("reader: too many nested lists or tuples", NOEXPR);
		return NIL;
	}
	m = node3(NIL, NIL, lst);		/* root */
	save(m);
	a = NIL;
	c = 0;
	while (1) {
		if (Error_flag) {
			unsave(1);
			return NIL;
		}
		n = read_form();
		if (n == END_OF_FILE)  {
			if (Load_level) {
				unsave(1);
				return END_OF_FILE;
			}
			sprintf(msg, "missing ')', started in line %d",
					Opening_line);
			error(msg, NOEXPR);
		}
		if (n == TOK_CONS) {
			if (c != 1 || lst == 0) {
				n = S_cons;
			}
			else {
				n = read_form();
				cdr(a) = n;
				if (	(!list_p(n) && !symbol_p(n)) ||
					read_form() != TOK_RBRACK
				) {
					error(badcons, NOEXPR);
					continue;
				}
				unsave(1);
				Level--;
				return m;
			}
		}
		if (n == TOK_RPAREN || n == TOK_RBRACK) {
			if (n != delim)
				error(n == TOK_RPAREN?
				  "expected ']' instead of `)'":
				  "expected `)' instead of `]'",
				  NOEXPR);
			break;
		}
		if (a == NIL)
			a = m;		/* First member: insert at root */
		else
			a = cdr(a);	/* Subsequent members: append */
		car(a) = n;
		set(cdr(a), node3(NIL, NIL, lst)); /* Space for next member */
		c++;
	}
	Level--;
	if (a != NIL)
		cdr(a) = NIL;	/* Remove trailing empty node */
	unsave(1);
	return c? m: NIL;
}

cell quote(cell n, cell quotation) {
	cell	q;

	q = node(n, NIL);
	return node(quotation, q);
}

#define is_exp_char(c) ((c) == 'e')

cell string_to_bignum(char *s) {
	cell	n, v;
	int	k, j, sign;

	sign = 1;
	if (s[0] == '-') {
		s++;
		sign = -1;
	}
	else if (s[0] == '+') {
		s++;
	}
	/* plan9's atol() interprets leading 0 as octal! */
	while (s[0] == '0' && s[1])
		s++;
	k = (int) strlen(s);
	n = NIL;
	while (k) {
		j = k <= DIGITS_PER_WORD? k: DIGITS_PER_WORD;
		v = atol(&s[k-j]);
		s[k-j] = 0;
		k -= j;
		if (k == 0)
			v *= sign;
		n = new_atom(v, n);
	}
	return new_atom(T_INTEGER, n);
}

cell string_to_real(char *s) {
	cell	mantissa, n;
	cell	exponent;
	int	found_dp;
	int	neg = 0;
	int	i, j, sgn;

	mantissa = make_integer(0);
	save(mantissa);
	exponent = 0;
	i = 0;
	if (s[i] == '~') {
		neg = 1;
		i++;
	}
	found_dp = 0;
	while (isdigit((int) s[i]) || s[i] == '.') {
		if (s[i] == '.') {
			i++;
			found_dp++;
			continue;
		}
		if (found_dp)
			exponent--;
		mantissa = bignum_shift_left(mantissa, 0);
		car(Stack) = mantissa;
		mantissa = bignum_add(mantissa, make_integer(s[i]-'0'));
		car(Stack) = mantissa;
		i++;
	}
	j = 0;
	for (n = cdr(mantissa); n != NIL; n = cdr(n))
		j++;
	if (is_exp_char(s[i])) {
		i++;
		if (s[i] == '~') {
			sgn = -1;
			n = string_to_bignum(&s[i+1]);
		}
		else {
			sgn = 1;
			n = string_to_bignum(&s[i]);
		}
		if (cddr(n) != NIL) {
			unsave(1);
			return error(
				"exponent too big in real number literal",
				make_string(s, strlen(s)));
		}
		exponent += sgn * integer_value("", n);
	}
	unsave(1);
	if (found_dp > 1)
		return error("invalid numeric literal: ",
			make_string(s, strlen(s)));
	n = make_real((neg? REAL_NEGATIVE: 0),
			exponent, cdr(mantissa));
	return real_normalize(n, NULL);
}

cell string_to_number(char *s) {
	int	i;

	for (i=0; s[i]; i++) {
		if (s[i] == '.' || is_exp_char(s[i]))
			return string_to_real(s);
	}
	return string_to_bignum(s);
}

/* Create a character literal. */
cell make_char(int x) {
	cell n;

	n = new_atom(x & 0xff, NIL);
	return new_atom(T_CHAR, n);
}

int strcmp_ci(char *s1, char *s2) {
	int	c1, c2;

	while (1) {
		c1 = tolower((int) *s1++);
		c2 = tolower((int) *s2++);
		if (!c1 || !c2 || c1 != c2)
			break;
	}
	return c1<c2? -1: c1>c2? 1: 0;
}

int memcmp_ci(char *s1, char *s2, int k) {
	int	c1 = 0, c2 = 0;

	while (k--) {
		c1 = tolower((int) *s1++);
		c2 = tolower((int) *s2++);
		if (c1 != c2)
			break;
	}
	return c1<c2? -1: c1>c2? 1: 0;
}

/* Create a string; K = length */
cell make_string(char *s, int k) {
	cell	n;

	n = new_vec(T_STRING, k+1);
	strncpy(string(n), s, k+1);
	return n;
}

/* Read a string literal. */
cell read_string(void) {
	char	s[TOKEN_LENGTH+1];
	cell	n;
	int	c, i, q;
	int	inv;

	i = 0;
	q = 0;
	c = read_c();
	inv = 0;
	while (q || c != '"') {
		if (c == EOF)
			return error("missing '\"' in string literal", NOEXPR);
		if (Error_flag)
			break;
		if (i >= TOKEN_LENGTH-2) {
			return error("string literal too long", NOEXPR);
			i--;
		}
		if (q && c != '"' && c != '\\') {
			s[i++] = '\\';
			inv = 1;
		}
		s[i] = c;
		q = !q && c == '\\';
		if (!q)
			i++;
		c = read_c();
	}
	s[i] = 0;
	n = make_string(s, i);
	if (inv)
		error("invalid escape sequence in string", n);
	return n;
}

/* Read a character literal. */
cell read_char(void) {
	cell	n;
	char	*s;
	int	c;

	if (read_c() != '"')
		return error("missing `\"' in char literal", NOEXPR);
	n = read_string();
	s = string(n);
	if (!strcmp(s, "backslash"))
		c = '\\';
	else if (!strcmp(s, "newline"))
		c = '\n';
	else if (!strcmp(s, "quote"))
		c = '"';
	else if (!strcmp(s, "space"))
		c = ' ';
	else if (!strcmp(s, "tab"))
		c = '\t';
	else if (strlen(s) != 1)
		return error("malformed string literal", n);
	else
		c = *s;
	return make_char(c);
}

#define OPSYM_CHARS	"!@$%^&*-/+<=>~`"

#define is_ident(c) (isalpha(c) || (c) == '_' || (c) == ':' || (c) == '\'')
#define is_opsym(c) (strchr(OPSYM_CHARS, (c)) != NULL)

cell funny_char(char *msg, int c) {
	char	buf[128];

	if (isprint(c))
		return error(msg, make_char(c));
	sprintf(buf, "%s, code", msg);
	return error(buf, make_integer(c));
}

cell read_number(int c) {
	char	s[TOKEN_LENGTH];
	int	i;

	i = 0;
	while (isdigit(c) || c == '.' || is_exp_char(c) || c == '~') {
		if (i >= TOKEN_LENGTH-2) {
			return error("numeric literal too long", NOEXPR);
			i--;
		}
		s[i] = c;
		i++;
		c = read_c();
	}
	s[i] = 0;
	reject(c);
	return string_to_number(s);
}

cell read_ident(int c, int scm) {
	char	s[TOKEN_LENGTH];
	int	i;

	i = 0;
	while (is_ident(c) || isdigit(c) || (scm && c == '!')) {
		if (i >= TOKEN_LENGTH-2) {
			return error("identifier too long", NOEXPR);
			i--;
		}
		s[i] = c;
		i++;
		c = read_c();
	}
	s[i] = 0;
	reject(c);
	if (!strcmp(s, "true"))
		return TRUE;
	else if (!strcmp(s, "false"))
		return FALSE;
	return symbol_ref(s);
}

cell read_opsym(int c) {
	char	s[TOKEN_LENGTH];
	int	i;

	i = 0;
	while (is_opsym(c)) {
		if (i >= TOKEN_LENGTH-2) {
			return error("operator symbol too long", NOEXPR);
			i--;
		}
		s[i] = c;
		i++;
		c = read_c();
	}
	s[i] = 0;
	reject(c);
	return symbol_ref(s);
}

int closing_paren(void) {
	int c = read_c();

	reject(c);
	return c == ')';
}

cell read_form(void) {
	int	c;

	c = read_c();
	while (1) {	/* Skip over spaces and comments */
		while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			if (c == '\n')
				Line_no++;
			if (Error_flag)
				return UNDEFINED;
			c = read_c();
		}
		if (c != ';')
			break;
		while (!Error_flag && c != '\n' && c != EOF)
			c = read_c();
		if (Error_flag)
			return UNDEFINED;
	}
	if (c == EOF)
		return END_OF_FILE;
	if (Error_flag)
		return UNDEFINED;
	if (c == '(') {
		return read_list(0, TOK_RPAREN);
	}
	else if (c == '[') {
		return read_list(LIST_TAG, TOK_RBRACK);
	}
	else if (c == '\'' || c == '`') {
		cell	n;

		if (closing_paren())
			return error("missing form after \"'\" or \"`\"",
					NOEXPR);
		Level++;
		n = quote(read_form(), c=='`'? S_quasiquote: S_quote);
		Level--;
		return n;
	}
	else if (c == ',') {
		if (closing_paren())
			return error("missing form after \",\"",
					NOEXPR);
		c = read_c();
		if (c == '@') {
			return quote(read_form(), S_unquote_splicing);
		}
		else {
			reject(c);
			return quote(read_form(), S_unquote);
		}
	}
	else if (c == '#') {
		return read_char();
	}
	else if (c == '"') {
		return read_string();
	}
	else if (c == ')') {
		if (!Level)
			return error("unexpected ')'", NOEXPR);
		return TOK_RPAREN;
	}
	else if (c == ']') {
		if (!Level)
			return error("unexpected ']'", NOEXPR);
		return TOK_RBRACK;
	}
	else if (c == ':') {
		c = read_c();
		if (c != ':') {
			reject(c);
			return read_ident(':', 0);
		}
		if (!Level)
			return S_cons;
		return TOK_CONS;
	}
	else if (isdigit(c)) {
		return read_number(c);
	}
	else if (is_ident(c)) {
		return read_ident(c, 1);
	}
	else if (is_opsym(c)) {
		return read_opsym(c);
	}
	else {
		return funny_char("funny input character", c);
	}
}

cell xread(void) {
	if (Streams[Instream] == NULL)
		return error("instream is not open", NOEXPR);
	Level = 0;
	return read_form();
}

/*
 * Printer
 */

char *ntoa(char *b, cell x, int w) {
	char	buf[40];
	int	i = 0, neg = 0;
	char	*p = &buf[sizeof(buf)-1];

	if (x < 0) {
		x = -x;
		neg = 1;
	}
	*p = 0;
	while (x || i == 0) {
		i++;
		if (i >= sizeof(buf)-1)
			fatal("ntoa: number too big");
		p--;
		*p = x % 10 + '0';
		x = x / 10;
	}
	while (i < (w-neg) && i < sizeof(buf)-1) {
		i++;
		p--;
		*p = '0';
	}
	if (neg) {
		if (i >= sizeof(buf)-1)
			fatal("ntoa: number too big");
		p--;
		*p = '-';
	}
	strcpy(b, p);
	return b;
}

void print_expanded_real(cell m, cell e, int n_digits, int neg) {
	char	buf[DIGITS_PER_WORD+3];
	int	k, first;
	int	dp_offset, old_offset;

	dp_offset = e+n_digits;
	if (neg)
		pr("-");
	if (dp_offset <= 0)
		pr("0");
	if (dp_offset < 0)
		pr(".");
	while (dp_offset < 0) {
		pr("0");
		dp_offset++;
	}
	dp_offset = e+n_digits;
	first = 1;
	while (m != NIL) {
		ntoa(buf, labs(car(m)), first? 0: DIGITS_PER_WORD);
		k = strlen(buf);
		old_offset = dp_offset;
		dp_offset -= k;
		if (dp_offset < 0 && old_offset >= 0) {
			memmove(&buf[k+dp_offset+1], &buf[k+dp_offset],
				-dp_offset+1);
			buf[k+dp_offset] = '.';
		}
		pr(buf);
		m = cdr(m);
		first = 0;
	}
	if (dp_offset >= 0) {
		while (dp_offset > 0) {
			pr("0");
			dp_offset--;
		}
		pr(".0");
	}
}

cell round_last(cell m, cell e, int n_digits, int flags) {
	cell	one;
	int	r;

	m = bignum_shift_right(new_atom(T_INTEGER, m));
	r = integer_value("print", cdr(m));
	m = car(m);
	if (r >= 5) {
		save(m);
		one = make_integer(1);
		m = bignum_add(m, one);
		unsave(1);
	}
	m = cdr(m);
	n_digits--;
	e++;
	m = make_real(flags, e, m);
	return real_normalize(m, "print");
}

/* Print real number. */
int print_real(cell n) {
	int	n_digits;
	cell	m, e;
	char	buf[DIGITS_PER_WORD+2];

	if (!real_p(n))
		return 0;
	m = x_real_mantissa(n);
	n_digits = count_digits(m);
	e = x_real_exponent(n);
	if (n_digits == MANTISSA_SIZE && e < 0) {
		print_real(round_last(m, e, n_digits, x_real_flags(n)));
		return 1;
	}
	if (e+n_digits > -4 && e+n_digits <= 6) {
		print_expanded_real(m, e, n_digits, x_real_negative_flag(n));
		return 1;
	}
	if (x_real_negative_flag(n))
		pr("-");
	ntoa(buf, car(m), 0);
	pr_raw(buf, 1);
	pr(".");
	pr(buf[1] || cdr(m) != NIL? &buf[1]: "0");
	m = cdr(m);
	while (m != NIL) {
		pr(ntoa(buf, car(m), DIGITS_PER_WORD));
		m = cdr(m);
	}
	pr("e");
	if (e+n_digits-1 >= 0)
		pr("+");
	pr(ntoa(buf, e+n_digits-1, 0));
	return 1;
}

/* Print integer. */
int print_integer(cell n) {
	int	first;
	char	buf[DIGITS_PER_WORD+2];

	if (!integer_p(n))
		return 0;
	n = cdr(n);
	first = 1;
	while (n != NIL) {
		pr(ntoa(buf, car(n), first? 0: DIGITS_PER_WORD));
		n = cdr(n);
		first = 0;
	}
	return 1;
}

/* Print expressions of the form (QUOTE X) as 'X. */
int print_quoted(cell n) {
	if (	car(n) == S_quote &&
		cdr(n) != NIL &&
		cddr(n) == NIL
	) {
		pr("'");
		print_form(cadr(n));
		return 1;
	}
	return 0;
}

int print_fun(cell n) {
	cell	p, c;

	if (procedure_p(n)) {
		pr("fn");
		n = caddr(n);
		for (p = n; p != NIL; p = cdr(p)) {
			pr(p==n? " ": " | ");
			c = caar(p);
			if (tuple_p(c) && car(c) == GUARD) {
				c = cadr(c);
				pr("!");
			}
			print_form(c);
		}
		return 1;
	}
	return 0;
}

int print_continuation(cell n) {
	if (continuation_p(n)) {
		pr("<continuation>");
		return 1;
	}
	return 0;
}

int print_char(cell n) {
	char	b[1];
	int	c;

	if (!char_p(n))
		return 0;
	if (!Displaying)
		pr("#\"");
	c = cadr(n);
	if (!Displaying && c == ' ')
		pr("space");
	else if (!Displaying && c == '\n')
		pr("newline");
	else {
		b[0] = c;
		pr_raw(b, 1);
	}
	if (!Displaying)
		pr("\"");
	return 1;
}

int print_string(cell n) {
	char	b[1];
	int	k;
	char	*s;

	if (!string_p(n))
		return 0;
	if (!Displaying)
		pr("\"");
	s = string(n);
	k = string_len(n)-1;
	while (k) {
		b[0] = *s++;
		if (!Displaying && (b[0] == '"' || b[0] == '\\'))
			pr("\\");
		pr_raw(b, 1);
		k--;
	}
	if (!Displaying)
		pr("\"");
	return 1;
}

int print_symbol(cell n) {
	char	b[2];
	int	k;
	char	*s;

	if (!symbol_p(n))
		return 0;
	s = symbol_name(n);
	k = symbol_len(n)-1;
	b[1] = 0;
	while (k) {
		b[0] = *s++;
		pr(b);
		k--;
	}
	return 1;
}

int print_primitive(cell n) {
	PRIM	*p;

	if (!primitive_p(n))
		return 0;
	pr("<primitive ");
	p = &Primitives[cadr(n)];
	pr(p->name);
	pr(">");
	return 1;
}

int print_syntax(cell n) {
	if (!syntax_p(n))
		return 0;
	pr("<macro>");
	return 1;
}

int print_vector(cell n) {
	cell	*p;
	int	k;

	if (!vector_p(n))
		return 0;
	p = vector(n);
	k = vector_len(n);
	while (k--) {
		print_form(*p++);
		if (k)
			pr(", ");
	}
	return 1;
}

int print_stream(cell n) {
	char	buf[100];

	if (!instream_p(n) && !outstream_p(n))
		return 0;
	sprintf(buf, "<%sstream %d>",
		instream_p(n)? "in": "out",
		(int) stream_id(n));
	pr(buf);
	return 1;
}

int print_cons(cell n) {
	cell	p;

	if (!list_p(n))
		return 0;
	for (p = n; p != NIL; p = cdr(p))
		if (atom_p(p))
			break;
	if (p == NIL)
		return 0;
	for (p = n; !atom_p(p); p = cdr(p)) {
		print_form(car(p));
		pr(" :: ");
	}
	print_form(p);
	return 1;
}

void print_virtual(cell n) {
	char	buf[100];

	sprintf(buf, "<virtual %ld>", n);
	pr(buf);
}

void x_print_form(cell n, int depth) {
	int	m;

	if (Streams[Outstream] == NULL) {
		error("outstream is not open", NOEXPR);
		return;
	}
	if (depth > MAX_IO_DEPTH) {
		error("printer: too many nested lists or vectors", NOEXPR);
		return;
	}
	if (n == NIL) {
		pr("[]");
	}
	else if (eof_p(n)) {
		pr("<eof>");
	}
	else if (n == NAN) {
		pr("<nan>");
	}
	else if (n == FALSE) {
		pr("false");
	}
	else if (n == TRUE) {
		pr("true");
	}
	else if (undefined_p(n)) {
		pr("<undefined>");
	}
	else if (n == GUARD) {
		pr("<guard>");
	}
	else if (unit_p(n)) {
		pr("()");
	}
	else if (virtual_p(n)) {
		print_virtual(n);
	}
	else {
		if (print_char(n)) return;
		if (print_fun(n)) return;
		if (print_continuation(n)) return;
		if (print_real(n)) return;
		if (print_integer(n)) return;
		if (print_primitive(n)) return;
		if (print_quoted(n)) return;
		if (print_string(n)) return;
		if (print_symbol(n)) return;
		if (print_syntax(n)) return;
		if (print_vector(n)) return;
		if (print_stream(n)) return;
		if (print_cons(n)) return;
		pr(list_p(n)? "[": "(");
		m = n;
		while (n != NIL) {
			if (Error_flag) return;
			if (Printer_limit && Printer_count > Printer_limit)
				return;
			x_print_form(car(n), depth+1);
			if (Error_flag) return;
			n = cdr(n);
			if (n != NIL && atom_p(n)) {
				pr(" :: ");
				x_print_form(n, depth+1);
				n = NIL;
			}
			if (n != NIL) pr(", ");
		}
		pr(list_p(m)? "]": ")");
	}
}

void print_form(cell n) {
	x_print_form(n, 0);
}

/*
 * Parser
 * Expressions
 */

void init_ops(void) {
	cell		n, s, a, ops, new;
	int		k;
	operator	*p, *q;

	ops = node(NIL, NIL);
	save(ops);
	q = Op_init;
	for (p = q; p->prec;) {
		a = node(NIL, NIL);
		save(a);
		while (p == q || p->prec == (p-1)->prec) {
			n = node(make_integer(p->assoc), NIL);
			save(n);
			k = strlen(p->name);
			s = make_string(p->name, k);
			unsave(1);
			n = node(s, n);
			car(a) = n;
			if (p->prec == (p+1)->prec) {
				set(cdr(a), node(NIL, NIL));
				a = cdr(a);
			}
			p++;
		}
		q = p;
		car(ops) = unsave(1);
		if (p->prec) {
			set(cdr(ops), node(NIL, NIL));
			ops = cdr(ops);
		}
	}
	Ops = unsave(1);
}

int find_binop(char *s, int *pp, int *ap) {
	cell	lev, op;
	int	prec;

	prec = 1;
	for (lev = Ops; lev != NIL; lev = cdr(lev), prec++) {
		for (op = car(lev); op != NIL; op = cdr(op)) {
			if (!strcmp(s, string(caar(op)))) {
				*pp = prec;
				*ap = integer_value("", cadar(op));
				return 1;
			}
		}
	}
	return 0;
}

int is_binop(cell x) {
	int	a, p;

	return symbol_p(x) && find_binop(symbol_name(x), &p, &a);
}

int keyword(char *s) {
	switch (*s) {
	case '<':	if (!strcmp(s, "<<")) return TOK_TO;
			return UNDEFINED;
	case '>':	if (!strcmp(s, ">>")) return TOK_FROM;
			return UNDEFINED;
	case 'a':	if (!strcmp(s, "also")) return TOK_ALSO;
			if (!strcmp(s, "and")) return TOK_AND;
			return UNDEFINED;
	case 'c':	if (!strcmp(s, "case")) return TOK_CASE;
			if (!strcmp(s, "compile")) return TOK_COMPILE;
			return UNDEFINED;
	case 'e':	if (!strcmp(s, "else")) return TOK_ELSE;
			if (!strcmp(s, "end")) return TOK_END;
			if (!strcmp(s, "eval")) return TOK_EVAL;
			if (!strcmp(s, "exception")) return TOK_EXCEPTION;
			return UNDEFINED;
	case 'f':	if (!strcmp(s, "fn")) return TOK_FN;
			if (!strcmp(s, "fun")) return TOK_FUN;
			return UNDEFINED;
	case 'h':	if (!strcmp(s, "handle")) return TOK_HANDLE;
			return UNDEFINED;
	case 'i':	if (!strcmp(s, "if")) return TOK_IF;
			if (!strcmp(s, "in")) return TOK_IN;
			if (!strcmp(s, "infix")) return TOK_INFIX;
			if (!strcmp(s, "infixr")) return TOK_INFIXR;
			return UNDEFINED;
	case 'l':	if (!strcmp(s, "let")) return TOK_LET;
			if (!strcmp(s, "local")) return TOK_LOCAL;
			return UNDEFINED;
	case 'n':	if (!strcmp(s, "nonfix")) return TOK_NONFIX;
			return UNDEFINED;
	case 'o':	if (!strcmp(s, "of")) return TOK_OF;
			if (!strcmp(s, "op")) return TOK_OP;
			if (!strcmp(s, "or")) return TOK_OR;
			return UNDEFINED;
	case 'r':	if (!strcmp(s, "raise")) return TOK_RAISE;
			return UNDEFINED;
	case 't':	if (!strcmp(s, "then")) return TOK_THEN;
			if (!strcmp(s, "type")) return TOK_TYPE;
			return UNDEFINED;
	case 'v':	if (!strcmp(s, "val")) return TOK_VAL;
			return UNDEFINED;
	case 'w':	if (!strcmp(s, "where")) return TOK_WHERE;
			return UNDEFINED;
	default:	return UNDEFINED;
	}
}

void block_comment(void) {
	int	n = 1;
	int	c, p = 0;

	c = read_c();
	while (n && !Error_flag) {
		if (c == '\n')
			Line_no++;
		else if (c == EOF) {
			error("missing '*)' in comment", NOEXPR);
			return;
		}
		if (c == '*' && p == '(')
			n++;
		else if (c == ')' && p == '*')
			n--;
		p = c;
		c = read_c();
	}
}

int scan(void) {
	int	c;
	cell	n;

	c = read_c();
	while (1) {	/* Skip over spaces and comments */
		while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			if (c == '\n')
				Line_no++;
			if (Error_flag)
				return UNDEFINED;
			c = read_c();
		}
		if (c == ';') {		/* ;; comment */
			c = read_c();
			if (c != ';') {
				reject(c);
				c = ';';
				break;
			}
		}
		else if (c == '(') {	/* (*...*) comment */
			c = read_c();
			if (c != '*') {
				reject(c);
				c = '(';
				break;
			}
			block_comment();
			c = read_c();
			continue;
		}
		else {
			break;
		}
		while (!Error_flag && c != '\n' && c != EOF)
			c = read_c();
		if (Error_flag)
			return UNDEFINED;
	}
	Attr = UNDEFINED;
	if (c == EOF)
		return END_OF_FILE;
	if (c == ';')
		return TOK_SEMI;
	if (c == ',')
		return TOK_COMMA;
	if (c == '|')
		return TOK_PIPE;
	if (c == '(')
		return TOK_LPAREN;
	if (c == ')')
		return TOK_RPAREN;
	if (c == '[')
		return TOK_LBRACK;
	if (c == ']')
		return TOK_RBRACK;
	if (c == '`')
		return TOK_APPLY;
	if (c == ':') {
		c = read_c();
		if (c == ':')
			return TOK_CONS;
		reject(c);
		Attr = read_ident(':', 0);
		return TOK_ID;
	}
	if (isdigit(c)) {
		Attr = read_number(c);
		return TOK_NUM;
	}
	if (is_ident(c)) {
		Attr = read_ident(c, 0);
		if (Attr == TRUE || Attr == FALSE)
			return TOK_BOOL;
		if ((n = keyword(symbol_name(Attr))) != UNDEFINED)
			return n;
		return TOK_ID;
	}
	if (is_opsym(c)) {
		if (c == '~') {
			if (isdigit(c = read_c())) {
				Attr = read_number(c);
				Attr = real_negate(Attr);
				return TOK_NUM;
			}
			else {
				reject(c);
				c = '~';
			}
		}
		Attr = read_opsym(c);
		if ((n = keyword(symbol_name(Attr))) != UNDEFINED)
			return n;
		return TOK_ID;
	}
	if (c == '"') {
		Attr = read_string();
		return TOK_STR;
	}
	if (c == '#') {
		c = read_c();
		reject(c);
		if (c == '!') {
			while (c != '\n' && c != EOF)
				c = read_c();
			reject(c);
			return scan();
		}
		if (c != '"')
			return TOK_REF;
		Attr = read_char();
		return TOK_CHR;
	}
	return funny_char("funny input character", c);
}

cell x_pattern(int k);
cell expr(void);
cell sequence(void);
cell primary(void);
cell pat_primary(int k);

#define pat(x)	car(x)
#define opr(x)	cdr(x)

cell list(int pat, int lst) {
	cell	n, m, a, new;
	int	c;
	char	badcons[] = "malformed list constructor (::)";
	char	msg[80];

	Level++;
	a = m = node(NIL, NIL);		/* root */
	save(m);
	c = 0;
	while (1) {
		if (Error_flag) {
			unsave(1);
			return NIL;
		}
		if (Token == END_OF_FILE)  {
			if (Load_level) {
				unsave(1);
				return END_OF_FILE;
			}
			sprintf(msg, "missing ')' or ']', started in line %d",
					Opening_line);
			error(msg, NOEXPR);
		}
		if (Token == TOK_RPAREN || Token == TOK_RBRACK) {
			if (c == 0)
				break;
			unsave(1);
			return error(Token == TOK_RPAREN?
					"unexpected `)' in tuple":
					"unexpected `]' in list",
					NOEXPR);
		}
		if (pat) {
			set(car(a), x_pattern(1));
			if (opr(car(a)) != NIL) {
				set(cdr(Guard_ops), node(NIL, NIL));
				Guard_ops = cdr(Guard_ops);
				car(Guard_ops) = opr(car(a));
			}
			car(a) = pat(car(a));
		}
		else {
			set(car(a), lst? expr(): sequence());
		}
		c++;
		if (Token == TOK_CONS) {
			Token = scan();
			if (c != 1 || lst == 0) {
				n = S_cons;
			}
			else {
				n = pat? pat_primary(1): primary();
				cdr(a) = n;
				if (	(!list_p(n) && !symbol_p(n)) ||
					Token != TOK_RBRACK) {
					error(badcons, NOEXPR);
					continue;
				}
				Token = scan();
				unsave(1);
				Level--;
				return m;
			}
		}
		if (Token != TOK_COMMA)
			break;
		Token = scan();
		set(cdr(a), node(NIL, NIL));	/* Space for next member */
		a = cdr(a);
	}
	if (	(Token != TOK_RPAREN && !lst) ||
		(Token != TOK_RBRACK && lst)
	) {
		unsave(1);
		return error(lst? "missing ']' in list":
				  "missing ')' in tuple",
					NOEXPR);
	}
	Token = scan();
	Level--;
	unsave(1);
	return c? m: lst? NIL: UNIT;
}

cell pp_list(cell x);

cell pat_list(int lst) {
	cell	n;

	if (Guard_lev == 0) {
		Guard_ops = node(NIL, NIL);
		save(Guard_ops);
	}
	Guard_lev++;
	n = list(1, lst);
	if (lst && n != NIL && n != UNIT)
		pp_list(n);
	Guard_lev--;
	if (Guard_lev == 0) {
		Guard_ops = unsave(1);
		save(n);
		if (cdr(Guard_ops) == NIL)
			Guard_ops = NIL;
		else if (cddr(Guard_ops) == NIL)
			Guard_ops = cadr(Guard_ops);
		else
			Guard_ops = node(S_also, cdr(Guard_ops));
		return node(unsave(1), Guard_ops);
	}
	return node(n, NIL);
}

cell pat_primary(int k) {
	int	t;
	cell	n;
	char	msg[] = "grouping not allowed in patterns; this is a bug!";

	switch(Token) {
	case TOK_ID:
	case TOK_STR:
	case TOK_NUM:
	case TOK_CHR:
	case TOK_BOOL:
		save(Attr);
		Token = scan();
		return node(unsave(1), NIL);
	case TOK_CONS:
		Token = scan();
		return node(S_cons, NIL);
	case TOK_LPAREN:
	case TOK_LBRACK:
		t = Token;
		Token = scan();
		n = pat_list(t == TOK_LBRACK);
		if (pat(n) == NIL || pat(n) == UNIT)
			return n;
		if (t == TOK_LPAREN && cdr(pat(n)) == NIL) {
			if (Guard_lev > 0)
				return error(msg, NOEXPR);
			return node(car(pat(n)), opr(n));
		}
		return n;
	default:
		if (!Error_flag)
			Token = scan();
		return error("syntax error at", Attr);
	}
}

#define is_fun(x)	!(number_p(x) || char_p(x) || boolean_p(x))

cell pat_funapp(int k) {
	cell	n, m, p, f, x;

	n = pat_primary(k);
	if (!k)
		return n;
	while (	(Token == TOK_ID || Token == TOK_STR || Token == TOK_NUM ||
		 Token == TOK_CHR || Token == TOK_BOOL ||
		 Token == TOK_LPAREN || Token == TOK_LBRACK ||
		 Token == TOK_OP)
		&&
		!is_binop(Attr) && Token != TOK_CONS
		&&
		is_fun(n)
	) {
		save(n);
		m = pat_primary(k);
		p = pat(m);
		save(p);
		f = opr(n) == NIL? pat(n): opr(n);
		x = opr(m) == NIL? pat(m): opr(m);
		n = node(x, NIL);
		n = node(f, n);
		n = node(p, n);
		unsave(2);
	}
	return n;
}

#define inf_ops()		car(Stack)
#define inf_first_op()		car(inf_ops())
#define inf_other_ops()		cdr(inf_ops())
#define inf_args()		cadr(Stack)
#define inf_first_arg()		car(inf_args())
#define inf_second_arg()	cadr(inf_args())
#define inf_other_args()	cdr(inf_args())

void pat_commit(char *s, int all) {
	int	tp = 0, ta = LEFT, sp = 0, sa;
	cell	n, a1, a2, p, op;

	find_binop(s, &tp, &ta);
	while (inf_ops() != NIL) {
		find_binop(symbol_name(inf_first_op()), &sp, &sa);
		if (!all && (tp > sp || (tp == sp && ta == RIGHT)))
			break;
		a2 = inf_first_arg();
		a1 = inf_second_arg();
		if (consname_p(inf_first_op())) {		/* x :id y */
			if (inf_first_op() == S_cons) {		/* [x :: y] */
				n = cons(pat(a1), pat(a2));
			}
			else {
				n = node(pat(a2), NIL);
				n = node(pat(a1), n);
				n = node(inf_first_op(), n);
			}
			save(n);
			op = opr(a2);
			if (opr(a1) != NIL) {
				if (op == NIL) {
					op = opr(a1);
				}
				else {
					op = node(op, NIL);
					op = node(a1, op);
					op = node(S_also, op);
				}
			}
			n = node(unsave(1), op);
		}
		else {
			a2 = opr(a2) == NIL? pat(a2): opr(a2);
			n = node(a2, NIL);	/* (op (tuple arg1 arg2)) */
			a1 = opr(a1) == NIL? pat(a1): opr(a1);
			n = node(a1, n);
			n = node(S_tuple, n);
			n = node(n, NIL);
			n = node(inf_first_op(), n);
			a1 = inf_second_arg();		/* add pattern */
			a2 = inf_first_arg();
			p = symbol_p(pat(a1))? pat(a1): pat(a2);
			n = node(p, n);
		}
		inf_ops() = inf_other_ops();	/* remove op */
		inf_args() = inf_other_args();	/* remove arg2 */
		inf_first_arg() = n;		/* replace arg1 */
	}
}

cell pat_infix(int k) {
	cell	ops, args, new;
	cell	n;

	n = pat_funapp(k);
	if ((Token == TOK_ID && is_binop(Attr)) || Token == TOK_CONS) {
		Tmp = n;
		save(Attr);
		args = NIL;
		save(args);
		ops = NIL;
		save(ops);
		set(cadr(Stack), node(n, cadr(Stack)));
		Tmp = NIL;
		if (Token == TOK_CONS)
			Attr = caddr(Stack) = S_cons;
		while (is_binop(Attr) && (Attr != S_equal || k > 0)) {
			pat_commit(symbol_name(Attr), 0);
			set(car(Stack), node(Attr, car(Stack)));
			Token = scan();
			caddr(Stack) = Attr;
			set(cadr(Stack), node(pat_funapp(k), cadr(Stack)));
			if (Token == TOK_CONS)
				Attr = caddr(Stack) = S_cons;
		}
		pat_commit("", 1);
		n = car(unsave(2));
		unsave(1);
		return n;
	}
	else {
		return n;
	}
}

cell pat_logop(int op, int k) {
	cell	n, a, p, new;

	n = op == TOK_ALSO? pat_infix(k): pat_logop(TOK_ALSO, k);
	if (Token != op)
		return n;
	Tmp = n;
	p = pat(n);
	n = opr(n) == NIL? pat(n): opr(n);
	save(p);
	a = node(n, NIL);
	Tmp = NIL;
	save(a);
	while (Token == op) {
		Token = scan();
		set(cdr(a), node(NIL, NIL));
		a = cdr(a);
		n = op == TOK_ALSO? pat_infix(k):
				pat_logop(TOK_ALSO, k);
		if (symbol_p(pat(n)))
			p = cadr(Stack) = pat(n);
		n = opr(n) == NIL? pat(n): opr(n);
		car(a) = n;
	}
	n = unsave(1);
	if (cdr(n) == NIL) {
		p = unsave(1);
		return node(p, car(n));
	}
	n = node(op==TOK_OR? S_or: S_also, n);
	return node(unsave(1), n);
}

cell x_pattern(int k) {
	cell	n, m;

	if (Token == TOK_ID && consname_p(Attr)) {
		save(Attr);
		Token = scan();
		if (Token != TOK_ID || Attr != S_equal) {
			Tmp = n = pat_primary(k);
			if (!tuple_p(pat(n))) { 
				m = node(pat(n), NIL);
				Tmp = n = node(m, opr(n));
			}
			n = node(unsave(1), pat(n));
			n = node(n, opr(Tmp));
			Tmp = NIL;
		}
		else {
			n = node(unsave(1), NIL);
		}
	}
	else {
		n = pat_logop(TOK_OR, k);
	}
	return n;
}

cell pattern(void) {
	cell	n;

	n = x_pattern(0);
	if (cons_p(n) && cdr(n) == NIL)
		return car(n);
	return node(GUARD, n);
}

cell logop(int op, int k);

cell match(int curry) {
	cell	p, x, n, m, new, curried, npat;

	m = node(NIL, NIL);
	save(m);
	curried = NIL;
	npat = 0;
	save(curried);
	for (;;) {
		p = pattern();
		npat++;
		while (	curry &&
			Token != TOK_WHERE &&
			!(Token == TOK_ID && Attr == S_equal) &&
			!Error_flag
		) {
			set(car(Stack), node(p, car(Stack)));
			p = pattern();
		}
		if (Token == TOK_WHERE) {
			save(p);
			Token = scan();
			x = logop(TOK_OR, 0);
			if (tuple_p(p) && car(p) == GUARD)
				error("implicit guard conflicts with `where'",
					NOEXPR);
			p = node(unsave(1), x);
			p = node(GUARD, p);
		}
		if (Token != TOK_ID || Attr != S_equal) {
			unsave(2);
			return error("missing '=' in match", NOEXPR);
		}
		save(p);
		Token = scan();
		x = expr();
		n = node(x, NIL);
		set(car(m), node(unsave(1), n));
		if (Token != TOK_PIPE)
			break;
		Token = scan();
		set(cdr(m), node(NIL, NIL));
		m = cdr(m);
	}
	curried = unsave(1);
	if (curried != NIL && npat > 1) {
		unsave(1);
		return error("currying cannot be combined with `|'", NOEXPR);
	}
	Tmp = n = unsave(1);
	save(curried);
	Tmp = NIL;
	while (curried != NIL) {
		n = node(S_fn, n);
		n = node(n, NIL);
		n = node(car(curried), n);
		n = node(n, NIL);
		curried = cdr(curried);
	}
	unsave(1);
	return n;
}

cell tuple_ref(void) {
	cell	n;

	save(Attr);
	Token = scan();
	n = primary();
	if (!symbol_p(n) && !tuple_p(n)) {
		unsave(1);
		return error("bad object after '#n'", Attr);
	}
	Tmp = n;
	n = node(unsave(1), NIL);
	n = node(Tmp, n);
	Tmp = NIL;
	n = node(S_tuple, n);
	n = node(n, NIL);
	n = node(S_ref, n);
	return n;
}

cell let_expr(void);
cell prog(void);

cell compile(void) {
	Token = scan();
	return quote(prog(), S_quote);
}

cell primary(void) {
	int	t;
	cell	n;

	switch(Token) {
	case TOK_ID:
	case TOK_STR:
	case TOK_NUM:
	case TOK_CHR:
	case TOK_BOOL:
		save(Attr);
		Token = scan();
		return unsave(1);
	case TOK_CONS:
		Token = scan();
		return S_cons;
	case TOK_LPAREN:
	case TOK_LBRACK:
		t = Token;
		Token = scan();
		n = list(0, t == TOK_LBRACK);
		if (n == NIL || n == UNIT)
			return n;
		if (t == TOK_LPAREN && cdr(n) == NIL) {
			return car(n);
		}
		n = node(t == TOK_LBRACK? S_list: S_tuple, n);
		return n;
	case TOK_FN:
		Token = scan();
		return node(S_fn, match(1));
	case TOK_OP:
		Token = scan();
		if (Token == TOK_CONS)
			Attr = S_cons;
		else if (Token != TOK_ID)
			return error("op: missing identifier at", Attr);
		save(Attr);
		Token = scan();
		return unsave(1);
	case TOK_REF:
		Token = scan();
		return tuple_ref();
	case TOK_LET:
		return let_expr();
	case TOK_EVAL:
		save(read_form());
		Token = scan();
		return unsave(1);
	case TOK_COMPILE:
		return compile();
	default:
		if (!Error_flag)
			Token = scan();
		return error("syntax error at", Attr);
	}
}

cell funapp(void) {
	cell	n, m;

	n = primary();
	while (	(Token == TOK_ID || Token == TOK_STR || Token == TOK_NUM ||
		 Token == TOK_CHR || Token == TOK_BOOL ||
		 Token == TOK_LPAREN || Token == TOK_LBRACK ||
		 Token == TOK_OP || Token == TOK_REF)
		&&
		!is_binop(Attr) && Token != TOK_CONS
		&&
		is_fun(n)
	) {
		save(n);
		m = primary();
		m = node(m, NIL);
		n = node(unsave(1), m);
	}
	return n;
}

void commit(char *s, int all) {
	int	tp = 0, ta = LEFT, sp = 0, sa;
	cell	n;

	find_binop(s, &tp, &ta);
	while (inf_ops() != NIL) {
		find_binop(symbol_name(inf_first_op()), &sp, &sa);
		if (!all && (tp > sp || (tp == sp && ta == RIGHT)))
			break;
		n = node(inf_first_arg(), NIL);	/* (op (tuple arg1 arg2)) */
		inf_args() = inf_other_args();	/* also remove arg2 */
		n = node(inf_first_arg(), n);	/* now other arg */
		n = node(S_tuple, n);
		n = node(n, NIL);
		n = node(inf_first_op(), n);
		inf_ops() = inf_other_ops();	/* remove op */
		inf_first_arg() = n;		/* replace arg1 */
	}
}

cell infix(int k) {
	cell	ops, args, new;
	cell	n;

	n = funapp();
	if ((Token == TOK_ID && is_binop(Attr)) || Token == TOK_CONS) {
		Tmp = n;
		save(Attr);
		args = NIL;
		save(args);
		ops = NIL;
		save(ops);
		set(cadr(Stack), node(n, cadr(Stack)));
		Tmp = NIL;
		if (Token == TOK_CONS)
			Attr = caddr(Stack) = S_cons;
		while (is_binop(Attr) && (Attr != S_equal || k > 0)) {
			commit(symbol_name(Attr), 0);
			set(car(Stack), node(Attr, car(Stack)));
			Token = scan();
			caddr(Stack) = Attr;
			set(cadr(Stack), node(funapp(), cadr(Stack)));
			if (Token == TOK_CONS)
				Attr = caddr(Stack) = S_cons;
		}
		commit("", 1);
		n = car(unsave(2));
		unsave(1);
		return n;
	}
	else {
		return n;
	}
}

cell apply_op(int k) {
	cell	n;

	n = infix(k);
	while (Token == TOK_APPLY) {
		save(n);
		Token = scan();
		n = apply_op(k);
		n = node(n, NIL);
		n = node(unsave(1), n);
	}
	return n;
}

cell logop(int op, int k) {
	cell	n, a, new;

	n = op == TOK_ALSO? apply_op(k): logop(TOK_ALSO, k);
	a = node(n, NIL);
	save(a);
	while (Token == op) {
		Token = scan();
		set(cdr(a), node(NIL, NIL));
		a = cdr(a);
		set(car(a), op == TOK_ALSO? apply_op(k): logop(TOK_ALSO, k));
	}
	n = unsave(1);
	if (cdr(n) == NIL)
		return car(n);
	return node(op==TOK_OR? S_or: S_also, n);
}

/*
 *  <expr> HANDLE <match>  --->  (call/cc
 *                                 (fn (k (%register k (fn <match>))
 *                                        (%unregister <expr>))))
 */

cell gensym(char *s);

cell handle_expr(void) {
	cell	n, m, h, k;

	n = logop(TOK_OR, 1);
	if (Token == TOK_HANDLE) {
		save(n);
		Token = scan();
		h = node(n, NIL);		/* ((%unregister expr)) */
		h = node(S_unregister, h);
		h = node(h, NIL);
		save(h);
		k = gensym("k");
		save(k);
		m = match(0);			/* (%register k (fn match)) */ 
		m = node(S_fn, m);
		m = node(m, NIL);
		m = node(k, m);
		m = node(S_register, m);
		h = node(m, cadr(Stack));	/* body = (reg expr unreg) */
		h = node(k, h);			/* handler = (fn (k body)) */
		h = node(h, NIL);
		h = node(S_fn, h);
		h = node(h, NIL);
		h = node(S_callcc, h);		/* (call/cc handler) */
		unsave(3);
		n = h;
	}
	return n;
}

cell raise_expr(void) {
	cell	n;

	if (Token == TOK_RAISE) {
		Token = scan();
		if (Token != TOK_ID || !consname_p(Attr))
			return error("exception expected after 'raise'",
					NOEXPR);
		n = node(Attr, NIL);
		n = node(S_raise, n);
		Token = scan();
		return n;
	}
	else {
		return handle_expr();
	}
}

cell ifexpr(void) {
	cell	n, c, p, a;

	if (Token == TOK_IF) {
		Token = scan();
		c = expr();
		if (Token != TOK_THEN)
			return error("missing 'then' after 'if'", NOEXPR);
		save(c);
		Token = scan();
		p = expr();
		if (Token != TOK_ELSE) {
			unsave(1);
			return error("missing 'else' after 'if'", NOEXPR);
		}
		save(p);
		Token = scan();
		a = expr();
		n = node(a, NIL);
		n = node(unsave(1), n);
		n = node(unsave(1), n);
		n = node(S_if, n);
		return n;
	}
	else {
		return raise_expr();
	}
}

cell case_expr(void) {
	cell	n, x;

	if (Token == TOK_CASE) {
		Token = scan();
		x = ifexpr();
		if (Token != TOK_OF)
			return error("missing 'of' in 'case'", NOEXPR);
		save(x);
		Token = scan();
		n = match(0);
		n = node(S_fn, n);
		Tmp = n;
		n = node(unsave(1), NIL);
		n = node(Tmp, n);
		Tmp = NIL;
		return n;
	}
	else {
		return ifexpr();
	}
}

cell expr(void) {
	cell	n, m;
	int	t;

	n = case_expr();
	if (Token == TOK_FROM || Token == TOK_TO) {
		save(n);
		t = Token;
		Token = scan();
		m = expr();
		n = node(m, NIL);
		n = node(unsave(1), n);
		return node(t == TOK_FROM? S_from: S_to, n);
	}
	return n;
}

cell sequence(void) {
	cell	s, n, new;

	s = node(NIL, NIL);
	save(s);
	for (;;) {
		set(car(s), expr());
		if (Token != TOK_SEMI)
			break;
		Token = scan();
		set(cdr(s), node(NIL, NIL));
		s = cdr(s);
	}
	n = unsave(1);
	return cdr(n) == NIL? car(n): node(S_begin, n);
}

/* Declarations */

void extract_vars(cell n) {
	cell	new;

	if (cons_p(n)) {
		extract_vars(cdr(n));
		extract_vars(car(n));
		return;
	}
	if (symbol_p(n)) {
		set(car(Stack), node(n, car(Stack)));
	}
}

cell make_defines(cell p, cell def) {
	cell	v, n, new;

	save(NIL);
	extract_vars(p);
	for (v = car(Stack); v != NIL; v = cdr(v)) {
		n = node(car(v), NIL);	/* (define x x) */
		n = node(car(v), n);
		n = node(S_define, n);
		car(def) = n;
		if (cdr(v) != NIL) {
			set(cdr(def), cons(NIL, NIL));
			def = cdr(def);
		}
	}
	unsave(1);
	return def;
}

cell val_decl(int inner) {
	cell	n, def, v, a, new;

	a = node(NIL, NIL);
	save(a);
	v = node(NIL, NIL);
	save(v);
	def = node(NIL, NIL);
	save(def);
	for (;;) {
		Token = scan();
		set(car(v), pattern());
		def = make_defines(car(v), def);
		if (Token != TOK_ID || Attr != S_equal) {
			unsave(3);
			return error("missing '=' in 'val'", Attr);
		}
		Token = scan();
		set(car(a), expr());
		if (Token != TOK_AND)
			break;
		set(cdr(def), node(NIL, NIL));
		def = cdr(def);
		set(cdr(v), node(NIL, NIL));
		v = cdr(v);
		set(cdr(a), node(NIL, NIL));
		a = cdr(a);
	}
	if (!inner && cdar(Stack) == NIL && symbol_p(caadr(Stack))) {
		unsave(1);
		n = cadr(Stack);
		n = node(car(unsave(1)), n);
		n = node(S_define, n);
		unsave(1);
		return n;
	}
	def = unsave(1);	/* ((fn (v... def...)) a...) */
	n = unsave(1);
	if (cdr(n) == NIL)
		n = car(n);
	n = node(n, def);
	n = node(n, NIL);
	n = node(S_fn, n);
	n = node(n, unsave(1));
	return n;
}

cell fun_decl(int inner) {
	cell	n, m, def, bind, new;

	bind = node(NIL, NIL);
	save(bind);
	def = node(NIL, NIL);
	save(def);
	for (;;) {
		Token = scan();
		if (Token != TOK_ID) {
			unsave(2);
			return error("identifier expected at", Attr);
		}
		n = node(Attr, NIL);
		n = node(Attr, n);
		set(car(def), node(S_define, n));
		Token = scan();
		n = match(1);
		n = node(cadar(def), n);
		car(bind) = n;
		if (Token != TOK_AND)
			break;
		set(cdr(bind), node(NIL, NIL));
		bind = cdr(bind);
		set(cdr(def), node(NIL, NIL));
		def = cdr(def);
	}
	if (!inner && cdar(Stack) == NIL) {
		m = caadr(Stack);		/* (name pattern)          */
		n = node(S_fn, cdr(m));		/* --> (name (fn pattern)) */
		n = node(n, NIL);
		n = node(car(m), n);
		unsave(2);
		return node(S_define, n);
	}
	def = unsave(1);
	n = node(unsave(1), def);
	n = node(S_letrec, n);
	return n;
}

cell type_decl(void) {
	cell	n, t, new;

	Token = scan();
	if (Token != TOK_ID || !consname_p(Attr))
		return error("constructor name expected at", Attr);
	save(Attr);
	Token = scan();
	if (Token != TOK_ID || Attr != S_equal) {
		unsave(1);
		return error("missing '=' after 'type'", NOEXPR);
	}
	Token = scan();
	t = node(NIL, NIL);
	save(t);
	for (;;) {
		if (Token != TOK_ID) {
			unsave(2);
			return error("constructor expected", NOEXPR);
		}
		save(Attr);
		Token = scan();
		if (Token == TOK_LPAREN) {
			Token = scan();
			n = node(NIL, NIL);
			save(n);
			while (Token == TOK_ID || Token == TOK_CONS) {
				if (Token == TOK_CONS)
					Attr = S_cons;
				car(n) = Attr;
				Token = scan();
				if (Token != TOK_COMMA)
					break;
				Token = scan();
				set(cdr(n), node(NIL, NIL));
				n = cdr(n);
			}
			if (Token != TOK_RPAREN) {
				unsave(4);
				return error("missing ')' in constructor",
						NOEXPR);
			}
			Token = scan();
			n = unsave(1);
			n = node(unsave(1), n);
			save(n);
		}
		car(t) = unsave(1);
		if (Token != TOK_PIPE)
			break;
		Token = scan();
		set(cdr(t), node(NIL, NIL));
		t = cdr(t);
	}
	n = unsave(1);
	n = node(unsave(1), n);
	n = node(S_define_type, n);
	return n;
}

cell decls(int local) {
	cell	d, ds, new;
	char	*msg;

	msg = "expected declaration in 'let' or 'local'";
	ds = NIL;
	save(ds);
	for (;;) {
		switch (Token) {
		case TOK_VAL:	d = val_decl(local); break;
		case TOK_FUN:	d = fun_decl(local); break;
		default:	unsave(1);
				return error(msg, NOEXPR);
		}
		if (local) {
			if (ds == NIL) {
				ds = car(Stack) = d;
			}
			else {
				if (car(ds) == S_letrec) {
					set(cddr(ds), node(d, NIL));
				}
				else {
					set(cdadar(ds), node(d, NIL));
				}
				ds = d;
			}
		}
		else {
			if (ds == NIL) {
				set(ds, node(d, NIL));
				car(Stack) = ds;
			}
			else {
				set(cdr(ds), node(d, NIL));
			}
		}
		if (Token != TOK_SEMI)
			break;
		Token = scan();
	}
	if (!local) {
		ds = unsave(1);
		if (cdr(ds) == NIL)
			return car(ds);
		return node(S_begin, ds);
	}
	return node(ds, unsave(1));
}

cell let_expr(void) {
	cell	d, x;

	Token = scan();
	d = decls(1);
	save(d);
	if (Token != TOK_IN) {
		unsave(1);
		return error("missing 'in' in 'let'", NOEXPR);
	}
	Token = scan();
	x = sequence();
	save(x);
	if (Token != TOK_END) {
		unsave(2);
		return error("missing 'end' in 'let'", NOEXPR);
	}
	Token = scan();
	x = unsave(1);
	x = node(x, NIL);
	d = unsave(1);
	if (caar(d) == S_letrec)
		cddar(d) = x;
	else
		cdadar(car(d)) = x;
	return cdr(d);
}

cell local_decl(void) {
	cell	d, x;

	Token = scan();
	d = decls(1);
	save(d);
	if (Token != TOK_IN) {
		unsave(1);
		return error("missing 'in' in 'local'", NOEXPR);
	}
	Token = scan();
	x = decls(0);
	save(x);
	if (Token != TOK_END) {
		unsave(2);
		return error("missing 'end' in 'local'", NOEXPR);
	}
	Token = scan();
	x = unsave(1);
	x = node(x, NIL);
	d = unsave(1);
	if (caar(d) == S_letrec)
		cddar(d) = x;
	else
		cdadar(car(d)) = x;
	return cdr(d);
}

cell locate_op(char *s) {
	cell	p, pp, n;

	pp = Ops;
	for (p = cdr(Ops); p != NIL; p = cdr(p)) {
		for (n = car(p); n != NIL; n = cdr(n)) {
			if (!strcmp(string(caar(n)), s))
				break;
		}
		if (n != NIL)
			break;
		pp = p;
	}
	return p == NIL? NIL: pp;
}

void delfixity(cell op) {
	cell	p;

	p = locate_op(symbol_name(op));
	if (p == NIL)
		return;
	cadr(p) = cdadr(p);
	if (cadr(p) == NIL)
		cdr(p) = cddr(p);
}

void setfixity(cell op, char *refop, int a, int rel) {
	cell	p, n, new;

	p = locate_op(refop);
	n = make_integer(a);
	n = node(n, NIL);
	save(n);
	n = make_string("", strlen(symbol_name(op)));
	strcpy(string(n), symbol_name(op));
	n = node(n, unsave(1));
	if (rel == S_less) {
		n = node(n, NIL);
		set(cdr(p), node(n, cdr(p)));
	}
	else if (rel == S_equal) {
		p = cdr(p);
		set(car(p), node(n, car(p)));
	}
	else if (rel == S_greater) {
		n = node(n, NIL);
		p = cdr(p);
		set(cdr(p), node(n, cdr(p)));
	}
}

cell infix_decl(int fix) {
	cell	mode;
	int	p, a;

	Token = scan();
	for (;;) {
		if (Token != TOK_ID)
			return error("identifier expected in fix def",
					NOEXPR);
		save(Attr);
		Token = scan();
		if (fix == NONE) {
			if (symbol_p(car(Stack)))
				delfixity(car(Stack));
		}
		else {
			delfixity(car(Stack));
			if (	Attr == S_less ||
				Attr == S_greater ||
				Attr == S_equal
			) {
				mode = Attr;
				Token = scan();
			}
			else {
				unsave(1);
				return error(
					"fix def expected one of {<,=,>}",
					NOEXPR);
			}
			if (Token == TOK_CONS) {
				Attr = S_cons;
			}
			else if (Token != TOK_ID) {
				unsave(1);
				return error(
				 "reference identifier expected in fix def",
					NOEXPR);
			}
			if (	symbol_p(Attr) &&
				!find_binop(symbol_name(Attr), &p, &a)
			) {
				unsave(1);
				return error(
				 "invalid reference identifier in fix def",
					Attr);
			}
			else {
				if (symbol_p(car(Stack)))
					setfixity(car(Stack),
						symbol_name(Attr),
						fix,
						mode);
			}
			Token = scan();
		}
		unsave(1);
		if (Token != TOK_COMMA)
			break;
		Token = scan();
	}
	return UNIT;
}

cell except_decl(void) {
	cell	a, n, new;

	a = cons(NIL, NIL);
	save(a);
	for (;;) {
		Token = scan();
		if (Token != TOK_ID || !consname_p(Attr))
			return error("constructor name expected at", Attr);
		save(Attr);
		Exceptions = node(Attr, Exceptions);
		n = node(Attr, NIL);		/* (define exn exn) */
		n = node(Attr, n);
		n = node(S_define_type, n);
		car(a) = n;
		unsave(1);
		Token = scan();
		if (Token != TOK_AND)
			break;
		set(cdr(a), cons(NIL, NIL));
		a = cdr(a);
	}
	n = unsave(1);
	return cdr(n) == NIL? car(n): node(S_begin, n);
}

cell prog(void) {
	switch (Token) {
	case TOK_VAL:		return val_decl(0);
	case TOK_FUN:		return fun_decl(0);
	case TOK_TYPE:		return type_decl();
	case TOK_LOCAL:		return local_decl();
	case TOK_INFIX:		return infix_decl(LEFT);
	case TOK_INFIXR:	return infix_decl(RIGHT);
	case TOK_NONFIX:	return infix_decl(NONE);
	case TOK_EXCEPTION:	return except_decl();
	case END_OF_FILE:	return END_OF_FILE;
	default:		return expr();
	}
}

/*
 * Special Form Handlers
 */

int length(cell n) {
	int	k = 0;

	while (n != NIL) {
		k++;
		n = cdr(n);
	}
	return k;
}

cell append_b(cell a, cell b) {
	cell	p, last = NIL;

	if (a == NIL)
		return b;
	p = a;
	while (p != NIL) {
		if (atom_p(p))
			fatal("append!: improper list");
		last = p;
		p = cdr(p);
	}
	cdr(last) = b;
	return a;
}

int argument_list_p(cell n) {
	if (n == NIL || symbol_p(n))
		return 1;
	if (atom_p(n))
		return 0;
	while (tuple_p(n)) {
		if (!symbol_p(car(n)))
			return 0;
		n = cdr(n);
	}
	return n == NIL || symbol_p(n);
}

#define hash(s, h) \
	do {					\
		h = 0;				\
		while (*s)			\
			h = ((h<<5)+h) ^ *s++;	\
	} while (0)

int hash_size(int n) {
	if (n < 5) return 5;
	if (n < 11) return 11;
	if (n < 23) return 23;
	if (n < 47) return 47;
	if (n < 97) return 97;
	if (n < 199) return 199;
	if (n < 499) return 499;
	if (n < 997) return 997;
	if (n < 9973) return 9973;
	return 19997;
}

void rehash(cell e) {
	unsigned int	i;
	cell		p, *v, new;
	unsigned int	h, k = hash_size(length(e));
	char		*s;

	if (Program == NIL || k < HASH_THRESHOLD)
		return;
	set(car(e), new_vec(T_VECTOR, k * sizeof(cell)));
	v = vector(car(e));
	for (i=0; i<k; i++)
		v[i] = NIL;
	p = cdr(e);
	while (p != NIL) {
		s = symbol_name(caar(p));
		h = 0;
		hash(s, h);
		new = node(car(p), v[h%k]);
		v = vector(car(e));
		v[h%k] = new;
		p = cdr(p);
	}
}

cell extend(cell v, cell a, cell e) {
	cell	n, new;

	n = make_binding(v, a);
	set(cdr(e), node(n, cdr(e)));
	if (box_value(S_loading) == FALSE)
		rehash(e);
	return e;
}

cell make_env(cell rib, cell env) {
	cell	e;

	Tmp = env;
	rib = node(NIL, rib);
	e = node(rib, env);
	Tmp = NIL;
	if (length(rib) >= HASH_THRESHOLD) {
		save(e);
		rehash(rib);
		unsave(1);
	}
	return e;
}

cell try_hash(cell v, cell e) {
	cell		*hv, p;
	unsigned int	h, k;
	char		*s;

	if (e == NIL || car(e) == NIL)
		return NIL;
	hv = vector(car(e));
	k = vector_len(car(e));
	s = symbol_name(v);
	hash(s, h);
	p = hv[h%k];
	while (p != NIL) {
		if (caar(p) == v)
			return car(p);
		p = cdr(p);
	}
	return NIL;
}

cell lookup(cell v, cell env, int req) {
	cell	e, n;

	while (env != NIL) {
		e = car(env);
		n = try_hash(v, e);
		if (n != NIL)
			return n;
		if (e != NIL)
			e = cdr(e);	/* skip over hash table */
		while (e != NIL) {
			if (v == caar(e))
				return car(e);
			e = cdr(e);
		}
		env = cdr(env);
	}
	if (!req)
		return NIL;
	if (special_p(v))
		error("invalid syntax", v);
	else
		error("symbol not bound", v);
	return NIL;
}

cell too_few_args(cell n) {
	return error("too few arguments", n);
}

cell too_many_args(cell n) {
	return error("too many arguments", n);
}

/* Set up sequence for AND, BEGIN, OR. */
cell make_sequence(int state, cell neutral, cell x, int *pc, int *ps) {
	if (cdr(x) == NIL) {
		return neutral;
	}
	else if (cddr(x) == NIL) {
		*pc = 1;
		return cadr(x);
	}
	else {
		*pc = 2;
		*ps = state;
		save(cdr(x));
		return cadr(x);
	}
}

#define sf_also(x, pc, ps) \
	make_sequence(EV_AND, TRUE, x, pc, ps)

#define sf_begin(x, pc, ps) \
	make_sequence(EV_BEGIN, UNIT, x, pc, ps)

cell sf_if(cell x, int *pc, int *ps) {
	cell	m;

	m = cdr(x);
	if (m == NIL || cdr(m) == NIL || cddr(m) == NIL)
		return too_few_args(x);
	if (cdddr(m) != NIL)
		return too_many_args(x);
	save(m);
	*pc = 2;
	*ps = EV_IF_PRED;
	return car(m);
}

cell make_undefineds(cell x) {
	cell	n;

	n = NIL;
	while (x != NIL) {
		n = node(UNDEFINED, n);
		x = cdr(x);
	}
	return n;
}

/*
 * (letrec              --->  ((fn ((f1 f2)
 *   ((f1 (pat1 expr1)               (set! f1 (fn (pat1 expr1)
 *        (pat2 expr2)                            (pat2 expr2)) 
 *        ...)                                    ...)
 *    (f2 (pat1 expr1)               (set! f2 (fn (pat1 expr1)
 *        (pat2 expr2)                            (pat2 expr2)
 *         ...))                                  ...)))
 *   expr ...)                       ((fn (() expr ...))))
 *                             undefined  
 *                             undefined)
 *
 * NB: this is equivalent to LETREC*, but we can get away with this,
 *     because we *only* ever bind functions.
 *
 * XXX This really should be a macro.
 */

cell make_recursive_fns(cell cs, cell body) {
	cell	p, c, n, fns, vars, undef, new;

	undef = make_undefineds(cs);
	save(undef);
	fns = body;			/* ((fn (() body ...))) */
	fns = node(NIL, fns);
	fns = node(fns, NIL);
	fns = node(S_fn, fns);
	fns = node(fns, NIL);
	fns = node(fns, NIL);
	save(fns);
	for (p = cs; p != NIL; p = cdr(p)) {
		c = car(p);		/* (set! name (fn case ...)) */
		n = cdr(c);
		n = node(S_fn, n);
		n = node(n, NIL);
		n = node(car(c), n);
		n = node(S_setb, n);
		set(car(Stack), node(n, car(Stack)));
	}
	vars = NIL;
	for (p = cs; p != NIL; p = cdr(p))
		vars = node(caar(p), vars);
	if (vars != NIL && cdr(vars) == NIL)
		vars = car(vars);
	fns = unsave(1);
	fns = node(vars, fns);
	fns = node(fns, NIL);
	fns = node(S_fn, fns);
	undef = unsave(1);
	return node(fns, undef);
}

cell sf_letrec(cell x, int *pc) {
	cell	p;

	if (!tuple_p(cdr(x)) || !tuple_p(cddr(x)))
		too_few_args(x);
	for (p = cadr(x); tuple_p(p); p = cdr(p)) {
		if (	!tuple_p(car(p)) ||
			!symbol_p(caar(p)) ||
			!tuple_p(cdar(p))
		)
			return error("letrec: malformed case", car(p));
	}
	if (p != NIL)
		return error("letrec: suntax error", x);
	*pc = 1;
	return make_recursive_fns(cadr(x), cddr(x));
}

cell sf_fn(cell x) {
	cell	n, a, cas, new;

	if (cdr(x) == NIL)
		return too_few_args(x);
	a = node(NIL, NIL);
	save(a);
	for (x = cdr(x); x != NIL; x = cdr(x)) {
		cas = car(x);
		if (!tuple_p(cas) || cdr(cas) == NIL)
			return error("fn: malformed case", cas);
		if (cddr(cas) != NIL) {
			n = node(S_begin, cdr(cas));
			n = node(n, NIL);
			n = node(car(cas), n);
			cas = n;
		}
		car(a) = cas;
		if (cdr(x) != NIL) {
			set(cdr(a), node(NIL, NIL));
			a = cdr(a);
		}
	}
	n = node(car(Stack), car(Stack));
	n = node(Environment, n);
	unsave(1);
	return new_atom(T_FUNCTION, n);
}

cell sf_quote(cell x) {
	if (cdr(x) == NIL)
		return too_few_args(x);
	if (cddr(x) != NIL)
		return too_many_args(x);
	return cadr(x);
}

#define sf_or(x, pc, ps) \
	make_sequence(EV_OR, FALSE, x, pc, ps)

cell sf_raise(cell x, int *pc, int *ps) {
	*pc = 2;
	*ps = EV_RAISE;
	return cadr(x);
}

cell sf_setb(cell x, int *pc, int *ps) {
	cell	n;
	int	k;

	k = length(x);
	if (k < 3) return too_few_args(x);
	if (k > 3) return too_many_args(x);
	if (!symbol_p(cadr(x)))
		return error("setb: expected id, got", cadr(x));
	n = lookup(cadr(x), Environment, 1);
	if (Error_flag)
		return NIL;
	save(n);
	*pc = 2;
	*ps = EV_SET_VAL;
	return caddr(x);
}

cell root_env(void) {
	cell	e;

	for (e = Environment; e != NIL && cdr(e) != NIL; e = cdr(e))
		;
	if (e == NIL)
		fatal("no root environment? oops!");
	return e;
}

cell find_local_variable(cell v, cell e) {
	if (e == NIL)
		return NIL;
	e = cdr(e);
	while (e != NIL) {
		if (v == caar(e))
			return car(e);
		e = cdr(e);
	}
	return NIL;
}

cell sf_define(int syntax, cell x, int *pc, int *ps) {
	cell	v, a, n, new, e;
	int	k;

	if (car(State_stack) == EV_ARGS)
		return error(syntax?
				"define_syntax: invalid context":
				"define: invalid context",
				x);
	k = length(x);
	if (k < 3)
		return too_few_args(x);
	if (k > 3)
		return too_many_args(x);
	if (!symbol_p(cadr(x)))
		return error(syntax?
				"define_syntax: expected id, got":
				"define: expected id, got",
				cadr(x));
	save(NIL);
	a = caddr(x);
	n = cadr(x);
	e = root_env();
	v = find_local_variable(n, car(e));
	if (v == NIL) {
		set(car(e), extend(n, UNDEFINED, car(e)));
		v = cadar(e);
	}
	car(Stack) = binding_box(v);
	*pc = 2;
	if (syntax)
		*ps = EV_MACRO;
	else
		*ps = EV_SET_VAL;
	return a;
}

/*
 * Convert :SYMBOL to SYMBOL:ID,
 * i.e. turn a constructor name into a variable name.
 */
cell a_name(cell n, int id) {
	char	s[TOKEN_LENGTH+10];

	if (!strcmp(string(n), "::"))
		strcpy(s, "%cons");
	else
		strcpy(s, string(n)+1);
	sprintf(&s[strlen(s)], ":%d", id);
	return symbol_ref(s);
}

/*
 * Make algebraic type constructor.
 *
 * (:cons var|type ...)  -->  (fn ((var|type' ...)
 *                                   (list ':cons var|type'' ...)))
 *
 * where TYPE'  = type:id
 *   and TYPE'' = (type-check ':cons type:id type)
 *
 * E.g.:
 *   (:cons x :L)  -->  (fn ((x L:0)
 *                             (list ':cons x (type-check ':cons L:0 L))))
 */

cell make_constructor(cell x) {
	cell	n, m, v, a, p, an, new;
	int	id = 0;

	if (!symbol_p(car(x)) || !consname_p(car(x)))
		return error("define_type: expected constructor, got",
				car(x));
	if (!tuple_p(cdr(x)))
		return error("constructor syntax error", x);
	a = node(NIL, NIL);
	save(a);
	for (p = cdr(x); p != NIL; p = cdr(p)) {
		if (!symbol_p(car(p))) {
			return error("non-symbol in constructor", car(p));
		}
		if (consname_p(car(p))) {
			set(car(a), a_name(car(p), id++));
		}
		else {
			car(a) = car(p);
		}
		if (cdr(p) != NIL) {
			set(cdr(a), node(NIL, NIL));
			a = cdr(a);
		}
	}
	a = node(NIL, NIL);
	save(a);
	id = 0;
	for (p = x; p != NIL; p = cdr(p)) {
		if (p == x) {			/* ':constructor */
			n = node(car(p), NIL);
			n = node(S_quote, n);
		}
		else if (consname_p(car(p))) {	/* (type-check c-name a-name */
						/*	       type) */
			if (car(p) == S_cons)
				n = node(S_primlist, NIL);
			else
				n = node(car(p), NIL);
			save(n);
			an = a_name(car(p), id++);
			unsave(1);
			n = node(an, n);
			save(n);
			m = node(car(x), NIL);	/* ':name */
			m = node(S_quote, m);
			unsave(1);
			n = node(m, n);
			n = node(S_typecheck, n);
		}
		else {				/* name */
			n = car(p);
		}
		car(a) = n;
		if (cdr(p) != NIL) {
			set(cdr(a), node(NIL, NIL));
			a = cdr(a);
		}
	}
	n = unsave(1);
	n = node(S_tuple, n);
	n = node(n, NIL);
	v = unsave(1);
	if (cdr(v) == NIL)
		v = car(v);
	n = node(v, n);
	n = node(n, NIL);
	n = node(S_fn, n);
	return n;
}

/*
 * Define new algebraic type.
 *
 * (define_type type-name pattern ...)
 *   -->  (begin (define type-name '(type-name pattern ...))
 *               (define pattern constructor)
 *               ...)
 *
 * Atomic constructors translate to constants:
 *
 * (define_type :X :Y)  -->  (begin (define :X '(:X :Y))
 *                                  (define :Y ':Y))
 *
 * Parameterized constructors translate to type-checking functions
 * as described in make_constructor(), above.
 *
 * Example:
 *
 *   (define_type :LIST :NIL (:cons x :LIST))
 *
 *     --> (begin (define :LIST '(:LIST :NIL (:cons x :LIST)))
 *                (define :NIL ':NIL)
 *                (define :cons
 *                        (fn ((x :LIST:0)
 *                               (list ':cons
 *                                     x
 *                                     (type-check ':cons LIST:0 :LIST))))))
 */

cell sf_define_type(cell x, int *pc) {
	cell	p, t, n, a, defs, type, new;

	if (!tuple_p(cdr(x)) || !tuple_p(cddr(x)))
		return error("define_type: syntax error", x);
	if (!symbol_p(cadr(x)))
		return error("define_type: expected id, got", cadr(x));
	a = node(NIL, NIL);
	save(a);
	for (p = cddr(x); p != NIL; p = cdr(p)) {
		t = car(p);
		if (symbol_p(t)) {		/* (name 'name) */
			n = node(t, NIL);
			n = node(S_quote, n);
			n = node(n, NIL);
			n = node(t, n);
		}
		else {				/* (name (fn (x y))) */
			n = make_constructor(t);
			n = node(n, NIL);
			n = node(car(t), n);
		}
		n = node(S_define, n);
		car(a) = n;
		if (cdr(p) != NIL) {
			set(cdr(a), node(NIL, NIL));
			a = cdr(a);
		}
	}
	type = cadr(x);			/* (define T '(T cons ...)) */
	type = node(type, cddr(x));
	type = node(type, NIL);
	type = node(S_quote, type);
	type = node(type, NIL);
	type = node(cadr(x), type);
	type = node(S_define, type);
	defs = unsave(1);		/* (begin type-def cons-def ...) */
	defs = node(type, defs);
	defs = node(S_begin, defs);
	*pc = 1;
	return defs;
}

cell sf_redirect(int out, cell x, int *pc, int *ps) {
	int	k;

	k = length(x);
	if (k < 3) return too_few_args(x);
	save(cons(S_begin, cddr(x)));
	*pc = 2;
	*ps = out? EV_OUTPUT: EV_INPUT;
	return cadr(x);
}

cell apply_special(cell x, int *pc, int *ps) {
	cell	sf;

	sf = car(x);
	if (sf == S_quote) return sf_quote(x);
	else if (sf == S_if) return sf_if(x, pc, ps);
	else if (sf == S_also) return sf_also(x, pc, ps);
	else if (sf == S_or) return sf_or(x, pc, ps);
	else if (sf == S_begin) return sf_begin(x, pc, ps);
	else if (sf == S_fn) return sf_fn(x);
	else if (sf == S_setb) return sf_setb(x, pc, ps);
	else if (sf == S_letrec) return sf_letrec(x, pc);
	else if (sf == S_define) return sf_define(0, x, pc, ps);
	else if (sf == S_raise) return sf_raise(x, pc, ps);
	else if (sf == S_from) return sf_redirect(0, x, pc, ps);
	else if (sf == S_to) return sf_redirect(1, x, pc, ps);
	else if (sf == S_define_type) return sf_define_type(x, pc);
	else if (sf == S_define_syntax) return sf_define(1, x, pc, ps);
	else fatal("internal: unknown special form");
	return UNDEFINED;
}

/*
 * Polymorphic primitives
 */

cell append_str(cell x) {
	cell	p, n;
	int	k, m;
	char	*s;

	k = 0;
	for (p = x; p != NIL; p = cdr(p))
		k += string_len(car(p))-1;
	n = make_string("", k);
	s = string(n);
	k = 0;
	for (p = x; p != NIL; p = cdr(p)) {
		m = string_len(car(p));
		memcpy(&s[k], string(car(p)), m);
		k += string_len(car(p))-1;
	}
	return n;
}

cell append_list(cell x) {
	cell	new, n, p, a, *pa;

	if (car(x) == NIL)
		return cadr(x);
	if (cadr(x) == NIL)
		return car(x);
	a = n = cons(NIL, NIL);
	pa = &a;
	save(n);
	for (p = car(x); p != NIL; p = cdr(p)) {
		car(a) = car(p);
		set(cdr(a), cons(NIL, NIL));
		pa = &cdr(a);
		a = cdr(a);
	}
	unsave(1);
	*pa = cadr(x);
	return n;
}

cell pp_append(cell x) {
	cell	n1, n2;

	n1 = car(x);
	n2 = cadr(x);
	if (list_p(n1) && list_p(n2))
		return append_list(x);
	if (string_p(n1) && string_p(n2))
		return append_str(x);
	return error("@: expected tuple of str/list, got", x);
}

cell pp_clone(cell x) {
	cell	v;
	int	k;

	if (vector_p(car(x))) {
		k = vector_len(car(x));
		v = new_vec(T_VECTOR, k * sizeof(cell));
		memcpy(vector(v), vector(car(x)), k * sizeof(cell));
		return v;
	}
	else {
		return car(x);
	}
}

cell pp_len(cell x) {
	int	k = 0;

	if (string_p(car(x)))
		return make_integer(string_len(car(x))-1);
	if (vector_p(car(x)))
		return make_integer(vector_len(car(x)));
	if (!list_p(car(x)) && !tuple_p(car(x)))
		error("len: expected str/list/tuple/vec, got", car(x));
	for (x = car(x); x != NIL; x = cdr(x))
		k++;
	return make_integer(k);
}

cell pp_order(cell x) {
	if (car(x) == UNIT)
		return make_integer(0);
	if (tuple_p(car(x)))
		return pp_len(x);
	return make_integer(1);
}

cell pp_rev(cell x) {
	cell	n, m;
	int	k, i;
	char	*s, *d;

	if (list_p(car(x))) {
		m = NIL;
		for (n = car(x); n != NIL; n = cdr(n))
			m = cons(car(n), m);
		return m;
	}
	if (string_p(car(x))) {
		s = string(car(x));
		k = string_len(car(x))-1;
		m = make_string("", k);
		d = string(m);
		for (i=0; i<k; i++)
			d[i] = s[k-i-1];
		d[i] = 0;
		return m;
	}
	return error("rev: expected list/str, got", car(x));
}

#define L(c) tolower(c)
int char_ine(int c1, int c2) { return L(c1) != L(c2); }
int char_ile(int c1, int c2) { return L(c1) <= L(c2); }
int char_ilt(int c1, int c2) { return L(c1) <  L(c2); }
int char_ieq(int c1, int c2) { return L(c1) == L(c2); }
int char_ige(int c1, int c2) { return L(c1) >= L(c2); }
int char_igt(int c1, int c2) { return L(c1) >  L(c2); }

int char_ne(int c1, int c2) { return c1 != c2; }
int char_le(int c1, int c2) { return c1 <= c2; }
int char_lt(int c1, int c2) { return c1 <  c2; }
int char_eq(int c1, int c2) { return c1 == c2; }
int char_ge(int c1, int c2) { return c1 >= c2; }
int char_gt(int c1, int c2) { return c1 >  c2; }

#define RT	k=0; return
int str_ile(char *s1, char *s2, int k) { RT strcmp_ci(s1, s2) <= 0; }
int str_ilt(char *s1, char *s2, int k) { RT strcmp_ci(s1, s2) <  0; }
int str_ige(char *s1, char *s2, int k) { RT strcmp_ci(s1, s2) >= 0; }
int str_igt(char *s1, char *s2, int k) { RT strcmp_ci(s1, s2) >  0; }
int str_ieq(char *s1, char *s2, int k) {
	return memcmp_ci(s1, s2, k) == 0;
}
int str_ine(char *s1, char *s2, int k) {
	return memcmp_ci(s1, s2, k) != 0;
}

int str_le(char *s1, char *s2, int k) { RT strcmp(s1, s2) <= 0; }
int str_lt(char *s1, char *s2, int k) { RT strcmp(s1, s2) <  0; }
int str_ge(char *s1, char *s2, int k) { RT strcmp(s1, s2) >= 0; }
int str_gt(char *s1, char *s2, int k) { RT strcmp(s1, s2) >  0; }
int str_eq(char *s1, char *s2, int k) {
	return memcmp(s1, s2, k) == 0;
}
int str_ne(char *s1, char *s2, int k) {
	return memcmp(s1, s2, k) != 0;
}

int real_noteq_p(cell x, cell y)   { return !real_equal_p(x, y); }
int real_napprox_p(cell x, cell y) { return !real_approx_p(x, y); }
int real_greater_p(cell x, cell y) { return  real_less_p(y, x); }
int real_lteq_p(cell x, cell y)    { return !real_greater_p(x, y); }
int real_gteq_p(cell x, cell y)    { return !real_less_p(x, y); }

cell cmp(char *name,
	int ncmp(cell x, cell y),
	int ccmp(int x, int y),
	int scmp(char *s1, char *s2, int k),
	cell x
) {
	int	k;
	char	msg[100], *s;

	if (*name == '=' && (virtual_p(car(x)) || virtual_p(cadr(x))))
		return (car(x) == cadr(x))? TRUE: FALSE;
	if (number_p(car(x)) && number_p(cadr(x)))
		return ncmp(car(x), cadr(x))? TRUE: FALSE;
	else if (char_p(car(x)) && char_p(cadr(x)))
		return ccmp(char_value(car(x)), char_value(cadr(x)))? TRUE:
			FALSE;
	else if (string_p(car(x)) && string_p(cadr(x))) {
		k = string_len(car(x));
		if (k != string_len(cadr(x))) {
			if (scmp == str_eq || scmp == str_ieq)
				return FALSE;
			if (scmp == str_ne || scmp == str_ine)
				return TRUE;
		}
		return !scmp(string(car(x)), string(cadr(x)), k)? FALSE: TRUE;
	}
	else {
		if (*name == '=')
			s = "num/char/str/bool/unit/nil";
		else
			s = "num/char/str";
		sprintf(msg, "%s: expected tuple of %s, but got", name, s);
		error(msg, x);
		return 0;
	}
}

#define RC return cmp
cell pp_equal(cell x)    { RC("=",   real_equal_p,   char_eq, str_eq, x); }
cell pp_noteq(cell x)    { RC("<>",  real_noteq_p,   char_ne, str_ne, x); }
cell pp_less(cell x)     { RC("<",   real_less_p,    char_lt, str_lt, x); }
cell pp_greater(cell x)  { RC(">",   real_greater_p, char_gt, str_gt, x); }
cell pp_lteq(cell x)     { RC("<=",  real_lteq_p,    char_le, str_le, x); }
cell pp_gteq(cell x)     { RC(">=",  real_gteq_p,    char_ge, str_ge, x); }
cell pp_iequal(cell x)   { RC("~=",  real_approx_p,  char_ieq, str_ieq, x); }
cell pp_inoteq(cell x)   { RC("~<>", real_napprox_p, char_ine, str_ine, x); }
cell pp_iless(cell x)    { RC("~<",  real_less_p,    char_ilt, str_ilt, x); }
cell pp_igreater(cell x) { RC("~>",  real_greater_p, char_igt, str_igt, x); }
cell pp_ilteq(cell x)    { RC("~<=", real_lteq_p,    char_ile, str_ile, x); }
cell pp_igteq(cell x)    { RC("~>=", real_gteq_p,    char_ige, str_ige, x); }

cell sublis(cell x, int p0, int pn, int tag) {
	cell	p, a, new;

	pn -= p0;
	if (pn == 0)
		return NIL;
	a = node3(NIL, NIL, tag);
	save(a);
	for (p = car(x); p0; p = cdr(p), p0--)
		;
	for (; pn; p = cdr(p), pn--) {
		car(a) = car(p);
		if (pn > 1) {
			set(cdr(a), node3(NIL, NIL, tag));
			a = cdr(a);
		}
	}
	return unsave(1);
}

/* XXX should merge tails, if possible */
cell pp_sub(cell x) {
	int	k;
	int	p0 = integer_value("sub", cadr(x));
	int	pn = integer_value("sub", caddr(x));
	char	*src, *dst;
	cell	n;

	if (!string_p(car(x)) && !list_p(car(x)))
		return error("sub: expected list/str, got", car(x));
	k = list_p(car(x)) || tuple_p(car(x))? length(car(x)):
						string_len(car(x))-1;
	if (p0 < 0 || p0 > k || pn < 0 || pn > k || pn < p0) {
		n = node(caddr(x), NIL);
		return error("sub: invalid range", node(cadr(x), n));
	}
	if (list_p(car(x)))
		return sublis(x, p0, pn, LIST_TAG);
	n = make_string("", pn-p0);
	dst = string(n);
	src = string(car(x));
	if (pn-p0 != 0)
		memcpy(dst, &src[p0], pn-p0);
	dst[pn-p0] = 0;
	return n;
}

cell pp_ref(cell x) {
	int	p, k, n;

	p = integer_value("ref", cadr(x));
	if (cons_p(car(x))) {
		for (n = car(x); n != NIL && p; n = cdr(n), p--)
			;
		if (n == NIL)
			return error("ref: index out of range", cadr(x));
		return car(n);
	}
	if (vector_p(car(x))) {
		k = vector_len(car(x))-1;
		if (p < 0 || p > k)
			return error("ref: index out of range", cadr(x));
		return vector(car(x))[p];
	}
	if (!string_p(car(x)))
		return error("ref: expected str/tuple/list/vec, got", car(x));
	k = string_len(car(x))-1;
	if (p < 0 || p >= k)
		return error("ref: index out of range", cadr(x));
	return make_char(string(car(x))[p]);
}

cell pp_set(cell x) {
	int	p, k;
	cell	n, a, new;

	p = integer_value("set", cadr(x));
	if (tuple_p(car(x))) {
		a = node(NIL, NIL);
		save(a);
		for (n = car(x); n != NIL && p; n = cdr(n), p--) {
			car(a) = car(n);
			set(cdr(a), node(NIL, NIL));
			a = cdr(a);
		}
		if (n == NIL)
			return error("set: index out of range", cadr(x));
		car(a) = caddr(x);
		cdr(a) = cdr(n);
		return unsave(1);
	}
	if (vector_p(car(x))) {
		k = vector_len(car(x));
		if (p < 0 || p >= k)
			return error("set: index out of range", cadr(x));
		vector(car(x))[p] = caddr(x);
		return car(x);
	}
	if (!string_p(car(x)))
		return error("ref: expected str/tuple/vec, got", car(x));
	if (!char_p(caddr(x)))
		return error("ref: expected char, got", caddr(x));
	k = string_len(car(x))-1;
	if (p < 0 || p >= k)
		return error("set: index out of range", cadr(x));
	n = make_string("", k);
	memcpy(string(n), string(car(x)), k+1);
	string(n)[p] = char_value(caddr(x));
	return n;
}

/*
 * Control primitives
 */

/*
 * (type-check c-name datum (type pattern ...))
 *
 * If DATUM is in PATTERN..., return DATUM,
 * else report a type error: "c-name: expected TYPE, got DATUM"
 */

cell pp_typecheck(cell x) {
	cell	cname, datum, type, ps, p;
	char	msg[TOKEN_LENGTH*2];

	cname = car(x);
	datum = cadr(x);
	ps = caddr(x);
	if (!tuple_p(ps) || !symbol_p(car(ps)))
		return error("type-check: bad type pattern", ps);
	type = car(ps);
	for (ps = cdr(ps); tuple_p(ps); ps = cdr(ps)) {
		p = car(ps);
		if (symbol_p(p) && p == datum)
			return datum;
		else if (p == S_cons && list_p(datum))
			return datum;
		else if (tuple_p(p) && tuple_p(datum) && car(p) == car(datum))
			return datum;
	}
	sprintf(msg, "%s: expected %s, got",
			string(cname), string(type));
	return error(msg, datum);
}

cell pp_call_cc(cell x) {
	cell	cc, n;

	cc = node(Stack, NIL);
	cc = node(Stack_bottom, cc);
	cc = node(State_stack, cc);
	cc = node(Environment, cc);
	cc = new_atom(T_CONTINUATION, cc);
	n = node(cc, NIL);
	n = node(car(x), n);
	return n;
}

cell resume(cell x) {
	cell	cc;

	if (cdr(x) == NIL)
		return too_few_args(x);
	if (cddr(x) != NIL)
		return too_many_args(x);
	cc = cdar(x);
	Environment = car(cc);
	State_stack = cadr(cc);
	Stack_bottom = caddr(cc);
	Stack = cadddr(cc);
	return cadr(x);
}

cell pp_registerb(cell x) {
	cell	n;

	n = node(car(x), cadr(x));
	Exnstack = node(n, Exnstack);
	return UNIT;
}

cell pp_unregisterb(cell x) {
	if (Exnstack == NIL)
		return error("internal error: no handlers registered",
			NOEXPR);
	Exnstack = cdr(Exnstack);
	return car(x);
}

/*
 * Predicates and Booleans
 */

int eqv_p(cell a, cell b) {
	if (a == b)
		return 1;
	if (char_p(a) && char_p(b) && char_value(a) == char_value(b))
		return 1;
	if (number_p(a) && number_p(b)) {
		if (real_p(a) != real_p(b))
			return 0;
		return real_equal_p(a, b);
	}
	return a == b;
}

cell pp_not(cell x) {
	return car(x) == FALSE? TRUE: FALSE;
}

/*
 * Pairs and Lists
 */

int assqv(char *who, int v, cell x, cell a) {
	cell	p;
	char	buf[64];

	for (p = a; p != NIL; p = cdr(p)) {
		if (!list_p(car(p))) {
			sprintf(buf, "%s: bad element in alist", who);
			return error(buf, p);
		}
		if (!v && x == caar(p))
			return car(p);
		if (v && eqv_p(x, caar(p)))
			return car(p);
	}
	return FALSE;
}

cell pp_cons(cell x) {
	return cons(car(x), cadr(x));
}

cell pp_list(cell x) {
	cell	p;

	for (p = x; p != NIL; p = cdr(p)) {
		Tag[p] |= LIST_TAG;
	}
	return x;
}

cell pp_tuple(cell x) {
	return x;
}

/*
 * Arithmetics
 */

cell pp_divide(cell x) {
	return real_divide(x, car(x), cadr(x));
}

cell pp_abs(cell x) {
	return real_abs(car(x));
}

cell limit(char *msg, int(*pred)(cell,cell), cell x) {
	cell	k;

	if (pred(car(x), cadr(x)))
		k = car(x);
	else
		k = cadr(x);
	if (!real_p(car(x)) && !real_p(cadr(x)))
		return k;
	if (integer_p(k))
		return bignum_to_real(k);
	return k;
}

cell pp_floor(cell x) {
	cell	n, m, e;

	x = car(x);
	if (integer_p(x))
		return x;
	e = x_real_exponent(x);
	if (e >= 0)
		return real_to_bignum(x);
	m = new_atom(T_INTEGER, x_real_mantissa(x));
	save(m);
	while (e < 0) {
		m = bignum_shift_right(m);
		m = car(m);
		car(Stack) = m;
		e++;
	}
	if (x_real_negative_p(x)) {
		/* sign not in mantissa! */
		m = bignum_add(m, make_integer(1));
	}
	unsave(1);
	n = make_real(x_real_flags(x), e, cdr(m));
	return real_to_bignum(n);
}

cell pp_max(cell x) {
	return limit("max: expected num, got", real_greater_p, x);
}

cell pp_min(cell x) {
	return limit("min: expected num, got", real_less_p, x);
}

cell pp_minus(cell x) {
	return real_subtract(car(x), cadr(x));
}

cell pp_plus(cell x) {
	return real_add(car(x), cadr(x));
}

cell pp_div(cell x) {
	x = bignum_divide(x, car(x), cadr(x));
        return car(x);
}

cell pp_real(cell x) {
	return number_p(car(x))? TRUE: FALSE;
}

cell pp_rem(cell x) {
	x = bignum_divide(x, car(x), cadr(x));
	return cdr(x);
}

cell pp_times(cell x) {
	return real_multiply(car(x), cadr(x));
}

cell pp_neg(cell x) {
	return real_negate(car(x));
}

cell pp_real_mant(cell x) {
	cell	m;

	if (integer_p(car(x)))
		return car(x);
	m = new_atom(T_INTEGER, x_real_mantissa(car(x)));
	if (x_real_negative_p(car(x)))
		m = bignum_negate(m);
	return m;
}

cell pp_real_exp(cell x) {
	if (integer_p(cadr(x)))
		return make_integer(0);
	return make_integer(x_real_exponent(car(x)));
}

/*
 * Type Predicates and Conversion
 */

cell pp_bool(cell x) {
	return boolean_p(car(x))? TRUE: FALSE;
}

cell pp_char(cell x) {
	return char_p(car(x))? TRUE: FALSE;
}

cell pp_ord(cell x) {
	return make_integer(char_value(car(x)));
}

cell pp_chr(cell x) {
	cell	n;

	n = integer_value("chr", car(x));
	if (n < 0 || n > 255)
		return error("chr: argument value out of range", car(x));
	return make_char(n);
}

cell pp_int(cell x) {
	return real_integer_p(car(x))? TRUE: FALSE;
}

cell implode(char *who, cell x) {
	cell	n;
	int	k = length(x);
	char	*s;
	char	buf[100];

	n = make_string("", k);
	s = string(n);
	while (x != NIL) {
		if (!char_p(car(x))) {
			sprintf(buf, "%s: expected list of char,"
					" got list containing",
				who);
			return error(buf, car(x));
		}
		*s++ = cadar(x);
		x = cdr(x);
	}
	*s = 0;
	return n;
}

cell pp_implode(cell x) {
	return implode("implode", car(x));
}

cell pp_explode(cell x) {
	char	*s;
	cell	n, a, new;
	int	k, i;

	k = string_len(car(x));
	n = NIL;
	a = NIL;
	for (i=0; i<k-1; i++) {
		s = string(car(x));
		if (n == NIL) {
			n = a = cons(make_char(s[i]), NIL);
			save(n);
		}
		else {
			set(cdr(a), cons(make_char(s[i]), NIL));
			a = cdr(a);
		}
	}
	if (n != NIL)
		unsave(1);
	return n;
}

cell pp_str(cell x) {
	return string_p(car(x))? TRUE: FALSE;
}

cell pp_vec(cell x) {
	return vector_p(car(x))? TRUE: FALSE;
}

/*
 * Characters
 */

cell pp_c_alphabetic(cell x) {
	return isalpha(char_value(car(x)))? TRUE: FALSE;
}

cell pp_c_downcase(cell x) {
	return make_char(tolower(char_value(car(x))));
}

cell pp_c_lower(cell x) {
	return islower(char_value(car(x)))? TRUE: FALSE;
}

cell pp_c_numeric(cell x) {
	return isdigit(char_value(car(x)))? TRUE: FALSE;
}

cell pp_c_upcase(cell x) {
	return make_char(toupper(char_value(car(x))));
}

cell pp_c_upper(cell x) {
	return isupper(char_value(car(x)))? TRUE: FALSE;
}

cell pp_c_whitespace(cell x) {
	int	c = char_value(car(x));

	return (c == ' '  || c == '\t' || c == '\n' ||
		c == '\r' || c == '\f')? TRUE: FALSE;
}

/*
 * Strings
 */

cell pp_newstr(cell x) {
	cell	n;
	int	k;
	char	*s;

	k = integer_value("string", car(x));
	if (k < 0)
		return error("string: got negative length", x);
	n = make_string("", k);
	s = string(n);
	memset(s, char_value(cadr(x)), k);
	s[k] = 0;
	return n;
}

/*
 * Vectors
 */

cell pp_newvec(cell x) {
	int	i, k;
	cell	n, *v, m;

	k = integer_value("newvec", car(x));
	if (k < 0)
		return error("newvec: got negative length", car(x));
	n = new_vec(T_VECTOR, k * sizeof(cell));
	v = vector(n);
	m = cadr(x);
	for (i=0; i<k; i++)
		v[i] = m;
	return n;
}

cell pp_setvec(cell x) {
	cell	fill = cadddr(x);
	int	i, k = vector_len(car(x));
	int	p0 = integer_value("setvec", cadr(x));
	int	pn = integer_value("setvec", caddr(x));
	cell	*v = vector(car(x));
	cell	n;

	if (p0 < 0 || p0 > k || pn < p0 || pn > k) {
		n = node(caddr(x), NIL);
		n = node(cadr(x), n);
		return error("setvec: bad range", n);
	}
	for (i=p0; i<pn; i++)
		v[i] = fill;
	return car(x);
}

/*
 * I/O
 */

void close_stream(int stream) {
	if (stream < 0 || stream >= MAX_STREAMS)
		return;
	if (Streams[stream] == NULL) {
		Stream_flags[stream] = 0;
		return;
	}
	fclose(Streams[stream]); /* already closed? don't care */
	Streams[stream] = NULL;
	Stream_flags[stream] = 0;
}

cell pp_close_stream(cell x) {
	close_stream(stream_id(car(x)));
	return UNIT;
}

cell make_stream(int sid, cell type) {
	cell	n;
	int	pf;

	pf = Stream_flags[sid];
	Stream_flags[sid] |= LOCK_TAG;
	n = new_atom(sid, NIL);
	n = node3(type, n, ATOM_TAG|STREAM_TAG);
	Stream_flags[sid] = pf;
	return n;
}

cell pp_write(cell x);

cell pp_print(cell x) {
	Displaying = 1;
	print_form(car(x));
	Displaying = 0;
	return UNIT;
}

cell pp_println(cell x) {
	pp_print(x);
	pr("\n");
	return UNIT;
}

cell pp_eof(cell x) {
	return car(x) == END_OF_FILE? TRUE: FALSE;
}

int new_stream(void) {
	int	i, tries;

	for (tries=0; tries<2; tries++) {
		for (i=0; i<MAX_STREAMS; i++) {
			if (Streams[i] == NULL) {
				return i;
			}
		}
		if (tries == 0)
			gc();
	}
	return -1;
}

int open_stream(char *path, char *mode) {
	int	i = new_stream();

	if (i < 0)
		return -1;
	Streams[i] = fopen(path, mode);
	if (Streams[i] == NULL)
		return -1;
	else
		return i;
}

cell eval(cell x);
int  isfirst(int t);

int load(char *file) {
	int	n;
	int	outer_lno;
	int	new_stream, old_stream;
	int	outer_loading;
	int	outer_tok;

	new_stream = open_stream(file, "r");
	if (new_stream == -1)
		return -1;
	Stream_flags[new_stream] |= LOCK_TAG;
	File_list = node(make_string(file, (int) strlen(file)), File_list);
	save(Environment);
	while (cdr(Environment) != NIL)
		Environment = cdr(Environment);
	outer_loading = box_value(S_loading);
	box_value(S_loading) = TRUE;
	old_stream = Instream;
	outer_lno = Line_no;
	Line_no = 1;
	Instream = new_stream;
	outer_tok = Token;
	Token = scan();
	while (!Error_flag) {
		Instream = new_stream;
		if (Token == TOK_SEMI)
			Token = scan();
		n = prog();
		Instream = old_stream;
		if (n == END_OF_FILE)
			break;
		if (!isfirst(Token) && Token != TOK_SEMI)
			error("syntax error at", Attr);
		if (!Error_flag)
			n = eval(n);
	}
	Token = outer_tok;
	close_stream(new_stream);
	Line_no = outer_lno;
	box_value(S_loading) = outer_loading;
	File_list = cdr(File_list);
	rehash(car(Environment));
	Environment = unsave(1);
	return 0;
}

cell pp_load(cell x) {
	char	file[TOKEN_LENGTH+1];

	if (string_len(car(x)) > TOKEN_LENGTH-3)
		return error("load: path too long", car(x));
	strcpy(file, string(car(x)));
	if (load(file) < 0) {
		strcat(file, ".m");
		if (load(file) < 0)
			return error("load: cannot open file", car(x));
	}
	return UNIT;
}

cell pp_instream(cell x) {
	int	p;

	p = open_stream(string(car(x)), "r");
	if (p < 0)
		return error("instream: could not open file", car(x));
	return make_stream(p, T_INSTREAM);
}

cell pp_outstream(cell x) {
	int	p;

	p = open_stream(string(car(x)), "w");
	if (p < 0)
		return error("outstream: could not open file", car(x));
	return make_stream(p, T_OUTSTREAM);
}

cell pp_appendstream(cell x) {
	int	p;

	p = open_stream(string(car(x)), "a");
	if (p < 0)
		return error("append_stream: could not open file", car(x));
	return make_stream(p, T_OUTSTREAM);
}

cell pp_peekc(cell x) {
	int	c;

	c = read_c();
	if (c == EOF)
		return END_OF_FILE;
	ungetc(c, Streams[Instream]);
	return make_char(c);
}

cell pp_readc(cell x) {
	int	c;

	c = read_c();
	if (c == EOF)
		return END_OF_FILE;
	return make_char(c);
}

cell pp_readln(cell x) {
	static char	buf[TOKEN_LENGTH+1];
	int		k;

	if (fgets(buf, TOKEN_LENGTH, Streams[Instream]) == NULL)
		return END_OF_FILE;
	k = strlen(buf);
	if (buf[k-1] == '\n')
		buf[--k] = 0;
	return make_string(buf, k);
}

/*
 * Extensions
 */

char *copy_string(char *s) {
	char	*new;

	new = malloc(strlen(s)+1);
	if (s == NULL)
		fatal("copy_string(): out of memory");
	strcpy(new, s);
	return new;
}

cell pp_error(cell x) {
	return error(string(car(x)), cadr(x));
}

cell gensym(char *prefix) {
	static long	g = 0;
	char		s[200];

	do {
		sprintf(s, "%s%ld", prefix, g);
		g++;
	} while (find_symbol(s) != NIL);
	return symbol_ref(s);
}

cell pp_gensym(cell x) {
	char	pre[101];
	int	k;

	if (x == NIL) {
		strcpy(pre, "g");
		k = 1;
	}
	else if (string_p(car(x))) {
		memcpy(pre, string(car(x)), 100);
		k = string_len(car(x));
	}
	else if (symbol_p(car(x))) {
		memcpy(pre, symbol_name(car(x)), 100);
		k = symbol_len(car(x));
	}
	else
		return error("gensym: expected str/id, got",
				car(x));
	if (k > 100)
		return error("gensym: prefix too long", car(x));
	pre[100] = 0;
	return gensym(pre);
}

cell pp_stats(cell x) {
	cell	n, m;
	int	o_run_stats;

	gcv(); /* start from a known state */
	reset_counter(&Reductions);
	reset_counter(&Conses);
	reset_counter(&Nodes);
	reset_counter(&Collections);
	o_run_stats = Run_stats;
	Run_stats = 1;
	Cons_stats = 0;
	n = eval(car(x));
	save(n);
	Run_stats = o_run_stats;
	n = counter_to_list(&Collections);
	n = node(n, NIL);
	save(n);
	car(Stack) = n;
	m = counter_to_list(&Nodes);
	n = node(m, n);
	car(Stack) = n;
	m = counter_to_list(&Conses);
	n = node(m, n);
	car(Stack) = n;
	m = counter_to_list(&Reductions);
	n = node(m, n);
	n = node(unsave(2), n);
	return n;
}

#ifdef unix

cell pp_argv(cell x) {
	cell	n;
	char	**cl;

	if (Command_line == NULL || *Command_line == NULL)
		return FALSE;
	n = integer_value("argv", car(x));
	cl = Command_line;
	for (; n--; cl++)
		if (*cl == NULL)
			return FALSE;
	return *cl == NULL? FALSE: make_string(*cl, strlen(*cl));
}

cell pp_environ(cell x) {
	char	*s;

	s = getenv(string(car(x)));
	if (s == NULL)
		return FALSE;
	return make_string(s, strlen(s));
}

cell pp_system(cell x) {
	int	r;

	r = system(string(car(x)));
	return make_integer(r >> 8);
}

#endif

/*
 * Evaluator
 */

PRIM Core_primitives[] = {
 { "*",                   pp_times,               2,  2, { REA,REA,___ } },
 { "+",                   pp_plus,                2,  2, { REA,REA,___ } },
 { "@",                   pp_append,              2,  2, { ___,___,___ } },
 { "-",                   pp_minus,               2,  2, { REA,REA,___ } },
 { "/",                   pp_divide,              2,  2, { REA,REA,___ } },
 { "::",                  pp_cons,                2,  2, { ___,LST,___ } },
 { "<",                   pp_less,                2,  2, { ___,___,___ } },
 { "<=",                  pp_lteq,                2,  2, { ___,___,___ } },
 { "<>",                  pp_noteq,               2,  2, { ___,___,___ } },
 { "=",                   pp_equal,               2,  2, { ___,___,___ } },
 { ">",                   pp_greater,             2,  2, { ___,___,___ } },
 { ">=",                  pp_gteq,                2,  2, { ___,___,___ } },
 { "~<",                  pp_iless,               2,  2, { ___,___,___ } },
 { "~<=",                 pp_ilteq,               2,  2, { ___,___,___ } },
 { "~<>",                 pp_inoteq,              2,  2, { ___,___,___ } },
 { "~=",                  pp_iequal,              2,  2, { ___,___,___ } },
 { "~>",                  pp_igreater,            2,  2, { ___,___,___ } },
 { "~>=",                 pp_igteq,               2,  2, { ___,___,___ } },
 { "abs",                 pp_abs,                 1,  1, { REA,___,___ } },
 { "append_stream",       pp_appendstream,        1,  1, { STR,___,___ } },
 { "bool",                pp_bool,                1,  1, { ___,___,___ } },
 { "call/cc",             pp_call_cc,             1,  1, { ___,___,___ } },
 { "c_alphabetic",        pp_c_alphabetic,        1,  1, { CHR,___,___ } },
 { "c_downcase",          pp_c_downcase,          1,  1, { CHR,___,___ } },
 { "c_lower",             pp_c_lower,             1,  1, { CHR,___,___ } },
 { "c_numeric",           pp_c_numeric,           1,  1, { CHR,___,___ } },
 { "c_upcase",            pp_c_upcase,            1,  1, { CHR,___,___ } },
 { "c_upper",             pp_c_upper,             1,  1, { CHR,___,___ } },
 { "c_whitespace",        pp_c_whitespace,        1,  1, { CHR,___,___ } },
 { "char",                pp_char,                1,  1, { ___,___,___ } },
 { "chr",                 pp_chr,                 1,  1, { INT,___,___ } },
 { "clone",               pp_clone,               1,  1, { ___,___,___ } },
 { "close",               pp_close_stream,        1,  1, { IOS,___,___ } },
 { "div",                 pp_div,                 2,  2, { INT,INT,___ } },
 { "eof",                 pp_eof,                 1,  1, { ___,___,___ } },
 { "explode",             pp_explode,             1,  1, { STR,___,___ } },
 { "floor",               pp_floor,               1,  1, { REA,___,___ } },
 { "implode",             pp_implode,             1,  1, { LST,___,___ } },
 { "instream",            pp_instream,            1,  1, { STR,___,___ } },
 { "int",                 pp_int,                 1,  1, { ___,___,___ } },
 { "len",                 pp_len,                 1,  1, { ___,___,___ } },
 { "list",                pp_list,                0, -1, { ___,___,___ } },
 { "load",                pp_load,                1,  1, { STR,___,___ } },
 { "max",                 pp_max,                 2,  2, { REA,REA,___ } },
 { "min",                 pp_min,                 2,  2, { REA,REA,___ } },
 { "newstr",              pp_newstr,              2,  2, { INT,CHR,___ } },
 { "newvec",              pp_newvec,              2,  2, { INT,___,___ } },
 { "not",                 pp_not,                 1,  1, { ___,___,___ } },
 { "ord",                 pp_ord,                 1,  1, { CHR,___,___ } },
 { "order",               pp_order,               1,  1, { ___,___,___ } },
 { "outstream",           pp_outstream,           1,  1, { STR,___,___ } },
 { "peekc",               pp_peekc,               1,  1, { UNI,___,___ } },
 { "print",               pp_print,               1,  1, { ___,___,___ } },
 { "println",             pp_println,             1,  1, { ___,___,___ } },
 { "readc",               pp_readc,               1,  1, { UNI,___,___ } },
 { "readln",              pp_readln,              1,  1, { UNI,___,___ } },
 { "real",                pp_real,                1,  1, { ___,___,___ } },
 { "real_exp",            pp_real_exp,            1,  1, { REA,___,___ } },
 { "real_mant",           pp_real_mant,           1,  1, { REA,___,___ } },
 { "ref",                 pp_ref,                 2,  2, { ___,INT,___ } },
 { "%register",           pp_registerb,           0, -1, { ___,___,___ } },
 { "rem",                 pp_rem,                 2,  2, { INT,INT,___ } },
 { "rev",                 pp_rev,                 1,  1, { ___,___,___ } },
 { "set",                 pp_set,                 3,  3, { ___,INT,___ } },
 { "setvec",              pp_setvec,              4,  4, { VEC,INT,INT } },
 { "str",                 pp_str,                 1,  1, { ___,___,___ } },
 { "sub",                 pp_sub,                 3,  3, { ___,INT,INT } },
 { "tuple",               pp_tuple,               0, -1, { ___,___,___ } },
 { "%typecheck",          pp_typecheck,           0, -1, { ___,___,___ } },
 { "%unregister",         pp_unregisterb,         0, -1, { ___,___,___ } },
 { "vec",                 pp_vec,                 1,  1, { ___,___,___ } },
 { "~",                   pp_neg,                 1,  1, { REA,___,___ } },
 /* Functions not covered by TML */
 { "error",               pp_error,               2,  2, { STR,___,___ } },
 { "gensym",              pp_gensym,              1,  1, { ___,___,___ } },
 { "stats",               pp_stats,               1,  1, { ___,___,___ } },
#ifdef unix
 { "argv",                pp_argv,                1,  1, { INT,___,___ } },
 { "environ",             pp_environ,             1,  1, { STR,___,___ } },
 { "system",              pp_system,              1,  1, { STR,___,___ } },
#endif
 { NULL }
};

cell expected(cell who, char *what, cell got) {
	char	msg[100];
	PRIM	*p;

	p = &Primitives[cadr(who)];
	sprintf(msg, "%s: expected %s, got", p->name, what);
	return error(msg, got);
}

cell apply_primitive(cell x) {
	PRIM	*p;
	cell	args, a;
	int	k, na, i;

	p = &Primitives[cadar(x)];
	if (p->max_args < 2)
		a = args = cdr(x);
	else if (tuple_p(cadr(x)))
		a = args = cadr(x);
	else
		return too_few_args(x);
	if (p->max_args >= 0) {
		k = length(args);
		if (k < p->min_args)
			return too_few_args(x);
		if (k > p->max_args && p->max_args >= 0)
			return too_many_args(x);
	}
	else {
		/* All variadic functions accept 0..n arguments of any type */
		k = 0;
	}
	na = p->max_args < 0? p->min_args: p->max_args;
	if (na > k)
		na = k;
	for (i=1; i<=na; i++) {
		switch (p->arg_types[i-1]) {
		case T_NONE:
			break;
		case T_BOOLEAN:
			if (!boolean_p(car(a)))
				return expected(car(x), "bool", car(a));
			break;
		case T_CHAR:
			if (!char_p(car(a)))
				return expected(car(x), "char", car(a));
			break;
		case T_INSTREAM:
			if (!instream_p(car(a)))
				return expected(car(x), "instream", car(a));
			break;
		case T_INTEGER:
			if (!integer_p(car(a)))
				return expected(car(x), "int", car(a));
			break;
		case T_OUTSTREAM:
			if (!outstream_p(car(a)))
				return expected(car(x), "outstream", car(a));
			break;
		case T_TUPLE:
			if (atom_p(car(a)))
				return expected(car(x), "tuple", car(a));
			break;
		case T_UNIT:
			if (car(a) != UNIT)
				return expected(car(x), "unit", car(a));
			break;
		case T_LIST:
			if (!list_p(car(a)))
				return expected(car(x), "list", car(a));
			break;
		case T_FUNCTION:
			if (	!procedure_p(car(a)) &&
				!primitive_p(car(a)) &&
				!continuation_p(car(a))
			)
				return expected(car(x), "fn", car(a));
			break;
		case T_REAL:
			if (!integer_p(car(a)) && !real_p(car(a)))
				return expected(car(x), "num", car(a));
			break;
		case T_STREAM:
			if (!instream_p(car(a)) && !outstream_p(car(a)))
				return expected(car(x), "stream", car(a));
			break;
		case T_STRING:
			if (!string_p(car(a)))
				return expected(car(x), "str", car(a));
			break;
		case T_SYMBOL:
			if (!symbol_p(car(a)))
				return expected(car(x), "id", car(a));
			break;
		case T_VECTOR:
			if (!vector_p(car(a)))
				return expected(car(x), "vec", car(a));
			break;
		}
		a = cdr(a);
	}
	return (*p->handler)(args);
}

int uses_transformer_p(cell x) {
	cell	y;

	if (atom_p(x) || car(x) == S_quote)
		return 0;
	if (tuple_p(x) && symbol_p(car(x))) {
		y = lookup(car(x), Environment, 0);
		if (y != NIL && syntax_p(binding_value(y)))
			return 1;
	}
	while (tuple_p(x)) {
		if (uses_transformer_p(car(x)))
			return 1;
		x = cdr(x);
	}
	return 0;
}

cell xeval(cell x, int cbn);

cell expand_syntax_1(cell x) {
	cell	y, m, n, a, app;

	if (Error_flag || atom_p(x) || car(x) == S_quote)
		return x;
	if (symbol_p(car(x))) {
		y = lookup(car(x), Environment, 0);
		if (y != NIL && syntax_p(binding_value(y))) {
			save(x);
			app = node(cdr(binding_value(y)), cdr(x));
			unsave(1);
			return xeval(app, 1);
		}
	}
	/*
	 * If DEFINE-SYNTAX is followed by (MACRO-NAME ...)
	 * unbind the MACRO-NAME first to avoid erroneous
	 * expansion.
	 */
	if (	car(x) == S_define_syntax &&
		cdr(x) != NIL &&
		tuple_p(cadr(x))
	) {
		m = lookup(caadr(x), Environment, 0);
		if (m != NIL)
			binding_value(m) = UNDEFINED;
	}
	n = a = NIL;
	save(n);
	while (tuple_p(x)) {
		m = node(expand_syntax_1(car(x)), NIL);
		if (n == NIL) {
			n = m;
			car(Stack) = n;
			a = n;
		}
		else {
			cdr(a) = m;
			a = cdr(a);
		}
		x = cdr(x);
	}
	cdr(a) = x;
	unsave(1);
	return n;
}

cell expand_syntax(cell x) {
	if (Error_flag || atom_p(x) || car(x) == S_quote)
		return x;
	save(x);
	while (!Error_flag) {
		if (!uses_transformer_p(x))
			break;
		x = expand_syntax_1(x);
		car(Stack) = x;
	}
	unsave(1);
	return x;
}

cell restore_state(void) {
	cell	v;

	if (State_stack == NIL)
		fatal("restore_state: stack underflow");
	v = car(State_stack);
	State_stack = cdr(State_stack);
	return v;
}

cell destructuring_bind(cell v, cell a) {
	cell	s;
	cell	rib, new;

	s = NIL;
	save(s);
	rib = NIL;
	save(rib);
	while (1) {
		if (symbol_p(v) || eqv_p(v, a)) {
			if (symbol_p(v) && consname_p(v) && v != a) {
				unsave(2);
				return UNDEFINED;
			}
			else {
				if (	symbol_p(v) &&
					!consname_p(v) &&
					v != S_ignore
				) {
					Tmp = make_binding(v, a);
					rib = node(Tmp, rib);
					car(Stack) = rib;
				}
				if (cadr(Stack) == NIL)
					break;
				a = caadr(Stack);
				v = cadadr(Stack);
				cadr(Stack) = cddadr(Stack);
			}
		}
		else if (cons_p(v) && cons_p(a) && Tag[v] == Tag[a]) {
			set(cadr(Stack), node(cdr(v), cadr(Stack)));
			set(cadr(Stack), node(cdr(a), cadr(Stack)));
			v = car(v);
			a = car(a);
			continue;
		}
		else {
			unsave(2);
			return UNDEFINED;
		}
	}
	rib = car(Stack);
	unsave(2);
	Tmp = NIL;
	return rib;
}

cell next_case(cell app, cell cas) {
	cell	n, f;

	f = car(app);
	n = node(caddr(f), cas);
	n = node(cadr(f), n);
	n = node3(T_FUNCTION, n, ATOM_TAG);
	return node(n, cdr(app));
}

cell fnmatch(cell n, int *ps) {
	cell	c, cas, p, a, v, rib, g;
	cell	new;

	save(Environment);
	p = car(n);
	Environment = cadr(p);
	if (cdr(n) != NIL && cddr(n) == NIL)
		a = cadr(n);
	else
		a = cdr(n);
	c = rib = UNDEFINED;
	for (cas = cdddr(p); cas != NIL; cas = cdr(cas)) {
		c = car(cas);
		v = car(c);
		g = 0;
		if (tuple_p(v) && car(v) == GUARD) {
			v = cadr(v);
			g =  1;
		}
		if ((rib = destructuring_bind(v, a)) == UNDEFINED)
			continue;
		if (!g)
			break;
		Environment = make_env(rib, Environment);
		save(n);
		set(car(Stack), next_case(n, cas));
		*ps = EV_GUARD;
		return cddar(c);
	}
	*ps = EV_BETA;
	if (rib == UNDEFINED)
		return error("no match", n);
	Environment = make_env(rib, Environment);
	return cadr(c);
}

int tail_call(void) {
	if (State_stack == NIL || car(State_stack) != EV_BETA)
		return 0;
	Tmp = unsave(1);
	Environment = car(Stack);
	unsave(2);
	restore_state();
	save(Tmp);
	Tmp = NIL;
	return 1;
}

cell raise_exception(cell e) {
	cell	cs, n, p, fn;

	for (p = Exnstack; p != NIL; p = cdr(p)) {
		fn = cdar(p);
		for (cs = cddr(fn); cs != NIL; cs = cdr(cs)) {
			if (caar(cs) == e) {
				Exnstack = cdr(p);
				n = node(e, NIL);
				n = node(fn, n);
				n = node(n, NIL);
				n = node(caar(p), n);
				return n;
			}
		}
	}
	return error("unhandled exception", e);
}

cell xeval(cell x, int cbn) {
	cell	m2,	/* Root of result list */
		a,	/* Used to append to result */
		rib;	/* Temp storage for args */
	cell	n, new;
	int	s,	/* Current state */
		c;	/* Continue */
	cell	name;	/* Name of procedure to apply */

	save(x);
	save(State_stack);
	save(Stack_bottom);
	Stack_bottom = Stack;
	s = EV_ATOM;
	c = 0;
	while (!Error_flag) {
		if (Run_stats)
			count(&Reductions);
		if (symbol_p(x)) {		/* Symbol -> Value */
			if (cbn) {
				Acc = x;
				cbn = 0;
			}
			else {
				Acc = lookup(x, Environment, 1);
				if (Error_flag)
					break;
				Acc = box_value(Acc);
			}
		}
		else if (auto_quoting_p(x) || cbn == 2) {
			Acc = x;		/* Object -> Object */
			cbn = 0;
		}
		else {				/* (...) -> Value */
			/*
			 * This block is used to DESCEND into lists.
			 * The following structure is saved on the
			 * Stack: RIB = (args append result source)
			 * The current s is saved on the State_stack.
			 */
			Acc = x;
			x = car(x);
			save_state(s);
			/* Check call-by-name built-ins and flag */
			if (special_p(x) || cbn) {
				cbn = 0;
				rib = node(Acc, Acc);	/* result/source */
				rib = node(NIL, rib);	/* append */
				rib = node(NIL, rib);	/* args */
				x = NIL;
			}
			else {
				Tmp = node(NIL, NIL);
				rib = node(Tmp, Acc);	/* result/source */
				rib = node(Tmp, rib);	/* append */
				rib = node(cdr(Acc), rib); /* args */
				Tmp = NIL;
				x = car(Acc);
			}
			save(rib);
			s = EV_ARGS;
			continue;
		}
		/*
		 * The following loop is used to ASCEND back to the
		 * root of a list, thereby performing BETA REDUCTION.
		 */
		while (!Error_flag) {
		if (s == EV_BETA) {
			/* Finish BETA reduction */
			Environment = unsave(1);
			unsave(1);		/* source expression */
			s = restore_state();
		}
		else if (s == EV_ARGS) {	/* append to list, reduce */
			rib = car(Stack);
			x = rib_args(rib);
			a = rib_append(rib);
			m2 = rib_result(rib);
			if (a != NIL) 	/* Append new member */
				car(a) = Acc;
			if (x == NIL) {	/* End of list */
				Acc = m2;
				/* Remember name of caller */
				name = car(rib_source(rib));
				if (primitive_p(car(Acc))) {
					if (cadar(Acc) == Callcc_magic)
						c = cbn = 1;
					Cons_stats = 1;
					Acc = x = apply_primitive(Acc);
					Cons_stats = 0;
				}
				else if (special_p(car(Acc))) {
					Acc = x = apply_special(Acc, &c, &s);
				}
				else if (procedure_p(car(Acc))) {
					name = symbol_p(name)? name: NIL;
					Called_procedures[Proc_ptr] = name;
					Proc_ptr++;
					if (Proc_ptr >= Proc_max)
						Proc_ptr = 0;
					tail_call();
					x = fnmatch(Acc, &s);
					c = 2;
				}
				else if (continuation_p(car(Acc))) {
					Acc = resume(Acc);
				}
				else {
					error("application of non-procedure",
						name);
					x = NIL;
				}
				if (c != 2) {
					unsave(1); /* drop source expr */
					s = restore_state();
				}
				/* Leave the ASCENDING loop and descend */
				/* once more into X. */
				if (c)
					break;
			}
			else if (atom_p(x)) {
				error("syntax error", rib_source(rib));
				x = NIL;
				break;
			}
			else {		/* X =/= NIL: append to list */
				/* Create space for next argument */
				Acc = node(NIL, NIL);
				cdr(a) = Acc;
				rib_append(rib) = cdr(a);
				rib_args(rib) = cdr(x);
				x = car(x);	/* evaluate next member */
				break;
			}
		}
		else if (s == EV_GUARD) {
			if (Acc != FALSE) {
				x = cdddar(unsave(1));	/* cases */
				x = cadar(x);		/* current body */
			}
			else {
				x = car(Stack);		/* application */
				n = cddddr(car(x));	/* next case */
				x = next_case(x, n);
				unsave(1);
				Environment = cdr(Environment);
				cbn = 1;
			}
			s = EV_BETA;
			c = 1;
			break;
		}
		else if (s == EV_IF_PRED) {
			x = unsave(1);
			unsave(1);	/* source expression */
			s = restore_state();
			if (Acc != FALSE)
				x = cadr(x);
			else
				x = caddr(x);
			c = 1;
			break;
		}
		else if (s == EV_AND || s == EV_OR) {
			Stack = node(cdar(Stack), cdr(Stack));
			if (	(Acc == FALSE && s == EV_AND) ||
				(Acc != FALSE && s == EV_OR) ||
				car(Stack) == NIL
			) {
				unsave(2);	/* state, source expr */
				s = restore_state();
				x = Acc;
				cbn = 2;
			}
			else if (cdar(Stack) == NIL) {
				x = caar(Stack);
				unsave(2);	/* state, source expr */
				s = restore_state();
			}
			else {
				x = caar(Stack);
			}
			c = 1;
			break;
		}
		else if (s == EV_BEGIN) {
			Stack = node(cdar(Stack), cdr(Stack));
			if (cdar(Stack) == NIL) {
				x = caar(Stack);
				unsave(2);	/* state, source expr */
				s = restore_state();
			}
			else {
				x = caar(Stack);
			}
			c = 1;
			break;
		}
		else if (s == EV_SET_VAL || s == EV_MACRO) {
			char err[] = "define_syntax: expected fn, got";

			if (s == EV_MACRO) {
				if (procedure_p(Acc)) {
					Acc = new_atom(T_SYNTAX, Acc);
				}
				if (syntax_p(Acc)) {
					/* Acc = Acc; */
				}
				else {
					error(err, Acc);
					break;
				}
			}
			x = unsave(1);
			unsave(1);	/* source expression */
			s = restore_state();
			box_value(x) = Acc;
			Acc = x = UNIT;
			c = 0;
			break;
		}
		else if (s == EV_RAISE) {
			x = raise_exception(Acc);
			unsave(1);
			s = restore_state();
			c = 0;
			break;
		}
		else if (s == EV_INPUT || s == EV_OUTPUT) {
			char inerr[]  = ">>: expected instream, got";
			char outerr[] = "<<: expected outstream, got";

			if (	instream_p(car(Stack)) ||
				outstream_p(car(Stack))
			) {
				if (s == EV_INPUT) {
					Stream_flags[Instream] &= ~LOCK_TAG;
					Instream = stream_id(car(Stack));
				}
				else {
					Stream_flags[Outstream] &= ~LOCK_TAG;
					Outstream = stream_id(car(Stack));
				}
				unsave(2);
				s = restore_state();
			}
			else {
				x = car(Stack);
				if (s == EV_INPUT) {
					if (!instream_p(Acc))
						error(inerr, Acc);
					set(car(Stack), make_stream(Instream,
								T_INSTREAM));
					Instream = stream_id(Acc);
					Stream_flags[Instream] |= LOCK_TAG;
				}
				else {
					if (!outstream_p(Acc))
						error(outerr, Acc);
					set(car(Stack), make_stream(Outstream,
								T_OUTSTREAM));
					Outstream = stream_id(Acc);
					Stream_flags[Outstream] |= LOCK_TAG;
				}
				c = 1;
				break;
			}
		}
		else { /* s == EV_ATOM */
			break;
		}}
		if (c) {	/* Continue evaluation if requested */
			c = 0;
			continue;
		}
		if (Stack == Stack_bottom)
			break;
	}
	Stack = Stack_bottom;
	Stack_bottom = unsave(1);
	State_stack = unsave(1);
	unsave(1);
	return Acc;		/* Return the evaluated expr */
}

void reset_calltrace(void) {
	int	i;

	for (i=0; i<MAX_CALL_TRACE; i++)
		Called_procedures[i] = NIL;
}

cell eval(cell x) {
	reset_calltrace();
	save(x);
	x = expand_syntax(x);
	unsave(1);
	x = xeval(x, 0);
	return x;
}

/*
 * REPL
 */

void clear_leftover_envs(void) {
	while (cdr(Environment) != NIL)
		Environment = cdr(Environment);
}

#ifndef NO_SIGNALS
 void keyboard_interrupt(int sig) {
	Instream = 0;
	Outstream = 1;
	error("interrupted", NOEXPR);
	signal(SIGINT, keyboard_interrupt);
 }

 void keyboard_quit(int sig) {
	fatal("received quit signal, exiting");
 }

 void terminated(int sig) {
	bye(1);
 }
#endif

int isfirst(int t) {
	return	Token != TOK_VAL ||
		Token != TOK_FUN ||
		Token != TOK_TYPE ||
		Token != TOK_LOCAL ||
		Token != TOK_INFIX ||
		Token != TOK_INFIXR ||
		Token != TOK_NONFIX ||
	 	Token != END_OF_FILE ||
		Token != TOK_EXCEPTION;
}

void recover(void) {
	while (!isfirst(Token) && Token != TOK_SEMI && Token != UNDEFINED)
		Token = scan();
}

void repl(void) {
	cell	n = NIL; /*LINT*/
	cell	sane_env;

	sane_env = node(NIL, NIL);
	save(sane_env);
	if (!Quiet_mode) {
		signal(SIGINT, keyboard_interrupt);
	}
	Token = TOK_SEMI;
	while (Token != END_OF_FILE) {
		Error_flag = 0;
		Instream = 0;
		Outstream = 1;
		clear_leftover_envs();
		reset_calltrace();
		car(sane_env) = Environment;
		Exnstack = NIL;
		if (!Quiet_mode) {
			pr("> ");
			flush();
		}
		if (Token == TOK_SEMI || Token == UNDEFINED)
			Token = scan();
		Program = prog();
		if (Token != END_OF_FILE && Token != TOK_SEMI) {
			error("syntax error at", Attr);
		}
		if (!Error_flag)
			n = eval(Program);
		if (!Error_flag && n != UNDEFINED) {
			print_form(n);
			pr("\n");
			box_value(S_it) = n;
		}
		if (Error_flag) {
			Environment = car(sane_env);
			recover();
		}
	}
	unsave(1);
	pr("\n");
}

/*
 * Startup and Initialization
 */

void grow_primitives(void) {
	Max_prims += PRIM_SEG_SIZE;
	Primitives = (PRIM *) realloc(Primitives, sizeof(PRIM) * Max_prims);
	if (Primitives == NULL)
		fatal("grow_primitives: out of physical memory");
}

void add_primitives(char *name, PRIM *p) {
	cell	v, n, new;
	int	i;

	if (name) {
		n = symbol_ref(name);
		set(box_value(S_extensions), node(n, box_value(S_extensions)));
	}
	for (i=0; p && p && p[i].name; i++) {
		if (Callcc_magic < 0 && !strcmp(p[i].name, "call/cc"))
			Callcc_magic = Last_prim;
		v = symbol_ref(p[i].name);
		n = new_atom(Last_prim, NIL);
		n = new_atom(T_PRIMITIVE, n);
		if (Last_prim >= Max_prims)
			grow_primitives();
		memcpy(&Primitives[Last_prim], &p[i], sizeof(PRIM));
		Last_prim++;
		Environment = extend(v, n, Environment);
	}
}

/* Extension prototypes; add your own here. */
void curs_init(void);
void sys_init(void);

void make_initial_env(void) {
	cell	new;

	Environment = node(NIL, NIL);
	Environment = extend(symbol_ref("it"), NIL, Environment);
	S_it = cadr(Environment);
	Environment = extend(symbol_ref("*extensions*"), NIL, Environment);
	S_extensions = cadr(Environment);
	Environment = extend(symbol_ref("*loading*"), FALSE, Environment);
	S_loading = cadr(Environment);
	Callcc_magic = -1;
	Last_prim = 0;
	Max_prims = 0;
	grow_primitives();
	add_primitives(NULL, Core_primitives);
	Environment = node(Environment, NIL);
	Program = TRUE; /* or rehash() will not work */
	rehash(car(Environment));
}

void init(void) {
	int	i;

	for (i=2; i<MAX_STREAMS; i++)
		Streams[i] = NULL;
	Streams[0] = stdin;
	Streams[1] = stdout;
	Streams[2] = stderr;
	Stream_flags[0] = LOCK_TAG;
	Stream_flags[1] = LOCK_TAG;
	Stream_flags[2] = LOCK_TAG;
	Instream = 0;
	Outstream = 1;
	Errstream = 2;
	Cons_segment_size = INITIAL_SEGMENT_SIZE;
	Vec_segment_size = INITIAL_SEGMENT_SIZE;
	Cons_pool_size = 0,
	Vec_pool_size = 0;
	Car = NULL,
	Cdr = NULL;
	Tag = NULL;
	Free_list = NIL;
	Vectors = NULL;
	Free_vecs = 0;
	Memory_limit_kn = DEFAULT_LIMIT_KN * 1024L;
	Stack = NIL,
	Stack_bottom = NIL;
	State_stack = NIL;
	Tmp_car = NIL;
	Tmp_cdr = NIL;
	Tmp = NIL;
	Symbols = NIL;
	Program = NIL;
	Exnstack = NIL;
	Exceptions = NIL;
	Proc_ptr = 0;
	Proc_max = 3;
	Environment = NIL;
	Acc = NIL;
	Level = 0;
	Guard_lev = 0;
	Line_no = 1;
	Error_flag = 0;
	Load_level = 0;
	Displaying = 0;
	Printer_limit = 0;
	Quiet_mode = 0;
	Command_line = NULL;
	Run_stats = 0;
	new_cons_segment();
	new_vec_segment();
	gc();
	S_also = symbol_ref("also");
	S_begin = symbol_ref("begin");
	S_callcc = symbol_ref("call/cc");
	S_cons = symbol_ref("::");
	S_define = symbol_ref("define");
	S_define_syntax = symbol_ref("define_syntax");
	S_define_type = symbol_ref("define_type");
	S_else = symbol_ref("else");
	S_equal = symbol_ref("=");
	S_fn = symbol_ref("fn");
	S_from = symbol_ref(">>");
	S_greater = symbol_ref(">");
	S_if = symbol_ref("if");
	S_ignore = symbol_ref("_");
	S_less = symbol_ref("<");
	S_letrec = symbol_ref("letrec");
	S_list = symbol_ref("list");
	S_or = symbol_ref("or");
	S_quasiquote = symbol_ref("quasiquote");
	S_quote = symbol_ref("quote");
	S_raise = symbol_ref("%raise");
	S_ref = symbol_ref("ref");
	S_register = symbol_ref("%register");
	S_setb = symbol_ref("set!");
	S_to = symbol_ref("<<");
	S_tuple = symbol_ref("tuple");
	S_typecheck = symbol_ref("%typecheck");
	S_unquote = symbol_ref("unquote");
	S_unquote_splicing = symbol_ref("unquote-splicing");
	S_unregister = symbol_ref("%unregister");
	Tmp = symbol_ref("x");
	S_primlist = node(S_cons, NIL);
	S_primlist = node(Tmp, S_primlist);
	Tmp = NIL;
	S_primlist = node(S_cons, S_primlist);
	S_primlist = node(S_primlist, NIL);
	S_primlist = node(S_quote, S_primlist);
	init_ops();
	make_initial_env();
	reset_calltrace();
}

void init_extensions(void) {
	cell	e, n;
	char	initproc[TOKEN_LENGTH+2];
	char	*s;
	char	*mlite = "mlite";

	e = box_value(S_extensions);
	while (mlite || e != NIL) {
		s = mlite? mlite: string(car(e));
		if (strlen(s)*2+1 >= TOKEN_LENGTH)
			fatal("init_extension(): procedure name too long");
		sprintf(initproc, "%s:%s", s, s);
		n = find_symbol(initproc);
		if (n != NIL) {
			n = node(n, NIL);
			eval(n);
		}
		e = mlite? e: cdr(e);
		mlite = NULL;
	}
}

void usage(int quit) {
	pr("Usage: mlite [-h?] [-i name] [-gqv] [-f prog [args]] ");
	pr("[-m size[m]]");
	nl();
	pr("             [-l prog] [-t count] [-- [args]]");
	nl();
	if (quit) bye(1);
}

void long_usage() {
	nl();
	usage(0);
	nl();
	pr("-h              display this summary (also -?)"); nl();
	pr("-i name         base name of image file (must be first option!)");
	nl();
	pr("-i -            ignore image, load mlite.m instead"); nl();
	pr("-f file [args]  run program and exit (implies -q)"); nl();
	pr("-g              print GC summaries (-gg = more)"); nl();
	pr("-l file         load program (may be repeated)"); nl();
	pr("-m n[m]         set memory limit to nK (or nM) nodes"); nl();
	pr("-q              be quiet (no banner, no prompt, exit on errors)");
	nl();
	pr("-t n            list up to N procedures in call traces"); nl();
	pr("-v              print version and exit"); nl();
	pr("-- [args]       pass subsequent arguments to program"); nl();
	nl();
}

void version_info() {
	char	buf[100];
	cell	x;

	nl();
	pr("mLite Language Interpreter by Nils M Holm");
	nl();
	nl();
	pr("version:         "); pr(VERSION);
#ifdef unix
	pr(" (unix)");
#else
 #ifdef plan9
	pr(" (plan 9)");
 #else
	pr(" (unknown)");
 #endif
#endif
	nl();
	pr("memory limit:    ");
	if (Memory_limit_kn) {
		sprintf(buf, "%ld", Memory_limit_kn / 1024);
		pr(buf); pr("K nodes"); nl();
	}
	else {
		pr("none"); nl();
	}
	pr("extensions:      ");
	if (box_value(S_extensions) == NIL)
		pr("-");
	for (x = box_value(S_extensions); x != NIL; x = cdr(x)) {
		print_form(car(x));
		if (cdr(x) != NIL)
			pr(" ");
	}
	nl();
	pr("mantissa size:   ");
	sprintf(buf, "%d", MANTISSA_SIZE);
	pr(buf); pr(" digits"); nl();
	nl();
	pr("This program is in the public domain.");
	nl();
	nl();
}

long get_size_k(char *s) {
	int	c;
	long	n;

	c = s[strlen(s)-1];
	n = atol(s);
	if (isdigit(c))
		;
	else if (c == 'M' || c == 'm')
		n *= 1024L;
	else
		usage(1);
	return n * 1024;
}

int main(int argc, char **argv) {
	int	run_script;

	if (argc > 1 && !strcmp(argv[1], "-i")) {
		if (argc < 2) {
			usage(1);
			bye(1);
		}
		argv += 2;
	}
	init();
	signal(SIGQUIT, keyboard_quit);
	signal(SIGTERM, terminated);
	if (load(argv[0]) != 0) fatal("Unable to load library file");
	argv++;
	init_extensions();
	while (*argv != NULL) {
		if (**argv != '-')
			break;
		(*argv)++;
		while (**argv) {
			switch (**argv)  {
			case '-':
				Command_line = ++argv;
				break;
			case 'f':
			case 'l':
				if (argv[1] == NULL)
					usage(1);
				run_script = **argv == 'f';
				if (run_script) {
					Quiet_mode = 1;
					Command_line = &argv[2];
				}
				if (load(argv[1]))
					error("program file not found",
						make_string(argv[1],
							(int)strlen(argv[1])));
				if (Error_flag)
					bye(1);
				if (run_script)
					bye(0);
				argv++;
				*argv = &(*argv)[strlen(*argv)];
				break;
			case 'g':
				Verbose_GC++;
				(*argv)++;
				break;
			case 'm':
				if (argv[1] == NULL)
					usage(1);
				Memory_limit_kn = get_size_k(argv[1]);
				argv++;
				*argv += strlen(*argv);
				break;
			case 'q':
				Quiet_mode = 1;
				(*argv)++;
				break;
			case 't':
				if (argv[1] == NULL)
					usage(1);
				Proc_max = atoi(argv[1]);
				if (Proc_max > MAX_CALL_TRACE)
					Proc_max = MAX_CALL_TRACE;
				argv++;
				*argv += strlen(*argv);
				break;
			case 'v':
				version_info();
				bye(0);
				break;
			case 'h':
			case '?':
				long_usage();
				bye(0);
				break;
			default:
				usage(1);
				break;
			}
			if (Command_line)
				break;
		}
		if (Command_line)
			break;
		argv++;
	}
	if (!Command_line && argv[0] != NULL)
		usage(1);
	if (!Quiet_mode)
		pr("mLite\n");
	repl();
	return 0;
}
