#!/bin/cc -run
/*
 * Klong interpreter
 * Nils M Holm, 2015--2022
 * In the public domain
 *
 * Under jurisdictions without a public domain, the CC0 applies.
 * See the file CC0 for a copy of the license.
 */

volatile int	Abort_flag;

/*
 * S9core Toolkit, Mk IVd
 * By Nils M Holm, 2007-2019
 * In the public domain
 *
 * Under jurisdictions without a public domain, the CC0 applies.
 * See the file CC0 for a copy of the license.
 */

#define S9_VERSION "20190402"

/*
 * Ugly prelude to deal with some system-dependent stuff.
 */

#ifdef __NetBSD__
 #ifndef unix
  #define unix
 #endif
#endif

#ifdef __unix
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

#ifdef __clang__
 #ifndef unix
  #define unix
 #endif
#endif

#ifndef unix
 #ifndef plan9
  #error "Either 'unix' or 'plan9' must be #defined."
 #endif
#endif

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

#ifdef plan9
 #include <u.h>
 #include <libc.h>
 #include <stdio.h>
 #include <ctype.h>
 #include <ape/limits.h>
 #define size_t	uvlong
 #define bye(x)	exits((x)? "error": NULL)
 #define ptrdiff_t vlong
#endif

#ifdef unix
 #include <stdlib.h>
 #include <stddef.h>
 #include <stdio.h>
 #include <string.h>
 #include <ctype.h>
 #include <limits.h>
 #define bye(x)	exit((x)? EXIT_FAILURE: EXIT_SUCCESS)
#endif

/*
 * Tunable parameters
 */

/* Default memory limits in K nodes and K cells; 0 = none */
#define S9_NODE_LIMIT	14013
#define S9_VECTOR_LIMIT	14013

/* Initial memory pool sizes */
#define S9_INITIAL_SEGMENT_SIZE	32768

/* Primitive segment size (slots) */
#define S9_PRIM_SEG_SIZE	256

/* Maximum number of open I/O ports */
#define S9_MAX_PORTS		32

/* Use 64-bit cells (don't do this!) */
/* #define S9_BITS_PER_WORD_64 */

/*
 * Non-tunable parameters
 */

/* A cell must be large enough to hold an integer segment;
 * see S9_INT_SEG_LIMIT, below
 */

#ifdef S9_BITS_PER_WORD_64
 #define s9_cell	ptrdiff_t
#else
 #define s9_cell	int
#endif

#ifdef S9_BITS_PER_WORD_64
 #define S9_DIGITS_PER_CELL	18
 #define S9_INT_SEG_LIMIT	1000000000000000000LL
 #define S9_MANTISSA_SEGMENTS	1
#else
 #define S9_DIGITS_PER_CELL	9
 #define S9_INT_SEG_LIMIT	1000000000L
 #define S9_MANTISSA_SEGMENTS	2
#endif

#define S9_MANTISSA_SIZE	(S9_MANTISSA_SEGMENTS * S9_DIGITS_PER_CELL)

/*
 * Node tags
 */

#define S9_ATOM_TAG	0x01	/* Atom, car = type, cdr = next */
#define S9_MARK_TAG	0x02	/* Mark */
#define S9_STATE_TAG	0x04	/* State */
#define S9_VECTOR_TAG	0x08	/* Vector, car = type, cdr = content */
#define S9_PORT_TAG	0x10	/* Atom is an I/O port (with ATOM_TAG) */
#define S9_USED_TAG	0x20	/* Port: in use */
#define S9_LOCK_TAG	0x40	/* Port: locked (do not finalize) */
#define S9_CONST_TAG	0x80	/* Node is immutable */

/*
 * Special objects
 */

#define s9_special_p(x)	((x) < 0)
#define S9_NIL		(-1)
#define S9_TRUE		(-2)
#define S9_FALSE	(-3)
#define S9_END_OF_FILE	(-4)
#define S9_UNDEFINED	(-5)
#define S9_UNSPECIFIC	(-6)
#define S9_VOID		(-7)

/*
 * Type tags
 */

#define S9_T_ANY		(-10)
#define S9_T_BOOLEAN		(-11)
#define S9_T_CHAR		(-12)
#define S9_T_INPUT_PORT		(-13)
#define S9_T_INTEGER		(-14)
#define S9_T_LIST		(-17)
#define S9_T_OUTPUT_PORT	(-15)
#define S9_T_PAIR		(-16)
#define S9_T_PRIMITIVE		(-18)
#define S9_T_FUNCTION		(-19)
#define S9_T_REAL		(-20)
#define S9_T_STRING		(-21)
#define S9_T_SYMBOL		(-22)
#define S9_T_SYNTAX		(-23)
#define S9_T_VECTOR		(-24)
#define S9_T_CONTINUATION	(-25)
#define S9_T_FIXNUM		(-26)
#define S9_T_NONE		(-99)

#define S9_USER_SPECIALS	(-100)

/*
 * Structures
 */

struct S9_counter {
	int	n, n1k, n1m, n1g, n1t;
};

#define s9_counter	struct S9_counter

struct S9_primitive {
	char	*name;
	s9_cell	(*handler)(void);
	int	min_args;
	int	max_args;	/* -1 = variadic */
	int	arg_types[3];
};

#define S9_PRIM    struct S9_primitive

/*
 * I/O
 */

#define s9_nl()		s9_prints("\n")

/*
 * Nested lists
 */

#define s9_car(x)          (S9_car[x])
#define s9_cdr(x)          (S9_cdr[x])
#define s9_caar(x)         (S9_car[S9_car[x]])
#define s9_cadr(x)         (S9_car[S9_cdr[x]])
#define s9_cdar(x)         (S9_cdr[S9_car[x]])
#define s9_cddr(x)         (S9_cdr[S9_cdr[x]])
#define s9_caaar(x)        (S9_car[S9_car[S9_car[x]]])
#define s9_caadr(x)        (S9_car[S9_car[S9_cdr[x]]])
#define s9_cadar(x)        (S9_car[S9_cdr[S9_car[x]]])
#define s9_caddr(x)        (S9_car[S9_cdr[S9_cdr[x]]])
#define s9_cdaar(x)        (S9_cdr[S9_car[S9_car[x]]])
#define s9_cdadr(x)        (S9_cdr[S9_car[S9_cdr[x]]])
#define s9_cddar(x)        (S9_cdr[S9_cdr[S9_car[x]]])
#define s9_cdddr(x)        (S9_cdr[S9_cdr[S9_cdr[x]]])
#define s9_caaaar(x)       (S9_car[S9_car[S9_car[S9_car[x]]]])
#define s9_caaadr(x)       (S9_car[S9_car[S9_car[S9_cdr[x]]]])
#define s9_caadar(x)       (S9_car[S9_car[S9_cdr[S9_car[x]]]])
#define s9_caaddr(x)       (S9_car[S9_car[S9_cdr[S9_cdr[x]]]])
#define s9_cadaar(x)       (S9_car[S9_cdr[S9_car[S9_car[x]]]])
#define s9_cadadr(x)       (S9_car[S9_cdr[S9_car[S9_cdr[x]]]])
#define s9_caddar(x)       (S9_car[S9_cdr[S9_cdr[S9_car[x]]]])
#define s9_cadddr(x)       (S9_car[S9_cdr[S9_cdr[S9_cdr[x]]]])
#define s9_cdaaar(x)       (S9_cdr[S9_car[S9_car[S9_car[x]]]])
#define s9_cdaadr(x)       (S9_cdr[S9_car[S9_car[S9_cdr[x]]]])
#define s9_cdadar(x)       (S9_cdr[S9_car[S9_cdr[S9_car[x]]]])
#define s9_cdaddr(x)       (S9_cdr[S9_car[S9_cdr[S9_cdr[x]]]])
#define s9_cddaar(x)       (S9_cdr[S9_cdr[S9_car[S9_car[x]]]])
#define s9_cddadr(x)       (S9_cdr[S9_cdr[S9_car[S9_cdr[x]]]])
#define s9_cdddar(x)       (S9_cdr[S9_cdr[S9_cdr[S9_car[x]]]])
#define s9_cddddr(x)       (S9_cdr[S9_cdr[S9_cdr[S9_cdr[x]]]])

/*
 * Access to fields of atoms
 */

#define s9_tag(n)		(S9_tag[n])

#define s9_string(n)		((char *) &Vectors[S9_cdr[n]])
#define s9_string_len(n)	(Vectors[S9_cdr[n] - 1])
#define s9_symbol_name(n)	(string(n))
#define s9_symbol_len(n)	(string_len(n))
#define s9_vector(n)		(&Vectors[S9_cdr[n]])
#define s9_vector_link(n)	(Vectors[S9_cdr[n] - 3])
#define s9_vector_index(n)	(Vectors[S9_cdr[n] - 2])
#define s9_vector_size(k)	(((k)+sizeof(s9_cell)-1) / \
                                 sizeof(s9_cell) + 3)
#define s9_vector_len(n)	(vector_size(string_len(n)) - 3)

#define s9_fixval(x)		cadr(x)
#define s9_small_int_value(x)	cadr(x)
#define s9_port_no(n)		(cadr(n))
#define s9_char_value(n)	(cadr(n))
#define s9_prim_slot(n)		(cadr(n))
#define s9_prim_info(n)		(&Primitives[prim_slot(n)])

/*
 * Type predicates
 */

#define s9_eof_p(n)		((n) == S9_END_OF_FILE)
#define s9_undefined_p(n)	((n) == S9_UNDEFINED)
#define s9_unspecific_p(n)	((n) == S9_UNSPECIFIC)

#define s9_boolean_p(n)	\
	((n) == S9_TRUE || (n) == S9_FALSE)

#define s9_integer_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && car(n) == S9_T_INTEGER)

#define s9_real_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && car(n) == S9_T_REAL)

#define s9_fix_p(n) \
        (!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && T_FIXNUM == car(n))

#define s9_number_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && \
	 (car(n) == S9_T_REAL || car(n) == S9_T_INTEGER))

#define s9_primitive_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && \
	 car(n) == S9_T_PRIMITIVE)

#define s9_function_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && \
	 car(n) == S9_T_FUNCTION)

#define s9_continuation_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && \
	 car(n) == S9_T_CONTINUATION)

#define s9_char_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && car(n) == S9_T_CHAR)

#define s9_syntax_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && car(n) == S9_T_SYNTAX)

#define s9_input_port_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && \
	 (tag(n) & S9_PORT_TAG) && car(n) == S9_T_INPUT_PORT)

#define s9_output_port_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_ATOM_TAG) && \
	 (tag(n) & S9_PORT_TAG) && car(n) == S9_T_OUTPUT_PORT)

#define s9_symbol_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_VECTOR_TAG) && car(n) == S9_T_SYMBOL)

#define s9_vector_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_VECTOR_TAG) && car(n) == S9_T_VECTOR)

#define s9_string_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_VECTOR_TAG) && car(n) == S9_T_STRING)

#define s9_constant_p(n) \
	(!s9_special_p(n) && (tag(n) & S9_CONST_TAG))

#define s9_atom_p(n) \
	(s9_special_p(n) || (tag(n) & S9_ATOM_TAG) || (tag(n) & S9_VECTOR_TAG))

#define s9_pair_p(x) (!s9_atom_p(x))

#define s9_small_int_p(n) (NIL == cddr(n))

#define s9_type_tag(n) \
	(S9_TRUE == (n)? S9_T_BOOLEAN: \
	 S9_FALSE == (n)? S9_T_BOOLEAN: \
	 (!s9_special_p(n) && (tag(n) & (S9_ATOM_TAG|S9_VECTOR_TAG))? car(n): \
	 S9_T_NONE))

/*
 * Allocators
 */

#define s9_cons(pa, pd)		s9_cons3((pa), (pd), 0)
#define s9_new_atom(pa, pd)	s9_cons3((pa), (pd), S9_ATOM_TAG)
#define s9_save(n)		(Stack = s9_cons((n), Stack))

/*
 * Bignum arithmetics
 */

#define s9_bignum_negative_p(a)	((cadr(a)) < 0)
#define s9_bignum_zero_p(a)	((cadr(a)) == 0)
#define s9_bignum_positive_p(a)	((cadr(a)) > 0)

/*
 * Real number structure
 */

#define S9_real_flags(x)	(cadr(x))
#define S9_real_exponent(x)	(caddr(x))
#define S9_real_mantissa(x)	(cdddr(x))

#define S9_REAL_NEGATIVE   0x01

#define S9_real_negative_flag(x)	(S9_real_flags(x) & S9_REAL_NEGATIVE)

/*
 * Real-number arithmetics
 */

#define S9_real_zero_p(x) \
	(car(S9_real_mantissa(x)) == 0 && cdr(S9_real_mantissa(x)) == S9_NIL)

#define S9_real_negative_p(x) \
	(S9_real_negative_flag(x) && !S9_real_zero_p(x))

#define S9_real_positive_p(x) \
	(!S9_real_negative_flag(x) && !S9_real_zero_p(x))

#define S9_real_negate(a) \
	S9_make_quick_real(S9_real_flags(a) & S9_REAL_NEGATIVE?	\
			S9_real_flags(a) & ~S9_REAL_NEGATIVE: \
			S9_real_flags(a) |  S9_REAL_NEGATIVE, \
			S9_real_exponent(a), S9_real_mantissa(a))

/*
 * Globals
 */

extern s9_cell	*S9_car,
		*S9_cdr;
extern char	*S9_tag;

extern s9_cell	*S9_vectors;

extern s9_cell	S9_stack;

extern s9_cell	*S9_gc_stack;
extern int	*S9_gc_stkptr;

extern S9_PRIM	*S9_primitives;

extern s9_cell	S9_zero,
		S9_one,
		S9_two,
		S9_ten;

extern s9_cell	S9_epsilon;

extern FILE	*S9_ports[];

extern int	S9_input_port,
		S9_output_port,
		S9_error_port;

/*
 * Prototypes
 */

#ifdef plan9
int	system(char *s);
#endif

void	s9_abort(void);
int	s9_aborted(void);
void	s9_add_image_vars(s9_cell **v);
s9_cell	s9_apply_prim(s9_cell f);
s9_cell	s9_argv_to_list(char **argv);
long	s9_asctol(char *s);
s9_cell	s9_bignum_abs(s9_cell a);
s9_cell	s9_bignum_add(s9_cell a, s9_cell b);
s9_cell	s9_bignum_divide(s9_cell a, s9_cell b);
int	s9_bignum_equal_p(s9_cell a, s9_cell b);
int	s9_bignum_even_p(s9_cell a);
int	s9_bignum_less_p(s9_cell a, s9_cell b);
s9_cell	s9_bignum_multiply(s9_cell a, s9_cell b);
s9_cell	s9_bignum_negate(s9_cell a);
s9_cell	s9_bignum_shift_left(s9_cell a, int fill);
s9_cell	s9_bignum_shift_right(s9_cell a);
s9_cell	s9_bignum_subtract(s9_cell a, s9_cell b);
s9_cell	s9_bignum_to_int(s9_cell x, int *of);
s9_cell	s9_bignum_to_real(s9_cell a);
s9_cell	s9_bignum_to_string(s9_cell x);
int	s9_blockread(char *s, int k);
void	s9_blockwrite(char *s, int k);
void	s9_close_port(int port);
void	s9_close_input_string(void);
s9_cell	s9_cons3(s9_cell pcar, s9_cell pcdr, int ptag);
int	s9_conses(s9_cell a);
void	s9_cons_stats(int x);
s9_cell	s9_copy_string(s9_cell x);
void	s9_count(s9_counter *c);
void	s9_countn(s9_counter *c, int n);
char	*s9_dump_image(char *path, char *magic);
int	s9_error_port(void);
void	s9_exponent_chars(char *s);
void	s9_fatal(char *msg);
s9_cell	s9_find_symbol(char *s);
s9_cell	s9_flat_copy(s9_cell n, s9_cell *lastp);
void	s9_flush(void);
int	s9_gc(void);
int	s9_gcv(void);
void	s9_gc_verbosity(int n);
void	s9_get_counters(s9_counter **nc, s9_counter **cc, s9_counter **vc,
			s9_counter **gc);
void	s9_mem_error_handler(void (*h)(int src));
void	s9_image_vars(s9_cell **v);
int	s9_input_port(void);
int	s9_inport_open_p(void);
int	s9_integer_string_p(char *s);
s9_cell	s9_intern_symbol(s9_cell y);
s9_cell	s9_int_to_bignum(int v);
int	s9_io_status(void);
void	s9_io_reset(void);
int	s9_length(s9_cell n);
char	*s9_load_image(char *path, char *magic);
int	s9_lock_port(int port);
s9_cell	s9_make_char(int c);
s9_cell	s9_make_integer(s9_cell i);
s9_cell	s9_make_norm_real(int flags, s9_cell exp, s9_cell mant);
s9_cell	s9_make_port(int portno, s9_cell type);
s9_cell	s9_make_primitive(S9_PRIM *p);
s9_cell	S9_make_real(int flags, s9_cell exp, s9_cell mant);
s9_cell	s9_make_real(int sign, s9_cell exp, s9_cell mant);
s9_cell	s9_make_string(char *s, int k);
s9_cell	s9_make_symbol(char *s, int k);
s9_cell	s9_make_vector(int k);
s9_cell	s9_mkfix(int i);
int	s9_new_port(void);
s9_cell	s9_new_vec(s9_cell type, int size);
int	s9_open_input_port(char *path);
char	*s9_open_input_string(char *s);
int	s9_open_output_port(char *path, int append);
int	s9_output_port(void);
int	s9_outport_open_p(void);
int	s9_port_eof(int p);
void	s9_prints(char *s);
int	s9_printer_limit(void);
void	s9_print_bignum(s9_cell n);
void	s9_print_expanded_real(s9_cell n);
void	s9_print_real(s9_cell n);
void	s9_print_sci_real(s9_cell n);
int	s9_readc(void);
s9_cell	s9_read_counter(s9_counter *c);
s9_cell	s9_real_abs(s9_cell a);
s9_cell	s9_real_add(s9_cell a, s9_cell b);
int	s9_real_approx_p(s9_cell a, s9_cell b);
s9_cell	s9_real_ceil(s9_cell x);
s9_cell	s9_real_divide(s9_cell a, s9_cell b);
int	s9_real_equal_p(s9_cell a, s9_cell b);
s9_cell	s9_real_exponent(s9_cell x);
s9_cell	s9_real_floor(s9_cell x);
s9_cell	s9_real_integer_p(s9_cell x);
int	s9_real_less_p(s9_cell a, s9_cell b);
s9_cell	s9_real_mantissa(s9_cell x);
s9_cell	s9_real_multiply(s9_cell a, s9_cell b);
s9_cell	s9_real_negate(s9_cell a);
s9_cell	s9_real_negative_p(s9_cell a);
s9_cell	s9_real_positive_p(s9_cell a);
s9_cell	s9_real_power(s9_cell x, s9_cell y);
s9_cell	s9_real_round(s9_cell x);
s9_cell	s9_real_sqrt(s9_cell x);
s9_cell	s9_real_subtract(s9_cell a, s9_cell b);
s9_cell	s9_real_to_bignum(s9_cell r);
s9_cell	s9_real_to_string(s9_cell r, int mode);
s9_cell	s9_real_trunc(s9_cell x);
s9_cell	s9_real_zero_p(s9_cell a);
void	s9_rejectc(int c);
void	s9_reset(void);
void	s9_reset_counter(s9_counter *c);
void	s9_reset_std_ports(void);
void	s9_run_stats(int x);
void	s9_fini(void);
void	s9_init(s9_cell **extroots, s9_cell *stack, int *stkptr);
s9_cell	s9_set_input_port(s9_cell port);
void	s9_set_node_limit(int k);
s9_cell	s9_set_output_port(s9_cell port);
void	s9_set_printer_limit(int k);
void	s9_set_vector_limit(int k);
int	s9_string_numeric_p(char *s);
s9_cell	s9_string_to_bignum(char *s);
s9_cell	s9_string_to_number(char *s);
s9_cell	s9_string_to_real(char *s);
s9_cell	s9_string_to_symbol(s9_cell x);
s9_cell	s9_symbol_ref(char *s);
s9_cell	s9_symbol_table(void);
s9_cell	s9_symbol_to_string(s9_cell x);
char	*s9_typecheck(s9_cell f);
int	s9_unlock_port(int port);
s9_cell	s9_unsave(int k);
void	s9_writec(int c);

/*
 * S9core Toolkit, Mk IVd
 * By Nils M Holm, 2007-2019
 * In the public domain
 *
 * Under jurisdictions without a public domain, the CC0 applies.
 * See the file CC0 for a copy of the license.
 */

/*
 * Remove S9_ and s9_ prefixes from common definitions
 */

#define cell	s9_cell
#define counter	s9_counter

#define special_p	s9_special_p

#define NIL		S9_NIL
#define TRUE		S9_TRUE
#define FALSE		S9_FALSE
#define END_OF_FILE	S9_END_OF_FILE
#define UNDEFINED	S9_UNDEFINED
#define UNSPECIFIC	S9_UNSPECIFIC
#define VOID		S9_VOID

#define T_ANY		S9_T_ANY
#define T_BOOLEAN	S9_T_BOOLEAN
#define T_CHAR		S9_T_CHAR
#define T_INPUT_PORT	S9_T_INPUT_PORT
#define T_INTEGER	S9_T_INTEGER
#define T_LIST		S9_T_LIST
#define T_OUTPUT_PORT	S9_T_OUTPUT_PORT
#define T_PAIR		S9_T_PAIR
#define T_PRIMITIVE	S9_T_PRIMITIVE
#define T_FUNCTION	S9_T_FUNCTION
#define T_REAL		S9_T_REAL
#define T_STRING	S9_T_STRING
#define T_SYMBOL	S9_T_SYMBOL
#define T_SYNTAX	S9_T_SYNTAX
#define T_VECTOR	S9_T_VECTOR
#define T_CONTINUATION	S9_T_CONTINUATION
#define T_FIXNUM	S9_T_FIXNUM
#define T_NONE		S9_T_NONE

#define USER_SPECIALS	S9_USER_SPECIALS

#define nl	s9_nl

#define string		s9_string
#define string_len	s9_string_len
#define symbol_name	s9_symbol_name
#define symbol_len	s9_symbol_len
#define vector		s9_vector
#define vector_link	s9_vector_link
#define vector_index	s9_vector_index
#define vector_size	s9_vector_size
#define vector_len	s9_vector_len
#define port_no		s9_port_no
#define fixval		s9_fixval
#define small_int_value	s9_small_int_value
#define char_value	s9_char_value
#define prim_slot	s9_prim_slot
#define prim_info	s9_prim_info

#define tag	s9_tag

#define car	s9_car
#define cdr	s9_cdr
#define caar	s9_caar
#define cadr	s9_cadr
#define cdar	s9_cdar
#define cddr	s9_cddr
#define caaar	s9_caaar
#define caadr	s9_caadr
#define cadar	s9_cadar
#define caddr	s9_caddr
#define cdaar	s9_cdaar
#define cdadr	s9_cdadr
#define cddar	s9_cddar
#define cdddr	s9_cdddr
#define caaaar	s9_caaaar
#define caaadr	s9_caaadr
#define caadar	s9_caadar
#define caaddr	s9_caaddr
#define cadaar	s9_cadaar
#define cadadr	s9_cadadr
#define caddar	s9_caddar
#define cadddr	s9_cadddr
#define cdaaar	s9_cdaaar
#define cdaadr	s9_cdaadr
#define cdadar	s9_cdadar
#define cdaddr	s9_cdaddr
#define cddaar	s9_cddaar
#define cddadr	s9_cddadr
#define cdddar	s9_cdddar
#define cddddr	s9_cddddr

#define Car		S9_car
#define Cdr		S9_cdr
#define Tag		S9_tag
#define Vectors		S9_vectors
#define Nullvec		S9_nullvec
#define Stack		S9_stack
#define Primitives	S9_primitives
#define Zero		S9_zero
#define One		S9_one
#define Two		S9_two
#define Ten		S9_ten
#define Epsilon		S9_epsilon
#define Ports		S9_ports
#define Input_port	S9_input_port
#define Output_port	S9_output_port
#define Error_port	S9_error_port

#define eof_p		s9_eof_p
#define undefined_p	s9_undefined_p
#define unspecific_p	s9_unspecific_p
#define boolean_p	s9_boolean_p
#define constant_p	s9_constant_p
#define integer_p	s9_integer_p
#define number_p	s9_number_p
#define primitive_p	s9_primitive_p
#define function_p	s9_function_p
#define continuation_p	s9_continuation_p
#define real_p		s9_real_p
#define fix_p		s9_fix_p
#define char_p		s9_char_p
#define syntax_p	s9_syntax_p
#define input_port_p	s9_input_port_p
#define output_port_p	s9_output_port_p
#define symbol_p	s9_symbol_p
#define vector_p	s9_vector_p
#define string_p	s9_string_p
#define atom_p		s9_atom_p
#define pair_p		s9_pair_p
#define small_int_p	s9_small_int_p
#define type_tag	s9_type_tag

#define cons		s9_cons
#define new_atom	s9_new_atom
#define save		s9_save

#define bignum_negative_p	s9_bignum_negative_p
#define bignum_zero_p		s9_bignum_zero_p
#define bignum_positive_p	s9_bignum_positive_p

#define Make_real		S9_make_real
#define Real_flags		S9_real_flags
#define Real_exponent		S9_real_exponent
#define Real_mantissa		S9_real_mantissa
#define REAL_NEGATIVE		S9_REAL_NEGATIVE
#define Real_negative_flag	S9_real_negative_flag
#define Real_zero_p		S9_real_zero_p
#define Real_negative_p		S9_real_negative_p
#define Real_positive_p		S9_real_positive_p
#define Real_negate		S9_real_negate

#define GC_stack	S9_gc_stack
#define GC_stkptr	S9_gc_stkptr

#ifndef S9_S9CORE
 #define apply_prim		s9_apply_prim
 #define argv_to_list		s9_argv_to_list
 #define asctol			s9_asctol
 #define bignum_abs		s9_bignum_abs
 #define bignum_add		s9_bignum_add
 #define bignum_divide		s9_bignum_divide
 #define bignum_equal_p		s9_bignum_equal_p
 #define bignum_even_p		s9_bignum_even_p
 #define bignum_less_p		s9_bignum_less_p
 #define bignum_multiply	s9_bignum_multiply
 #define bignum_negate		s9_bignum_negate
 #define bignum_shift_left	s9_bignum_shift_left
 #define bignum_shift_right	s9_bignum_shift_right
 #define bignum_subtract	s9_bignum_subtract
 #define bignum_to_int		s9_bignum_to_int
 #define bignum_to_real		s9_bignum_to_real
 #define bignum_to_string	s9_bignum_to_string
 #define blockread		s9_blockread
 #define blockwrite		s9_blockwrite
 #define close_input_string	s9_close_input_string
 #define close_port		s9_close_port
 #define cons3			s9_cons3
 #define conses			s9_conses
 #define cons_stats		s9_cons_stats
 #define copy_string		s9_copy_string
 #define count			s9_count
 #define dump_image		s9_dump_image
 #define error_port		s9_error_port
 #define exponent_chars		s9_exponent_chars
 #define fatal			s9_fatal
 #define find_symbol		s9_find_symbol
 #define flat_copy		s9_flat_copy
 #define flush			s9_flush
 #define gc			s9_gc
 #define gc_verbosity		s9_gc_verbosity
 #define gcv			s9_gcv
 #define get_counters		s9_get_counters
//  #define image_vars		s9_image_vars
 #define input_port		s9_input_port
 #define inport_open_p		s9_inport_open_p
 #define integer_string_p	s9_integer_string_p
 #define intern_symbol		s9_intern_symbol
 #define int_to_bignum		s9_int_to_bignum
 #define io_reset		s9_io_reset
 #define io_status		s9_io_status
 #define length			s9_length
 #define load_image		s9_load_image
 #define lock_port		s9_lock_port
 #define make_char		s9_make_char
 #define make_integer		s9_make_integer
 #define make_port		s9_make_port
 #define make_primitive		s9_make_primitive
 #define make_real		s9_make_real
 #define make_string		s9_make_string
 #define make_symbol		s9_make_symbol
 #define make_vector		s9_make_vector
 #define mem_error_handler	s9_mem_error_handler
 #define mkfix			s9_mkfix
 #define new_port		s9_new_port
 #define new_vec		s9_new_vec
 #define open_input_port	s9_open_input_port
 #define open_input_string	s9_open_input_string
 #define open_output_port	s9_open_output_port
 #define output_port		s9_output_port
 #define outport_open_p		s9_outport_open_p
 #define port_eof		s9_port_eof
 #define print_bignum		s9_print_bignum
 #define print_expanded_real	s9_print_expanded_real
 #define print_real		s9_print_real
 #define print_sci_real		s9_print_sci_real
 #define printer_limit		s9_printer_limit
 #define prints			s9_prints
 #define read_counter		s9_read_counter
 #define readc			s9_readc
 #define real_abs		s9_real_abs
 #define real_add		s9_real_add
 #define real_approx_p		s9_real_approx_p
 #define real_ceil		s9_real_ceil
 #define real_divide		s9_real_divide
 #define real_equal_p		s9_real_equal_p
 #define real_exponent		s9_real_exponent
 #define real_floor		s9_real_floor
 #define real_integer_p		s9_real_integer_p
 #define real_less_p		s9_real_less_p
 #define real_mantissa		s9_real_mantissa
 #define real_multiply		s9_real_multiply
 #define real_negate		s9_real_negate
 #define real_negative_p	s9_real_negative_p
 #define real_positive_p	s9_real_positive_p
 #define real_power		s9_real_power
 #define real_round		s9_real_round
 #define real_sqrt		s9_real_sqrt
 #define real_subtract		s9_real_subtract
 #define real_to_bignum		s9_real_to_bignum
 #define real_to_string		s9_real_to_string
 #define real_trunc		s9_real_trunc
 #define real_zero_p		s9_real_zero_p
 #define rejectc		s9_rejectc
 #define reset_counter		s9_reset_counter
 #define reset_std_ports	s9_reset_std_ports
 #define run_stats		s9_run_stats
 #define set_input_port		s9_set_input_port
 #define set_node_limit		s9_set_node_limit
 #define set_output_port	s9_set_output_port
 #define set_printer_limit	s9_set_printer_limit
 #define set_vector_limit	s9_set_vector_limit
 #define string_numeric_p	s9_string_numeric_p
 #define string_to_bignum	s9_string_to_bignum
 #define string_to_number	s9_string_to_number
 #define string_to_real		s9_string_to_real
 #define string_to_symbol	s9_string_to_symbol
 #define symbol_ref		s9_symbol_ref
 #define symbol_table		s9_symbol_table
 #define symbol_to_string	s9_symbol_to_string
 #define typecheck		s9_typecheck
 #define unlock_port		s9_unlock_port
 #define unsave			s9_unsave
 #define writec			s9_writec
#endif

#define VERSION		"20221212"

#ifdef plan9
 #define handle_sigquit()
 #define handle_sigint()	notify(keyboard_interrupt)
#else
 #include <time.h>
 #include <signal.h>
 #ifdef EDIT
  #include <unistd.h>
  #include <termios.h>
 #endif
 #define handle_sigquit()	signal(SIGQUIT, keyboard_quit)
 #define handle_sigint()	signal(SIGINT, keyboard_interrupt)
#endif

#define TOKEN_LENGTH	256
#define MIN_DICT_LEN	13

#define NTRACE		10

#define DFLPATH		".:lib"

#define T_DICTIONARY	(USER_SPECIALS-1)
#define T_VARIABLE	(USER_SPECIALS-2)
#define T_PRIMOP	(USER_SPECIALS-3)
#define T_BARRIER	(USER_SPECIALS-4)
#define STRING_NIL	(USER_SPECIALS-5)
#define NO_VALUE	(USER_SPECIALS-6)

#define list_p(x) \
	(pair_p(x) || NIL == (x))

#define dictionary_p(n) \
        (!special_p(n) && (tag(n) & S9_ATOM_TAG) && car(n) == T_DICTIONARY)
#define dict_data(x)	cddr(x)
#define dict_table(x)	vector(dict_data(x))
#define dict_len(x)	vector_len(cddr(x))
#define dict_size(x)	cadr(x)

#define fun_immed(x)	cadr(x)
#define fun_arity(x)	caddr(x)
#define fun_body(x)	cdddr(x)

#define variable_p(n) \
        (!special_p(n) && (tag(n) & S9_ATOM_TAG) && car(n) == T_VARIABLE)
#define var_symbol(x)	cadr(x)
#define var_name(x)	symbol_name(var_symbol(x))
#define var_value(x)	cddr(x)

#define primop_p(n) \
        (!special_p(n) && (tag(n) & S9_ATOM_TAG) && car(n) == T_PRIMOP)
#define primop_slot(x)	cadr(x)

#define syntax_body(x)	cdr(x)

cell	Dstack;
cell	Sys_dict;
cell	Safe_dict;
cell	Frame;
int	State;
cell	Tmp;
cell	Locals;
cell	Barrier;
cell	S, F;
cell	Prog, P;
cell	Tok, T;
cell	Epsilon_var;
cell	Loading;
cell	Module;
cell	Mod_funvars;
char	Modname[TOKEN_LENGTH+1];
cell	Locnames;
cell	Local_id;
int	To_chan, From_chan;
int	Prog_chan;
cell	Trace[NTRACE];
int	Traceptr;
int	Report;
int	Quiet;
int	Script;
int	Debug;
int	Transcript;
char	Inbuf[TOKEN_LENGTH+1];
int	Listlev;
int	Incond;
int	Infun;
int	Line;
int	Display;
char	Image_path[TOKEN_LENGTH+13];

volatile int	Intr;

/* VM opcode symbols */

cell	S_amend, S_amendd, S_argv, S_atom, S_call0, S_call1, S_call2,
	S_call3, S_char, S_clear, S_conv, S_cut, S_def, S_div, S_down,
	S_drop, S_each, S_each2, S_eachl, S_eachp, S_eachr, S_enum,
	S_eq, S_expand, S_find, S_first, S_floor, S_form, S_format,
	S_format2, S_fun0, S_fun1, S_fun2, S_fun3, S_group, S_gt,
	S_host, S_if, S_imm1, S_imm2, S_index, S_indexd, S_intdiv,
	S_it, S_iter, S_join, S_list, S_lslit, S_lt, S_match, S_max,
	S_min, S_minus, S_newdict, S_neg, S_not, S_over, S_over2, S_plus,
	S_pop0, S_pop1, S_pop2, S_pop3, S_power, S_prog, S_range,
	S_recip, S_rem, S_reshape, S_rev, S_rot, S_sconv, S_siter,
	S_sover, S_sover2, S_swhile, S_shape, S_size, S_split, S_swap,
	S_syscall, S_take, S_thisfn, S_times, S_transp, S_up, S_undef,
	S_while, S_x, S_y, S_z, S_fastpow, S_edit, S_cols;

cell	HL_power;

enum Adverb_states {
	S_EVAL, S_APPIF, S_APPLY, S_EACH, S_EACH2, S_EACHL, S_EACHP,
	S_EACHR, S_OVER, S_CONV, S_ITER, S_WPRED, S_WEXPR, S_S_OVER,
	S_S_CONV, S_S_ITER, S_S_WPRED, S_S_WEXPR
};

struct OP_ {
	char	*name;
	int	syntax;
	void	(*handler)(void);
};

#define OP	struct OP_

struct SYS_ {
	char	*name;
	int	arity;
	void	(*handler)(void);
};

#define SYS	struct SYS_

cell *GC_root[] = {
	&S, &F, &Dstack, &Frame, &Sys_dict, &Safe_dict, &Tmp, &Barrier,
	&Tok, &Prog, &P, &T, &S_it, &Locals, &Module, &Mod_funvars,
	&Locnames, &Loading, NULL
};

cell *Image_vars[] = {
	&Barrier, &Dstack, &Epsilon_var, &F, &Frame, &Loading,
	&Local_id, &Locals, &Locnames, &Mod_funvars, &Module,
	&P, &Prog, &S, &Safe_dict, &Sys_dict, &T, &Tmp, &Tok,
	&S_amend, &S_amendd, &S_argv, &S_atom, &S_call0, &S_call1,
	&S_call2, &S_call3, &S_char, &S_clear, &S_conv, &S_cut, &S_def,
	&S_div, &S_down, &S_drop, &S_each, &S_each2, &S_eachl, &S_eachp,
	&S_eachr, &S_enum, &S_eq, &S_expand, &S_find, &S_first, &S_floor,
	&S_form, &S_format, &S_format2, &S_fun0, &S_fun1, &S_fun2, &S_fun3,
	&S_group, &S_gt, &S_host, &S_if, &S_imm1, &S_imm2, &S_index,
	&S_indexd, &S_intdiv, &S_it, &S_iter, &S_join, &S_list, &S_lslit,
	&S_lt, &S_match, &S_max, &S_min, &S_minus, &S_neg, &S_newdict,
	&S_not, &S_over, &S_over2, &S_plus, &S_pop0, &S_pop1, &S_pop2,
	&S_pop3, &S_power, &S_prog, &S_range, &S_recip, &S_rem, &S_reshape,
	&S_rev, &S_rot, &S_sconv, &S_siter, &S_sover, &S_sover2, &S_swhile,
	&S_shape, &S_size, &S_split, &S_swap, &S_syscall, &S_take,
	&S_thisfn, &S_times, &S_transp, &S_up, &S_undef, &S_while, &S_x,
	&S_y, &S_z, &HL_power, &S_fastpow, &S_edit, &S_cols,
	NULL
};

/*
 * Allocators
 */

/* From http://planetmath.org/goodhashtableprimes */

static int hashsize(int n) {
        if (n < 5) return 5;
	if (n < 11) return 11;
	if (n < 23) return 23;
	if (n < 47) return 47;
	if (n < 97) return 97;
	if (n < 193) return 193;
	if (n < 389) return 389;
	if (n < 769) return 769;
	if (n < 1543) return 1543;
	if (n < 3079) return 3079;
	if (n < 6151) return 6151;
	if (n < 12289) return 12289;
	if (n < 24593) return 24593;
	if (n < 49157) return 49157;
	if (n < 98317) return 98317;
	if (n < 196613) return 196613;
	if (n < 786433) return 786433;
	if (n < 1572869) return 1572869;
	if (n < 3145739) return 3145739;
	if (n < 6291469) return 6291469;
	if (n < 12582917) return 12582917;
	if (n < 25165843) return 25165843;
	if (n < 50331653) return 50331653;
	if (n < 100663319) return 100663319;
	if (n < 201326611) return 201326611;
	if (n < 402653189) return 402653189;
	if (n < 805306457) return 805306457;
	return 1610612741;
}

static cell make_dict(int k) {
	cell	d;
	int	i;

	k = hashsize(k);
	d = make_vector(k);
	for (i = 0; i < k; i++)
		vector(d)[i] = NIL;
	d = new_atom(0, d);
	d = new_atom(T_DICTIONARY, d);
	return d;
}

static cell make_function(cell body, int immed, int arity) {
	return new_atom(T_FUNCTION,
			new_atom(immed,
				new_atom(arity, body)));
}

static cell find_var(char *s) {
	cell	p;

	for (p = Sys_dict; p != NIL; p = cdr(p)) {
		if (strcmp(s, var_name(car(p))) == 0)
			return car(p);
	}
	return UNDEFINED;
}

static cell make_variable(char *s, cell v) {
	cell	n;
	char	name[TOKEN_LENGTH+1];

	n = find_var(s);
	if (n != UNDEFINED) return n;
	strcpy(name, s);
	save(v);
	n = symbol_ref(name);
	n = cons(n, v);
	n = new_atom(T_VARIABLE, n);
	Sys_dict = cons(n, Sys_dict);
	unsave(1);
	return n;
}

static cell var_ref(char *s) {
	return make_variable(s, NIL);
}

static cell make_primop(int slot, int syntax) {
	cell	n;

	n = new_atom(slot, NIL);
	return new_atom(syntax? T_SYNTAX: T_PRIMOP, n);
}

/*
 * Error handling
 */

void kg_write(cell x);

static void printtrace(void) {
	int	i, j;

	prints("kg: trace:");
	i = Traceptr;
	for (j=0; j<NTRACE; j++) {
		if (i >= NTRACE)
			i = 0;
		if (Trace[i] != UNDEFINED) {
			prints(" ");
			prints(symbol_name(Trace[i]));
		}
		i++;
	}
	nl();
}

static cell error(char *msg, cell arg) {
	int	p = set_output_port(Error_port);
	char	buf[100];

	Incond = 0;
	Infun = 0;
	Listlev = 0;
	if (0 == Report) {
		s9_abort();
		return UNDEFINED;
	}
	if (s9_aborted())
		return UNDEFINED;
	s9_abort();
	if (Quiet)
		set_output_port(Error_port);
	prints("kg: error: ");
	if (Loading != NIL) {
		kg_write(car(Loading));
		sprintf(buf, ": %d: ", Line);
		prints(buf);
	}
	prints(msg);
	if (arg != VOID) {
		prints(": ");
		kg_write(arg);
	}
	nl();
	if (Debug) {
		printtrace();
	}
	set_output_port(p);
	if (Quiet)
		bye(1);
	return UNDEFINED;
}

/*
 * Reader
 */

#ifdef EDIT

 #define bell() write(1, "\007", 1)
 #define back() write(1, "\b", 1);

 static void refresh(char *s, int k) {
	int	i;

	for (i=0; i < k; i++)
		write(1, &s[i], 1);
	write(1, " ", 1);
	for (i=0; i <= k; i++)
		back();
 }

 static int intvalue(cell x);

 #define MAXHIST 20

 char	History[MAXHIST][TOKEN_LENGTH+1];
 int    Hin = 0, Hout = 0, Hnum = 0;

 static void erase(char *s, int p, int k) {
	int	i;

	write(1, &s[p], k-p);
	for (i=0; i<k; i++) {
		back();
		write(1," ",1);
		back();
	}
 }

 static char *editline(char *s, int lim) {
	char		c;
	int		p = 0, k = 0, i;
	struct termios	t, r;

	if (lim > intvalue(var_value(S_cols))-9)
		lim = intvalue(var_value(S_cols))-9;
	tcgetattr(0, &t);
	tcgetattr(0, &r);
	cfmakeraw(&r);
	tcsetattr(0, TCSANOW, &r);
	for (;;) {
		read(0, &c, 1);
		if ('\r' == c) {
			break;
		}
		else if ('C'-'@' == c) {
			tcsetattr(0, TCSANOW, &t);
			error("interrupted", VOID);
			Intr = 1;
			return NULL;
		}
		else if ('B'-'@' == c && p > 0) {
			p--;
			back();
		}
		else if ('F'-'@' == c && p < k) {
			write(1, &s[p], 1);
			p++;
		}
		else if ('A'-'@' == c) {
			while (p > 0) {
				back();
				p--;
			}
		}
		else if ('E'-'@' == c) {
			write(1, &s[p], k-p);
			p = k;
		}
		else if ('D'-'@' == c) {
			if (0 == k) {
				tcsetattr(0, TCSANOW, &t);
				return NULL;
			}
			if (p < k) {
				for (i=p; i<k-1; i++)
					s[i] = s[i+1];
				k--;
				refresh(&s[p], k-p);
			}
		}
		else if (('P'-'@' == c || 'N'-'@' == c) && Hnum > 0) {
			if (p != 0 && p != k) {
				bell();
				continue;
			}
			erase(s, p, k);
			if ('N'-'@' == c && ++Hout >= Hnum) Hout = 0;
			strcpy(s, History[Hout]);
			p = k = strlen(s);
			write(1,s,k);
			if ('P'-'@' == c && --Hout < 0) Hout = Hnum-1;
		}
		else if ('H'-'@' == c && p > 0) {
			p--;
			for (i=p; i<k-1; i++)
				s[i] = s[i+1];
			k--;
			back();
			refresh(&s[p], k-p);
		}
		else if ('U'-'@' == c) {
			erase(s, p, k);
			p = k = 0;
		}
		else if (' ' <= c && c <= '~') {
			if (k >= lim-1) {
				bell();
				continue;
			}
			k++;
			for (i=k; i>p; i--)
				s[i] = s[i-1];
			s[p] = c;
			refresh(&s[p], k-p);
			write(1, &s[p], 1);
			p++;
		}
		else {
			bell();
		}
	}
	Line++;
	s[k] = 0;
	tcsetattr(0, TCSANOW, &t);
	nl();
	Hout = Hin-1;
	if (Hout < 0) Hout = Hnum-1;
	if (k > 0) {
		Hout = Hin;
		strcpy(History[Hin], s);
		if (++Hin > Hnum) Hnum = Hin;
		if (Hin >= MAXHIST) Hin = 0;
	}
	return s;
 }

static int false_p(cell);

#endif /* EDIT */

static char *kg_getline(char *s, int k) {
	int	c = 0;
	char	*p = s;

#ifdef EDIT
	if (input_port() == 0 && !false_p(var_value(S_edit)))
		return editline(s, k);
#endif
	while (k--) {
		c = readc();
		if (EOF == c || '\n' == c)
			break;
		*s++ = c;
	}
	Line++;
	*s = 0;
	return EOF == c && p == s? NULL: p;
}

#define is_white(c) \
	(' '  == (c) ||	\
	 '\t' == (c) ||	\
	 '\n' == (c) ||	\
	 '\r' == (c) ||	\
	 '\f' == (c))

#define is_symbolic(c) \
	(isalpha(c) || isdigit(c) || (c) == '.')

#define is_special(c) \
	(!isalpha(c) && !isdigit(c) && (c) >= ' ')

static int skip_white(void) {
	int	c;

	for (;;) {
		c = readc();
		while (is_white(c) && (NIL == Loading || c != '\n'))
			c = readc();
		if ('\n' == c)
			Line++;
		if (Loading != NIL && '\n' == c) {
			if (Listlev > 0 || Incond || Infun)
				continue;
			return EOF;
		}
		return c;
	}
}

static void comment(void) {
	int	c;

	c = readc();
	for (;;) {
		if (EOF == c) {
			error("missing end of comment", VOID);
			return;
		}
		if ('"' == c) {
			c = readc();
			if (c != '"') {
				rejectc(c);
				break;
			}
		}
		c = readc();
	}
}

static int skip(void) {
	int	c;

	for (;;) {
		if (Intr) return EOF;
		c = skip_white();
		if (EOF == c) {
			if (input_port() < 0 && (Listlev || Incond || Infun))
			{
				if (!Quiet) {
					prints("        ");
					flush();
				}
				close_input_string();
				if (kg_getline(Inbuf, TOKEN_LENGTH) == NULL)
					return EOF;
				open_input_string(Inbuf);
			}
			else {
				return EOF;
			}
		}
		else if (':' == c) {
			c = readc();
			if (c != '"') {
				rejectc(c);
				return ':';
			}
			comment();
		}
		else {
			return c;
		}
	}
}

static cell read_string(void) {
	int	c, i;
	char	s[TOKEN_LENGTH+1];

	i = 0;
	c = readc();
	for (;;) {
		if (EOF == c)
			return error("missing end of string", VOID);
		if ('"' == c) {
			c = readc();
			if (c != '"') {
				rejectc(c);
				break;
			}
		}
		if (i < TOKEN_LENGTH)
			s[i++] = c;
		else if (TOKEN_LENGTH == i) {
			i++;
			error("string too long", make_string(s, i));
		}
		c = readc();
	}
	s[i] = 0;
	return make_string(s, i);
}

static void mkglobal(char *s) {
	char	*p;

	if ((p = strchr(s, '`')) != NULL)
		*p = 0;
}

static int is_local(char *s) {
	return strchr(s, '`') != NULL;
}

static int is_funvar(char *s) {
	cell	m, p;

	for (m = Mod_funvars; m != NIL; m = cdr(m)) {
		for (p = car(m); p != NIL; p = cdr(p)) {
			if (!strcmp(symbol_name(car(p)), s))
				return 1;
		}
	}
	return 0;
}

static int in_module(char *s) {
	cell	m;
	char	b[TOKEN_LENGTH+1];
	int	g;

	if (UNDEFINED == Module)
		return 0;
	if (is_funvar(s))
		return 0;
	g = !is_local(s);
	for (m = Module; m != NIL; m = cdr(m)) {
		strcpy(b, var_name(car(m)));
		if (g) {
			mkglobal(b);
		}
		if (!strcmp(s, b))
			return 1;
	}
	return 0;
}

static char *mklocal(char *s) {
	cell	loc, p;
	int	id;
	char	b[TOKEN_LENGTH+1];

	for (loc = Locnames; loc != NIL; loc = cdr(loc)) {
		id = caar(loc);
		for (p = cdar(loc); p != NIL; p = cdr(p)) {
			strcpy(b, symbol_name(car(p)));
			mkglobal(b);
			if (!strcmp(s, b)) {
				sprintf(s, "%s`%d", b, id);
				return s;
			}
		}
	}
	return NULL;
}

static void mkmodlocal(char *s) {
	if (strlen(s)+strlen(Modname) >= TOKEN_LENGTH-1)
		error("in-module symbol too long", make_string(s,strlen(s)));
	strcat(s, "`");
	strcat(s, Modname);
}

static cell read_sym(int c, int mod) {
	char	s[TOKEN_LENGTH+1];
	int	i;

	i = 0;
	while (is_symbolic(c)) {
		if (i < TOKEN_LENGTH)
			s[i++] = c;
		else if (TOKEN_LENGTH == i) {
			i++;
			error("symbol too long", make_string(s, i));
		}
		c = readc();
	}
	rejectc(c);
	s[i] = 0;
	if (mklocal(s) != NULL)
		;
	else if (0 == s[1] && ('x' == *s || 'y' == *s || 'z' == *s))
		;
	else if (mod &&
		 (in_module(s) ||
		 (Module != UNDEFINED && find_var(s) == UNDEFINED))
	) {
		mkmodlocal(s);
	}
	if (Listlev)
		return symbol_ref(s);
	return make_variable(s, NO_VALUE);
}

static cell read_num(int c) {
	char	s[TOKEN_LENGTH+1];
	int	i, c2 = 0;

	i = 0;
	if ('-' == c) {
		s[i++] = c;
		c = readc();
	}
	while (	isdigit(c) ||
		'.' == c ||
		'e' == c ||
		('e' == c2 && '+' == c) ||
		('e' == c2 && '-' == c))
	{
		if (i < TOKEN_LENGTH)
			s[i++] = c;
		else if (TOKEN_LENGTH == i) {
			i++;
			error("number too long", make_string(s, i));
		}
		c2 = c;
		c = readc();
	}
	rejectc(c);
	s[i] = 0;
	if (!string_numeric_p(s))
		return error("invalid number", make_string(s, i));
	return string_to_number(s);
}

static cell read_char(void) {
	return make_char(readc());
}

static cell read_xnum(int pre, int neg) {
	char	digits[] = "0123456789abcdef";
	char	buf[100];
	cell	base, num;
	int	c, p, nd;
	int	radix;

	c = pre;
	switch (c) {
	case 'b':
		radix = 2; break;
	case 'o':
		radix = 8; break;
	case 'x':
		radix = 16; break;
	default:
		radix = 10; break;
	}
	base = make_integer(radix);
	save(base);
	num = Zero;
	save(num);
	if (radix != 10)
		c = tolower(readc());
	nd = 0;
	while (1) {
		p = 0;
		while (digits[p] && digits[p] != c)
			p++;
		if (p >= radix) {
			if (0 == nd) {
				sprintf(buf, "invalid digit in #%c number",
					pre);
				unsave(2);
				return error(buf, make_char(c));
			}
			break;
		}
		num = bignum_multiply(num, base);
		car(Stack) = num;
		num = bignum_add(num, make_integer(p));
		car(Stack) = num;
		nd++;
		c = tolower(readc());
	}
	unsave(2);
	if (!nd) {
		sprintf(buf, "digits expected after #%c", pre);
		return error(buf, VOID);
	}
	rejectc(c);
	return neg? bignum_negate(num): num;
}

static cell kg_read(void);

static cell read_list(int dlm) {
	int	c, k = 0;
	cell	a, n;

	Listlev++;
	a = cons(NIL, NIL);
	save(a);
	c = skip();
	while (c != dlm) {
		rejectc(c);
		n = kg_read();
		if (eof_p(n)) {
			unsave(1);
			return error("unexpected end of list/dict", VOID);
		}
		car(a) = n;
		c = skip();
		if (c != dlm) {
			n = cons(NIL, NIL);
			cdr(a) = n;
			a = cdr(a);
		}
		k++;
	}
	Listlev--;
	a = unsave(1);
	if (0 == k)
		return NIL;
	return a;
}

static int string_hash(char *s) {
	unsigned	h = 0xdeadbeef;

	while (*s)
		h = ((h << 5) + h) ^ *s++;
	return abs((int) h);
}

static int hash2(cell x, int k) {
	int	of;

	if (0 == k)
		return 0;
	if (integer_p(x))
		return abs(bignum_to_int(x, &of) % k);
	if (char_p(x))
		return char_value(x) % k;
	if (string_p(x))
		return abs(string_hash(string(x)) % k);
	if (symbol_p(x))
		return abs(string_hash(symbol_name(x)) % k);
	if (variable_p(x))
		return abs(string_hash(var_name(x)) % k);
	if (pair_p(x))
		return abs((length(x) * hash2(car(x), k)) % k);
	return 0;
}

static int tuple_p(cell x) {
	return pair_p(x) && pair_p(cdr(x)) && NIL == cddr(x);
}

static cell list_to_dict(cell x) {
	cell	d, *v, e;
	int	n, k, i, h;

	save(x);
	n = length(x);
	k = hashsize(n);
	d = make_vector(k);
	save(d);
	v = vector(d);
	for (i = 0; i < k; i++)
		v[i] = NIL;
	while (x != NIL) {
		if (!tuple_p(car(x))) {
			unsave(2);
			return error("malformed dictionary entry", car(x));
		}
		h = hash2(caar(x), k);
		v = vector(d);
		e = cons(car(x), v[h]);
		v = vector(d);
		v[h] = e;
		x = cdr(x);
	}
	unsave(2);
	d = new_atom(n, d);
	return new_atom(T_DICTIONARY, d);
}

static cell read_dict(void) {
	cell	x;

	x = read_list('}');
	return list_to_dict(x);
}

static cell shifted(void) {
	int	c;
	char	buf[3];
	cell	n;

	c = readc();
	if (isalpha(c) || '.' == c) {
		n = read_sym(c, 0);
		if (variable_p(n))
			return var_symbol(n);
		return n;
	}
	else if ('"' == c || isdigit(c)) {
		rejectc(c);
		return kg_read();
	}
	else if ('{' == c) {
		return read_dict();
	}
	else {
		buf[0] = ':';
		buf[1] = c;
		buf[2] = 0;
		return symbol_ref(buf);
	}
}

static cell read_op(int c) {
	char	buf[3];

	buf[1] = buf[2] = 0;
	buf[0] = c;
	if ('\\' == c) {
		c = readc();
		if ('~' == c || '*' == c)
			buf[1] = c;
		else
			rejectc(c);
	}
	return symbol_ref(buf);
}

#ifdef plan9
static int system(char *cmd) {
	int	r;
	Waitmsg	*w;

	r = fork();
	if (r < 0) {
		return -1;
	}
	else if (0 == r) {
		execl("/bin/rc", "/bin/rc", "-c", cmd, 0);
		exits("execl() failed");
	}
	else {
		w = wait();
		r = w->msg[0] != 0;
		free(w);
	}
	return r;
}
#endif

static void inventory(char *buf) {
	char	cmd[TOKEN_LENGTH+20], kpbuf[TOKEN_LENGTH+1];
	char	*p, *s;

#ifdef SAFE
	error("shell access disabled", VOID);
	return;
#endif
	if (buf[0]) {
		p = buf;
		sprintf(cmd, "cd %s; ls *.kg", buf);
	}
	else {
		p = getenv("KLONGPATH");
		if (NULL == p)
			p = DFLPATH;
		strncpy(kpbuf, p, TOKEN_LENGTH);
		kpbuf[TOKEN_LENGTH] = 0;
		p = kpbuf;
		if (strlen(p) >= TOKEN_LENGTH) {
			error("KLONGPATH too long!", VOID);
			return;
		}
		s = strchr(p, ':');
		if (s != NULL)
			*s = 0;
		sprintf(cmd, "cd %s; ls *.kg", p);
	}
	printf("%s:\n", p);
	system(cmd);
}

static void transcribe(cell x, int input) {
	cell	p;

	if (Transcript < 0)
		return;
	p = set_output_port(Transcript);
	if (input) {
		Display = 1;
		prints("\t");
	}
	kg_write(x);
	nl();
	Display = 0;
	set_output_port(p);
}

static void transcript(char *path) {
	if (Transcript >= 0) {
		close_port(Transcript);
		Transcript = -1;
		prints("transcript closed"); nl();
	}
	if (NULL == path || 0 == *path)
		return;
	if ((Transcript = open_output_port(path, 1)) < 0) {
		error("could not open transcript file",
			make_string(path, strlen(path)));
		return;
	}
	lock_port(Transcript);
	prints("sending transcript to: ");
	prints(path); nl();
}

static void eval(cell x);

static void apropos(char *s) {
	cell	x;

	if (0 == *s) {
		error("Usage: ']a function/operator' or ']a all'", VOID);
		return;
	}
	x = cons(S_pop1, NIL);
	x = cons(S_call1, x);
	x = cons(var_value(var_ref("help")), x);
	if (NIL == car(x)) {
		error("help function not loaded, try ]lhelp", VOID);
		return;
	}
	if (strcmp(s, "all") == 0)
		s = "";
	x = cons(make_string(s, strlen(s)), x);
	save(x);
	eval(x);
	unsave(1);
}

static cell load(cell x, int v, int scr);

static void meta_command(void) {
	int	cmd, c, i;
	char	buf[TOKEN_LENGTH];

	cmd = skip();
	c = skip();
	for (i=0; c != EOF; i++) {
		if (i < TOKEN_LENGTH-2)
			buf[i] = c;
		c = readc();
	}
	buf[i] = 0;
	switch (cmd) {
	case '!':
#ifdef SAFE
		error("shell access disabled", VOID);
#else
		system(buf);
#endif
		break;
	case 'a':
	case 'h':
		apropos(buf);
		break;
	case 'i':
		inventory(buf);
		break;
	case 'l':
		load(make_string(buf, strlen(buf)), 1, 0);
		open_input_string("");
		break;
	case 'q':
		prints("bye!"); nl();
		bye(0);
		break;
	case 't':
		transcript(buf);
		break;
	default:
		prints("! cmd     run shell command"); nl();
		prints("a fn/op   describe function/operator (apropos)"); nl();
		prints("i [dir]   inventory (of given directory)"); nl();
		prints("l file    load file.kg"); nl();
		prints("q         quit"); nl();
		prints("t [file]  transcript to file (none = off)"); nl();
		break;
	}
}

static cell kg_read(void) {
	int	c;

	c = skip();
	switch (c) {
	case '"':
		return read_string();
	case '0':
		c = tolower(readc());
		if ('c' == c)
			return read_char();
		if ('x' == c || 'o' == c || 'b' == c)
			return read_xnum(c, 0);
		rejectc(c);
		return read_num('0');
	case ':':
		return shifted();
	case '[':
		return read_list(']');
	case EOF:
		return END_OF_FILE;
	default:
		if ('-' == c && Listlev > 0) {
			c = readc();
			rejectc(c);
			if (isdigit(c))
				return read_num('-');
			return read_op('-');
		}
		if (	']' == c &&
			0 == Listlev &&
			0 == Incond &&
			0 == Infun &&
			NIL == Loading
		) {
			meta_command();
			return kg_read();
		}
		if (isdigit(c))
			return read_num(c);
		if (is_symbolic(c))
			return read_sym(c, 1);
		return read_op(c);
	}
}

/*
 * Printer
 */

static void write_char(cell x) {
	char	b[4];

	sprintf(b, Display? "%c": "0c%c", (int) char_value(x));
	prints(b);
}

static void write_string(cell x) {
	int	i, k;
	char	*s;
	char	b[2];

	if (Display) {
		blockwrite(string(x), string_len(x)-1);
		return;
	}
	b[1] = 0;
	prints("\"");
	k = string_len(x)-1;
	s = string(x);
	for (i = 0; i < k; i++) {
		if ('"' == s[i])
			prints("\"");
		b[0] = s[i];
		prints(b);
	}
	prints("\"");
}

static void write_list(cell x) {
	prints("[");
	while (x != NIL) {
		kg_write(car(x));
		if (cdr(x) != NIL)
			prints(" ");
		x = cdr(x);
	}
	prints("]");
}

static void write_dict(cell x) {
	int	k, i, first = 1;
	cell	*v, e;

	x = cddr(x);
	k = vector_len(x);
	prints(":{");
	v = vector(x);
	for (i = 0; i < k; i++) {
		if (NIL == v[i])
			continue;
		if (!first)
			prints(" ");
		first = 0;
		for (e = v[i]; e != NIL; e = cdr(e)) {
			kg_write(car(e));
			if (cdr(e) != NIL)
				prints(" ");
		}
	}
	prints("}");
}

static void write_chan(cell x) {
	char	b[100];

	sprintf(b, ":%schan.%d",
			input_port_p(x)? "in": "out",
			(int) port_no(x));
	prints(b);
}

static void write_primop(cell x) {
	char	b[100];

	sprintf(b, ":primop-%d", (int) primop_slot(x));
	prints(b);
}

static void write_fun(cell x) {
	if (0 == fun_arity(x))
		prints(":nilad");
	else if (1 == fun_arity(x))
		prints(":monad");
	else if (2 == fun_arity(x))
		prints(":dyad");
	else
		prints(":triad");
}

void kg_write(cell x) {
	if (NIL == x)
		prints("[]");
	else if (symbol_p(x)) {
		prints(":");
		prints(symbol_name(x));
	}
	else if (variable_p(x))
		prints(var_name(x));
	else if (undefined_p(x))
		prints(":undefined");
	else if (string_p(x))
		write_string(x);
	else if (integer_p(x))
		print_bignum(x);
	else if (char_p(x))
		write_char(x);
	else if (real_p(x))
		print_real(x);
	else if (eof_p(x))
		prints(":eof");
	else if (dictionary_p(x))
		write_dict(x);
	else if (input_port_p(x) || output_port_p(x))
		write_chan(x);
	else if (function_p(x))
		write_fun(x);
	else if (primop_p(x))
		write_primop(x);
	else if (x == Barrier)
		prints(":barrier");
	else if (x == STRING_NIL)
		prints(":stringnil");
	else
		write_list(x);
}

/*
 * Dictionaries
 */

static int dict_contains(cell x, cell y);

static int match(cell a, cell b) {
	int	k;

	if (a == b)
		return 1;
	if (real_p(a) && number_p(b)) {
		return real_approx_p(a, b);
	}
	if (number_p(a) && real_p(b)) {
		return real_approx_p(a, b);
	}
	if (number_p(a) && number_p(b)) {
		return real_equal_p(a, b);
	}
	if (symbol_p(a) && variable_p(b)) {
		return a == var_symbol(b);
	}
	if (variable_p(a) && symbol_p(b)) {
		return var_symbol(a) == b;
	}
	if (variable_p(a) && variable_p(b)) {
		return var_symbol(a) == var_symbol(b);
	}
	if (char_p(a) && char_p(b)) {
		return char_value(a) == char_value(b);
	}
	if (string_p(a) && string_p(b)) {
		k = string_len(a);
		if (string_len(b) == k)
			return memcmp(string(a), string(b), k) == 0;
		return 0;
	}
	if (pair_p(a) && pair_p(b)) {
		if (length(a) != length(b))
			return 0;
		while (a != NIL && b != NIL) {
			if (!match(car(a), car(b)))
				return 0;
			a = cdr(a);
			b = cdr(b);
		}
		return 1;
	}
	if (dictionary_p(a) && dictionary_p(b)) {
		return dict_contains(a, b) && dict_contains(b, a);
	}
	return 0;
}

static cell dict_find(cell d, cell k) {
	cell	x, *v;
	int	h;

	if (0 == dict_len(d))
		return NIL;
	h = hash2(k, dict_len(d));
	v = dict_table(d);
	x = v[h];
	while (x != NIL) {
		if (match(caar(x), k))
			return x;
		x = cdr(x);
	}
	return UNDEFINED;
}

static cell dict_lookup(cell d, cell k) {
	cell	n;

	n = dict_find(d, k);
	if (!pair_p(n)) return n;
	return car(n);
}

static cell resize_dict(cell old_d, int k) {
	int	old_k, i, h;
	cell	d, e, n;

	save(old_d);
	if (k < MIN_DICT_LEN)
		k = MIN_DICT_LEN;
	d = make_dict(k);
	k = dict_len(d);
	save(d);
	old_k = dict_len(old_d);
	for (i = 0; i < old_k; i++) {
		for (e = dict_table(old_d)[i]; e != NIL; e = cdr(e)) {
			h = hash2(caar(e), k);
			n = cons(car(e), dict_table(d)[h]);
			dict_table(d)[h] = n;
		}
	}
	dict_size(d) = dict_size(old_d);
	d = unsave(1);
	unsave(1);
	return d;
}

static cell grow_dict(cell d) {
	cell	n;

	n = resize_dict(d, (dict_len(d) + 1) * 2 - 1);
	dict_data(d) = dict_data(n);
	return n;
}

static cell copy_dict(cell d) {
	/* see hashsize() about the -1 */
	return resize_dict(d, dict_len(d)-1);
}

static cell dict_add(cell d, cell k, cell v) {
	cell	x, e, n;
	int	h;

	if (dictionary_p(k))
		return error("bad dictionary key", k);
	Tmp = k;
	save(v);
	save(k);
	Tmp = NIL;
	x = dict_find(d, k);
	if (x != UNDEFINED) {
		n = cons(k, cons(v, NIL));
		car(x) = n;
		unsave(2);
		return d;
	}
	if (dict_size(d) >= dict_len(d))
		d = grow_dict(d);
	save(d);
	h = hash2(k, dict_len(d));
	e = cons(v, NIL);
	e = cons(k, e);
	e = cons(e, dict_table(d)[h]);
	dict_table(d)[h] = e;
	dict_size(d)++;
	unsave(3);
	return d;
}

static cell dict_remove(cell d, cell k) {
	cell	*x, *v;
	int	h;

	if (0 == dict_len(d))
		return NIL;
	h = hash2(k, dict_len(d));
	v = dict_table(d);
	x = &v[h];
	while (*x != NIL) {
		if (match(caar(*x), k)) {
			*x = cdr(*x);
			dict_size(d)--;
			break;
		}
		x = &cdr(*x);
	}
	return d;
}

static cell dict_to_list(cell x) {
	cell	n, p, new, last;
	int	i, k;

	n = cons(NIL, NIL);
	save(n);
	k = dict_len(x);
	last = NIL;
	for (i = 0; i < k; i++) {
		for (p = dict_table(x)[i]; p != NIL; p = cdr(p)) {
			car(n) = car(p);
			new = cons(NIL, NIL);
			cdr(n) = new;
			last = n;
			n = cdr(n);
		}
	}
	n = unsave(1);
	if (NIL == last)
		return NIL;
	cdr(last) = NIL;
	return n;
}

static int dict_contains(cell x, cell y) {
	cell	p, t;

	p = dict_to_list(x);
	save(p);
	while (p != NIL) {
		t = dict_lookup(y, caar(p));
		if (UNDEFINED == t || match(cadar(p), cadr(t)) == 0) {
			unsave(1);
			return 0;
		}
		p = cdr(p);
	}
	unsave(1);
	return 1;
}

/*
 * Backend Routines
 */

static int intvalue(cell x) {
	int	of;

	return bignum_to_int(x, &of);
}

static cell flatcopy(cell x, cell *p) {
	cell	n, m;

	n = cons(NIL, NIL);
	save(n);
	while (x != NIL) {
		car(n) = car(x);
		x = cdr(x);
		if (x != NIL) {
			m = cons(NIL, NIL);
			cdr(n) = m;
			n = cdr(n);
		}
	}
	*p = n;
	return unsave(1);
}

static cell rev(cell x) {
	cell	n = NIL;

	for (; x != NIL; x = cdr(x))
		n = cons(car(x), n);
	return n;
}

static cell revb(cell n) {
	cell    m, h;

	if (NIL == n)
		return NIL;
	m = NIL;
	while (n != NIL) {
		h = cdr(n);
		cdr(n) = m;
		m = n;
		n = h;
	}
	return m;
}

static cell rev_string(cell x) {
	cell	s;
	char	*src, *dst;
	int	i, k;

	k = string_len(x)-1;
	s = make_string("", k);
	src = string(x);
	dst = &string(s)[k];
	*dst-- = 0;
	for (i = 0; i < k; i++)
		*dst-- = *src++;
	return s;
}

static cell append(cell a, cell b) {
	cell	n, m;

	if (NIL == a)
		return b;
	if (NIL == b)
		return a;
	n = flatcopy(a, &m);
	cdr(m) = b;
	return n;
}

static cell amend(cell x, cell y) {
	cell	n, v, new, p = NIL;
	int	i, k;

	if (NIL == x || NIL == cdr(x))
		return y;
	n = cons(NIL, NIL);
	save(n);
	v = car(x);
	x = cdr(x);
	i = 0;
	k = -1;
	if (NIL != x) {
		if (!integer_p(car(x))) {
			unsave(1);
			return error("amend: expected integer, got", car(x));
		}
		k = intvalue(car(x));
		x = cdr(x);
	}
	while (y != NIL) {
		if (i == k) {
			car(n) = v;
			if (x != NIL) {
				if (!integer_p(car(x))) {
					unsave(1);
					return
					 error("amend: expected integer, got",
						car(x));
				}
				k = intvalue(car(x));
				x = cdr(x);
			}
		}
		else {
			car(n) = car(y);
		}
		y = cdr(y);
		p = n;
		new = cons(NIL, NIL);
		cdr(n) = new;
		n = cdr(n);
		i++;
	}
	if (k >= i || x != NIL)
		error("amend: range error", make_integer(k));
	n = unsave(1);
	if (p != NIL)
		cdr(p) = NIL;
	else
		n = NIL;
	return n;
}

static cell amend_substring(cell x, cell y) {
	cell	s, v, p;
	int	i, k, kv, r;

	v = car(x);
	x = cdr(x);
	r = 0;
	k = string_len(y)-1;
	kv = string_len(v)-1;
	for (p=x; p != NIL; p = cdr(p)) {
		if (!integer_p(car(p))) {
			unsave(1);
			return error("amend: expected integer, got", car(p));
		}
		i = intvalue(car(p));
		if (i > r) r = i;
		if (i < 0 || r > k)
			return error("amend: range error", car(p));
	}
 	k = (kv+r)>k? kv+r: k;
	s = make_string("", k);
	if (k == 0)
		return s;
	memcpy(string(s), string(y), k);
	save(s);
	while (x != NIL) {
		i = intvalue(car(x));
		memcpy(&string(s)[i], string(v), kv);
		x = cdr(x);
	}
	return unsave(1);
}

static cell amend_string(cell x, cell y) {
	cell	s, v, c;
	int	i, k;

	if (NIL == x || NIL == cdr(x))
		return y;
	v = car(x);
	if (string_p(v))
		return amend_substring(x, y);
	if (!char_p(v))
		return error("amend: expected char, got", v);
	c = char_value(v);
	s = copy_string(y);
	save(s);
	k = string_len(s)-1;
	x = cdr(x);
	while (x != NIL) {
		if (!integer_p(car(x))) {
			unsave(1);
			return error("amend: expected integer, got", car(x));
		}
		i = intvalue(car(x));
		if (i < 0 || i >= k) {
			unsave(1);
			return error("amend: range error", car(x));
		}
		string(s)[i] = c;
		x = cdr(x);
	}
	return unsave(1);
}

static cell amend_in_depth(cell x, cell y, cell v) {
	cell	n, new, p = NIL;
	int	i, k;

	if (NIL == x) {
		if (!atom_p(y))
			return error("amend-in-depth: shape error", y);
		return v;
	}
	if (atom_p(y))
		return error("amend-in-depth: shape error", x);
	n = cons(NIL, NIL);
	save(n);
	i = 0;
	if (!integer_p(car(x))) {
		unsave(1);
		return error("amend-in-depth: expected integer, got", car(x));
	}
	k = intvalue(car(x));
	while (y != NIL) {
		if (i == k) {
			new = amend_in_depth(cdr(x), car(y), v);
			car(n) = new;
		}
		else {
			car(n) = car(y);
		}
		y = cdr(y);
		p = n;
		new = cons(NIL, NIL);
		cdr(n) = new;
		n = cdr(n);
		i++;
	}
	if (k >= i)
		error("amend-in-depth: range error", make_integer(k));
	if (p != NIL)
		cdr(p) = NIL;
	return unsave(1);
}

static cell cut(cell x, cell y) {
	cell	seg, segs, offs, new, cutoff = NIL;
	int	k, i, n, orig;

	if (NIL == x)
		return cons(y, NIL);
	if (integer_p(x))
		x = cons(x, NIL);
	orig = y;
	save(x);
	offs = x;
	segs = cons(NIL, NIL);
	save(segs);
	i = 0;
	while (x != NIL) {
		if (!integer_p(car(x))) {
			unsave(2);
			return error("cut: expected integer, got", car(x));
		}
		k = intvalue(car(x));
		x = cdr(x);
		seg = cons(NIL, NIL);
		save(seg);
		if (i > k)
			return error("cut: range error", offs);
		for (n = 0; i < k; i++, n++) {
			if (NIL == y) {
				unsave(3);
				return error("cut: length error", offs);
			}
			car(seg) = car(y);
			if (i < k-1) {
				new = cons(NIL, NIL);
				cdr(seg) = new;
				seg = cdr(seg);
			}
			y = cdr(y);
		}
		seg = unsave(1);
		if (0 == n)
			seg = NIL;
		car(segs) = seg;
		new = cons(NIL, NIL);
		cutoff = segs;
		cdr(segs) = new;
		segs = cdr(segs);
	}
	if (NIL == orig && cutoff != NIL)
		cdr(cutoff) = NIL;
	else
		car(segs) = y;
	segs = unsave(1);
	unsave(1);
	return segs;
}

static cell cut_string(cell x, cell y) {
	cell	seg, segs, offs, new, orig, cutoff = NIL;
	int	k0, k1, n;

	if (NIL == x)
		return cons(y, NIL);
	if (integer_p(x))
		x = cons(x, NIL);
	orig = y;
	save(x);
	offs = x;
	segs = cons(NIL, NIL);
	save(segs);
	k0 = 0;
	k1 = 0; /*LINT*/
	n = string_len(y);
	while (x != NIL) {
		if (!integer_p(car(x))) {
			unsave(2);
			return error("cut: expected integer, got", car(x));
		}
		k1 = intvalue(car(x));
		x = cdr(x);
		if (k0 > k1 || k1 >= n)
			return error("cut: range error", offs);
		seg = make_string("", k1-k0);
		memcpy(string(seg), string(y)+k0, k1-k0+1);
		string(seg)[k1-k0] = 0;
		car(segs) = seg;
		new = cons(NIL, NIL);
		cutoff = segs;
		cdr(segs) = new;
		segs = cdr(segs);
		k0 = k1;
	}
	if (string_len(orig) < 2 && cutoff != NIL) {
		cdr(cutoff) = NIL;
	}
	else {
		n -= k1;
		new = make_string("", n-1);
		car(segs) = new;
		memcpy(string(car(segs)), string(y)+k1, n);
	}
	segs = unsave(1);
	unsave(1);
	return segs;
}

#define empty_p(x) \
	(NIL == x || (string_p(x) && string_len(x) < 2))

static int false_p(cell x) {
	return  NIL == x ||
		Zero == x ||
		(number_p(x) && real_zero_p(x)) ||
		empty_p(x);
}

static cell drop(cell x, cell y) {
	int	k, r = 0;

	k = intvalue(y);
	if (k < 0) {
		x = rev(x);
		k = -k;
		r = 1;
	}
	save(x);
	while (x != NIL && k--)
		x = cdr(x);
	if (r)
		x = rev(x);
	unsave(1);
	return x;
}

static cell drop_string(cell x, cell y) {
	int	k, n;
	cell	new;

	n = string_len(x)-1;
	k = intvalue(y);
	if (0 == k)
		return x;
	if (k < 0) {
		k = -k;
		if (k > n)
			return make_string("", 0);
		new = make_string("", n - k);
		memcpy(string(new), string(x), n-k+1);
		string(new)[n-k] = 0;
		return new;
	}
	if (k > n)
		return make_string("", 0);
	new = make_string("", n - k);
	memcpy(string(new), string(x)+k, n-k+1);
	return new;
}

static cell expand(cell x) {
	cell	n, new;
	int	k, pos, last = NIL;

	if (NIL == x || (number_p(x) && real_zero_p(x)))
		return NIL;
	if (integer_p(x))
		x = cons(x, NIL);
	save(x);
	n = cons(NIL, NIL);
	save(n);
	pos = 0;
	while (x != NIL) {
		if (!integer_p(car(x)))
			return error("expand: expected integer, got", car(x));
		k = intvalue(car(x));
		if (k < 0)
			return error("expand: range error", car(x));
		while (k) {
			new = make_integer(pos);
			car(n) = new;
			if (k > 0 || cdr(x) != NIL) {
				new = cons(NIL, NIL);
				cdr(n) = new;
				last = n;
				n = cdr(n);
			}
			k--;
		}
		x = cdr(x);
		pos++;
	}
	n = unsave(1);
	unsave(1);
	if (NIL == last)
		return NIL;
	if (last != NIL && NIL == cadr(last))
		cdr(last) = NIL;
	return n;
}

static cell find(cell x, cell y) {
	cell	n, p, new;
	int	k = 0;

	p = NIL;
	n = cons(NIL, NIL);
	save(n);
	while (y != NIL) {
		if (match(x, car(y))) {
			new = make_integer(k);
			car(n) = new;
			new = cons(NIL, NIL);
			cdr(n) = new;
			p = n;
			n = cdr(n);
		}
		y = cdr(y);
		k++;
	}
	n = unsave(1);
	if (p != NIL) {
		cdr(p) = NIL;
		return n;
	}
	else {
		return NIL;
	}
}

static cell find_string(cell x, cell y) {
	cell	n, p, new;
	int	i, k, c;

	c = char_value(x);
	k = string_len(y);
	p = NIL;
	n = cons(NIL, NIL);
	save(n);
	for (i = 0; i < k; i++) {
		if (string(y)[i] == c) {
			new = make_integer(i);
			car(n) = new;
			new = cons(NIL, NIL);
			cdr(n) = new;
			p = n;
			n = cdr(n);
		}
	}
	n = unsave(1);
	if (p != NIL) {
		cdr(p) = NIL;
		return n;
	}
	else {
		return NIL;
	}
}

static cell find_substring(cell x, cell y) {
	cell	n, p, new;
	int	i, k, ks;

	ks = string_len(x)-1;
	k = string_len(y)-1;
	p = NIL;
	n = cons(NIL, NIL);
	save(n);
	for (i = 0; i <= k-ks; i++) {
		if (memcmp(&string(y)[i], string(x), ks) == 0) {
			new = make_integer(i);
			car(n) = new;
			new = cons(NIL, NIL);
			cdr(n) = new;
			p = n;
			n = cdr(n);
		}
	}
	n = unsave(1);
	if (p != NIL) {
		cdr(p) = NIL;
		return n;
	}
	else {
		return NIL;
	}
}

static cell list_to_vector(cell m) {
	cell	n, vec;
	int	k;
	cell	*p;

	k = 0;
	for (n = m; n != NIL; n = cdr(n))
		k++;
	vec = new_vec(T_VECTOR, k*sizeof(cell));
	p = vector(vec);
	for (n = m; n != NIL; n = cdr(n)) {
		*p = car(n);
		p++;
	}
	return vec;
}

static cell string_to_vector(cell s) {
	cell	v, new;
	int	k, i;

	k = string_len(s)-1;
	v = new_vec(T_VECTOR, k*sizeof(cell));
	save(v);
	for (i = 0; i < k; i++) {
		new = make_char(string(s)[i]);
		vector(v)[i] = new;
	}
	unsave(1);
	return v;
}

static cell ndxvec_to_list(cell x) {
	cell	n, new;
	int	k, i;

	k = vector_len(x);
	if (0 == k)
		return NIL;
	n = cons(NIL, NIL);
	save(n);
	for (i=0; i<k; i++) {
		new = make_integer(vector(x)[i]);
		car(n) = new;
		if (i < k-1) {
			new = cons(NIL, NIL);
			cdr(n) = new;
			n = cdr(n);
		}
	}
	n = unsave(1);
	return n;
}

/*
 * Avoid gap sizes of 2^n in shellsort.
 * Hopefully the compiler will optimize the switch.
 * Worst case for shellsort with fixgap() is Theta(n^1.5),
 * meaning
 * 1,000,000,000 steps to sort one million elements
 * instead of
 * 1,000,000,000,000 steps (Theta(n^2)).
 */

static int fixgap(int k) {
	switch (k) {
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
	case 128:
	case 256:
	case 512:
	case 1024:
	case 2048:
	case 4096:
	case 8192:
	case 16384:
	case 32768:
	case 65536:
	case 131072:
	case 262144:
	case 524288:
	case 1048576:
	case 2097152:
	case 4194304:
	case 8388608:
	case 16777216:
	case 33554432:
	case 67108864:
	case 134217728:
	case 268435456:
	case 536870912:
	case 1073741824:return k-1;
	default:	return k;
	}
}

static void shellsort(cell vals, cell ndxs, int count, int (*p)(cell, cell)) {
	int	gap, i, j;
	cell	tmp, *vv, *nv;

	for (gap = fixgap(count/2); gap > 0; gap = fixgap(gap / 2)) {
		for (i = gap; i < count; i++) {
			for (j = i-gap; j >= 0; j -= gap) {
				vv = vector(vals);
				nv = vector(ndxs);
				if (p(vv[nv[j]], vv[nv[j+gap]]))
					break;
				tmp = nv[j];
				nv[j] = nv[j+gap];
				nv[j+gap] = tmp;
			}
		}
	}
}

static cell grade(int (*p)(cell, cell), cell x) {
	cell	vals, ndxs, *v;
	int	i, k;

	vals = list_to_vector(x);
	save(vals);
	k = vector_len(vals);
	/*
	 * allocate vector as string, so it does not get scanned during GC
	 */
	ndxs = new_vec(T_STRING, k * sizeof(cell));
	save(ndxs);
	v = vector(ndxs);
	for (i = 0; i < k; i++)
		v[i] = i;
	shellsort(vals, ndxs, k, p);
	ndxs = car(Stack);
	ndxs = ndxvec_to_list(ndxs);
	unsave(2);
	return ndxs;
}

static cell string_to_list(cell s, cell xnil) {
	cell	n, new;
	int	k, i;

	k = string_len(s) - 1;
	if (k < 1)
		return NIL;
	n = cons(NIL, xnil);
	save(n);
	for (i = 0; i < k; i++) {
		new = make_char(string(s)[i]);
		car(n) = new;
		if (i+1 < k) {
			new = cons(NIL, xnil);
			cdr(n) = new;
			n = cdr(n);
		}
	}
	return unsave(1);
}

static cell list_to_string(cell x) {
	cell	p, n;
	int	k;
	char	*s;

	if (NIL == x)
		return make_string("", 0);
	if (atom_p(x))
		return x;
	k = 0;
	for (p=x; p != NIL; p = cdr(p)) {
		if (!char_p(car(p)))
			return x;
		k++;
	}
	n = make_string("", k);
	s = string(n);
	for (p=x; p != NIL; p = cdr(p))
		*s++ = char_value(car(p));
	*s = 0;
	return n;
}

static void conv_to_strlst(cell x) {
	cell	n;

	while (x != NIL) {
		n = list_to_string(car(x));
		car(x) = n;
		x = cdr(x);
	}
}

static cell group(cell x) {
	cell	n, f, p, g, new, ht;
	int	i;

	if (NIL == x)
		return NIL;
	ht = make_dict(length(x));
	save(ht);
	for (i = 0, n = x; n != NIL; n = cdr(n), i++) {
		p = dict_lookup(ht, car(n));
		p = cons(make_integer(i), atom_p(p)? NIL: cadr(p));
		ht = dict_add(ht, car(n), p);
		car(Stack) = ht;
		if (undefined_p(ht)) {
			unsave(1);
			return UNDEFINED;
		}
	}
	g = cons(NIL, NIL);
	save(g);
	p = g;
	for (n = x; n != NIL; n = cdr(n)) {
		f = dict_lookup(ht, car(n));
		if (f != UNDEFINED) {
			car(g) = revb(cadr(f));
			new = cons(NIL, NIL);
			cdr(g) = new;
			p = g;
			g = cdr(g);
			dict_remove(ht, car(n));
		}
	}
	cdr(p) = NIL;
	x = unsave(1);
	unsave(1);
	return x;
}

static cell group_string(cell x) {
	cell	n;

	n = string_to_list(x, NIL);
	save(n);
	n = group(n);
	unsave(1);
	return n;
}

static cell join(cell y, cell x) {
	cell	n, p;

	if (list_p(x)) {
		if (list_p(y))
			x = append(y, x);
		else
			x = cons(y, x);
		return x;
	}
	if (list_p(y)) {
		if (NIL == y) {
			x = cons(x, NIL);
		}
		else {
			y = flatcopy(y, &p);
			save(y);
			x = cons(x, NIL);
			cdr(p) = x;
			unsave(1);
			x = y;
		}
		return x;
	}
	if (char_p(x) && char_p(y)) {
		n = make_string("", 2);
		string(n)[1] = char_value(x);
		string(n)[0] = char_value(y);
		return n;
	}
	if (string_p(x) && char_p(y)) {
		n = make_string("", string_len(x));
		memcpy(string(n) + 1, string(x), string_len(x));
		string(n)[0] = intvalue(y);
		return n;
	}
	if (char_p(x) && string_p(y)) {
		n = make_string("", string_len(y));
		memcpy(string(n), string(y), string_len(y));
		string(n)[string_len(y)-1] = intvalue(x);
		return n;
	}
	if (string_p(x) && string_p(y)) {
		n = make_string("", string_len(x) + string_len(y)-2);
		memcpy(string(n), string(y), string_len(y));
		memcpy(string(n) + string_len(y)-1, string(x), string_len(x));
		return n;
	}
	n = cons(x, NIL);
	n = cons(y, n);
	return n;
}

static int less(cell x, cell y) {
	if (number_p(x)) {
		if (!number_p(y))
			return error("grade: expected number, got", y);
		return real_less_p(x, y);
	}
	if (symbol_p(x)) {
		if (!symbol_p(y))
			return error("grade: expected symbol, got", y);
		return strcmp(symbol_name(x), symbol_name(y)) < 0;
	}
	if (char_p(x)) {
		if (!char_p(y))
			return error("grade: expected char, got", y);
		return char_value(x) < char_value(y);
	}
	if (string_p(x)) {
		if (!string_p(y))
			return error("grade: expected string, got", y);
		return strcmp(string(x), string(y)) < 0;
	}
	if (!list_p(x))
		return error("grade: expected list, got", x);
	if (!list_p(y))
		return error("grade: expected list, got", y);
	while (x != NIL && y != NIL) {
		if (less(car(x), car(y)))
			return 1;
		if (less(car(y), car(x)))
			return 0;
		x = cdr(x);
		y = cdr(y);
	}
	return y != NIL;
}

static int more(cell x, cell y) {
	return less(y, x);
}

static cell ndx(cell x, int k, cell y) {
	int	i;
	cell	n, new;
	cell	*v;

	if (integer_p(y)) {
		i = intvalue(y);
		if (i < 0 || i >= k)
			return error("index: range error", y);
		v = vector(x);
		return v[i];
	}
	if (NIL == y)
		return NIL;
	if (atom_p(y))
		return error("index: expected integer, got", y);
	n = cons(NIL, NIL);
	save(n);
	for (; y != NIL; y = cdr(y)) {
		new = ndx(x, k, car(y));
		car(n) = new;
		if (cdr(y) != NIL) {
			new = cons(NIL, NIL);
			cdr(n) = new;
			n = cdr(n);
		}
	}
	n = unsave(1);
	return n;
}

static cell ndx_in_depth(cell x, cell y) {
	cell	p;
	int	k;

	if (atom_p(y))
		y = cons(y, NIL);
	save(y);
	p = x;
	while (NIL != y) {
		if (!integer_p(car(y))) {
			unsave(1);
			return error("index-in-depth: expected integer, got",
					car(y));
		}
		k = intvalue(car(y));
		if (atom_p(p)) {
			unsave(1);
			return error("index-in-depth: shape error", p);
		}
		while (k > 0 && p != NIL) {
			p = cdr(p);
			k--;
		}
		if (atom_p(p)) {
			unsave(1);
			return error("index-in-depth: shape error", p);
		}
		p = car(p);
		y = cdr(y);
	}
	unsave(1);
	return p;
}

static cell reshape3(cell shape, cell *next, cell src) {
	int	i, k, str = 1;
	cell	n, new;

	if (NIL == shape) {
		if (NIL == *next)
			*next = src;
		n = car(*next);
		*next = cdr(*next);
		return n;
	}
	n = cons(NIL, NIL);
	save(n);
	if (!integer_p(car(shape))) {
		unsave(1);
		return error("reshape: expected integer, got", car(shape));
	}
	k = intvalue(car(shape));
	if (-1 == k) {
		k = length(src)/2;
	}
	if (k < 1) {
		unsave(1);
		return error("reshape: range error", car(shape));
	}
	for (i = 0; i < k; i++) {
		new = reshape3(cdr(shape), next, src);
		if (!char_p(new))
			str = 0;
		car(n) = new;
		if (i < k - 1) {
			new = cons(NIL, NIL);
			cdr(n) = new;
			n = cdr(n);
		}
	}
	n = unsave(1);
	if (str) {
		save(n);
		n = list_to_string(n);
		unsave(1);
	}
	return n;
}

static cell joinstrs(cell x) {
	cell	n, last = NIL, new;

	n = cons(NIL, NIL);
	save(n);
	while (x != NIL) {
		car(n) = car(x);
		if (string_p(car(x))) {
			while (cdr(x) != NIL && string_p(cadr(x))) {
				new = join(car(n), cadr(x));
				car(n) = new;
				x = cdr(x);
			}
		}
		new = cons(NIL, NIL);
		cdr(n) = new;
		last = n;
		n = cdr(n);
		x = cdr(x);
	}
	n = unsave(1);
	if (NIL == last) {
		n = NIL;
	}
	else {
		cdr(last) = NIL;
		if (string_p(car(n)) && NIL == cdr(n))
			n = car(n);
	}
	return n;
}

static cell range(cell x) {
	cell	n, m, p, new;
	int	i, k,str = 0;

	if (list_p(x))
		n = group(x);
	else
		n = group_string(x);
	if (NIL == n)
		return string_p(x)? make_string("", 0): NIL;
	save(n);
	if (!atom_p(n))
		for (p = n; p != NIL; p = cdr(p))
			car(p) = caar(p);
	k = intvalue(car(n));
	if (string_p(x)) {
		x = string_to_list(x, NIL);
		str = 1;
	}
	save(x);
	m = cons(NIL, NIL);
	save(m);
	for (i = 0, p = x; p != NIL; p = cdr(p), i++) {
		if (i == k) {
			car(m) = car(p);
			n = cdr(n);
			if (NIL == n)
				break;
			k = intvalue(car(n));
			new = cons(NIL, NIL);
			cdr(m) = new;
			m = cdr(m);
		}
	}
	n = unsave(1);
	if (str) {
		car(Stack) = n;
		n = list_to_string(n);
	}
	unsave(2);
	return n;
}

static cell reshape(cell x, cell y) {
	if (atom_p(x))
		x = cons(x, NIL);
	save(x);
	car(Stack) = x;
	x = joinstrs(x);
	car(Stack) = x;
	if (string_p(x)) {
		x = string_to_list(x, NIL);
		car(Stack) = x;
	}
	if (atom_p(y))
		y = cons(y, NIL);
	save(y);
	x = reshape3(y, &x, x);
	unsave(2);
	return x;
}

static cell take(cell x, int k) {
	cell	a, n, m;
	int	r = 0;

	if (NIL == x || 0 == k)
		return NIL;
	a = x;
	if (k < 0) {
		a = x = rev(x);
		k = -k;
		r = 1;
	}
	save(x);
	n = cons(NIL, NIL);
	save(n);
	while (k--) {
		if (NIL == x)
			x = a;
		car(n) = car(x);
		x = cdr(x);
		if (k) {
			m = cons(NIL, NIL);
			cdr(n) = m;
			n = cdr(n);
		}
	}
	x = car(Stack);
	if (r) {
		x = rev(x);
	}
	unsave(2);
	return x;
}

static cell take_string(cell x, int k) {
	cell	n;
	int	len, i, j;

	len = string_len(x) - 1;
	if (len < 1 || 0 == k)
		return make_string("", 0);
	n = make_string("", abs(k));
	if (k > 0) {
		j = 0;
		for (i = 0; i < k; i++) {
			string(n)[i] = string(x)[j];
			if (++j >= len)
				j = 0;
		}
	}
	else {
		k = -k;
		j = len-1;
		for (i = k-1; i >= 0; i--) {
			string(n)[i] = string(x)[j];
			if (--j < 0)
				j = len-1;
		}
	}
	return n;
}

static cell rotate(cell x, cell y) {
	int	k, rot;
	cell	n, m, p;

	if (NIL == x)
		return NIL;
	k = length(x);
	if (k < 2)
		return x;
	rot = intvalue(y) % k;
	if (rot > 0) {
		n = take(x, -rot);
		save(n);
		m = take(x, k-rot);
		for (p = n; cdr(p) != NIL; p = cdr(p))
			;
		cdr(p) = m;
		unsave(1);
		return n;
	}
	else {
		n = take(x, -rot);
		save(n);
		for (p = x; rot++; p = cdr(p))
			;
		p = flatcopy(p, &m);
		cdr(m) = n;
		unsave(1);
		return p;
	}
}

static cell rotate_string(cell x, cell y) {
	int	k, rot;
	cell	n;

	k = string_len(x) - 1;
	if (0 == k)
		return x;
	rot = intvalue(y) % k;
	if (k < 2 || 0 == rot)
		return x;
	if (rot > 0) {
		n = make_string("", k);
		memcpy(string(n), string(x)+k-rot, rot);
		memcpy(string(n)+rot, string(x), k-rot);
		return n;
	}
	else {
		rot = -rot;
		n = make_string("", k);
		memcpy(string(n), string(x)+rot, k-rot);
		memcpy(string(n)+k-rot, string(x), rot);
		return n;
	}
}

static cell eqv_p(cell a, cell b) {
	if (a == b)
		return 1;
	if (number_p(a) && number_p(b))
		return real_equal_p(a, b);
	return 0;
}

static cell common_prefix(cell x, cell y) {
	cell	n, p, new;

	if (atom_p(x) || atom_p(y))
		return NIL;
	p = n = cons(NIL, NIL);
	save(n);
	while (x != NIL && y != NIL) {
		if (!eqv_p(car(x), car(y)))
			break;
		car(n) = car(x);
		p = n;
		new = cons(NIL, NIL);
		cdr(n) = new;
		n = cdr(n);
		x = cdr(x);
		y = cdr(y);
	}
	cdr(p) = NIL;
	n = unsave(1);
	return NIL == car(n)? NIL: n;
}

static cell shape(cell x) {
	cell	s, s2, p, new;

	if (string_p(x))
		return cons(make_integer(string_len(x)-1), NIL);
	if (atom_p(x))
		return Zero;
	if (NIL == cdr(x))
		return cons(One, NIL);
	s = shape(car(x));
	save(s);
	for (p = cdr(x); p != NIL; p = cdr(p)) {
		s2 = shape(car(p));
		save(s2);
		new = common_prefix(s, s2);
		cadr(Stack) = new;
		unsave(1);
		s = car(Stack);
	}
	s2 = make_integer(length(x));
	s = unsave(1);
	if (!bignum_equal_p(s2, One))
		s = cons(s2, s);
	return s;
}

static cell split(cell x, cell y) {
	cell	grp;
	cell	n, m, new;
	int	i = -1, k = -1;

	if (NIL == x)
		return NIL;
	if (integer_p(y))
		y = cons(y, NIL);
	save(y);
	grp = y;
	n = cons(NIL, NIL);
	save(n);
	m = cons(NIL, NIL);
	save(m);
	while (x != NIL) {
		if (i >= k) {
			if (!integer_p(car(y))) {
				unsave(3);
				return error("split: expected integer, got",
						car(y));
			}
			if (i >= 0) {
				car(n) = unsave(1);
				m = cons(NIL, NIL);
				save(m);
				new = cons(NIL, NIL);
				cdr(n) = new;
				n = cdr(n);
				y = cdr(y);
				if (NIL == y)
					y = grp;
			}
			k = intvalue(car(y));
			if (k < 1) {
				unsave(3);
				return error("split: range error",
						make_integer(k));
			}
			i = 0;
		}
		car(m) = car(x);
		if (cdr(x) != NIL && i < k-1) {
			new = cons(NIL, NIL);
			cdr(m) = new;
			m = cdr(m);
		}
		x = cdr(x);
		i++;
	}
	car(n) = unsave(1);
	n = unsave(1);
	unsave(1);
	return n;
}

static cell split_string(cell x, cell y) {
	cell	grp;
	cell	n, new;
	int	i, k, len;

	len = string_len(x) - 1;
	if (len < 1)
		return NIL;
	if (integer_p(y))
		y = cons(y, NIL);
	save(y);
	grp = y;
	n = cons(NIL, NIL);
	save(n);
	i = 0;
	while (i < len) {
		if (!integer_p(car(y))) {
			unsave(2);
			return error("split: expected integer, got", car(y));
		}
		k = intvalue(car(y));
		if (k < 1) {
			unsave(3);
			return error("split: range error", car(y));
		}
		y = cdr(y);
		if (NIL == y)
			y = grp;
		if (k > len-i)
			k = len-i;
		new = make_string("", k);
		memcpy(string(new), string(x)+i, k);
		string(new)[k] = 0;
		car(n) = new;
		i += k;
		if (i < len) {
			new = cons(NIL, NIL);
			cdr(n) = new;
			n = cdr(n);
		}
	}
	n = unsave(1);
	unsave(1);
	return n;
}

static int anylist(cell x) {
	cell    p; 

	for (p=x; p != NIL; p = cdr(p))
		if (pair_p(car(p)))
			return 1; 
	return 0;
}

static int anynil(cell x) {
	cell    p; 

	for (p=x; p != NIL; p = cdr(p))
		if (NIL == car(p))
			return 1; 
	return 0;
}

static cell transpose(cell x) {
	cell	n, m, p, q, dummy, new;

	if (atom_p(x) || !anylist(x))
		return x;
	save(flatcopy(x, &dummy));
	n = cons(NIL, NIL);
	save(n);
	for (;;) {
		if (anynil(cadr(Stack)))
			break;
		m = cons(NIL, NIL);
		save(m);
		for (p=caddr(Stack); p != NIL; p = cdr(p)) {
			q = car(p);
			if (atom_p(q)) {
				car(m) = q;
			}
			else {
				car(m) = car(q);
				car(p) = cdar(p);
			}
			if (cdr(p) != NIL) {
				new = cons(NIL, NIL);
				cdr(m) = new;
				m = cdr(m);
			}
		}
		m = unsave(1);
		car(n) = m;
		if (anynil(cadr(Stack)))
			break;
		new = cons(NIL, NIL);
		cdr(n) = new;
		n = cdr(n);
	}
	n = unsave(1);
	if (anylist(unsave(1)))
		error("transpose: shape error", x);
	return n;
}

/*
 * Virtual Machine
 */

static void push(cell x) {
	Dstack = cons(x, Dstack);
}

static cell pop(void) {
	cell	n;

	if (NIL == Dstack)
		return error("stack underflow", VOID);
	n = car(Dstack);
	Dstack = cdr(Dstack);
	return n;
}

static cell	(*F2)(cell x, cell y);
static cell	(*F1)(cell x);
static char	*N, B[100];

static cell rec2(cell x, cell y) {
	cell	n, p, q, new;

	if (atom_p(x) && atom_p(y))
		return (*F2)(x, y);
	n = cons(NIL, NIL);
	save(n);
	if (list_p(x) && list_p(y)) {
		for (p=x, q=y; p != NIL && q != NIL; p = cdr(p), q = cdr(q)) {
			new = rec2(car(p), car(q));
			car(n) = new;
			if (cdr(p) != NIL) {
				new = cons(NIL, NIL);
				cdr(n) = new;
				n = cdr(n);
			}
		}
		if (p != NIL || q != NIL) {
			n = cons(y, NIL);
			n = cons(x, n);
			unsave(1);
			sprintf(B, "%s: shape error", N);
			return error(B, n);
		}
	}
	else if (list_p(x)) {
		for (p=x; p != NIL; p = cdr(p)) {
			new = rec2(car(p), y);
			car(n) = new;
			if (cdr(p) != NIL) {
				new = cons(NIL, NIL);
				cdr(n) = new;
				n = cdr(n);
			}
		}
	}
	else if (list_p(y)) {
		for (p=y; p != NIL; p = cdr(p)) {
			new = rec2(x, car(p));
			car(n) = new;
			if (cdr(p) != NIL) {
				new = cons(NIL, NIL);
				cdr(n) = new;
				n = cdr(n);
			}
		}
	}
	return unsave(1);
}

static void dyadrec(char *s, cell (*f)(cell, cell), cell x, cell y) {
	cell	r;

	N = s;
	F2 = f;
	r = rec2(x, y);
	Dstack = cdr(Dstack);
	car(Dstack) = r;
}

static cell rec1(cell x) {
	cell	n, p, new;

	if (atom_p(x))
		return (*F1)(x);
	n = cons(NIL, NIL);
	save(n);
	for (p=x; p != NIL; p = cdr(p)) {
		new = rec1(car(p));
		car(n) = new;
		if (cdr(p) != NIL) {
			new = cons(NIL, NIL);
			cdr(n) = new;
			n = cdr(n);
		}
	}
	return unsave(1);
}

static void monadrec(char *s, cell (*f)(cell), cell x) {
	cell	r;

	N = s;
	F1 = f;
	r = rec1(x);
	car(Dstack) = r;
}

static void save_vars(cell vars) {
	cell	n, v;

	for (v = vars; v != NIL; v = cdr(v)) {
		if (!variable_p(car(v))) {
			error("non-variable in variable list", vars);
			return;
		}
		n = cons(var_value(car(v)), car(Locals));
		var_value(car(v)) = NO_VALUE;
		car(Locals) = n;
	}
	n = cons(rev(vars), car(Locals));
	car(Locals) = n;
}

static void unsave_vars(void) {
	cell	v, a;

	var_value(S_thisfn) = caar(Locals);
	car(Locals) = cdar(Locals);
	if (car(Locals) != NIL) {
		v = caar(Locals);
		for (a = cdar(Locals); a != NIL; a = cdr(a)) {
			var_value(car(v)) = car(a);
			v = cdr(v);
		}
	}
	Locals = cdr(Locals);
}

#define ONE_ARG(name) \
	if (NIL == Dstack) { \
		error("too few arguments", make_variable(name, NIL)); \
		return; \
	}

#define TWO_ARGS(name) \
	if (NIL == Dstack || NIL == cdr(Dstack)) { \
		error("too few arguments", make_variable(name, NIL)); \
		return; \
	}

#define THREE_ARGS(name) \
	if (NIL == Dstack || NIL == cdr(Dstack) || NIL == cddr(Dstack)) { \
		error("too few arguments", make_variable(name, NIL)); \
		return; \
	}

static void unknown1(char *name) {
	cell	n;
	char	b[100];

	n = cons(car(Dstack), NIL);
	sprintf(b, "%s: type error", name);
	error(b, n);
}

static void unknown2(char *name) {
	cell	n;
	char	b[100];

	n = cons(car(Dstack), NIL);
	n = cons(cadr(Dstack), n);
	sprintf(b, "%s: type error", name);
	error(b, n);
}

static void unknown3(char *name) {
	cell	n;
	char	b[100];

	n = cons(car(Dstack), NIL);
	n = cons(cadr(Dstack), n);
	n = cons(caddr(Dstack), n);
	sprintf(b, "%s: type error", name);
	error(b, n);
}

static void binop(cell (*op)(cell, cell), cell x, cell y) {
	cell	r;

	r = op(y, x);
	pop();
	car(Dstack) = r;
}

/*
 * Operators (primitive functions)
 */

static void op_amend(void) {
	cell	x, y;

	TWO_ARGS("amend")
	y = cadr(Dstack);
	x = car(Dstack);
	if (list_p(y) && list_p(x)) {
		x = amend(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	if (string_p(y) && list_p(x)) {
		x = amend_string(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("amend");
}

static void op_amendd(void) {
	cell	x, y;

	TWO_ARGS("amendd")
	y = cadr(Dstack);
	x = car(Dstack);
	if (pair_p(y) && pair_p(x)) {
		x = amend_in_depth(cdr(x), y, car(x));
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("amendd");
}

static void op_apply(void) {
	cell	x, y;
	cell	n, m, f;
	int	na;

	TWO_ARGS("apply")
	y = cadr(Dstack);
	x = car(Dstack);
	if (function_p(y)) {
		if (!list_p(x)) {
			x = cons(x, NIL);
			car(Dstack) = x;
		}
		na = fun_arity(y);
		if (na != length(x)) {
			error("apply: wrong number of arguments", x);
			return;
		}
		if (NIL == x) {
			n = cons(S_pop0, NIL);
			n = cons(S_call0, n);
			n = cons(y, n);
			Dstack = cdr(Dstack);
			car(Dstack) = n;
		}
		else {
			f = flatcopy(x, &m);
			Dstack = cdr(Dstack);
			car(Dstack) = f;
			n = cons(1==na? S_pop1: 2==na? S_pop2: S_pop3, NIL);
			n = cons(1==na? S_call1: 2==na? S_call2: S_call3, n);
			n = cons(y, n);
			cdr(m) = n;
		}
		State = S_APPLY;
		return;
	}
	unknown2("apply");
}

static void op_atom(void) {
	cell	x;

	ONE_ARG("atom");
	x = car(Dstack);
	car(Dstack) = string_p(x) && !empty_p(x)? Zero: atom_p(x)? One: Zero;
}

#define has_locals(x) \
	(list_p(car(x)) && list_p(cdr(x)) && S_clear == cadr(x))

static void call(int arity) {
	cell	x, y, z, f, n;
	char	name[] = "call";

	if (3 == arity) {
		if (	NIL == Dstack ||
			NIL == cdr(Dstack) ||
			NIL == cddr(Dstack) ||
			NIL == cdddr(Dstack)
		) {
			error("too few arguments", make_variable(name, NIL));
			return;
		}
		y = cdr(Dstack);
		z = cdddr(Dstack);
		x = car(z);
		car(z) = car(y);
		car(y) = x;
	}
	else if (2 == arity) {
		THREE_ARGS(name);
		y = cdr(Dstack);
		z = cddr(Dstack);
		x = car(z);
		car(z) = car(y);
		car(y) = x;
	}
	else if (1 == arity) {
		TWO_ARGS(name);
	}
	else {
		ONE_ARG(name);
	}
	x = car(Dstack);
	if (function_p(x)) {
		if (fun_arity(x) != arity) {
			error("wrong arity", x);
			return;
		}
		Locals = cons(NIL, Locals);
		f = x;
		x = fun_body(x);
		if (has_locals(x)) {
			save_vars(car(x));
			x = cddr(x);
		}
		n = cons(var_value(S_thisfn), car(Locals));
		car(Locals) = n;
		var_value(S_thisfn) = f;
		car(Dstack) = x;
		State = S_APPLY;
		return;
	}
	unknown1(name);
}

static void op_call0(void) { call(0); }
static void op_call1(void) { call(1); }
static void op_call2(void) { call(2); }
static void op_call3(void) { call(3); }

static cell safe_char(cell x) {
	int	k;

	if (integer_p(x)) {
		k = intvalue(x);
		if (k < 0 || k > 255)
			return error("char: domain error", x);
		return make_char(k);
	}
	return error("char: type error", cons(x, NIL));
}

static void op_char(void) {
	cell	x;

	ONE_ARG("char");
	x = car(Dstack);
	monadrec("char", safe_char, x);
}

static void op_clear(void) {
	pop();
}

static void conv(int s) {
	cell	x, y, n;

	TWO_ARGS("converge")
	y = cadr(Dstack);
	x = car(Dstack);
	if (function_p(x)) {
		if (S_CONV == s)
			push(y);
		else
			push(cons(y, NIL));
		push(Barrier);
		n = cons(x, NIL);
		push(cons(y, n));
		State = s;
		return;
	}
	unknown2("converge");
}

static void op_conv(void) {
	conv(S_CONV);
}

static void op_cut(void) {
	cell	x, y;

	TWO_ARGS("cut")
	y = cadr(Dstack);
	x = car(Dstack);
	if ((list_p(y) || integer_p(y)) && list_p(x)) {
		binop(cut, x, y);
		return;
	}
	if ((list_p(y) || integer_p(y)) && string_p(x)) {
		binop(cut_string, x, y);
		return;
	}
	unknown2("cut");
}

static void op_def(void) {
	cell	x, y, n;
	char	name[TOKEN_LENGTH+1];

	TWO_ARGS("define")
	y = cadr(Dstack);
	x = car(Dstack);
	if (symbol_p(y)) {
		y = make_variable(symbol_name(y), NIL);
		var_value(y) = x;
		if (Module != UNDEFINED) {
			Module = cons(y, Module);
			strcpy(name, var_name(y));
			mkglobal(name);
			n = make_variable(name, NIL);
			var_value(n) = var_value(y);
		}
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("define");
}

static cell safe_div(cell x, cell y) {
	if (number_p(x) && number_p(y)) {
		return real_divide(x, y);
	}
	return error("divide: type error", cons(x, cons(y, NIL)));
}

static void op_div(void) {
	cell	x, y;

	TWO_ARGS("divide")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("divide", safe_div, y, x);
}

static void op_down(void) {
	cell	x;

	ONE_ARG("grade-down");
	x = car(Dstack);
	if (list_p(x)) {
		x = grade(more, x);
		car(Dstack) = x;
		return;
	}
	if (string_p(x)) {
		x = string_to_list(x, NIL);
		car(Dstack) = x;
		x = grade(more, x);
		car(Dstack) = x;
		return;
	}
	unknown1("grade-down");
}

static void op_drop(void) {
	cell	x, y;

	TWO_ARGS("drop")
	y = cadr(Dstack);
	x = car(Dstack);
	if (integer_p(y) && list_p(x)) {
		binop(drop, y, x);
		return;
	}
	if (integer_p(y) && string_p(x)) {
		binop(drop_string, y, x);
		return;
	}
	if (dictionary_p(x)) {
		x = dict_remove(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("drop");
}

static void op_each(void) {
	cell	x, y, n;

	TWO_ARGS("each")
	y = cadr(Dstack);
	x = car(Dstack);
	if ((list_p(y) || string_p(y)) && function_p(x)) {
		if (NIL == y || empty_p(y)) {
			Dstack = cdr(Dstack);
			car(Dstack) = y;
		}
		else {
			push(NIL);
			push(Barrier);
			n = cons(x, NIL);
			if (string_p(y)) {
				save(n);
				push(cons(make_char(string(y)[0]), n));
				unsave(1);
				y = string_to_list(y, STRING_NIL);
			}
			else {
				push(cons(car(y), n));
			}
			cadr(cdddr(Dstack)) = cdr(y);
			State = S_EACH;
		}
		return;
	}
	if (dictionary_p(y) && function_p(x)) {
		x = dict_to_list(y);
		cadr(Dstack) = x;
		op_each();
		return;
	}
	if (function_p(x)) {
		n = cons(x, NIL);
		n = cons(y, n);
		Dstack = cdr(Dstack);
		car(Dstack) = n;
		State = S_APPLY;
		return;
	}
	unknown2("each");
}

static void op_each2(void) {
	cell	x, y, z, n;

	THREE_ARGS("each-2")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if (	(list_p(z) || string_p(z)) &&
		(list_p(y) || string_p(y)) &&
		function_p(x)
	) {
		if (empty_p(y) || empty_p(z)) {
			Dstack = cddr(Dstack);
			if (NIL == y || NIL == z)
				x = NIL;
			else
				x = make_string("", 0);
			car(Dstack) = x;
		}
		else {
			if (string_p(y)) {
				y = string_to_list(y, STRING_NIL);
				cadr(Dstack) = y;
			}
			if (string_p(z)) {
				z = string_to_list(z, STRING_NIL);
				caddr(Dstack) = z;
			}
			push(NIL);
			push(Barrier);
			n = cons(x, NIL);
			n = cons(car(y), n);
			push(cons(car(z), n));
			cadr(cdddr(Dstack)) = cdr(y);
			caddr(cdddr(Dstack)) = cdr(z);
			State = S_EACH2;
		}
		return;
	}
	if (function_p(x)) {
		n = cons(x, NIL);
		n = cons(y, n);
		n = cons(z, n);
		Dstack = cddr(Dstack);
		car(Dstack) = n;
		State = S_APPLY;
		return;
	}
	unknown3("each-2");
}

static void op_eachl(void) {
	cell	x, y, z, n;

	THREE_ARGS("each-left")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if ((list_p(y) || string_p(y)) && function_p(x)) {
		if (NIL == y || empty_p(y)) {
			Dstack = cddr(Dstack);
			car(Dstack) = y;
		}
		else {
			if (string_p(y)) {
				y = string_to_list(y, STRING_NIL);
				cadr(Dstack) = y;
			}
			push(NIL);
			push(Barrier);
			n = cons(x, NIL);
			n = cons(car(y), n);
			push(cons(z, n));
			cadr(cdddr(Dstack)) = cdr(y);
			State = S_EACHL;
		}
		return;
	}
	if (function_p(x)) {
		n = cons(x, NIL);
		n = cons(y, n);
		n = cons(z, n);
		Dstack = cddr(Dstack);
		car(Dstack) = n;
		State = S_APPLY;
		return;
	}
	unknown3("each-left");
}

static void op_eachp(void) {
	cell	x, y, n;

	TWO_ARGS("each-pair")
	y = cadr(Dstack);
	x = car(Dstack);
	if ((list_p(y) || string_p(y)) && function_p(x)) {
		if (	NIL == y || NIL == cdr(y) ||
			empty_p(y) || (string_p(y) && string_len(y) < 3)
		) {
			Dstack = cdr(Dstack);
			car(Dstack) = y;
		}
		else {
			if (string_p(y)) {
				y = string_to_list(y, STRING_NIL);
				cadr(Dstack) = y;
			}
			push(NIL);
			push(Barrier);
			n = cons(x, NIL);
			n = cons(cadr(y), n);
			push(cons(car(y), n));
			cadr(cdddr(Dstack)) = cdr(y);
			State = S_EACHP;
		}
		return;
	}
	Dstack = cdr(Dstack);
	car(Dstack) = y;
}

static void op_eachr(void) {
	cell	x, y, z, n;

	THREE_ARGS("each-right")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if ((list_p(y) || string_p(y)) && function_p(x)) {
		if (NIL == y || empty_p(y)) {
			Dstack = cddr(Dstack);
			car(Dstack) = y;
		}
		else {
			if (string_p(y)) {
				y = string_to_list(y, STRING_NIL);
				cadr(Dstack) = y;
			}
			push(NIL);
			push(Barrier);
			n = cons(x, NIL);
			n = cons(z, n);
			push(cons(car(y), n));
			cadr(cdddr(Dstack)) = cdr(y);
			State = S_EACHR;
		}
		return;
	}
	if (function_p(x)) {
		n = cons(x, NIL);
		n = cons(z, n);
		n = cons(y, n);
		Dstack = cddr(Dstack);
		car(Dstack) = n;
		State = S_APPLY;
		return;
	}
	unknown3("each-right");
}

static void op_enum(void) {
	cell	x, n, new;
	int	i, k;

	ONE_ARG("enumerate");
	x = car(Dstack);
	if (integer_p(x)) {
		k = intvalue(x);
		if (k < 0) {
			error("enumerate: domain error", x);
			return;
		}
		if (0 == k) {
			car(Dstack) = NIL;
			return;
		}
		n = cons(NIL, NIL);
		save(n);
		for (i = 0; i < k; i++) {
			new = make_integer(i);
			car(n) = new;
			if (i < k-1) {
				new = cons(NIL, NIL);
				cdr(n) = new;
				n = cdr(n);
			}
		}
		car(Dstack) = unsave(1);
		return;
	}
	unknown1("enumerate");
}

static int compare(char *name,
	    int (*ncmp)(cell a, cell b),
	    int (*ccmp)(cell a, cell b),
	    int (*scmp)(char *s1, char *s2, int k1, int k2),
	    cell x, cell y
) {
	char	*sx, *sy;
	int	kx, ky;

	if (number_p(y) && number_p(x))
		return ncmp(y, x);
	if (char_p(y) && char_p(x))
		return ccmp(y, x);
	if (variable_p(x))
		return compare(name, ncmp, ccmp, scmp, var_symbol(x), y);
	if (variable_p(y))
		return compare(name, ncmp, ccmp, scmp, x, var_symbol(y));
	if ((string_p(x) && string_p(y)) || (symbol_p(x) && symbol_p(y))) {
		sx = string_p(x)? string(x): symbol_name(x);
		sy = string_p(y)? string(y): symbol_name(y);
		kx = string_p(x)? string_len(x): symbol_len(x);
		ky = string_p(y)? string_len(y): symbol_len(y);
		return scmp(sy, sx, ky, kx);
	}
	unknown2(name);
	return 0;
}

static int str_equal_p(char *s1, char *s2, int k1, int k2) {
	return k1 == k2 && memcmp(s1, s2, k1) == 0;
}

static int str_less_p(char *s1, char *s2, int k1, int k2) {
	int	k = k1 < k2? k1: k2;

	return memcmp(s1, s2, k) < 0;
}

static int chr_equal_p(cell x, cell y) {
	return char_value(x) == char_value(y);
}

static int chr_less_p(cell x, cell y) {
	return char_value(x) < char_value(y);
}

static cell safe_eq_p(cell x, cell y) {
	return compare("equal", real_equal_p, chr_equal_p, str_equal_p, y, x)?
			One: Zero;
}

static void op_eq(void) {
	cell	x, y;

	TWO_ARGS("equal")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("equal", safe_eq_p, y, x);
}

static cell safe_gt_p(cell x, cell y) {
	return compare("more", real_less_p, chr_less_p, str_less_p, x, y)?
			One: Zero;
}

static void op_gt(void) {
	cell	x, y;

	TWO_ARGS("more")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("more", safe_gt_p, y, x);
}

static cell safe_lt_p(cell x, cell y) {
	return compare("less", real_less_p, chr_less_p, str_less_p, y, x)?
			One: Zero;
}

static void op_lt(void) {
	cell	x, y;

	TWO_ARGS("less")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("less", safe_lt_p, y, x);
}

static void op_expand(void) {
	cell	x;

	ONE_ARG("expand");
	x = car(Dstack);
	if (integer_p(x) || list_p(x)) {
		x = expand(x);
		car(Dstack) = x;
		return;
	}
	unknown1("expand");
}

static void op_find(void) {
	cell	x, y, n;

	TWO_ARGS("find")
	y = cadr(Dstack);
	x = car(Dstack);
	if (list_p(y)) {
		n = find(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = n;
		return;
	}
	if (string_p(y) && char_p(x)) {
		n = find_string(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = n;
		return;
	}
	if (string_p(y) && string_p(x)) {
		n = find_substring(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = n;
		return;
	}
	if (dictionary_p(y) && !dictionary_p(x)) {
		x = dict_lookup(y, x);
		Dstack = cdr(Dstack);
		car(Dstack) = UNDEFINED==x? x: cadr(x);
		return;
	}
	unknown2("find");
}

static cell safe_floor(cell x) {
	if (integer_p(x)) return x;
	if (number_p(x)) {
		x = real_floor(x);
		if (real_exponent(x) < S9_MANTISSA_SIZE)
			x = real_to_bignum(x);
		return x;
	}
	return error("floor: type error", cons(x, NIL));
}

static void op_floor(void) {
	cell	x;

	ONE_ARG("floor");
	x = car(Dstack);
	monadrec("floor", safe_floor, x);
}

static int intpart(cell x) {
	x = real_floor(x);
	save(x);
	x = real_to_bignum(x);
	unsave(1);
	return intvalue(x);
}

static int fracpart(cell x) {
	cell	n;

	n = real_floor(x);
	save(n);
	n = real_subtract(x, n);
	n = real_mantissa(n);
	unsave(1);
	return intvalue(n);
}

static cell form(cell x, cell proto) {
	#define L	1024
	char	*s, *p, buf[L];
	cell	n;

	if (string_len(x) > L)
		return UNDEFINED;
	strcpy(buf, string(x));
	if (integer_p(proto)) {
		if (!integer_string_p(buf))
			return UNDEFINED;
		return string_to_bignum(buf);
	}
	if (real_p(proto)) {
		if (!string_numeric_p(buf))
			return UNDEFINED;
		return string_to_real(buf);
	}
	if (string_p(proto)) {
		return x;
	}
	if (char_p(proto)) {
		if (string_len(x) != 2)
			return UNDEFINED;
		return make_char(string(x)[0]);
	}
	if (symbol_p(proto)) {
		s = string(x);
		if (':' == s[0]) s++;
		if (!isalpha(s[0]))
			return UNDEFINED;
		for (p = s; *p; p++)
			if (!is_symbolic(*p))
				return UNDEFINED;
		if (s != string(x)) {
			n = make_string("", string_len(x)-2);
			strcpy(string(n), string(x)+1);
			save(n);
			n = string_to_symbol(n);
			unsave(1);
			return n;
		}
		return string_to_symbol(x);
	}
	return x;
}

static cell safe_form(cell x, cell y) {
	if (string_p(x)) {
		return form(x, y);
	}
	return error("form: expected string, got", x);
}

static void op_form(void) {
	cell	x, y;

	TWO_ARGS("form");
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("form", safe_form, x, y);
}

static cell format_symbol(cell x) {
	cell	n;

	n = make_string("", symbol_len(x));
	string(n)[0] = ':';
	strcpy(&string(n)[1], symbol_name(x));
	return n;
}

static cell safe_format(cell x) {
	char	b[2];

	if (string_p(x))
		return x;
	if (symbol_p(x))
		return format_symbol(x);
	if (variable_p(x))
		return format_symbol(var_symbol(x));
	if (integer_p(x))
		return bignum_to_string(x);
	if (char_p(x)) {
		b[0] = char_value(x);
		b[1] = 0;
		return make_string(b, 1);
	}
	if (real_p(x))
		return real_to_string(x, 0);
	return UNDEFINED;
}

static void op_format(void) {
	cell	x;

	ONE_ARG("format");
	x = car(Dstack);
	monadrec("format", safe_format, x);
}

static cell safe_format2(cell x, cell y) {
	cell	n, m;
	int	k, kf, kp, off, p;

	if (integer_p(y)) {
		k = intvalue(y);
		kf = 0;
	}
	else if (real_p(y)) {
		n = real_abs(y);
		k = intpart(n);
		kf = fracpart(n);
	}
	else {
		return error("format2: type error", cons(x, cons(y, NIL)));
	}
	n = safe_format(x);
	save(n);
	if (real_p(x) && strchr(string(n), 'e') == NULL && kf > 0) {
		k = abs(k);
		kp = k;
		k = abs(k) + kf+1;
		p = strlen(strchr(string(n), '.')) - 1;
		off = kp - string_len(n) + p + 2;
		if (k >= string_len(n) && p <= kf) {
			m = make_string("", k);
			memset(string(m), ' ', k);
			string(m)[k] = 0;
			memcpy(string(m)+off, string(n), string_len(n)-1);
			memset(string(m)+k-kf+p, '0', kf-p);
			n = m;
		}
	}
	else {
		if (abs(k) >= string_len(n)) {
			m = make_string("", abs(k));
			memset(string(m), ' ', abs(k));
			string(m)[abs(k)] = 0;
			if (k > 0) {
				memcpy(string(m), string(n), string_len(n)-1);
			}
			else {
				k = -k;
				memcpy(string(m)+k-string_len(n)+1, string(n),
					string_len(n));
			}
			n = m;
		}
	}
	unsave(1);
	return n;
}

static void op_format2(void) {
	cell	x, y;

	TWO_ARGS("format2");
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("format2", safe_format2, x, y);
}

static void fun(int immed, int arity) {
	cell	x;

	ONE_ARG("function");
	x = car(Dstack);
	if (list_p(x)) {
		x = make_function(x, immed, arity);
		car(Dstack) = x;
		return;
	}
	unknown1("function");
}

static void op_fun0(void) { fun(0, 0); }
static void op_fun1(void) { fun(0, 1); }
static void op_fun2(void) { fun(0, 2); }
static void op_fun3(void) { fun(0, 3); }
static void op_imm1(void) { fun(1, 1); }
static void op_imm2(void) { fun(1, 2); }

static void op_group(void) {
	cell	x;

	ONE_ARG("group");
	x = car(Dstack);
	if (list_p(x)) {
		x = group(x);
		car(Dstack) = x;
		return;
	}
	if (string_p(x)) {
		x = group_string(x);
		car(Dstack) = x;
		return;
	}
	unknown1("group");
}

static void op_first(void) {
	cell	x;

	ONE_ARG("first");
	x = car(Dstack);
	if (pair_p(x)) {
		car(Dstack) = caar(Dstack);
		return;
	}
	if (string_p(x)) {
		if (!empty_p(x))
			x = make_char(string(x)[0]);
		car(Dstack) = x;
		return;
	}
	/* identity */
}

static void op_if(void) {
	cell	x, y, z;

	THREE_ARGS("if")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if (list_p(y) && list_p(x)) {
		Dstack = cddr(Dstack);
		if (false_p(z))
			car(Dstack) = x;
		else
			car(Dstack) = y;
		State = S_APPIF;
		return;
	}
	unknown3("if");
}

static void op_index(void) {
	cell	x, y;

	TWO_ARGS("index")
	y = cadr(Dstack);
	x = car(Dstack);
	if (function_p(y)) {
		op_apply();
		return;
	}
	if ((list_p(y) || string_p(y)) && (list_p(x) || integer_p(x))) {
		if (string_p(y))
			y = string_to_vector(y);
		else
			y = list_to_vector(y);
		save(y);
		x = ndx(y, vector_len(y), x);
		unsave(1);
		y = cadr(Dstack);
		car(Dstack) = x; /* protect x */
		if (string_p(y))
			x = list_to_string(x);
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("index");
}

static void op_indexd(void) {
	cell	x, y;

	TWO_ARGS("index-in-depth")
	y = cadr(Dstack);
	x = car(Dstack);
	if (function_p(y)) {
		op_apply();
		return;
	}
	if (pair_p(y) && (pair_p(x) || integer_p(x))) {
		x = ndx_in_depth(y, x);
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("index-in-depth");
}

static cell safe_intdiv(cell x, cell y) {
	if (integer_p(x) && integer_p(y)) {
		x = bignum_divide(x, y);
		if (undefined_p(x))
			return error("division by zero", VOID);
		return car(x);
	}
	return error("integer-divide: type error", cons(x, cons(y, NIL)));
}

static void op_intdiv(void) {
	cell	x, y;

	TWO_ARGS("integer-divide")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("integer-divide", safe_intdiv, y, x);
}

static void iter(int s) {
	cell	x, y, z, n;

	THREE_ARGS("iterate")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if (integer_p(z) && function_p(x)) {
		if (bignum_zero_p(z)) {
			Dstack = cddr(Dstack);
			car(Dstack) = y;
		}
		else {
			n = bignum_abs(z);
			caddr(Dstack) = n;
			if (S_S_ITER == s)
				push(cons(y, NIL));
			push(Barrier);
			n = cons(x, NIL);
			push(cons(y, n));
			State = s;
		}
		return;
	}
	unknown3("iterate");
}

static void op_iter(void) {
	iter(S_ITER);
}

static void op_join(void) {
	cell	y, x, n;

	TWO_ARGS("join")
	y = cadr(Dstack);
	x = car(Dstack);
	if (dictionary_p(x) && tuple_p(y)) {
		x = dict_add(x, car(y), cadr(y));
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	if (tuple_p(x) && dictionary_p(y)) {
		x = dict_add(y, car(x), cadr(x));
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	n = join(y, x);
	Dstack = cdr(Dstack);
	car(Dstack) = n;
}

static void op_list(void) {
	cell	x, n;

	ONE_ARG("list");
	x = car(Dstack);
	if (char_p(x)) {
		n = make_string("", 1);
		string(n)[0] = char_value(x);
		string(n)[1] = 0;
		car(Dstack) = n;
		return;
	}
	x = cons(x, NIL);
	car(Dstack) = x;
}

static void op_match(void) {
	cell	x, y;

	TWO_ARGS("match")
	y = cadr(Dstack);
	x = car(Dstack);
	x = match(x, y)? One: Zero;
	Dstack = cdr(Dstack);
	car(Dstack) = x;
}

static cell safe_max(cell x, cell y) {
	if (number_p(y) && number_p(x)) {
		return real_less_p(x, y)? y: x;
	}
	return error("max: type error", cons(x, cons(y, NIL)));
}

static void op_max(void) {
	cell	x, y;

	TWO_ARGS("max")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("max", safe_max, y, x);
}

static cell safe_min(cell x, cell y) {
	if (number_p(y) && number_p(x)) {
		return real_less_p(x, y)? x: y;
	}
	return error("min: type error", cons(x, cons(y, NIL)));
}

static void op_min(void) {
	cell	x, y;

	TWO_ARGS("min")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("min", safe_min, y, x);
}

static cell safe_real_subtract(cell x, cell y) {
	if (number_p(x) && number_p(y))
		return real_subtract(x, y);
	return error("minus: type error", cons(x, cons(y, NIL)));
}

static void op_minus(void) {
	cell	x, y;

	TWO_ARGS("minus")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("minus", safe_real_subtract, y, x);
}

static cell safe_negate(cell x) {
	if (number_p(x))
		return real_negate(x);
	return error("negate: type error", cons(x, NIL));
}

static void op_neg(void) {
	cell	x;

	ONE_ARG("negate");
	x = car(Dstack);
	monadrec("negate", safe_negate, x);
}

static void op_newdict(void) {
	cell	x;

	x = copy_dict(car(Dstack));
	car(Dstack) = x;
}

static cell safe_not(cell x) {
	return false_p(x)? One: Zero;
}

static void op_not(void) {
	cell	x;

	ONE_ARG("not");
	x = car(Dstack);
	monadrec("negate", safe_not, x);
}

static void over(int s) {
	cell	x, y, n;

	TWO_ARGS("over")
	y = cadr(Dstack);
	x = car(Dstack);
	if ((list_p(y) || string_p(y)) && function_p(x)) {
		n = cons(NIL, cddr(Dstack));
		cddr(Dstack) = n;
		if (NIL == y || empty_p(y)) {
			Dstack = cddr(Dstack);
			car(Dstack) = y;
		}
		else if (NIL == cdr(y)) {
			Dstack = cddr(Dstack);
			if (S_OVER == s)
				y = car(y);
			car(Dstack) = y;
		}
		else if (string_p(y) && string_len(y) < 3) {
			Dstack = cddr(Dstack);
			y = make_char(string(y)[0]);
			if (S_S_OVER == s)
				y = cons(y, NIL);
			car(Dstack) = y;
		}
		else {
			if (string_p(y)) {
				y = string_to_list(y, STRING_NIL);
			}
			cadr(Dstack) = y;
			push(cons(car(y), NIL));
			push(Barrier);
			n = cons(x, NIL);
			n = cons(cadr(y), n);
			push(cons(car(y), n));
			cadr(cdddr(Dstack)) = cddr(y);
			State = s;
		}
		return;
	}
	if (S_S_OVER == s)
		y = cons(y, NIL);
	Dstack = cdr(Dstack);
	car(Dstack) = y;
}

static void op_over(void) {
	over(S_OVER);
}

static void over_n(int s) {
	cell	x, y, z, n;

	THREE_ARGS("over/n")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if ((list_p(y) || string_p(y)) && function_p(x)) {
		if (NIL == y || empty_p(y)) {
			Dstack = cddr(Dstack);
			car(Dstack) = z;
		}
		else {
			if (string_p(y)) {
				y = string_to_list(y, STRING_NIL);
			}
			save(y);
			cadr(Dstack) = cdr(y);
			push(cons(z, NIL));
			push(Barrier);
			n = cons(x, NIL);
			n = cons(car(y), n);
			push(cons(z, n));
			State = s;
			unsave(1);
		}
		return;
	}
	if (function_p(x)) {
		n = NIL;
		if (S_S_OVER == s) {
			n = cons(S_join, n);
			n = cons(S_list, n);
		}
		n = cons(x, n);
		n = cons(y, n);
		n = cons(z, n);
		if (S_S_OVER == s) n = cons(z, n);
		Dstack = cddr(Dstack);
		car(Dstack) = n;
		State = S_APPLY;
		return;
	}
	unknown3("over/n");
}

static void op_over_n(void) {
	over_n(S_OVER);
}

static cell safe_real_add(cell x, cell y) {
	if (number_p(x) && number_p(y))
		return real_add(x, y);
	return error("plus: type error", cons(x, cons(y, NIL)));
}

static void op_plus(void) {
	cell	x, y;

	TWO_ARGS("plus")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("plus", safe_real_add, y, x);
}

static void op_pop0(void) {
	unsave_vars();
}

static void op_pop1(void) {
	cdr(Dstack) = cddr(Dstack);
	unsave_vars();
}

static void op_pop2(void) {
	cdr(Dstack) = cdddr(Dstack);
	unsave_vars();
}

static void op_pop3(void) {
	cdr(Dstack) = cddddr(Dstack);
	unsave_vars();
}

static cell safe_power(cell x, cell y) {
	if (number_p(x) && number_p(y)) {
		return real_power(x, y);
	}
	return error("power: type error", cons(x, cons(y, NIL)));
}

static void op_pow(void) {
	cell	x, y;

	TWO_ARGS("power")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("power", safe_power, y, x);
}

static void op_range(void) {
	cell	x;

	ONE_ARG("range");
	x = car(Dstack);
	if (list_p(x) || string_p(x)) {
		x = range(x);
		car(Dstack) = x;
		return;
	}
	unknown1("range");
}

static cell safe_recip(cell x) {
	if (number_p(x))
		return real_divide(One, x);
	return error("reciprocal: type error", cons(x, NIL));
}

static void op_recip(void) {
	cell	x;

	ONE_ARG("reciprocal");
	x = car(Dstack);
	monadrec("reciprocal", safe_recip, x);
}

static cell safe_rem(cell x, cell y) {
	if (integer_p(y) && integer_p(x)) {
		x = bignum_divide(x, y);
		if (undefined_p(x))
			return error("division by zero", VOID);
		return cdr(x);
	}
	return error("rem: type error", cons(x, cons(y, NIL)));
}

static void op_rem(void) {
	cell	x, y;

	TWO_ARGS("rem")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("rem", safe_rem, y, x);
}

static void op_reshape(void) {
	cell	y, x;

	TWO_ARGS("reshape");
	y = cadr(Dstack);
	x = car(Dstack);
	if (integer_p(y)) {
		if (bignum_zero_p(y)) {
			pop();
			car(Dstack) = x;
			return;
		}
		binop(reshape, y, x);
		return;
	}
	if (list_p(y)) {
		binop(reshape, y, x);
		return;
	}
	unknown2("reshape");
}

static void op_rot(void) {
	cell	x, y;

	TWO_ARGS("rotate");
	y = cadr(Dstack);
	x = car(Dstack);
	if (atom_p(x) && !string_p(x)) {
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	if (integer_p(y) && list_p(x)) {
		x = rotate(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	if (integer_p(y) && string_p(x)) {
		x = rotate_string(x, y);
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("rotate");
}

static void op_rev(void) {
	cell	x;

	ONE_ARG("reverse");
	x = car(Dstack);
	if (list_p(x)) {
		x = rev(car(Dstack));
		car(Dstack) = x;
		return;
	}
	if (string_p(x)) {
		x = rev_string(car(Dstack));
		car(Dstack) = x;
		return;
	}
	/* identity */
}

static void op_s_conv(void) {
	conv(S_S_CONV);
}

static void op_s_iter(void) {
	iter(S_S_ITER);
}

static void op_s_over(void) {
	over(S_S_OVER);
}

static void op_s_over_n(void) {
	over_n(S_S_OVER);
}

static void op_shape(void) {
	cell	x;

	ONE_ARG("shape");
	x = car(Dstack);
	if (string_p(x) && string_len(x) > 1) {
		x = cons(make_integer(string_len(x)-1), NIL);
		car(Dstack) = x;
		return;
	}
	if (pair_p(x)) {
		x = shape(car(Dstack));
		car(Dstack) = x;
		return;
	}
	car(Dstack) = Zero;
}

static void op_size(void) {
	cell	x, new;

	ONE_ARG("size");
	x = car(Dstack);
	if (list_p(x)) {
		x = make_integer(length(x));
		car(Dstack) = x;
		return;
	}
	if (number_p(x)) {
		x = real_abs(x);
		car(Dstack) = x;
		return;
	}
	if (char_p(x)) {
		new = make_integer(char_value(x));
		car(Dstack) = new;
		return;
	}
	if (string_p(x)) {
		x = make_integer(string_len(x)-1);
		car(Dstack) = x;
		return;
	}
	if (dictionary_p(x)) {
		x = make_integer(dict_size(x));
		car(Dstack) = x;
		return;
	}
	unknown1("size");
}

static void op_split(void) {
	cell	x, y;

	TWO_ARGS("split")
	y = cadr(Dstack);
	x = car(Dstack);
	if ((pair_p(y) || integer_p(y)) && list_p(x)) {
		binop(split, y, x);
		return;
	}
	if ((pair_p(y) || integer_p(y)) && string_p(x)) {
		binop(split_string, y, x);
		return;
	}
	unknown2("split");
}

static void op_swap(void) {
	cell	x;

	TWO_ARGS("swap")
	x = car(Dstack);
	car(Dstack) = cadr(Dstack);
	cadr(Dstack) = x;
}

static void sysfn(int id);

static void op_syscall(void) {
	cell	x;
	int	id;

	ONE_ARG("syscall");
	x = car(Dstack);
	if (integer_p(x)) {
		id = intvalue(x);
		Dstack = cdr(Dstack);
		sysfn(id);
		return;
	}
	unknown1("syscall");
}

static void op_take(void) {
	cell	x, y;

	TWO_ARGS("take")
	y = cadr(Dstack);
	x = car(Dstack);
	if (integer_p(y) && list_p(x)) {
		x = take(x, intvalue(y));
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	if (integer_p(y) && string_p(x)) {
		x = take_string(x, intvalue(y));
		Dstack = cdr(Dstack);
		car(Dstack) = x;
		return;
	}
	unknown2("take");
}

static cell safe_real_multiply(cell x, cell y) {
	if (number_p(x) && number_p(y))
		return real_multiply(x, y);
	return error("times: type error", cons(x, cons(y, NIL)));
}

static void op_times(void) {
	cell	x, y;

	TWO_ARGS("times")
	y = cadr(Dstack);
	x = car(Dstack);
	dyadrec("times", safe_real_multiply, y, x);
}

static void op_transp(void) {
	cell	x;

	ONE_ARG("transpose");
	x = car(Dstack);
	if (list_p(x)) {
		x = transpose(x);
		car(Dstack) = x;
		return;
	}
	unknown1("transpose");
}

static void op_up(void) {
	cell	x;

	ONE_ARG("grade-up");
	x = car(Dstack);
	if (list_p(x)) {
		x = grade(less, x);
		car(Dstack) = x;
		return;
	}
	if (string_p(x)) {
		x = string_to_list(x, NIL);
		car(Dstack) = x;
		x = grade(less, x);
		car(Dstack) = x;
		return;
	}
	unknown1("grade-up");
}

static void op_undef(void) {
	cell	x;

	ONE_ARG("undefined");
	x = car(Dstack);
	car(Dstack) = UNDEFINED == x? One: Zero;
}

static void op_while(void) {
	cell	x, y, z, n;

	THREE_ARGS("while")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if (function_p(z) && function_p(x)) {
		push(Barrier);
		n = cons(S_pop1, NIL);
		n = cons(S_call1, n);
		n = cons(z, n);
		push(cons(y, n));
		State = S_WPRED;
		return;
	}
	unknown3("while");
}

static void op_s_while(void) {
	cell	x, y, z, n;

	THREE_ARGS("while")
	z = caddr(Dstack);
	y = cadr(Dstack);
	x = car(Dstack);
	if (function_p(z) && function_p(x)) {
		n = cons(y, NIL);
		cadr(Dstack) = n;
		push(Barrier);
		n = cons(S_pop1, NIL);
		n = cons(S_call1, n);
		n = cons(z, n);
		push(cons(y, n));
		State = S_S_WPRED;
		return;
	}
	unknown3("while");
}

static void op_x(void) {
	push(NIL == Frame? UNDEFINED: car(Frame));
}

static void op_y(void) {
	push(NIL == Frame || NIL == cdr(Frame)? UNDEFINED: cadr(Frame));
}

static void op_z(void) {
	push(NIL == Frame || NIL == cdr(Frame) || NIL == cddr(Frame)?
		UNDEFINED: caddr(Frame));
}

OP Ops[] = {
	{ "%amend",   0, op_amend   },
	{ "%amendd",  0, op_amendd  },
	{ "%atom",    0, op_atom    },
	{ "%call0",   1, op_call0   },
	{ "%call1",   1, op_call1   },
	{ "%call2",   1, op_call2   },
	{ "%call3",   1, op_call3   },
	{ "%char",    0, op_char    },
	{ "%clear",   0, op_clear   },
	{ "%conv",    1, op_conv    },
	{ "%cut",     0, op_cut     },
	{ "%def",     0, op_def     },
	{ "%div",     0, op_div     },
	{ "%down",    0, op_down    },
	{ "%drop",    0, op_drop    },
	{ "%each",    1, op_each    },
	{ "%each2",   1, op_each2   },
	{ "%eachl",   1, op_eachl   },
	{ "%eachp",   1, op_eachp   },
	{ "%eachr",   1, op_eachr   },
	{ "%enum",    0, op_enum    },
	{ "%eq",      0, op_eq      },
	{ "%expand",  0, op_expand  },
	{ "%find",    0, op_find    },
	{ "%first",   0, op_first   },
	{ "%floor",   0, op_floor   },
	{ "%form",    0, op_form    },
	{ "%format",  0, op_format  },
	{ "%format2", 0, op_format2 },
	{ "%fun0",    0, op_fun0    },
	{ "%fun1",    0, op_fun1    },
	{ "%fun2",    0, op_fun2    },
	{ "%fun3",    0, op_fun3    },
	{ "%group",   0, op_group   },
	{ "%gt",      0, op_gt      },
	{ "%if",      1, op_if      },
	{ "%imm1",    0, op_imm1    },
	{ "%imm2",    0, op_imm2    },
	{ "%index",   1, op_index   },
	{ "%indexd",  1, op_indexd  },
	{ "%intdiv",  0, op_intdiv  },
	{ "%iter",    1, op_iter    },
	{ "%join",    0, op_join    },
	{ "%list",    0, op_list    },
	{ "%lt",      0, op_lt      },
	{ "%match",   0, op_match   },
	{ "%max",     0, op_max     },
	{ "%min",     0, op_min     },
	{ "%minus",   0, op_minus   },
	{ "%neg",     0, op_neg     },
	{ "%newdict", 0, op_newdict },
	{ "%not",     0, op_not     },
	{ "%over",    1, op_over    },
	{ "%over2",   1, op_over_n  },
	{ "%plus",    0, op_plus    },
	{ "%pop0",    0, op_pop0    },
	{ "%pop1",    0, op_pop1    },
	{ "%pop2",    0, op_pop2    },
	{ "%pop3",    0, op_pop3    },
	{ "%power",   0, op_pow     },
	{ "%range",   0, op_range   },
	{ "%recip",   0, op_recip   },
	{ "%rem",     0, op_rem     },
	{ "%reshape", 0, op_reshape },
	{ "%rev",     0, op_rev     },
	{ "%rot",     0, op_rot     },
	{ "%sconv",   1, op_s_conv  },
	{ "%siter",   1, op_s_iter  },
	{ "%sover",   1, op_s_over  },
	{ "%sover2",  1, op_s_over_n},
	{ "%swhile",  1, op_s_while },
	{ "%shape",   0, op_shape   },
	{ "%size",    0, op_size    },
	{ "%siter",   0, op_s_iter  },
	{ "%split",   0, op_split   },
	{ "%swap",    0, op_swap    },
	{ "%syscall", 0, op_syscall },
	{ "%take",    0, op_take    },
	{ "%times",   0, op_times   },
	{ "%transp",  0, op_transp  },
	{ "%up",      0, op_up      },
	{ "%undef",   0, op_undef   },
	{ "%while",   1, op_while   },
	{ "%x",       0, op_x       },
	{ "%y",       0, op_y       },
	{ "%z",       0, op_z       },
	{ NULL }
};

/*
 * Built-in System Functions
 */

static void sys_close(void) {
	cell	x;

	ONE_ARG(".cc");
	x = car(Dstack);
	push(car(Dstack));
	if (input_port_p(x) || output_port_p(x)) {
		if (x == From_chan) {
			set_input_port(0);
			From_chan = 0;
		}
		if (x == To_chan) {
			set_output_port(1);
			To_chan = 1;
		}
		if (	port_no(x) != 0 &&
			port_no(x) != 1 &&
			port_no(x) != 2)
		{
			close_port(port_no(x));
		}
		car(Dstack) = NIL;
		return;
	}
	unknown1(".cc");
}

static void sys_comment(void) {
	cell	x;
	char	msg[80];
	int	sln, k;

	ONE_ARG(".comment");
	x = car(Dstack);
	push(car(Dstack));
	if (string_p(x)) {
		sln = Line;
		k = strlen(string(x));
		set_input_port(Prog_chan);
		for (;;) {
			if (kg_getline(Inbuf, TOKEN_LENGTH) == NULL) {
				sprintf(msg, "undelimited comment (line %d)", 
					sln);
				error(msg, VOID);
				break;
			}
			if (!strncmp(string(x), Inbuf, k))
				break;
		}
		return;
	}
	unknown1(".comment");
}

static void sys_delete(void) {
	cell	x;

	ONE_ARG(".df");
	x = car(Dstack);
	push(car(Dstack));
	if (string_p(x)) {
		if (remove(string(x)) < 0)
			error("could not delete", x);
		return;
	}
	unknown1(".df");
}

static void sys_display(void) {
	cell	x;

	ONE_ARG(".p");
	x = car(Dstack);
	Display = 1;
	if (!outport_open_p())
		error(".d: writing to closed channel",
			make_port(To_chan, T_OUTPUT_PORT));
	else
		kg_write(x);
	Display = 0;
	push(car(Dstack));
}

static cell evalstr(cell x);

static void sys_eval(void) {
	cell	x;

	ONE_ARG(".E");
	x = car(Dstack);
	if (string_p(x)) {
		x = evalstr(x);
		car(Dstack) = x;
		push(car(Dstack));
		return;
	}
	unknown1(".E");
}

static void sys_flush(void) {
	if (!outport_open_p())
		error(".fl: writing to closed channel",
			make_port(To_chan, T_OUTPUT_PORT));
	else
		flush();
	push(NIL);
}

static void sys_fromchan(void) {
	cell	x, old;

	ONE_ARG(".fc");
	x = car(Dstack);
	push(car(Dstack));
	old = From_chan;
	if (false_p(x)) {
		set_input_port(0);
		old = make_port(old, T_INPUT_PORT);
		car(Dstack) = old;
		return;
	}
	if (input_port_p(x)) {
		From_chan = port_no(x);
		set_input_port(port_no(x));
		old = make_port(old, T_INPUT_PORT);
		car(Dstack) = old;
		return;
	}
	unknown1(".fc");
}

static void sys_infile(void) {
	cell	x;
	int	p;

	ONE_ARG(".ic");
	x = car(Dstack);
	push(car(Dstack));
	if (string_p(x)) {
		if ((p = open_input_port(string(x))) < 0) {
			error(".ic: failed to open input file", x);
			return;
		}
		x = make_port(p, T_INPUT_PORT);
		car(Dstack) = x;
		return;
	}
	unknown1(".ic");
}

static void sys_load(void) {
	cell	x;

	ONE_ARG(".l");
	x = car(Dstack);
	push(car(Dstack));
	if (string_p(x)) {
		x = load(x, 0, 0);
		car(Dstack) = x;
		return;
	}
	unknown1(".l");
}

static void sys_module(void) {
	cell	x;

	ONE_ARG(".module");
	x = car(Dstack);
	push(car(Dstack));
	if (symbol_p(x)) {
		if (Module != UNDEFINED) {
			error("nested module; contained in",
				make_symbol(Modname, strlen(Modname)));
		}
		else {
			Module = NIL;
			Mod_funvars = NIL;
			strcpy(Modname, symbol_name(x));
		}
		return;
	}
	if (false_p(x)) {
		if (UNDEFINED == Module)
			error("no module open", VOID);
		Module = UNDEFINED;
		return;
	}
	unknown1(".module");
}

static void sys_more(void) {
	ONE_ARG(".mi");
	if (!inport_open_p())
		error(".mi: testing closed channel",
			make_port(From_chan, T_INPUT_PORT));
	push(port_eof(input_port())? Zero: One);
}

static void outfile(int append) {
	cell	x;
	int	p;

	ONE_ARG(".oc");
	x = car(Dstack);
	push(car(Dstack));
	if (string_p(x)) {
		if ((p = open_output_port(string(x), append)) < 0) {
			if (append)
				error(".ac: failed to open output file", x);
			else
				error(".oc: failed to open output file", x);
			return;
		}
		x = make_port(p, T_OUTPUT_PORT);
		car(Dstack) = x;
		return;
	}
	unknown1(".oc");
}

static void sys_outfile(void) {
	outfile(0);
}

static void sys_appfile(void) {
	outfile(1);
}

static void sys_print(void) {
	if (!outport_open_p())
		error(".p: writing to closed channel",
			make_port(To_chan, T_OUTPUT_PORT));
	else
		sys_display();
	nl();
}

static void sys_randnum(void) {
	cell	x, n, e = 0;

	n = rand() % S9_INT_SEG_LIMIT;
	x = 0;
	while (n > 0) {
		x = x*10+n%10;
		e++;
		n /= 10;
	}
	x = make_real(1, -e, make_integer(x));
	push(x);
}

static void sys_read(void) {
	if (!inport_open_p())
		error(".r: reading from closed channel",
			make_port(From_chan, T_INPUT_PORT));
	else {
		push(kg_read());
		while (readc() != '\n' && readc() != EOF)
			;
	}
}

static void sys_readln(void) {
	cell	x = NIL;
	int	c;

	save(x);
	if (!inport_open_p()) {
		error(".rl: reading from closed channel",
			make_port(From_chan, T_INPUT_PORT));
	}
	else {
		while ((c = readc()) != '\n') {
			if (EOF == c)
				break;
			x = cons(make_char(c), x);
			car(Stack) = x;
		}
		x = rev(x);
	}
	car(Stack) = x;
	x = list_to_string(x);
	unsave(1);
	push(x);
}

static cell compile(char *p);

static void sys_readstr(void) {
	cell	x;

	if (s9_aborted())
		return;
	ONE_ARG(".rs");
	x = car(Dstack);
	if (string_p(x)) {
		Report = 0;
		x = compile(string(x));
		Report = 1;
		car(Dstack) = car(x);
		if (s9_aborted()) {
			s9_reset();
			car(Dstack) = UNDEFINED;
		}
		push(car(Dstack));
		return;
	}
	unknown1(".rs");
}

static void sys_system(void) {
	cell	x;
	int	r;

#ifdef SAFE
	error("shell access disabled", VOID);
	return;
#endif
	ONE_ARG(".sys");
	x = car(Dstack);
	if (string_p(x)) {
		r = system(string(x));
		x = make_integer(r);
		car(Dstack) = x;
		push(car(Dstack));
		return;
	}
	unknown1(".sys");
}

#ifndef plan9
static void sys_pclock(void) {
	clock_t	t;
	cell	x, y;

	t = clock();
	x = make_integer(t);
	save(x);
	y = make_integer(10000000);
	x = bignum_multiply(x, y);
	car(Stack) = x;
	y = make_integer(CLOCKS_PER_SEC);
	x = bignum_divide(x, y);
	x = car(x);
	x = make_real(1, -7, x);
	unsave(1);
	push(x);
}
#endif

static void sys_tochan(void) {
	cell	x, old;

	ONE_ARG(".tc");
	x = car(Dstack);
	push(car(Dstack));
	old = To_chan;
	if (false_p(x)) {
		set_output_port(1);
		old = make_port(old, T_OUTPUT_PORT);
		car(Dstack) = old;
		return;
	}
	if (output_port_p(x)) {
		To_chan = port_no(x);
		set_output_port(port_no(x));
		old = make_port(old, T_OUTPUT_PORT);
		car(Dstack) = old;
		return;
	}
	unknown1(".tc");
}

static void sys_write(void) {
	ONE_ARG(".w");
	if (!outport_open_p())
		error(".w: writing to closed channel",
			make_port(To_chan, T_OUTPUT_PORT));
	else
		kg_write(car(Dstack));
	push(car(Dstack));
}

static void sys_exit(void) {
	cell	x;

	ONE_ARG(".x");
	x = car(Dstack);
	bye(false_p(x) == 0);
}

SYS Sysfns[] = {
	{ ".ac",      1, sys_appfile },
	{ ".cc",      1, sys_close   },
	{ ".comment", 1, sys_comment },
	{ ".df",      1, sys_delete  },
	{ ".d",       1, sys_display },
	{ ".E",       1, sys_eval    },
	{ ".fc",      1, sys_fromchan},
	{ ".fl",      0, sys_flush   },
	{ ".ic",      1, sys_infile  },
	{ ".l",       1, sys_load    },
	{ ".mi",      1, sys_more    },
	{ ".module",  1, sys_module  },
	{ ".oc",      1, sys_outfile },
	{ ".p",       1, sys_print   },
#ifndef plan9
	{ ".pc",      0, sys_pclock  },
#endif
	{ ".r",       0, sys_read    },
	{ ".rl",      0, sys_readln  },
	{ ".rn",      0, sys_randnum },
	{ ".rs",      1, sys_readstr },
	{ ".sys",     1, sys_system  },
	{ ".tc",      1, sys_tochan  },
	{ ".w",       1, sys_write   },
	{ ".x",       1, sys_exit    },
	{ NULL }
};

static void sysfn(int id) {
	(*Sysfns[id].handler)();
}

/*
 * Virtual Machine, Adverb Handlers
 */

static cell next_conv(cell s) {
	cell	y0, y1;
	cell	n;

	y0 = cadr(Dstack);
	y1 = car(Dstack);
	if (match(y0, y1)) {
		n = y0;
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	cadr(Dstack) = y1;
	car(Dstack) = Barrier;
	n = cons(caddr(Dstack), NIL);
	push(cons(y1, n));
	return s;
}

static cell next_each(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack);
	cell	n;

	if (NIL == car(y) || STRING_NIL == car(y)) {
		n = revb(car(Dstack));
		car(Dstack) = n; /* protect x */
		if (STRING_NIL == car(y))
			n = list_to_string(n);
		Dstack = cddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	push(Barrier);
	n = cons(car(x), NIL);
	push(cons(caar(y), n));
	car(y) = cdar(y);
	return s;
}

static cell next_each2(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack), z = cdddr(Dstack);
	cell	n;

	if (	NIL == car(y) || NIL == car(z) ||
		STRING_NIL == car(y) || STRING_NIL == car(z)
	) {
		n = revb(car(Dstack));
		if (STRING_NIL == car(y) || STRING_NIL == car(z))
			n = list_to_string(n);
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	push(Barrier);
	n = cons(car(x), NIL);
	n = cons(caar(y), n);
	push(cons(caar(z), n));
	car(y) = cdar(y);
	car(z) = cdar(z);
	return s;
}

static cell next_eachl(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack), z = cdddr(Dstack);
	cell	n;

	if (NIL == car(y) || STRING_NIL == car(y)) {
		n = revb(car(Dstack));
		if (STRING_NIL == car(y))
			n = list_to_string(n);
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	push(Barrier);
	n = cons(car(x), NIL);
	n = cons(caar(y), n);
	push(cons(car(z), n));
	car(y) = cdar(y);
	return s;
}

static cell next_eachp(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack);
	cell	n;

	if (	NIL == car(y) || NIL == cdar(y) ||
		STRING_NIL == car(y) || STRING_NIL == cdar(y)
	) {
		n = revb(car(Dstack));
		if (STRING_NIL == car(y) || STRING_NIL == cdar(y))
			n = list_to_string(n);
		Dstack = cddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	push(Barrier);
	n = cons(car(x), NIL);
	n = cons(cadar(y), n);
	push(cons(caar(y), n));
	car(y) = cdar(y);
	return s;
}

static cell next_eachr(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack), z = cdddr(Dstack);
	cell	n;

	if (NIL == car(y) || STRING_NIL == car(y)) {
		n = revb(car(Dstack));
		if (STRING_NIL == car(y))
			n = list_to_string(n);
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	push(Barrier);
	n = cons(car(x), NIL);
	n = cons(car(z), n);
	push(cons(caar(y), n));
	car(y) = cdar(y);
	return s;
}

static cell next_over(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack);
	cell	n;

	if (NIL == car(y) || STRING_NIL == car(y)) {
		n = car(Dstack);
		if (STRING_NIL == car(y))
			n = list_to_string(n);
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	push(Barrier);
	n = cons(car(x), NIL);
	n = cons(caar(y), n);
	push(cons(cadr(Dstack), n));
	car(y) = cdar(y);
	return s;
}

static cell next_iter(cell s) {
	cell	x = cdr(Dstack), z = cdddr(Dstack);
	cell	n;

	n = bignum_subtract(car(z), One);
	car(z) = n;
	if (bignum_zero_p(car(z))) {
		n = car(Dstack);
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	n = cons(car(x), NIL);
	n = cons(car(Dstack), n);
	car(Dstack) = Barrier;
	push(n);
	return s;
}

static cell next_s_conv(cell s) {
	cell	ys, y1;
	cell	n;

	ys = cadr(Dstack);
	y1 = car(Dstack);
	if (match(car(ys), y1)) {
		n = ys;
		Dstack = cdddr(Dstack);
		car(Dstack) = revb(n);
		return cdr(s);
	}
	n = cons(y1, ys);
	cadr(Dstack) = n;
	car(Dstack) = Barrier;
	n = cons(caddr(Dstack), NIL);
	push(cons(y1, n));
	return s;
}

static cell next_s_iter(cell s) {
	cell	x = cdr(Dstack), z = cdddr(Dstack);
	cell	n;

	n = bignum_subtract(car(z), One);
	car(z) = n;
	if (bignum_zero_p(car(z))) {
		n = car(Dstack);
		Dstack = cdddr(Dstack);
		car(Dstack) = revb(n);
		return cdr(s);
	}
	n = cons(car(x), NIL);
	n = cons(caar(Dstack), n);
	push(Barrier);
	push(n);
	return s;
}

static cell next_s_over(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack);
	cell	n;

	if (NIL == car(y) || STRING_NIL == car(y)) {
		if (STRING_NIL == car(y))
			conv_to_strlst(car(Dstack));
		n = revb(car(Dstack));
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	push(Barrier);
	n = cons(car(x), NIL);
	n = cons(caar(y), n);
	push(cons(caadr(Dstack), n));
	car(y) = cdar(y);
	return s;
}

static cell next_s_wpred(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack);
	cell	n;

	if (false_p(car(Dstack))) {
		n = car(y);
		Dstack = cdddr(Dstack);
		if (NIL != n)
			n = cdr(n);
		car(Dstack) = revb(n);
		return cdr(s);
	}
	n = cons(car(x), NIL);
	n = cons(caar(y), n);
	car(Dstack) = Barrier;
	push(n);
	car(S) = S_S_WEXPR;
	return s;
}

static cell next_s_wexpr(cell s) {
	cell	y = cddr(Dstack), z = cdddr(Dstack);
	cell	n;

	n = cons(car(Dstack), car(y));
	car(y) = n;
	n = cons(S_pop1, NIL);
	n = cons(S_call1, n);
	n = cons(car(z), n);
	n = cons(caar(y), n);
	car(Dstack) = Barrier;
	push(n);
	car(S) = S_S_WPRED;
	return s;
}

static cell next_wpred(cell s) {
	cell	x = cdr(Dstack), y = cddr(Dstack);
	cell	n;

	if (false_p(car(Dstack))) {
		n = car(y);
		Dstack = cdddr(Dstack);
		car(Dstack) = n;
		return cdr(s);
	}
	n = cons(car(x), NIL);
	n = cons(car(y), n);
	car(Dstack) = Barrier;
	push(n);
	car(S) = S_WEXPR;
	return s;
}

static cell next_wexpr(cell s) {
	cell	y = cddr(Dstack), z = cdddr(Dstack);
	cell	n;

	car(y) = car(Dstack);
	n = cons(S_pop1, NIL);
	n = cons(S_call1, n);
	n = cons(car(z), n);
	n = cons(car(y), n);
	car(Dstack) = Barrier;
	push(n);
	car(S) = S_WPRED;
	return s;
}

static void acc(void) {
	cell	n;

	n = cons(car(Dstack), cadr(Dstack));
	Dstack = cdr(Dstack);
	car(Dstack) = n;
}

static void mov(void) {
	cell	n;

	n = car(Dstack);
	Dstack = cdr(Dstack);
	car(Dstack) = n;
}

static cell next(cell s) {
	cell	n;

	if (cadr(Dstack) != Barrier)
		return error("adverb arity error", VOID);
	n = car(Dstack);
	Dstack = cdr(Dstack);
	car(Dstack) = n;
	switch (car(s)) {
	case S_EACH:	acc(); return next_each(s);
	case S_EACH2:	acc(); return next_each2(s);
	case S_EACHL:	acc(); return next_eachl(s);
	case S_EACHP:	acc(); return next_eachp(s);
	case S_EACHR:	acc(); return next_eachr(s);
	case S_OVER:	mov(); return next_over(s);
	case S_CONV:	return next_conv(s);
	case S_ITER:	return next_iter(s);
	case S_WPRED:	return next_wpred(s);
	case S_WEXPR:	return next_wexpr(s);
	case S_S_OVER:	acc(); return next_s_over(s);
	case S_S_CONV:	return next_s_conv(s);
	case S_S_ITER:	acc(); return next_s_iter(s);
	case S_S_WPRED:	return next_s_wpred(s);
	case S_S_WEXPR:	return next_s_wexpr(s);
	default:	error("stack dump", Dstack);
			fatal("bad state");
	}
	return UNDEFINED; /*LINT*/
}

/*
 * Virtual Machine, Interpreter
 */

static cell savestate(cell f, cell fr, cell s) {
	if (NIL == f) {
		return s;
	}
	return cons(f, cons(fr, s));
}

static void cleartrace(void) {
	int	i;

	for (i=0; i<NTRACE; i++)
		Trace[i] = UNDEFINED;
	Traceptr = 0;
}

static void funtrace(cell v) {
	if (function_p(var_value(v))) {
		if (Traceptr >= NTRACE)
			Traceptr = 0;
		Trace[Traceptr++] = var_symbol(v);
	}
}

static void eval(cell x) {
	cell		y;
	int		skip = 0;

	save(x);
	save(S);
	save(F);
	save(Frame);
	Frame = Dstack;
	S = NIL;
	F = cdr(x);
	x = car(x);
	State = S_EVAL;
	cleartrace();
	for (s9_reset(); 0 == Abort_flag;) {
		if (skip) {
			skip = 0;
		}
		else {
			if (variable_p(x)) {
				if (Debug) {
					funtrace(x);
				}
				y = var_value(x);
				if (NO_VALUE == y) {
					error("undefined", x);
					break;
				}
				x = y;
			}
			if (primop_p(x)) {
				(Ops[primop_slot(x)].handler)();
			}
			else if (syntax_p(x)) {
				State = S_EVAL;
				(Ops[primop_slot(x)].handler)();
				if (State != S_EVAL) {
					S = savestate(F, Frame, S);
					if (State != S_APPIF)
						Frame = cdr(Dstack);
					if (	S_APPLY == State ||
						S_APPIF == State
					)
						State = S_EVAL;
					else
						S = new_atom(State, S);
					F = pop();
				}
			}
			else if (function_p(x) && fun_immed(x)) {
				S = savestate(F, Frame, S);
				Frame = Dstack;
				F = fun_body(x);
			}
			else {
				push(x);
			}
		}
		if (F != NIL) {
			x = car(F);
			F = cdr(F);
		}
		else if (S != NIL) {
			if (atom_p(S)) {
				y = S;
				S = next(S);
				/*
				 * next_*() uses return cdr(s) to exit,
				 * so y =/= S is certain in this case.
				 */
				if (y == S)
					F = pop();
				skip = 1;
			}
			else {
				F = car(S);
				Frame = cadr(S);
				S = cddr(S);
				x = car(F);
				F = cdr(F);
			}
		}
		else {
			Frame = NIL;
			break;
		}
	}
	Frame = unsave(1);
	F = unsave(1);
	S = unsave(1);
	unsave(1);
}

/*
 * Bytecode Compiler, Parser
 */

static int adverb_arity(cell a, int ctx) {
	int	c0, c1;

	c0 = symbol_name(a)[0];
	c1 = symbol_name(a)[1];
	if ('\'' == c0) return ctx;
	if (':'  == c0 && '\\' == c1) return 2;
	if (':'  == c0 && '\'' == c1) return 2;
	if (':'  == c0 && '/'  == c1) return 2;
	if ('/'  == c0) return 2;
	if (':'  == c0 && '~'  == c1) return 1;
	if (':'  == c0 && '*'  == c1) return 1;
	if ('\\' == c0 &&   0  == c1) return 2;
	if ('\\' == c0 && '~'  == c1) return 1;
	if ('\\' == c0 && '*'  == c1) return 1;
	error("internal: adverb_arity(): unknown adverb", a);
	return 0;
}

static cell monadsym(char *b) {
	char	*m = "expected monad, got";

	switch (b[0]) {
	case '!': return S_enum;
	case '#': return S_size;
	case '$': return S_format;
	case '%': return S_recip;
	case '&': return S_expand;
	case '*': return S_first;
	case '+': return S_transp;
	case ',': return S_list;
	case '-': return S_neg;
	case '<': return S_up;
	case '=': return S_group;
	case '>': return S_down;
	case '?': return S_range;
	case '@': return S_atom;
	case '^': return S_shape;
	case '_': return S_floor;
	case '|': return S_rev;
	case '~': return S_not;
	case ':':
		switch (b[1]) {
		case '#': return S_char;
		case '_': return S_undef;
		default:  error(m, make_symbol(b, 2));
		}
		break;
	}
	error("monad expected", make_symbol(b, strlen(b)));
	return UNDEFINED;
}

static cell dyadsym(char *b) {
	switch (b[0]) {
	case '!': return S_rem;
	case '#': return S_take;
	case '$': return S_format2;
	case '%': return S_div;
	case '&': return S_min;
	case '*': return S_times;
	case '+': return S_plus;
	case ',': return S_join;
	case '-': return S_minus;
	case '<': return S_lt;
	case '=': return S_eq;
	case '>': return S_gt;
	case '?': return S_find;
	case '@': return S_index;
	case '^': return S_power;
	case '_': return S_drop;
	case '|': return S_max;
	case '~': return S_match;
	case ':':
		switch (b[1]) {
		case '@': return S_indexd;
		case '#': return S_split;
		case '$': return S_form;
		case '%': return S_intdiv;
		case '+': return S_rot;
		case '-': return S_amendd;
		case ':': return S_def;
		case '=': return S_amend;
		case '^': return S_reshape;
		case '_': return S_cut;
		}
	}
	error("dyad expected", make_symbol(b, strlen(b)));
	return UNDEFINED;
}

static cell opsym(cell op, int ctx) {
	if (1 == ctx)
		return monadsym(symbol_name(op));
	else
		return dyadsym(symbol_name(op));
}

static char *opname2(cell y) {
	static char	b[100];

	if (y == S_amend) return ":=";
	if (y == S_amendd) return ":-";
	if (y == S_atom) return "@";
	if (y == S_char) return ":#";
	if (y == S_conv) return ":~";
	if (y == S_cut) return ":_";
	if (y == S_def) return "::";
	if (y == S_div) return "%";
	if (y == S_down) return ">";
	if (y == S_drop) return "_";
	if (y == S_each) return "'";
	if (y == S_each2) return "'";
	if (y == S_eachl) return ":\\";
	if (y == S_eachp) return ":'";
	if (y == S_eachr) return ":/";
	if (y == S_enum) return "~";
	if (y == S_eq) return "=";
	if (y == S_expand) return "&";
	if (y == S_find) return "?";
	if (y == S_first) return "*";
	if (y == S_floor) return "_";
	if (y == S_form) return ":$";
	if (y == S_format) return "$";
	if (y == S_format2) return "$";
	if (y == S_group) return "=";
	if (y == S_gt) return ">";
	if (y == S_if) return ":[";
	if (y == S_index) return "@";
	if (y == S_indexd) return ":@";
	if (y == S_intdiv) return ":%";
	if (y == S_iter) return ":*";
	if (y == S_join) return ",";
	if (y == S_list) return ",";
	if (y == S_lt) return "<";
	if (y == S_match) return "~";
	if (y == S_max) return "|";
	if (y == S_min) return "&";
	if (y == S_minus) return "-";
	if (y == S_neg) return "-";
	if (y == S_not) return "~";
	if (y == S_over) return "/";
	if (y == S_over2) return "/";
	if (y == S_plus) return "+";
	if (y == S_power) return "^";
	if (y == S_range) return "?";
	if (y == S_recip) return "";
	if (y == S_rem) return "!";
	if (y == S_reshape) return ":^";
	if (y == S_rev) return "|";
	if (y == S_rot) return ":+";
	if (y == S_sconv) return "\\~";
	if (y == S_siter) return "\\*";
	if (y == S_sover) return "\\";
	if (y == S_sover2) return "\\";
	if (y == S_swhile) return "\\~";
	if (y == S_shape) return "^";
	if (y == S_size) return "#";
	if (y == S_split) return ":#";
	if (y == S_take) return "#";
	if (y == S_times) return "*";
	if (y == S_transp) return "+";
	if (y == S_up) return "<";
	if (y == S_undef) return ":_";
	if (y == S_while) return ":~";
	if (y == S_x) return "x";
	if (y == S_y) return "y";
	if (y == S_z) return "z";
	strcpy(b, string(var_symbol(y)));
	return b;
}

static cell opname(cell y) {
	char	*s;

	s = opname2(y);
	return make_symbol(s, strlen(s));
}

static cell adverbsym(cell adv, int ctx) {
	int	c0, c1;

	c0 = symbol_name(adv)[0];
	c1 = symbol_name(adv)[1];
	switch (c0) {
	case '\'': return 2 == ctx? S_each2: S_each;
	case '/':  return 2 == ctx? S_over2: S_over;
	case '\\': if ('~' == c1)
			return 2 == ctx? S_swhile: S_sconv;
		   else if ('*' == c1)
			return S_siter;
		   else
			return 2 == ctx? S_sover2: S_sover;
	case ':':  switch (c1) {
			case '\\': return S_eachl;
			case '\'': return S_eachp;
			case '/':  return S_eachr;
			case '*':  return S_iter;
			case '~':  return 2 == ctx? S_while: S_conv;
		   }
		   break;
	}
	return UNDEFINED;
}

static int adverb_p(cell x) {
	int	c0, c1;

	if (!symbol_p(x))
		return 0;
	c0 = symbol_name(x)[0];
	c1 = symbol_name(x)[1];
	return	('\'' == c0 &&    0 == c1) ||
		( ':' == c0 && '\\' == c1) ||
		( ':' == c0 && '\'' == c1) ||
		( ':' == c0 &&  '/' == c1) ||
		( '/' == c0 &&    0 == c1) ||
		( ':' == c0 &&  '~' == c1) ||
		( ':' == c0 &&  '*' == c1) ||
		('\\' == c0 &&    0 == c1) ||
		('\\' == c0 &&  '~' == c1) ||
		('\\' == c0 &&  '*' == c1);
}

static int fundef_arity(cell x) {
	int	n, m;

	if (pair_p(x)) {
		if (car(x) == S_fun0 ||
		    car(x) == S_fun1 ||
		    car(x) == S_fun2 ||
		    car(x) == S_fun3
		)
			return 0;
		n = 0;
		while (x != NIL) {
			m = fundef_arity(car(x));
			if (m > n) n = m;
			x = cdr(x);
		}
		return n;
	}
	if (S_x == x) return 1;
	if (S_y == x) return 2;
	if (S_z == x) return 3;
	return 0;
}

static cell funtype(cell f) {
	int	k;

	k = fundef_arity(f);
	return 	0 == k? S_fun0:
		1 == k? S_fun1:
		2 == k? S_fun2:
		S_fun3;
}

#define syntax_error() \
	error("syntax error", Tok)

#define token() \
	kg_read()

static int cmatch(int c) {
	return symbol_p(Tok) && symbol_name(Tok)[0] == c;
}

static int cmatch2(char *c2) {
	return	symbol_p(Tok) &&
		symbol_name(Tok)[0] == c2[0] &&
		symbol_name(Tok)[1] == c2[1];
}

static void expect(int c) {
	char	b[100];

	if (cmatch(c)) {
		Tok = token();
	}
	else {
		sprintf(b, "expected :%c, got", c);
		error(b, Tok);
	}
}

static cell expr(void);
static cell prog(int fun);

static cell function(int ctx, cell *t) {
	if (cmatch('{')) {
		Tok = token();
		Infun++;
		T = prog(1);
		car(T) = funtype(T);
		Infun--;
		expect('}');
		return T;
	}
	else if (variable_p(Tok)) {
		T = Tok;
		Tok = token();
		return T;
	}
	else {
		if (t != NULL) *t = Tok;
		T = opsym(Tok, ctx);
		Tok = token();
		return T;
	}
}

static cell conditional(void) {
	cell	n;

	Incond++;
	Tok = token();
	n = cons(expr(), NIL);
	save(n);
	expect(';');
	n = cons(expr(), n);
	car(Stack) = n;
	if (cmatch2(":|")) {
		Incond--;
		n = cons(conditional(), n);
		unsave(1);
		return cons(S_if, revb(n));
	}
	else {
		expect(';');
		n = cons(expr(), n);
		car(Stack) = n;
		Incond--;
		expect(']');
		n = unsave(1);
		return cons(S_if, revb(n));
	}
}

static cell funapp_or_proj(cell v, int proj, int *fn) {
	int	n, pa;
	cell	a;

	save(v);
	pa = 0;
	Tok = token();
	a = NIL;
	save(a);
	if (cmatch(')')) {
		Tok = token();
		n = 0;
	}
	else {
		if (proj && cmatch(';')) {
			a = cons(S_x, a);
			car(Stack) = a;
			pa++;
		}
		else {
			a = cons(expr(), a);
			car(Stack) = a;
		}
		n = 1;
		if (cmatch(';')) {
			Tok = token();
			if (proj && (cmatch(';') || cmatch(')'))) {
				a = cons(pa? S_y: S_x, a);
				car(Stack) = a;
				pa++;
			}
			else {
				a = cons(expr(), a);
				car(Stack) = a;
			}
			n = 2;
			if (cmatch(';')) {
				Tok = token();
				if ( proj && cmatch(')')) {
					a = cons(pa? S_y: S_x, a);
					car(Stack) = a;
					pa++;
				}
				else {
					a = cons(expr(), a);
					car(Stack) = a;
				}
				n = 3;
			}
		}
		expect(')');
		if (pa >= n)
			error("too few arguments in projection", VOID);
	}
	v = cons(v, revb(a));
	if (pa) {
		v = cons(n>2? S_call3: n>1? S_call2: S_call1, v);
		v = cons(v, NIL);
		v = cons(pa>1? S_fun2: S_fun1, v);
	}
	else {
		v = cons(n>2? S_call3:
			 n>1? S_call2:
			 n>0? S_call1:
			 S_call0,
			 v);
	}
	if (proj && pa > 0 && (cmatch('(') || cmatch2(":("))) {
		v = funapp_or_proj(v, 0, NULL);
		pa = 0;
	}
	if (fn != NULL) *fn = pa != 0;
	unsave(2);
	return v;
}

static cell apply_adverbs(cell f, cell a1, int ctx) {
	cell	adv, n, ex;

	Tmp = f;
	save(a1);
	save(f);
	Tmp = NIL;
	adv = ex = cons(NIL, NIL);
	if (a1 != VOID)
		adv = cons(a1, adv);
	adv = cons(f, adv);
	save(adv);
	while (adverb_p(Tok)) {
		adv = cons(adverbsym(Tok, ctx), adv);
		car(Stack) = adv;
		Tok = token();
		ctx = 1;
	}
	n = expr();
	car(ex) = n;
	f = unsave(1);
	unsave(2);
	return f;
}

#define is_var(x) \
	(x == var_name(Tok)[0] && 0 == var_name(Tok)[1])

#define operator_p(x) \
	(symbol_p(x) && is_special(symbol_name(x)[0]))

static cell factor(void) {
	cell	f, v, op;
	int	fn;

	if (	number_p(Tok) ||
		char_p(Tok) ||
		string_p(Tok)
	) {
		T = Tok;
		Tok = token();
		return T;
	}
	else if (dictionary_p(Tok)) {
		T = cons(Tok, NIL);
		T = cons(S_newdict, T);
		Tok = token();
		return T;
	}
	else if (list_p(Tok)) {
		T = cons(S_lslit, Tok);
		Tok = token();
		return T;
	}
	else if (variable_p(Tok)) {
		v = Tok;
		save(v);
		if (Infun) {
			     if (is_var('x')) v = S_x;
			else if (is_var('y')) v = S_y;
			else if (is_var('z')) v = S_z;
		}
		Tok = token();
		fn = 1;
		if (cmatch('(') || cmatch2(":("))
			v = funapp_or_proj(v, 1, &fn);
		if (adverb_p(Tok)) {
			if (!fn) error("missing verb", Tok);
			v = apply_adverbs(v, VOID, 1);
		}
		unsave(1);
		return v;
	}
	else if (cmatch('(')) {
		Tok = token();
		T = expr();
		expect(')');
		return T;
	}
	else if (cmatch('{')) {
		f = function(1, &op);
		save(f);
		fn = 1;
		if (cmatch('(') || cmatch2(":("))
			f = funapp_or_proj(f, 1, &fn);
		if (adverb_p(Tok)) {
			if (!fn) error("missing verb", Tok);
			f = apply_adverbs(f, VOID, 1);
		}
		unsave(1);
		return f;
	}
	else if (cmatch2(":[")) {
		return conditional();
	}
	else if (operator_p(Tok)) {
		f = function(1, &op);
		if (adverb_p(Tok)) {
			f = opsym(op, adverb_arity(Tok, 1));
			f = apply_adverbs(f, VOID, 1);
		}
		else {
			save(f);
			f = cons(f, cons(NIL, NIL));
			car(Stack) = f;
			v = expr();
			cadr(f) = v;
			unsave(1);
		}
		return f;
	}
	else if (symbol_p(Tok)) {
		T = Tok;
		Tok = token();
		return T;
	}
	else {
		syntax_error();
		return UNDEFINED;
	}
}

#define is_delimiter(s) \
	(')' == s[0] || \
	 '}' == s[0] || \
	 ']' == s[0] || \
	 ';' == s[0] || \
	 (':' == s[0] && '|' == s[1]))

static cell expr(void) {
	cell	f, a, y, n, dy;
	cell	op;
	char	*s;
	char	name[TOKEN_LENGTH+1];
	int	fn;

	a = factor();
	while (operator_p(Tok) || variable_p(Tok) || cmatch('{')) {
		save(a);
		if (variable_p(Tok))
			s = var_name(Tok);
		else
			s = symbol_name(Tok);
		if (operator_p(Tok) && is_delimiter(s)) {
			unsave(1);
			break;
		}
		if (':' == s[0] && ':' == s[1]) {
			if (S_x == a) error("'x' is read-only", VOID);
			if (S_y == a) error("'y' is read-only", VOID);
			if (S_z == a) error("'z' is read-only", VOID);
			if (variable_p(a)) {
				if (!Infun &&
				    Module != UNDEFINED &&
				    !is_local(var_name(a)) &&
				    !is_funvar(var_name(a))
				) {
					strcpy(name, var_name(a));
					mkmodlocal(name);
					y = make_variable(name, NIL);
					a = var_symbol(y);
				}
				else {
					a = var_symbol(a);
				}
				car(Stack) = a;
			}
		}
		dy = Tok;
		save(dy);
		op = 0;
		f = function(2, &op);
		save(f);
		if ((!operator_p(dy) || '{' == symbol_name(dy)[0]) &&
		    (cmatch('(') || cmatch2(":(")))
		{
			f = funapp_or_proj(f, 1, &fn);
			car(Stack) = f;
			if (!fn) error("dyad expected", dy);
		}
		if (adverb_p(Tok)) {
			if (op) f = opsym(op, adverb_arity(Tok, 2));
			a = apply_adverbs(f, a, 2);
		}
		else {
			n = cons(expr(), NIL);
			n = cons(a, n);
			a = cons(f, n);
			if (!operator_p(dy) || '{' == symbol_name(dy)[0])
				a = cons(S_call2, a);
		}
		unsave(3);
	}
	return a;
}

static cell rename_locals(cell loc, int id) {
	cell	n, a, nn;
	char	b1[TOKEN_LENGTH], b2[TOKEN_LENGTH+1];

	if (NIL == loc)
		return NIL;
	n = cons(NIL, NIL);
	save(n);
	while (loc != NIL) {
		strcpy(b1, symbol_name(car(loc)));
		mkglobal(b1);
		sprintf(b2, "%s`%d", b1, id);
		nn = symbol_ref(b2);
		car(n) = nn;
		loc = cdr(loc);
		if (loc != NIL) {
			a = cons(NIL, NIL);
			cdr(n) = a;
			n = cdr(n);
		}
	}
	n = unsave(1);
	return n;
}

static cell prog(int fun) {
	cell	p, ps, n, mfvs, locns, new;
	char	*s;
	int	first = 1;

	mfvs = Mod_funvars;
	locns = Locnames;
	save(ps = NIL);
	for (;;) {
		p = expr();
		ps = cons(p, ps);
		car(Stack) = ps;
		if (!cmatch(';'))
			break;
		if (fun && first && pair_p(p) && car(p) == S_lslit) {
			if (Module != UNDEFINED)
				Mod_funvars = cons(cdr(p), Mod_funvars);
			Locnames = cons(new_atom(Local_id, cdr(p)),
					Locnames);
			new = rename_locals(cdr(p), Local_id++);
			cdr(p) = new;
			car(ps) = p;
		}
		first = 0;
		Tok = token();
	}
	Mod_funvars = mfvs;
	Locnames = locns;
	car(Stack) = ps = revb(ps);
	if (	fun &&
		ps != NIL &&
		pair_p(car(ps)) &&
		S_lslit == caar(ps) &&
		cdr(ps) != NIL)
	{
		for (p = car(ps); p != NIL; p = cdr(p)) {
			if (symbol_p(car(p))) {
				s = symbol_name(car(p));
				n = make_variable(s, NO_VALUE);
				car(p) = n;
			}
		}
	}
	return cons(S_prog, unsave(1));
}

static cell parse(char *p) {
	cell	x;

	open_input_string(p);
	Tok = token();
	if (END_OF_FILE == Tok) {
		close_input_string();
		return END_OF_FILE;
	}
	x = prog(0);
	if (Tok != END_OF_FILE)
		syntax_error();
	close_input_string();
	return x;
}

/*
 * Bytecode Compiler, Code generator
 */

static void emit(cell p) {
	cell	new;

	save(p);
	if (NIL == P) {
		P = Prog = cons(p, NIL);
	}
	else {
		new = cons(p, NIL);
		cdr(P) = new;
		P = cdr(P);
	}
	unsave(1);
}

static void	comp(cell p);

static void comp_funcall(cell p, int k) {
	cell	q;
	int	i;

	if (length(cddr(p)) != k)
		error("wrong argument count", cdr(p));
	for (i=0, q = cddr(p); i<k; q = cdr(q), i++)
		comp(car(q));
	comp(cadr(p));
	emit(3==k? S_call3: 2==k? S_call2: 1==k? S_call1: S_call0);
	emit(3==k? S_pop3: 2==k? S_pop2: 1==k? S_pop1: S_pop0);
}

static void comp_fundef(cell p, int k) {
	cell	q, f;

	f = P;
	for (q = cdr(p); q != NIL; q = cdr(q)) {
		comp(car(q));
		if (cdr(q) != NIL)
			emit(S_clear);
	}
	P = f; emit(NIL == f? Prog: cdr(f));
	emit(3==k? S_fun3: 2==k? S_fun2: 1==k? S_fun1: S_fun0);
}

#define adverb_op_p(op) \
	(op) == S_conv || (op) == S_each || (op) == S_sconv || \
	(op) == S_iter || (op) == S_siter || (op) == S_while || \
	(op) == S_swhile || (op) == S_eachp || (op) == S_over || \
	(op) == S_sover || (op) == S_each2 || (op) == S_eachl || \
	(op) == S_eachr || (op) == S_over2 || (op) == S_sover2

static int adverb_op_arity(cell op) {
	if (op == S_conv) return 1;
	if (op == S_each) return 1;
	if (op == S_sconv) return 1;
	if (op == S_iter) return 1;
	if (op == S_siter) return 1;
	if (op == S_while) return 1;
	if (op == S_swhile) return 1;
	if (op == S_eachp) return 2;
	if (op == S_over) return 2;
	if (op == S_sover) return 2;
	if (op == S_each2) return 2;
	if (op == S_eachl) return 2;
	if (op == S_eachr) return 2;
	if (op == S_over2) return 2;
	if (op == S_sover2) return 2;
	error("internal: adverb_op_arity():  bad adverb op", op);
	return 0;
}

static int adverb_op_ctx(cell op) {
	if (op == S_conv) return 1;
	if (op == S_each) return 1;
	if (op == S_sconv) return 1;
	if (op == S_iter) return 2;
	if (op == S_siter) return 2;
	if (op == S_while) return 2;
	if (op == S_swhile) return 2;
	if (op == S_eachp) return 1;
	if (op == S_over) return 1;
	if (op == S_sover) return 1;
	if (op == S_each2) return 2;
	if (op == S_eachl) return 2;
	if (op == S_eachr) return 2;
	if (op == S_over2) return 2;
	if (op == S_sover2) return 2;
	error("internal: adverb_op_ctx():  bad adverb op", op);
	return 0;
}

static void comp_adverb(cell p, int args) {
	cell	f, q;
	int	ctx, aa, nest;

	aa = adverb_op_arity(car(p));
	if (args) {
		f = p;
		for (q = p; adverb_op_p(car(q)); q = cdr(q))
			f = q;
		ctx = adverb_op_ctx(car(f));
		if (2 == ctx) {
			if (NIL == cdr(q) || NIL == cddr(q) || cdddr(q) != NIL)
				error("wrong adverb context", opname(car(f)));
			comp(caddr(q));
			comp(cadr(q));
			emit(S_swap);
		}
		else {
			if (NIL == cdr(q) || cddr(q) != NIL)
				error("wrong adverb context", car(f));
			comp(cadr(q));
		}
	}
	nest = 0;
	if (adverb_op_p(cadr(p))) {
		f = P;
		comp_adverb(cdr(p), 0);
		P = f; emit(cdr(f));
		nest = 1;
	}
	if (nest && aa > 1)
		error("monad expected in chained adverb", opname(car(p)));
	if (	variable_p(cadr(p)) &&
		cadr(p) != S_x &&
		cadr(p) != S_y &&
		cadr(p) != S_z &&
		'%' == var_name(cadr(p))[0])
	{
		if (!nest) emit(cons(cadr(p), NIL));
		emit(2==aa? S_imm2: S_imm1);
	}
	else {
		f = P;
		comp(cadr(p));
		emit(2==aa? S_call2: S_call1);
		emit(2==aa? S_pop2: S_pop1);
		P = f; emit(cdr(f));
		emit(2==aa? S_imm2: S_imm1);
	}
	emit(car(p));
}

static void comp(cell p) {
	cell	op, q;

	if (!atom_p(p))
		op = car(p);
	else
		op = UNDEFINED;
	if (atom_p(p)) {
		emit(p);
	}
	else if (op == S_lslit) {
		emit(cdr(p));
	}
	else if (op == S_newdict) {
		emit(cadr(p));
		emit(S_newdict);
	}
	else if (op == S_atom || op == S_char || op == S_down ||
		 op == S_enum || op == S_expand || op == S_first ||
		 op == S_floor || op == S_format || op == S_group ||
		 op == S_list || op == S_neg || op == S_not ||
		 op == S_range || op == S_recip || op == S_rev ||
		 op == S_shape || op == S_size || op == S_transp ||
		 op == S_up || op == S_undef)
	{
		if (NIL == cdr(p) || cddr(p) != NIL)
			error("wrong argument count", op);
		comp(cadr(p));
		emit(car(p));
	}
	else if (op == S_amend || op == S_amendd || op == S_cut ||
		 op == S_def || op == S_div || op == S_drop ||
		 op == S_eq || op == S_find || op == S_form ||
		 op == S_format2 || op == S_gt || op == S_index ||
		 op == S_indexd || op == S_intdiv || op == S_join ||
		 op == S_lt || op == S_match || op == S_max ||
		 op == S_min || op == S_minus || op == S_plus ||
		 op == S_power || op == S_rem || op == S_reshape ||
		 op == S_rot || op == S_split || op == S_take ||
		 op == S_times)
	{
		if (NIL == cdr(p) || NIL == cddr(p) || cdddr(p) != NIL)
			error("wrong argument count", op);
		if (	op == S_power &&
			HL_power != UNDEFINED &&
			!false_p(var_value(S_fastpow)))
		{
			comp(cadr(p));
			comp(caddr(p));
			emit(HL_power);
			emit(S_call2);
			emit(S_pop2);
		}
		else {
			comp(caddr(p));
			comp(cadr(p));
			emit(S_swap);
			emit(car(p));
		}
	}
	else if (op == S_conv || op == S_each || op == S_sconv ||
		 op == S_iter || op == S_siter || op == S_while ||
		 op == S_swhile || op == S_eachp || op == S_over ||
		 op == S_sover || op == S_each2 || op == S_eachl ||
		 op == S_eachr || op == S_over2 || op == S_sover2)
	{
		comp_adverb(p, 1);
	}
	else if (op == S_prog) {
		for (p = cdr(p); p != NIL; p = cdr(p)) {
			comp(car(p));
			if (cdr(p) != NIL)
				emit(S_clear);
		}
	}
	else if (op == S_if) {
		comp(cadr(p));
		q = P; comp(caddr(p));
		P = q; emit(cdr(q));
		q = P; comp(cadddr(p));
		P = q; emit(cdr(q));
		emit(S_if);
	}
	else if (op == S_call0)  { comp_funcall(p, 0); }
	else if (op == S_call1)  { comp_funcall(p, 1); }
	else if (op == S_call2)  { comp_funcall(p, 2); }
	else if (op == S_call3)  { comp_funcall(p, 3); }
	else if (op == S_fun0)   { comp_fundef(p, 0); }
	else if (op == S_fun1)   { comp_fundef(p, 1); }
	else if (op == S_fun2)   { comp_fundef(p, 2); }
	else if (op == S_fun3)   { comp_fundef(p, 3); }
	else {
		error("internal: unknown operator in AST", p);
	}
}

static cell compile(char *s) {
	cell	p;

	p = parse(s);
	if (END_OF_FILE == p)
		return END_OF_FILE;
	save(p);
	P = Prog = NIL;
	comp(p);
	if (Debug) {
		kg_write(Prog);
		nl();
	}
	unsave(1);
	return Prog;
}

/*
 * Interpreters
 */

static cell pjoin(cell a, cell b) {
	Tmp = b;
	save(a);
	save(b);
	Tmp = NIL;
	a = join(a, b);
	unsave(2);
	return a;
}

static cell load(cell x, int v, int scr) {
	int	p, oldp, oline, oldprog;
	char	*s, *kp, kpbuf[TOKEN_LENGTH+1];
	cell	n = NIL; /*LINT*/
	cell	r = NIL;

	save(x);
	if ('.' == string(x)[0] || '/' == string(x)[0]) {
		save(NIL);
		n = x;
		p = open_input_port(string(x));
		if (p < 0) {
			n = pjoin(x, make_string(".kg", 3));
			car(Stack) = n;
			p = open_input_port(string(n));
		}
	}
	else {
		s = getenv("KLONGPATH");
		if (NULL == s) {
			strcpy(kpbuf, DFLPATH);
		}
		else {
			strncpy(kpbuf, s, TOKEN_LENGTH);
			kpbuf[TOKEN_LENGTH] = 0;
			if (strlen(kpbuf) >= TOKEN_LENGTH) {
				error("KLONGPATH too long!", VOID);
				return UNDEFINED;
		}
		}
		kp = strtok(kpbuf, ":");
		p = -1;
		save(NIL);
		while (kp != NULL) {
			n = pjoin(make_string("/", 1), x);
			car(Stack) = n;
			n = pjoin(make_string(kp, strlen(kp)), n);
			if ((p = open_input_port(string(n))) >= 0)
				break;
			n = pjoin(n, make_string(".kg", 3));
			if ((p = open_input_port(string(n))) >= 0)
				break;
			kp = strtok(NULL, ":");
		}
	}
	if (p < 0) {
		error(".l: cannot open file", x);
		unsave(2);
		return UNDEFINED;
	}
	if (1 == v && 0 == Quiet && NIL == Loading) {
		prints("loading ");
		kg_write(n);
		nl();
	}
	close_input_string();
	lock_port(p);
	oldp = input_port();
	set_input_port(p);
	oline = Line;
	Line = 1;
	Loading = cons(x, Loading);
	oldprog = Prog_chan;
	Prog_chan = p;
	save(r);
	if (scr) kg_getline(kpbuf, TOKEN_LENGTH);
	for (;;) {
		Tok = token();
		if (port_eof(p))
			break;
		if (END_OF_FILE == Tok)
			continue;
		x = prog(0);
		if (Tok != END_OF_FILE)
			syntax_error();
		save(x);
		P = Prog = 0;
		comp(x);
		x = Prog;
		unsave(1);
		if (s9_aborted())
			break;
		set_input_port(oldp);
		eval(x);
		r = car(Stack) = car(Dstack);
		set_input_port(p);
		op_clear();
	}
	Prog_chan = oldprog;
	Loading = cdr(Loading);
	Line = oline;
	set_input_port(oldp);
	unlock_port(p);
	close_port(p);
	unsave(3);
	return r;
}

static cell evalstr(cell x) {
	char	buf[TOKEN_LENGTH+1];

	if (string_len(x) >= TOKEN_LENGTH)
		return error("evalstr: expression too long", VOID);
	strcpy(buf, string(x));
	x = compile(buf);
	if (END_OF_FILE == x)
		return END_OF_FILE;
	if (s9_aborted())
		return UNDEFINED;
	eval(x);
	if (s9_aborted())
		return UNDEFINED;
	return pop();
}

static void eval_arg(char *s, int echo) {
	cell	x;

	x = make_string(s, strlen(s));
	save(x);
	x = evalstr(x);
	unsave(1);
	if (s9_aborted())
		bye(1);
	if (0 == echo)
		return;
	kg_write(x);
	nl();
}

static void interpret(void) {
	cell	x;

	for (;;) {
		Prog_chan = 0;
		reset_std_ports();
		Dstack = NIL;
		s9_reset();
		Intr = 0;
		if (!Quiet) {
			prints("        ");
			flush();
		}
		if (kg_getline(Inbuf, TOKEN_LENGTH) == NULL && Intr == 0)
			break;
		if (Intr)
			continue;
		x = compile(Inbuf);
		transcribe(make_string(Inbuf, strlen(Inbuf)), 1);
		if (s9_aborted() || atom_p(x))
			continue;
		Safe_dict = Sys_dict;
		eval(x);
		if (s9_aborted()) {
			Sys_dict = Safe_dict;
			continue;
		}
		set_output_port(1);
		kg_write(car(Dstack));
		nl();
		transcribe(car(Dstack), 0);
		var_value(S_it) = car(Dstack);
	}
}

static void make_image_file(char *s) {
	char	magic[16];
	char	errbuf[128];
	char	*r;

	memcpy(magic, "KLONGYYYYMMDD___", 16);
	memcpy(&magic[5], VERSION, 8);
	// image_vars(Image_vars);
	r = dump_image(s, magic);
	if (NULL == r)
		return;
	sprintf(errbuf, "kg: dump_image(): %s", r);
	fatal(errbuf);
}

static void load_image_file(void) {
	char	magic[16];
	char	kpbuf[TOKEN_LENGTH+1];
	char	errbuf[128];
	char	*r, *kp;
	FILE	*f;

	memcpy(magic, "KLONGYYYYMMDD___", 16);
	memcpy(&magic[5], VERSION, 8);
	if (NULL == (kp = getenv("KLONGPATH")))
		kp = DFLPATH;
	if (strlen(kp) >= TOKEN_LENGTH)
		fatal("KLONGPATH too long");
	strcpy(kpbuf, kp);
	kp = strtok(kpbuf, ":");
	while (kp != NULL) {
		sprintf(Image_path, "%s/klong.image", kp);
		f = fopen(Image_path, "r");
		if (f != NULL) {
			r = load_image(Image_path, magic);
			if (r != NULL) {
				fprintf(stderr, "kg: bad image file: %s\n",
					Image_path);
				sprintf(errbuf, "load_image(): %s", r);
				fatal(errbuf);
			}
		}
		kp = strtok(NULL, ":");
	}
	HL_power = find_var(".pow");
}

/*
 * Initialization and Startup
 */

static void init(void) {
	int	i;
	cell	n, op;

	cleartrace();
	s9_init(GC_root, NULL, NULL);
	// image_vars(Image_vars);
	Dstack = NIL;
	Frame = NIL;
	Locals = NIL;
	Quiet = 0;
	Script = 0;
	Sys_dict = NIL;
	From_chan = 0;
	To_chan = 1;
	Prog_chan = 0;
	Line = 1;
	Listlev = 0;
	Incond = 0;
	Report = 1;
	Transcript = -1;
	Infun = 0;
	Loading = NIL;
	Module = UNDEFINED;
	Locnames = NIL;
	Local_id = 0;
	Display = 0;
	Intr = 0;
	Image_path[0] = 0;
	HL_power = UNDEFINED;
	Barrier = new_atom(T_BARRIER, 0);
	srand(time(NULL)*123);
	for (i = 0; Ops[i].name != NULL; i++) {
		op = make_primop(i, Ops[i].syntax);
		make_variable(Ops[i].name, op);
	}
	S_amend = var_ref("%amend");
	S_amendd = var_ref("%amendd");
	S_atom = var_ref("%atom");
	S_call0 = var_ref("%call0");
	S_call1 = var_ref("%call1");
	S_call2 = var_ref("%call2");
	S_call3 = var_ref("%call3");
	S_char = var_ref("%char");
	S_clear = var_ref("%clear");
	S_conv = var_ref("%conv");
	S_cut = var_ref("%cut");
	S_def = var_ref("%def");
	S_div = var_ref("%div");
	S_down = var_ref("%down");
	S_drop = var_ref("%drop");
	S_each = var_ref("%each");
	S_each2 = var_ref("%each2");
	S_eachl = var_ref("%eachl");
	S_eachp = var_ref("%eachp");
	S_eachr = var_ref("%eachr");
	S_enum = var_ref("%enum");
	S_eq = var_ref("%eq");
	S_expand = var_ref("%expand");
	S_find = var_ref("%find");
	S_first = var_ref("%first");
	S_floor = var_ref("%floor");
	S_form = var_ref("%form");
	S_format = var_ref("%format");
	S_format2 = var_ref("%format2");
	S_fun0 = var_ref("%fun0");
	S_fun1 = var_ref("%fun1");
	S_fun2 = var_ref("%fun2");
	S_fun3 = var_ref("%fun3");
	S_group = var_ref("%group");
	S_gt = var_ref("%gt");
	S_if = var_ref("%if");
	S_imm1 = var_ref("%imm1");
	S_imm2 = var_ref("%imm2");
	S_index = var_ref("%index");
	S_indexd = var_ref("%indexd");
	S_intdiv = var_ref("%intdiv");
	S_it = var_ref("it");
	S_iter = var_ref("%iter");
	S_join = var_ref("%join");
	S_list = var_ref("%list");
	S_lslit = var_ref("%lslit");
	S_lt = var_ref("%lt");
	S_match = var_ref("%match");
	S_max = var_ref("%max");
	S_min = var_ref("%min");
	S_minus = var_ref("%minus");
	S_neg = var_ref("%neg");
	S_newdict = var_ref("%newdict");
	S_not = var_ref("%not");
	S_over = var_ref("%over");
	S_over2 = var_ref("%over2");
	S_plus = var_ref("%plus");
	S_pop0 = var_ref("%pop0");
	S_pop1 = var_ref("%pop1");
	S_pop2 = var_ref("%pop2");
	S_pop3 = var_ref("%pop3");
	S_power = var_ref("%power");
	S_prog = var_ref("%prog");
	S_range = var_ref("%range");
	S_recip = var_ref("%recip");
	S_rem = var_ref("%rem");
	S_reshape = var_ref("%reshape");
	S_rev = var_ref("%rev");
	S_rot = var_ref("%rot");
	S_sconv = var_ref("%sconv");
	S_siter = var_ref("%siter");
	S_sover = var_ref("%sover");
	S_sover2 = var_ref("%sover2");
	S_swhile = var_ref("%swhile");
	S_shape = var_ref("%shape");
	S_size = var_ref("%size");
	S_split = var_ref("%split");
	S_swap = var_ref("%swap");
	S_syscall = var_ref("%syscall");
	S_take = var_ref("%take");
	S_times = var_ref("%times");
	S_transp = var_ref("%transp");
	S_up = var_ref("%up");
	S_undef = var_ref("%undef");
	S_while = var_ref("%while");
	S_x = var_ref("%x");
	S_y = var_ref("%y");
	S_z = var_ref("%z");
	S_argv = var_ref(".a");
	n = var_ref(".cin");
	var_value(n) = make_port(0, T_INPUT_PORT);
	n = var_ref(".cout");
	var_value(n) = make_port(1, T_OUTPUT_PORT);
	n = var_ref(".cerr");
	var_value(n) = make_port(2, T_OUTPUT_PORT);
	S_cols = var_ref(".cols");
	var_value(S_cols) = make_integer(80);
	Epsilon_var = make_variable(".e", Epsilon);
	S_edit = var_ref(".edit");
	var_value(S_edit) = make_integer(1);
	S_thisfn = var_ref(".f");
	S_fastpow = var_ref(".fastpow");
	var_value(S_fastpow) = make_integer(0);
	S_host = var_ref(".h");
#ifdef plan9
	n = symbol_ref("plan9");
#else
	n = symbol_ref("unix");
#endif
	var_value(S_host) = n;
	n = var_ref(".helpdb");
	var_value(n) = NIL;
	for (i = 0; Sysfns[i].name != NULL; i++) {
		n = cons(S_syscall, NIL);
		save(n);
		n = make_integer(i);
		n = cons(n, unsave(1));
		op = make_function(n, 0, Sysfns[i].arity);
		make_variable(Sysfns[i].name, op);
	}
}

#ifdef plan9
 void keyboard_interrupt(void *dummy, char *note) {
	USED(dummy);
	if (strstr(note, "interrupt") == NULL)
		noted(NDFLT);
	reset_std_ports();
	error("interrupted", VOID);
	Intr = 1;
	noted(NCONT);
 }
#else
 void keyboard_interrupt(int sig) {
	reset_std_ports();
	error("interrupted", VOID);
	Intr = 1;
	handle_sigint();
 }

 void keyboard_quit(int sig) {
	fatal("quit signal received, exiting");
 }
#endif

static void usage(int x) {
	prints("Usage: kg [-dhnqsuv?] [-e expr] [-l file] [-o file]");
	nl();
	prints("          [-r expr] [-t file] [file [args]] [-- args]");
	nl();
	if (x) {
		bye(1);
	}
}

static void longusage(void) {
	char	*s;

	nl();
	prints("Klong ");
	prints(VERSION);
	prints(" by Nils M Holm, in the public domain");
	nl();
	nl();
	usage(0);
	nl();
	prints("-d         debug (print bytecode and call traces)"); nl();
	prints("-e expr    evaluate expression, no interactive mode"); nl();
	prints("-l file    load program from file"); nl();
	prints("-n         clean start: don't parse KLONGOPTS, don't"); nl();
	prints("           load any image (must be first option!)"); nl();
	prints("-o file    output image to file, then exit"); nl();
	prints("-q         quiet (no banner, no prompt, exit on errors)");
	nl();
	prints("-r expr    run expr (like -e, but don't print result)"); nl();
	prints("-s         skip first line (#!) in script files"); nl();
	prints("-t file    send transcript to file"); nl();
	prints("-u         allocate unlimited memory (use with care!)"); nl();
	prints("file args  run program with arguments, then exit"); nl();
	prints("-- args    bind remaining arguments to .a"); nl();
	nl();
	if ((s = getenv("KLONGPATH")) != NULL) {
		prints("KLONGPATH = ");
		prints(s);
		nl();
	}
	if ((s = getenv("KLONGOPTS")) != NULL) {
		prints("KLONGOPTS = ");
		prints(s);
		nl();
	}
	if (Image_path[0] != 0) {
		prints("Imagefile = ");
		prints(Image_path);
		nl();
	}
	nl();
	bye(0);
}

static cell	New;

#define setargv(a) \
	do { \
		New = argv_to_list(&argv[a]); \
		var_value(S_argv) = New; \
	} while (0)

static int readopts(int argc, char **argv, int *p_loop, int *p_endargs) {
	int	i, j, echo, loop = 1, endargs = 0;

	for (i=1; i<argc; i++) {
		if (argv[i][0] != '-')
			break;
		for (j=1; argv[i][j]; j++) {
			switch (argv[i][j]) {
			case '?':
			case 'v':
			case 'h':	longusage();
					break;
			case 'd':	Debug = 1;
					break;
			case 'r':
			case 'e':	echo = argv[i][j] == 'e';
					if (++i >= argc)
						usage(1);
					Quiet = 1;
					setargv(i+1);
					eval_arg(argv[i], echo);
					j = strlen(argv[i])-1;
					loop = 0;
					break;
			case 'l':	if (++i >= argc)
						usage(1);
					load(make_string(argv[i],
							strlen(argv[i])),
						0, 0);
					j = strlen(argv[i])-1;
					break;
			case 'n':	fprintf(stderr, "kg: -n must be"
							" first option\n");
					bye(1);
			case 'o':	if (++i >= argc)
						usage(1);
					make_image_file(argv[i]);
					bye(0);
					break;
			case 'q':	Quiet = 1;
					break;
			case 's':	Script = 1;
					break;
			case 't':	if (++i >= argc)
						usage(1);
					transcript(argv[i]);
					j = strlen(argv[i])-1;
					break;
			case 'u':	set_node_limit(0);
					set_vector_limit(0);
					break;
			case '-':	endargs = 1;
					break;
			default:	usage(1);
					break;
			}
		}
		if (endargs) {
			i++;
			break;
		}
	}
	if (p_endargs != NULL && p_loop != NULL) {
		*p_endargs = endargs;
		*p_loop = loop;
	}
	return i;
}

static void klongopts(void) {
	#define MAX 100
	char	*a[MAX], *k, *kp;
	int	i;

	if ((k = getenv("KLONGOPTS")) == NULL)
		return;
	k = strdup(k);
	kp = strtok(k, " ");
	a[0] = "";
	i = 1;
	while (kp != NULL) {
		if (i >= MAX-1)
			fatal("too many KLONGOPTS arguments");
		a[i++] = kp;
		kp = strtok(NULL, " ");
	}
	a[i] = NULL;
	readopts(i, a, NULL, NULL);
	free(k);
}

int main(int argc, char **argv) {
	int	i, loop, endargs;

	init();
	if (!(argc > 1 && '-' == argv[1][0] && 'n' == argv[1][1])) {
		klongopts();
		load_image_file();
		i = readopts(argc, argv, &loop, &endargs);
	}
	else {
		i = readopts(argc-1, argv+1, &loop, &endargs) + 1;
	}
	if (endargs == 0 && i < argc) {
		Quiet = 1;
		setargv(i+1);
		load(make_string(argv[i], strlen(argv[i])), 0, Script);
		bye(0);
	}
	if (!Quiet) {
		handle_sigint();
		handle_sigquit();
		prints("        Klong ");
		prints(VERSION);
		nl();
	}
	if (loop) {
		setargv(i);
		interpret();
	}
	if (!Quiet) prints("bye!\n");
	bye(0);
	return 0;
}


/*
 * S9core Toolkit, Mk IVd
 * By Nils M Holm, 2007-2019
 * In the public domain
 *
 * Under jurisdictions without a public domain, the CC0 applies.
 * See the file CC0 for a copy of the license.
 */

#define S9_S9CORE

#undef apply_prim
#undef argv_to_list
#undef asctol
#undef bignum_abs
#undef bignum_add
#undef bignum_divide
#undef bignum_equal_p
#undef bignum_even_p
#undef bignum_less_p
#undef bignum_multiply
#undef bignum_negate
#undef bignum_shift_left
#undef bignum_shift_right
#undef bignum_subtract
#undef bignum_to_int
#undef bignum_to_real
#undef bignum_to_string
#undef blockread
#undef blockwrite
#undef close_input_string
#undef close_port
#undef cons3
#undef conses
#undef cons_stats
#undef copy_string
#undef count
#undef dump_image
#undef error_port
#undef exponent_chars
#undef fatal
#undef find_symbol
#undef flat_copy
#undef flush
#undef gc
#undef gc_verbosity
#undef gcv
#undef get_counters
// #undef image_vars
#undef input_port
#undef inport_open_p
#undef integer_string_p
#undef intern_symbol
#undef int_to_bignum
#undef io_reset
#undef io_status
#undef length
#undef load_image
#undef lock_port
#undef make_char
#undef make_integer
#undef make_port
#undef make_primitive
#undef make_real
#undef make_string
#undef make_symbol
#undef make_vector
#undef mem_error_handler
#undef mkfix
#undef new_port
#undef new_vec
#undef open_input_port
#undef open_input_string
#undef open_output_port
#undef output_port
#undef outport_open_p
#undef port_eof
#undef print_bignum
#undef print_expanded_real
#undef print_real
#undef print_sci_real
#undef printer_limit
#undef prints
#undef read_counter
#undef readc
#undef real_abs
#undef real_add
#undef real_approx_p
#undef real_ceil
#undef real_divide
#undef real_equal_p
#undef real_exponent
#undef real_floor
#undef real_integer_p
#undef real_less_p
#undef real_mantissa
#undef real_multiply
#undef real_negate
#undef real_negative_p
#undef real_positive_p
#undef real_power
#undef real_round
#undef real_sqrt
#undef real_subtract
#undef real_to_bignum
#undef real_to_string
#undef real_trunc
#undef real_zero_p
#undef rejectc
#undef reset_counter
#undef reset_std_ports
#undef run_stats
#undef set_input_port
#undef set_node_limit
#undef set_output_port
#undef set_printer_limit
#undef set_vector_limit
#undef string_numeric_p
#undef string_to_bignum
#undef string_to_number
#undef string_to_real
#undef string_to_symbol
#undef symbol_ref
#undef symbol_table
#undef symbol_to_string
#undef typecheck
#undef unlock_port
#undef unsave
#undef writec

/*
 * Scheme 9 from Empty Space, Refactored
 * By Nils M Holm, 2007-2019
 * In the public domain
 *
 * Interface for extension procedures.
 */

extern cell	Rts;
extern int	Sp;

#define parg(n)	car(vector(*GC_stack)[*GC_stkptr-(n)])
#define narg()	fixval(vector(*GC_stack)[*GC_stkptr])

#define BOL T_BOOLEAN  
#define CHR T_CHAR     
#define INP T_INPUT_PORT
#define INT T_INTEGER  
#define LST T_LIST     
#define OUP T_OUTPUT_PORT
#define PAI T_PAIR     
#define FUN T_FUNCTION 
#define REA T_REAL     
#define STR T_STRING   
#define SYM T_SYMBOL   
#define VEC T_VECTOR   
#define ___ T_ANY

void add_primitives(char *name, S9_PRIM *p);
// void error(char *msg, cell expr);
cell integer_value(char *src, cell x);


/*
 * Global state
 */

static int	Cons_segment_size,
		Vec_segment_size;
static int	Cons_pool_size,
		Vec_pool_size;

static int	Verbose_GC = 0;

s9_cell		*Car,
		*Cdr;
char		*Tag;

s9_cell		*Vectors;
s9_cell		Nullvec;
s9_cell		Nullstr;
s9_cell		Blank;

cell		Stack;

static cell	Protect;
static int	Protp;

static cell	Free_list;
static cell	Free_vecs;

S9_PRIM		*Primitives;
static int	Last_prim,
		Max_prims;

static cell	Tmp_car,
		Tmp_cdr;
		// Tmp;

static cell	Symbols;
static cell	Symhash;

static int	Printer_count,
		Printer_limit;

static int	IO_error;

FILE		*Ports[S9_MAX_PORTS];
static char	Port_flags[S9_MAX_PORTS];

int		Input_port,
		Output_port,
		Error_port;

static char	*Str_outport;
static int	Str_outport_len;
static char	*Str_inport;
static char	Rejected[2];

static long     Node_limit,
		Vector_limit;

static char	*Exponent_chars;
// static cell	**Image_vars;

static void	(*Mem_error_handler)(int src);

/* Predefined bignum literals */
cell	Zero,
	One,
	Two,
	Ten;

/* Smallest value by which two real numbers can differ:
 * 10 ^ -(S9_MANTISSA_SIZE+1)
 */
cell	Epsilon;

/* Internal GC roots */
static cell	*GC_int_roots[] = {
			&Stack, &Symbols, &Symhash, &Tmp, &Tmp_car,
			&Tmp_cdr, &Zero, &One, &Two, &Ten, &Epsilon,
			&Nullvec, &Nullstr, &Blank, &Protect, NULL };

/* External GC roots */
static cell	**GC_ext_roots = NULL;

/* GC stack */
cell	*S9_gc_stack;
int	*S9_gc_stkptr;

/*
 * Internal vector representation
 */

#define RAW_VECTOR_LINK         0
#define RAW_VECTOR_INDEX        1
#define RAW_VECTOR_SIZE         2
#define RAW_VECTOR_DATA         3

/*
 * Internal node protection
 */

#ifdef S9_BITS_PER_WORD_64
 #define PROT_STACK_LEN	400
#else
 #define PROT_STACK_LEN	200
#endif

static void prot(cell x) {
	if (Protp >= PROT_STACK_LEN-1)
		s9_fatal("internal prot() stack overflow");
	vector(Protect)[++Protp] = x;
}

static cell unprot(int n) {
	cell	x;

	if (Protp - n < -1)
		s9_fatal("internal prot() stack underflow");
	x = vector(Protect)[Protp-n+1];
	while (n) {
		vector(Protect)[Protp--] = UNDEFINED;
		n--;
	}
	return x;
}

#define pref(n)	(vector(Protect)[Protp-(n)])

/*
 * Counting
 */

static int	Run_stats, Cons_stats;

static s9_counter	Conses,
			Nodes,
			Vecspace,
			Collections;

void s9_run_stats(int x) {
	Run_stats = x;
	if (Run_stats) {
		s9_reset_counter(&Nodes);
		s9_reset_counter(&Conses);
		s9_reset_counter(&Vecspace);
		s9_reset_counter(&Collections);
	}
}

void s9_cons_stats(int x) {
	Cons_stats = x;
}

void s9_reset_counter(s9_counter *c) {
	c->n = 0;
	c->n1k = 0;
	c->n1m = 0;
	c->n1g = 0;
	c->n1t = 0;
}

void s9_count(s9_counter *c) {
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
					c->n1g -= 1000;
					c->n1t++;
				}
			}
		}
	}
}

void s9_countn(s9_counter *c, int n) {
	c->n += n;
	if (c->n >= 1000) {
		c->n1k += c->n / 1000;
		c->n = c->n % 1000;
		if (c->n1k >= 1000) {
			c->n1m += c->n1k / 1000;
			c->n1k = c->n1k % 1000;
			if (c->n1m >= 1000) {
				c->n1g += c->n1m / 1000;
				c->n1m = c->n1m % 1000;
				if (c->n1g >= 1000) {
					c->n1t += c->n1g / 1000;
					c->n1g = c->n1g % 1000;
				}
			}
		}
	}
}

cell s9_read_counter(s9_counter *c) {
	cell	n, m;

	n = s9_make_integer(c->n);
	n = cons(n, NIL);
	prot(n);
	m = s9_make_integer(c->n1k);
	n = cons(m, n);
	pref(0) = n;
	m = s9_make_integer(c->n1m);
	n = cons(m, n);
	pref(0) = n;
	m = s9_make_integer(c->n1g);
	n = cons(m, n);
	pref(0) = n;
	m = s9_make_integer(c->n1t);
	n = cons(m, n);
	unprot(1);
	return n;
}

void s9_get_counters(s9_counter **nc, s9_counter **cc, s9_counter **vc,
			s9_counter **gc) {
	*nc = &Nodes;
	*cc = &Conses;
	*vc = &Vecspace;
	*gc = &Collections;
}

/*
 * Raw I/O
 */

int s9_inport_open_p(void) {
	return Ports[Input_port] != NULL;
}

int s9_outport_open_p(void) {
	return Ports[Output_port] != NULL;
}

int s9_readc(void) {
	int	c, i;

	if (Str_inport != NULL) {
		for (i=1; i>=0; i--) {
			if (Rejected[i] > -1) {
				c = Rejected[i];
				Rejected[i] = -1;
				return c;
			}
		}
		if (0 == *Str_inport) {
			return EOF;
		}
		else {
			return *Str_inport++;
		}
	}
	else {
		if (!s9_inport_open_p())
			s9_fatal("s9_readc(): input port is not open");
		return getc(Ports[Input_port]);
	}
}

void s9_rejectc(int c) {
	if (Str_inport != NULL) {
		if (Rejected[0] == -1)
			Rejected[0] = c;
		else
			Rejected[1] = c;
	}
	else {
		ungetc(c, Ports[Input_port]);
	}
}

void s9_writec(int c) {
	if (!s9_outport_open_p())
		s9_fatal("s9_writec(): output port is not open");
	(void) putc(c, Ports[Output_port]);
}

char *s9_open_input_string(char *s) {
	char	*os;

	os = Str_inport;
	Str_inport = s;
	Rejected[0] = Rejected[1] = -1;
	return os;
}

void s9_close_input_string(void) {
	Str_inport = NULL;
}

void s9_flush(void) {
	if (fflush(Ports[Output_port]))
		IO_error = 1;
}

void s9_set_printer_limit(int k) {
	Printer_limit = k;
	Printer_count = 0;
}

int s9_printer_limit(void) {
	return Printer_limit && Printer_count >= Printer_limit;
}

void s9_blockwrite(char *s, int k) {
	if (Str_outport) {
		if (k >= Str_outport_len) {
			k = Str_outport_len;
			IO_error = 1;
		}
		memcpy(Str_outport, s, k);
		Str_outport += k;
		Str_outport_len -= k;
		*Str_outport = 0;
		return;
	}
	if (!s9_outport_open_p())
		s9_fatal("s9_blockwrite(): output port is not open");
	if (Printer_limit && Printer_count > Printer_limit) {
		if (Printer_limit > 0)
			fwrite("...", 1, 3, Ports[Output_port]);
		Printer_limit = -1;
		return;
	}
	if (fwrite(s, 1, k, Ports[Output_port]) != k)
		IO_error = 1;
	if (Output_port == 1 && s[k-1] == '\n')
		s9_flush();
	Printer_count += k;
}

int s9_blockread(char *s, int k) {
	int	n;

	if (!s9_inport_open_p())
		s9_fatal("s9_blockread(): input port is not open");
	n = fread(s, 1, k, Ports[Input_port]);
	if (n < 0) IO_error = 1;
	return n;
}

void s9_prints(char *s) {
	s9_blockwrite(s, strlen(s));
}

int s9_io_status(void) {
	return IO_error? -1: 0;
}

void s9_io_reset(void) {
	IO_error = 0;
}

/*
 * Error Handling
 */

void s9_fatal(char *msg) {
	fprintf(stderr, "S9core: fatal error: ");
	fprintf(stderr, "%s\n", msg);
	bye(1);
}

void s9_abort(void) {
	Abort_flag = 1;
}

void s9_reset(void) {
	Abort_flag = 0;
}

int s9_aborted(void) {
	return Abort_flag;
}

/*
 * Memory Management
 */

void s9_set_node_limit(int n) {
	Node_limit = n * 1024L;
}

void s9_set_vector_limit(int n) {
	Vector_limit = n * 1024L;
}

void s9_gc_verbosity(int n) {
	Verbose_GC = n;
}

void s9_mem_error_handler(void (*h)(int src)) {
	Mem_error_handler = h;
}

static void new_cons_segment(void) {
	Car = realloc(Car, sizeof(cell)*(Cons_pool_size+Cons_segment_size));
	Cdr = realloc(Cdr, sizeof(cell)*(Cons_pool_size+Cons_segment_size));
	Tag = realloc(Tag, Cons_pool_size + Cons_segment_size);
	if (Car == NULL || Cdr == NULL || Tag == NULL)
		s9_fatal("new_cons_segment: out of physical memory");
	memset(&car(Cons_pool_size), 0, Cons_segment_size * sizeof(cell));
	memset(&cdr(Cons_pool_size), 0, Cons_segment_size * sizeof(cell));
	memset(&Tag[Cons_pool_size], 0, Cons_segment_size);
	Cons_pool_size += Cons_segment_size;
	Cons_segment_size = Cons_pool_size / 2;
}

static void new_vec_segment(void) {
	Vectors = realloc(Vectors, sizeof(cell) *
			(Vec_pool_size + Vec_segment_size));
	if (Vectors == NULL)
		s9_fatal("new_vec_segment: out of physical memory");
	memset(&Vectors[Vec_pool_size], 0, Vec_segment_size * sizeof(cell));
	Vec_pool_size += Vec_segment_size;
	Vec_segment_size = Vec_pool_size / 2;
}

/*
 * Mark nodes which can be accessed through N.
 * Using the Deutsch/Schorr/Waite pointer reversal algorithm.
 * S0: M==0, S==0, unvisited, process CAR (vectors: process 1st slot);
 * S1: M==1, S==1, CAR visited, process CDR (vectors: process next slot);
 * S2: M==1, S==0, completely visited, return to parent.
 */

static void mark(cell n) {
	cell	p, parent, *v;
	int	i;

	parent = NIL;
	while (1) {
		if (s9_special_p(n) || (Tag[n] & S9_MARK_TAG)) {
			if (parent == NIL)
				break;
			if (Tag[parent] & S9_VECTOR_TAG) { /* S1 --> S1|done */
				i = vector_index(parent);
				v = vector(parent);
				if (Tag[parent] & S9_STATE_TAG &&
				    i+1 < vector_len(parent)
				) {			/* S1 --> S1 */
					p = v[i+1];
					v[i+1] = v[i];
					v[i] = n;
					n = p;
					vector_index(parent) = i+1;
				}
				else {			/* S1 --> done */
					Tag[parent] &= ~S9_STATE_TAG;
					p = parent;
					parent = v[i];
					v[i] = n;
					n = p;
				}
			}
			else if (Tag[parent] & S9_STATE_TAG) {	/* S1 --> S2 */
				p = cdr(parent);
				cdr(parent) = car(parent);
				car(parent) = n;
				Tag[parent] &= ~S9_STATE_TAG;
				/* Tag[parent] |=  S9_MARK_TAG; */
				n = p;
			}
			else {				/* S2 --> done */
				p = parent;
				parent = cdr(p);
				cdr(p) = n;
				n = p;
			}
		}
		else if (Tag[n] & S9_VECTOR_TAG) {	/* S0 --> S1|S2 */
			Tag[n] |= S9_MARK_TAG;
			/* Tag[n] &= ~S9_STATE_TAG; */
			vector_link(n) = n;
			if (car(n) == T_VECTOR && vector_len(n) != 0) {
				Tag[n] |= S9_STATE_TAG;
				vector_index(n) = 0;
				v = vector(n);
				p = v[0];
				v[0] = parent;
				parent = n;
				n = p;
			}
		}
		else if (Tag[n] & S9_ATOM_TAG) {	/* S0 --> S2 */
			if (input_port_p(n) || output_port_p(n))
				Port_flags[port_no(n)] |= S9_USED_TAG;
			p = cdr(n);
			cdr(n) = parent;
			/*Tag[n] &= ~S9_STATE_TAG;*/
			parent = n;
			n = p;
			Tag[parent] |= S9_MARK_TAG;
		}
		else {					/* S0 --> S1 */
			p = car(n);
			car(n) = parent;
			Tag[n] |= S9_MARK_TAG;
			parent = n;
			n = p;
			Tag[parent] |= S9_STATE_TAG;
		}
	}
}

/* Mark and sweep GC. */
int s9_gc(void) {
	int	i, k, sk = 0;
	char	buf[100];

	if (Run_stats)
		s9_count(&Collections);
	for (i=0; i<S9_MAX_PORTS; i++) {
		if (Port_flags[i] & S9_LOCK_TAG)
			Port_flags[i] |= S9_USED_TAG;
		else if (i == Input_port || i == Output_port)
			Port_flags[i] |= S9_USED_TAG;
		else
			Port_flags[i] &= ~S9_USED_TAG;
	}
	if (GC_stack && *GC_stack != NIL) {
		sk = string_len(*GC_stack);
		string_len(*GC_stack) = (1 + *GC_stkptr) * sizeof(cell);
	}
	for (i=0; GC_int_roots[i] != NULL; i++) {
		mark(*GC_int_roots[i]);
	}
	if (GC_ext_roots) {
		for (i=0; GC_ext_roots[i] != NULL; i++)
			mark(*GC_ext_roots[i]);
	}
	if (GC_stack && *GC_stack != NIL) {
		string_len(*GC_stack) = sk;
	}
	k = 0;
	Free_list = NIL;
	for (i=0; i<Cons_pool_size; i++) {
		if (!(Tag[i] & S9_MARK_TAG)) {
			cdr(i) = Free_list;
			Free_list = i;
			k++;
		}
		else {
			Tag[i] &= ~S9_MARK_TAG;
		}
	}
	for (i=0; i<S9_MAX_PORTS; i++) {
		if (!(Port_flags[i] & S9_USED_TAG) && Ports[i] != NULL) {
			fclose(Ports[i]);
			Ports[i] = NULL;
		}
	}
	if (Verbose_GC > 1) {
		sprintf(buf, "GC: %d nodes reclaimed", k);
		s9_prints(buf); nl();
		s9_flush();
	}
	return k;
}

/* Allocate a fresh node and initialize with PCAR,PCDR,PTAG. */
cell s9_cons3(cell pcar, cell pcdr, int ptag) {
	cell	n;
	int	k;
	char	buf[100];

	if (Run_stats) {
		s9_count(&Nodes);
		if (	Cons_stats &&
			0 == (ptag & (S9_ATOM_TAG|S9_VECTOR_TAG|S9_PORT_TAG))
		)
			s9_count(&Conses);
	}
	if (Free_list == NIL) {
		if (ptag == 0)
			Tmp_car = pcar;
		if (!(ptag & S9_VECTOR_TAG))
			Tmp_cdr = pcdr;
		k = s9_gc();
		/*
		 * Performance increases dramatically if we
		 * do not wait for the pool to run dry.
		 * In fact, don't even let it come close to that.
		 */
		if (k < Cons_pool_size / 2) {
			if (	Node_limit &&
				Cons_pool_size + Cons_segment_size
					> Node_limit
			) {
				if (Mem_error_handler)
					(*Mem_error_handler)(1);
				else
					s9_fatal("s9_cons3: hit memory limit");
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
					s9_prints(buf); nl();
					s9_flush();
				}
				s9_gc();
			}
		}
		Tmp_car = Tmp_cdr = NIL;
	}
	if (Free_list == NIL)
		s9_fatal(
		  "s9_cons3: failed to recover from low memory condition");
	n = Free_list;
	Free_list = cdr(Free_list);
	car(n) = pcar;
	cdr(n) = pcdr;
	Tag[n] = ptag;
	return n;
}

/* Mark all vectors unused */
static void unmark_vectors(void) {
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
int s9_gcv(void) {
	int	v, k, to, from;
	char	buf[100];

	unmark_vectors();
	s9_gc();		/* re-mark live vectors */
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
		s9_prints(buf); nl();
		s9_flush();
	}
	Free_vecs = to;
	return k;
}

/* Allocate vector from pool */
cell s9_new_vec(cell type, int size) {
	cell	n;
	int	i, v, wsize;
	char	buf[100];

	wsize = vector_size(size);
	if (Run_stats) {
		s9_countn(&Vecspace, wsize);
	}
	if (Free_vecs + wsize >= Vec_pool_size) {
		s9_gcv();
		while (	Free_vecs + wsize >=
			Vec_pool_size - Vec_pool_size / 2
		) {
			if (	Vector_limit &&
				Vec_pool_size + Vec_segment_size
					> Vector_limit
			) {
				if (Mem_error_handler)
					(*Mem_error_handler)(2);
				else
					s9_fatal("new_vec: hit memory limit");
				break;
			}
			else {
				new_vec_segment();
				s9_gcv();
				if (Verbose_GC) {
					sprintf(buf,
						"GC: new_vec: new segment,"
						 " cells = %d",
						Vec_pool_size);
					s9_prints(buf); nl();
					s9_flush();
				}
			}
		}
	}
	if (Free_vecs + wsize >= Vec_pool_size)
		s9_fatal(
		  "new_vec: failed to recover from low memory condition");
	v = Free_vecs;
	Free_vecs += wsize;
	n = s9_cons3(type, v + RAW_VECTOR_DATA, S9_VECTOR_TAG);
	Vectors[v + RAW_VECTOR_LINK] = n;
	Vectors[v + RAW_VECTOR_INDEX] = 0;
	Vectors[v + RAW_VECTOR_SIZE] = size;
	if (type == T_VECTOR) {
		for (i = RAW_VECTOR_DATA; i<wsize; i++)
			Vectors[v+i] = UNDEFINED;
	}
	return n;
}

/* Pop K nodes off the Stack, return last one. */
cell s9_unsave(int k) {
	cell	n = NIL; /*LINT*/

	while (k) {
		if (Stack == NIL)
			s9_fatal("s9_unsave: stack underflow");
		n = car(Stack);
		Stack = cdr(Stack);
		k--;
	}
	return n;
}

static unsigned hash(char *s) {
	unsigned int	h = 0;

	while (*s) h = ((h<<5)+h) ^ *s++;
	return h;
}

static int hash_size(int n) {
	if (n < 47) return 47;
	if (n < 97) return 97;
	if (n < 199) return 199;
	if (n < 499) return 499;
	if (n < 997) return 997;
	if (n < 9973) return 9973;
	if (n < 19997) return 19997;
	return 39989;
}

#define intval(x) cadr(x)

static void rehash_symbols(void) {
	unsigned int	i;
	cell		*v, n, p, new;
	unsigned int	h, k;

	if (NIL == Symhash)
		k = hash_size(s9_length(Symbols));
	else
		k = hash_size(intval(vector(Symhash)[0]));
	Symhash = s9_new_vec(T_VECTOR, (k+1) * sizeof(cell));
	v = vector(Symhash);
	for (i=1; i<=k; i++) v[i] = NIL;
	i = 0;
	for (p = Symbols; p != NIL; p = cdr(p)) {
		h = hash(symbol_name(car(p)));
		n = cons(car(p), NIL);
		n = cons(n, vector(Symhash)[h%k+1]);
		vector(Symhash)[h%k+1] = n;
		i++;
	}
	new = s9_make_integer(i);
	vector(Symhash)[0] = new;
}

void add_symhash(cell x) {
	cell		n, new;
	unsigned int	h, i, k;

	if (NIL == Symhash) {
		rehash_symbols();
		return;
	}
	i = intval(vector(Symhash)[0]);
	k = vector_len(Symhash)-1;
	if (i > k) {
		rehash_symbols();
		return;
	}
	h = hash(symbol_name(x));
	n = cons(x, NIL);
	n = cons(n, vector(Symhash)[h%k+1]);
	vector(Symhash)[h%k+1] = n;
	new = s9_make_integer(i+1);
	vector(Symhash)[0] = new;
}

cell s9_find_symbol(char *s) {
	unsigned int	h, k;
	cell		n;

	if (NIL == Symhash) return NIL;
	k = vector_len(Symhash)-1;
	h = hash(s);
	for (n = vector(Symhash)[h%k+1]; n != NIL; n = cdr(n))
		if (!strcmp(s, symbol_name(caar(n))))
			return caar(n);
	return NIL;
}

/*
cell s9_find_symbol(char *s) {
	cell	y;

	y = Symbols;
	while (y != NIL) {
		if (!strcmp(symbol_name(car(y)), s))
			return car(y);
		y = cdr(y);
	}
	return NIL;
}
*/

cell s9_make_symbol(char *s, int k) {
	cell	n;

	n = s9_new_vec(T_SYMBOL, k+1);
	strcpy(symbol_name(n), s);
	return n;
}

cell s9_intern_symbol(cell y) {
	Symbols = cons(y, Symbols);
	add_symhash(y);
	return y;
}

cell s9_symbol_table(void) {
	return Symbols;
}

cell s9_symbol_ref(char *s) {
	cell	y, new;

	y = s9_find_symbol(s);
	if (y != NIL)
		return y;
	new = s9_make_symbol(s, strlen(s));
	return s9_intern_symbol(new);
}

cell s9_make_string(char *s, int k) {
	cell	n;

	if (0 == k) return Nullstr;
	n = s9_new_vec(T_STRING, k+1);
	strncpy(string(n), s, k+1);
	return n;
}

cell s9_make_vector(int k) {
	if (0 == k) return Nullvec;
	return s9_new_vec(T_VECTOR, k * sizeof(cell));
}

cell s9_mkfix(int v) {
	cell	n;

	n = new_atom(v, NIL);
	return new_atom(T_FIXNUM, n);
}

cell s9_make_integer(cell i) {
	cell	n;

	switch (i) {
	case 0:		return Zero;
	case 1:		return One;
	case 2:		return Two;
	case 10:	return Ten;
	default:
		n = new_atom(i, NIL);
		return new_atom(T_INTEGER, n);
	}
}

static cell make_init_integer(cell i) {
	cell	n;

	n = new_atom(i, NIL);
	return new_atom(T_INTEGER, n);
}

cell s9_make_char(int x) {
	cell n;

	if (' ' == x) return Blank;
	n = new_atom(x & 0xff, NIL);
	return new_atom(T_CHAR, n);
}

static cell real_normalize(cell x);

static cell S9_make_quick_real(int flags, cell exp, cell mant) {
	cell	n;

	n = new_atom(exp, mant);
	n = new_atom(flags, n);
	n = new_atom(T_REAL, n);
	return n;
}

cell S9_make_real(int flags, cell exp, cell mant) {
	cell	r;

	prot(mant);
	r = S9_make_quick_real(flags, exp, mant);
	r = real_normalize(r);
	unprot(1);
	return r;
}

cell s9_make_real(int sign, cell exp, cell mant) {
	cell	m;
	int	i;

	i = 0;
	for (m = cdr(mant); m != NIL; m = cdr(m))
		i++;
	if (i > S9_MANTISSA_SIZE)
		return UNDEFINED;
	return S9_make_real(sign < 0? REAL_NEGATIVE: 0, exp, cdr(mant));
}

static void grow_primitives(void) {
	Max_prims += S9_PRIM_SEG_SIZE;
	Primitives = (S9_PRIM *) realloc(Primitives,
					sizeof(S9_PRIM) * Max_prims);
	if (Primitives == NULL)
		s9_fatal("grow_primitives: out of physical memory");
}

cell s9_make_primitive(S9_PRIM *p) {
	cell	n;

	n = new_atom(Last_prim, NIL);
	n = new_atom(T_PRIMITIVE, n);
	if (Last_prim >= Max_prims)
		grow_primitives();
	memcpy(&Primitives[Last_prim], p, sizeof(S9_PRIM));
	Last_prim++;
	return n;
}

cell s9_make_port(int portno, cell type) {
	cell	n;
	int	pf;

	pf = Port_flags[portno];
	Port_flags[portno] |= S9_LOCK_TAG;
	n = new_atom(portno, NIL);
	n = s9_cons3(type, n, S9_ATOM_TAG|S9_PORT_TAG);
	Port_flags[portno] = pf;
	return n;
}

cell s9_string_to_symbol(cell x) {
	cell	y, n, k;

	y = s9_find_symbol(string(x));
	if (y != NIL)
		return y;
	/*
	 * Cannot pass content to s9_make_symbol(), because
	 * string(x) may move during GC.
	 */
	k = string_len(x);
	n = s9_make_symbol("", k-1);
	memcpy(symbol_name(n), string(x), k);
	return s9_intern_symbol(n);
}

cell s9_symbol_to_string(cell x) {
	cell	n, k;

	/*
	 * Cannot pass name to s9_make_string(), because
	 * symbol_name(x) may move during GC.
	 */
 	k = symbol_len(x);
	n = s9_make_string("", k-1);
	memcpy(string(n), symbol_name(x), k);
	return n;
}

cell s9_copy_string(cell x) {
	cell	n, k;

	/*
	 * See s9_string_to_symbol(), above.
	 */
 	k = string_len(x);
	n = s9_make_string("", k-1);
	memcpy(string(n), string(x), k);
	return n;
}

/*
 * Miscellanea
 */

int s9_length(cell n) {
	int	k;

	for (k = 0; n != NIL; n = cdr(n))
		k++;
	return k;
}

int s9_conses(cell n) {
	int	k;

	for (k = 0; pair_p(n); n = cdr(n))
		k++;
	return k;
}

cell s9_flat_copy(cell n, cell *lastp) {
	cell	a, m, last, new;

	if (n == NIL) {
		if (lastp != NULL)
			lastp[0] = NIL;
		return NIL;
	}
	m = s9_cons3(NIL, NIL, Tag[n]);
	prot(m);
	a = m;
	last = m;
	while (n != NIL) {
		car(a) = car(n);
		last = a;
		n = cdr(n);
		if (n != NIL) {
			new = s9_cons3(NIL, NIL, Tag[n]);
			cdr(a) = new;
			a = cdr(a);
		}
	}
	unprot(1);
	if (lastp != NULL)
		lastp[0] = last;
	return m;
}

long s9_asctol(char *s) {
	while (*s == '0' && s[1])
		s++;
	return atol(s);
}

static char *ntoa(char *b, cell x, int w) {
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
			s9_fatal("ntoa: number too big");
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
			s9_fatal("ntoa: number too big");
		p--;
		*p = '-';
	}
	strcpy(b, p);
	return b;
}

cell s9_argv_to_list(char **argv) {
	int	i;
	cell	a, n;

	if (argv[0] == NULL) return NIL;
	a = cons(NIL, NIL);
	prot(a);
	for (i = 0; argv[i] != NULL; i++) {
		n = s9_make_string(argv[i], strlen(argv[i]));
		car(a) = n;
		if (argv[i+1] != NULL) {
			n = cons(NIL, NIL);
			cdr(a) = n;
			a = cdr(a);
		}
	}
	return unprot(1);
}

#ifdef plan9

int system(char *cmd) {
	Waitmsg	*w;
	int	pid;
	char	*argv[] = { "/bin/rc", "-c", cmd, NULL };
	
	switch (pid = fork()) {
	case -1:
		return -1;
	case 0:
		exec(argv[0], argv);
		bye(1);
	default:
		while ((w = wait()) != NULL) {
			if (w->pid == pid) {
				if (w->msg[0] == 0) {
					free(w);
					return 0;
				}
				free(w);
				return 1;
			}
			free(w);
		}
		return 0;
	}
}

#endif /* plan9 */

/*
 * Bignums
 */

cell s9_bignum_abs(cell a) {
	cell	n;

	prot(a);
	n = new_atom(labs(cadr(a)), cddr(a));
	n = new_atom(T_INTEGER, n);
	unprot(1);
	return n;
}

cell s9_bignum_negate(cell a) {
	cell	n;

	prot(a);
	n = new_atom(-cadr(a), cddr(a));
	n = new_atom(T_INTEGER, n);
	unprot(1);
	return n;
}

static cell reverse_segments(cell n) {
	cell	m;

	m = NIL;
	while (n != NIL) {
		m = new_atom(car(n), m);
		n = cdr(n);
	}
	return m;
}

int s9_bignum_even_p(cell a) {
	while (cdr(a) != NIL)
		a = cdr(a);
	return car(a) % 2 == 0;
}

cell s9_bignum_add(cell a, cell b);
cell s9_bignum_subtract(cell a, cell b);

static cell Bignum_add(cell a, cell b) {
	cell	fa, fb, result, r;
	int	carry;

	if (bignum_negative_p(a)) {
		if (bignum_negative_p(b)) {
			/* -A+-B --> -(|A|+|B|) */
			a = s9_bignum_abs(a);
			prot(a);
			a = s9_bignum_add(a, s9_bignum_abs(b));
			unprot(1);
			return s9_bignum_negate(a);
		}
		else {
			/* -A+B --> B-|A| */
			return s9_bignum_subtract(b, s9_bignum_abs(a));
		}
	}
	else if (bignum_negative_p(b)) {
		/* A+-B --> A-|B| */
		return s9_bignum_subtract(a, s9_bignum_abs(b));
	}
	/* A+B */
	a = reverse_segments(cdr(a));
	prot(a);
	b = reverse_segments(cdr(b));
	prot(b);
	carry = 0;
	result = NIL;
	prot(result);
	while (a != NIL || b != NIL || carry) {
		fa = a == NIL? 0: car(a);
		fb = b == NIL? 0: car(b);
		r = fa + fb + carry;
		carry = 0;
		if (r >= S9_INT_SEG_LIMIT) {
			r -= S9_INT_SEG_LIMIT;
			carry = 1;
		}
		result = new_atom(r, result);
		pref(0) = result;
		if (a != NIL) a = cdr(a);
		if (b != NIL) b = cdr(b);
	}
	unprot(3);
	return new_atom(T_INTEGER, result);
}

cell s9_bignum_add(cell a, cell b) {
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	a = Bignum_add(a, b);
	unprot(2);
	return a;
}

int s9_bignum_less_p(cell a, cell b) {
	int	ka, kb, neg_a, neg_b;

	neg_a = bignum_negative_p(a);
	neg_b = bignum_negative_p(b);
	if (neg_a && !neg_b) return 1;
	if (!neg_a && neg_b) return 0;
	ka = s9_length(a);
	kb = s9_length(b);
	if (ka < kb) return neg_a? 0: 1;
	if (ka > kb) return neg_a? 1: 0;
	Tmp = b;
	a = s9_bignum_abs(a);
	prot(a);
	b = s9_bignum_abs(b);
	unprot(1);
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

int s9_bignum_equal_p(cell a, cell b) {
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

static cell Bignum_subtract(cell a, cell b) {
	cell	fa, fb, result, r;
	int	borrow;

	if (bignum_negative_p(a)) {
		if (bignum_negative_p(b)) {
			/* -A--B --> -A+|B| --> |B|-|A| */
			a = s9_bignum_abs(a);
			prot(a);
			a = s9_bignum_subtract(s9_bignum_abs(b), a);
			unprot(1);
			return a;
		}
		else {
			/* -A-B --> -(|A|+B) */
			return s9_bignum_negate(
				s9_bignum_add(s9_bignum_abs(a), b));
		}
	}
	else if (bignum_negative_p(b)) {
		/* A--B --> A+|B| */
		return s9_bignum_add(a, s9_bignum_abs(b));
	}
	/* A-B, A<B --> -(B-A) */
	if (s9_bignum_less_p(a, b))
		return s9_bignum_negate(s9_bignum_subtract(b, a));
	/* A-B, A>=B */
	a = reverse_segments(cdr(a));
	prot(a);
	b = reverse_segments(cdr(b));
	prot(b);
	borrow = 0;
	result = NIL;
	prot(result);
	while (a != NIL || b != NIL || borrow) {
		fa = a == NIL? 0: car(a);
		fb = b == NIL? 0: car(b);
		r = fa - fb - borrow;
		borrow = 0;
		if (r < 0) {
			r += S9_INT_SEG_LIMIT;
			borrow = 1;
		}
		result = new_atom(r, result);
		pref(0) = result;
		if (a != NIL) a = cdr(a);
		if (b != NIL) b = cdr(b);
	}
	unprot(3);
	while (car(result) == 0 && cdr(result) != NIL)
		result = cdr(result);
	return new_atom(T_INTEGER, result);
}

cell s9_bignum_subtract(cell a, cell b) {
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	a = Bignum_subtract(a, b);
	unprot(2);
	return a;
}

cell s9_bignum_shift_left(cell a, int fill) {
	cell	r, c, result;
	int	carry;

	prot(a);
	a = reverse_segments(cdr(a));
	prot(a);
	carry = fill;
	result = NIL;
	prot(result);
	while (a != NIL) {
		if (car(a) >= S9_INT_SEG_LIMIT/10) {
			c = car(a) / (S9_INT_SEG_LIMIT/10);
			r = car(a) % (S9_INT_SEG_LIMIT/10) * 10;
			r += carry;
			carry = c;
		}
		else {
			r = car(a) * 10 + carry;
			carry = 0;
		}
		result = new_atom(r, result);
		pref(0) = result;
		a = cdr(a);
	}
	if (carry)
		result = new_atom(carry, result);
	result = new_atom(T_INTEGER, result);
	unprot(3);
	return result;
}

/* Result: (a/10 . a%10) */
cell s9_bignum_shift_right(cell a) {
	cell	r, c, result;
	int	carry;

	prot(a);
	a = cdr(a);
	prot(a);
	carry = 0;
	result = NIL;
	prot(result);
	while (a != NIL) {
		c = car(a) % 10;
		r = car(a) / 10;
		r += carry * (S9_INT_SEG_LIMIT/10);
		carry = c;
		result = new_atom(r, result);
		pref(0) = result;
		a = cdr(a);
	}
	result = reverse_segments(result);
	if (car(result) == 0 && cdr(result) != NIL)
		result = cdr(result);
	result = new_atom(T_INTEGER, result);
	pref(0) = result;
	carry = s9_make_integer(carry);
	result = cons(result, carry);
	unprot(3);
	return result;
}

cell s9_bignum_multiply(cell a, cell b) {
	int	neg;
	cell	r, i, result;

	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	neg = bignum_negative_p(a) != bignum_negative_p(b);
	a = s9_bignum_abs(a);
	prot(a);
	b = s9_bignum_abs(b);
	prot(b);
	result = Zero;
	prot(result);
	while (!bignum_zero_p(a)) {
		r = s9_bignum_shift_right(a);
		i = caddr(r);
		a = car(r);
		pref(2) = a;
		while (i) {
			if (Abort_flag) {
				unprot(5);
				return Zero;
			}
			result = s9_bignum_add(result, b);
			pref(0) = result;
			i--;
		}
		b = s9_bignum_shift_left(b, 0);
		pref(1) = b;
	}
	if (neg)
		result = s9_bignum_negate(result);
	unprot(5);
	return result;
}

/*
 * Equalize A and B, e.g.:
 * A=123, B=12345 --> 12300, 100
 * Return (scaled-a . scaling-factor)
 */
static cell bignum_equalize(cell a, cell b) {
	cell	r, f, r0, f0;

	r0 = a;
	prot(r0);
	f0 = One;
	prot(f0);
	r = r0;
	prot(r);
	f = f0;
	prot(f);
	while (s9_bignum_less_p(r, b)) {
		pref(3) = r0 = r;
		pref(2) = f0 = f;
		r = s9_bignum_shift_left(r, 0);
		pref(1) = r;
		f = s9_bignum_shift_left(f, 0);
		pref(0) = f;
	}
	unprot(4);
	return cons(r0, f0);
}

/* Result: (a/b . a%b) */
static cell Bignum_divide(cell a, cell b) {
	int	neg, neg_a;
	cell	result, f;
	int	i;
	cell	c, c0;

	neg_a = bignum_negative_p(a);
	neg = neg_a != bignum_negative_p(b);
	a = s9_bignum_abs(a);
	prot(a);
	b = s9_bignum_abs(b);
	prot(b);
	if (s9_bignum_less_p(a, b)) {
		if (neg_a)
			a = s9_bignum_negate(a);
		unprot(2);
		return cons(Zero, a);
	}
	b = bignum_equalize(b, a);
	pref(1) = b;
	pref(0) = a;
	c = NIL;
	prot(c);
	c0 = NIL;
	prot(c0);
	f = cdr(b);
	b = car(b);
	pref(3) = b;
	prot(f);
	result = Zero;
	prot(result);
	while (!bignum_zero_p(f)) {
		c = Zero;
		pref(3) = c;
		pref(2) = c0 = c;
		i = 0;
		while (!s9_bignum_less_p(a, c)) {
			pref(2) = c0 = c;
			c = s9_bignum_add(c, b);
			pref(3) = c;
			i++;
		}
		result = s9_bignum_shift_left(result, i-1);
		pref(0) = result;
		a = s9_bignum_subtract(a, c0);
		pref(4) = a;
		f = s9_bignum_shift_right(f);
		f = car(f);
		pref(1) = f;
		b = s9_bignum_shift_right(b);
		b = car(b);
		pref(5) = b;
	}
	if (neg)
		result = s9_bignum_negate(result);
	pref(0) = result;
	if (neg_a)
		a = s9_bignum_negate(a);
	unprot(6);
	return cons(result, a);
}

cell s9_bignum_divide(cell a, cell b) {
	if (bignum_zero_p(b))
		return UNDEFINED;
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	a = Bignum_divide(a, b);
	unprot(2);
	return a;
}

/*
 * Real Number Arithmetics
 */

static cell count_digits(cell m) {
	int	k;
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
		k += S9_DIGITS_PER_CELL;
		m = cdr(m);
	}
	return k;
}

cell s9_real_exponent(cell x) {
	if (integer_p(x)) return 0;
	return Real_exponent(x);
}

cell s9_real_mantissa(cell x) {
	cell	m;

	if (integer_p(x))
		return x;
	m = new_atom(T_INTEGER, Real_mantissa(x));
	if (Real_negative_p(x))
		m = s9_bignum_negate(m);
	return m;
}

/*
 * Remove trailing zeros and move the decimal
 * point to the END of the mantissa, e.g.:
 * real_normalize(1.234e0) --> 1234e-3
 *
 * Limit the mantissa to S9_MANTISSA_SEGMENTS
 * machine words. This may cause a loss of
 * precision.
 *
 * Also handle numeric overflow/underflow.
 */

static cell real_normalize(cell x) {
	cell	m, e, r;
	int	dgs;

	prot(x);
	e = Real_exponent(x);
	m = new_atom(T_INTEGER, Real_mantissa(x));
	prot(m);
	dgs = count_digits(cdr(m));
	while (dgs > S9_MANTISSA_SIZE) {
		r = s9_bignum_shift_right(m);
		m = car(r);
		pref(0) = m;
		dgs--;
		e++;
	}
	while (!bignum_zero_p(m)) {
		r = s9_bignum_shift_right(m);
		if (!bignum_zero_p(cdr(r)))
			break;
		m = car(r);
		pref(0) = m;
		e++;
	}
	if (bignum_zero_p(m))
		e = 0;
	r = new_atom(e, NIL);
	if (count_digits(r) > S9_DIGITS_PER_CELL) {
		unprot(2);
		return UNDEFINED;
	}
	r = S9_make_quick_real(Real_flags(x), e, cdr(m));
	unprot(2);
	return r;
}

cell s9_bignum_to_real(cell a) {
	int	e, flags, d;
	cell	m, n;

	prot(a);
	m = s9_flat_copy(a, NULL);
	cadr(m) = labs(cadr(m));
	e = 0;
	if (s9_length(cdr(m)) > S9_MANTISSA_SEGMENTS) {
		d = count_digits(cdr(m));
		while (d > S9_MANTISSA_SIZE) {
			m = s9_bignum_shift_right(m);
			m = car(m);
			e++;
			d--;
		}
	}
	flags = bignum_negative_p(a)? REAL_NEGATIVE: 0;
	n = S9_make_quick_real(flags, e, cdr(m));
	n = real_normalize(n);
	unprot(1);
	return n;
}

cell s9_real_negate(cell a) {
	if (integer_p(a))
		return s9_bignum_negate(a);
	Tmp = a;
	a = Real_negate(a);
	Tmp = NIL;
	return a;
}

cell s9_real_negative_p(cell a) {
	if (integer_p(a))
		return bignum_negative_p(a);
	return Real_negative_p(a);
}

cell s9_real_positive_p(cell a) {
	if (integer_p(a))
		return bignum_positive_p(a);
	return Real_positive_p(a);
}

cell s9_real_zero_p(cell a) {
	if (integer_p(a))
		return bignum_zero_p(a);
	return Real_zero_p(a);
}

cell s9_real_abs(cell a) {
	if (integer_p(a))
		return s9_bignum_abs(a);
	if (Real_negative_p(a)) {
		Tmp = a;
		a = Real_negate(a);
		Tmp = NIL;
		return a;
	}
	return a;
}

/*
 * Scale the number R so that it gets exponent DESIRED_E
 * without changing its value. When there is not enough
 * room for scaling the mantissa of R, return UNDEFINED.
 * E.g.: scale_mantissa(1.0e0, -2, 0) --> 100.0e-2
 *
 * Allow the mantissa to grow to MAX_SIZE segments.
 */

static cell scale_mantissa(cell r, cell desired_e, int max_size) {
	int	dgs;
	cell	n, e;

	dgs = count_digits(Real_mantissa(r));
	if (max_size && (max_size - dgs < Real_exponent(r) - desired_e))
		return UNDEFINED;
	n = new_atom(T_INTEGER, s9_flat_copy(Real_mantissa(r), NULL));
	prot(n);
	e = Real_exponent(r);
	while (e > desired_e) {
		n = s9_bignum_shift_left(n, 0);
		pref(0) = n;
		e--;
	}
	unprot(1);
	return S9_make_quick_real(Real_flags(r), e, cdr(n));
}

static void autoscale(cell *pa, cell *pb) {
	if (Real_exponent(*pa) < Real_exponent(*pb)) {
		*pb = scale_mantissa(*pb, Real_exponent(*pa),
					S9_MANTISSA_SIZE*2);
		return;
	}
	if (Real_exponent(*pa) > Real_exponent(*pb)) {
		*pa = scale_mantissa(*pa, Real_exponent(*pb),
					S9_MANTISSA_SIZE*2);
	}
}

cell shift_mantissa(cell m) {    
	m = new_atom(T_INTEGER, m);
	prot(m);
	m = s9_bignum_shift_right(m);
	unprot(1);
	return cdar(m);
}

static int real_compare(cell a, cell b, int approx) {
	cell	ma, mb, d, e;
	int	p;

	if (integer_p(a) && integer_p(b))
		return s9_bignum_equal_p(a, b);
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	if (integer_p(a)) {
		a = s9_bignum_to_real(a);
		pref(1) = a;
	}
	if (integer_p(b)) {
		prot(a);
		b = s9_bignum_to_real(b);
		unprot(1);
		pref(0) = b;
	}
	if (Real_zero_p(a) && Real_zero_p(b)) {
		unprot(2);
		return 1;
	}
	if (Real_negative_p(a) != Real_negative_p(b)) {
		unprot(2);
		return 0;
	}
	if (approx) {
		d = s9_real_abs(s9_real_subtract(a, b));
		/* integer magnitudes */
		ma = count_digits(Real_mantissa(a))+Real_exponent(a);
		mb = count_digits(Real_mantissa(b))+Real_exponent(b);
		if (ma != mb) {
			unprot(2);
			return 0;
		}
		p = ma-S9_MANTISSA_SIZE;
		prot(d);
		e = S9_make_quick_real(0, p, cdr(One));
		unprot(3);
		return !s9_real_less_p(e, d);
	}
	unprot(2);
	if (Real_exponent(a) != Real_exponent(b))
		return 0;
	ma = Real_mantissa(a);
	mb = Real_mantissa(b);
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

int s9_real_equal_p(cell a, cell b) {
	return real_compare(a, b, 0);
}

int s9_real_approx_p(cell a, cell b) {
	return real_compare(a, b, 1);
}

int s9_real_less_p(cell a, cell b) {
	cell	ma, mb;
	int	ka, kb, neg;
	int	dpa, dpb;

	if (integer_p(a) && integer_p(b))
		return s9_bignum_less_p(a, b);
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	if (integer_p(a))
		a = s9_bignum_to_real(a);
	if (integer_p(b)) {
		prot(a);
		b = s9_bignum_to_real(b);
		unprot(1);
	}
	unprot(2);
	if (Real_negative_p(a) && !Real_negative_p(b)) return 1;
	if (Real_negative_p(b) && !Real_negative_p(a)) return 0;
	if (Real_zero_p(a) && Real_positive_p(b)) return 1;
	if (Real_zero_p(b) && Real_positive_p(a)) return 0;
	neg = Real_negative_p(a);
	dpa = count_digits(Real_mantissa(a)) + Real_exponent(a);
	dpb = count_digits(Real_mantissa(b)) + Real_exponent(b);
	if (dpa < dpb) return neg? 0: 1;
	if (dpa > dpb) return neg? 1: 0;
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	autoscale(&a, &b);
	unprot(2);
	if (a == UNDEFINED) return neg? 1: 0;
	if (b == UNDEFINED) return neg? 0: 1;
	ma = Real_mantissa(a);
	mb = Real_mantissa(b);
	ka = s9_length(ma);
	kb = s9_length(mb);
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

cell s9_real_add(cell a, cell b) {
	cell	r, m, e, aa, ab;
	int	flags, nega, negb;

	if (integer_p(a) && integer_p(b))
		return s9_bignum_add(a, b);
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	if (integer_p(a))
		a = s9_bignum_to_real(a);
	prot(a);
	if (integer_p(b))
		b = s9_bignum_to_real(b);
	prot(b);
	if (Real_zero_p(a)) {
		unprot(4);
		return b;
	}
	if (Real_zero_p(b)) {
		unprot(4);
		return a;
	}
	autoscale(&a, &b);
	if (a == UNDEFINED || b == UNDEFINED) {
		ab = s9_real_abs(pref(0));
		prot(ab);
		aa = s9_real_abs(pref(2));
		unprot(1);
		b = unprot(1);
		a = unprot(1);
		unprot(2);
		return s9_real_less_p(aa, ab)? b: a;
	}
	pref(1) = a;
	pref(0) = b;
	e = Real_exponent(a);
	nega = Real_negative_p(a);
	negb = Real_negative_p(b);
	a = new_atom(T_INTEGER, Real_mantissa(a));
	if (nega)
		a = s9_bignum_negate(a);
	pref(1) = a;
	b = new_atom(T_INTEGER, Real_mantissa(b));
	if (negb)
		b = s9_bignum_negate(b);
	pref(0) = b;
	m = s9_bignum_add(a, b);
	flags = bignum_negative_p(m)? REAL_NEGATIVE: 0;
	r = s9_bignum_abs(m);
	r = S9_make_quick_real(flags, e, cdr(r));
	r = real_normalize(r);
	unprot(4);
	return r;
}

cell s9_real_subtract(cell a, cell b) {
	cell	r;

	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	if (integer_p(b))
		b = s9_bignum_negate(b);
	else
		b = Real_negate(b);
	prot(b);
	r = s9_real_add(a, b);
	unprot(3);
	return r;
}

cell s9_real_multiply(cell a, cell b) {
	cell	r, m, e, ma, mb, ea, eb, neg;

	if (integer_p(a) && integer_p(b))
		return s9_bignum_multiply(a, b);
	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	if (integer_p(a))
		a = s9_bignum_to_real(a);
	prot(a);
	if (integer_p(b))
		b = s9_bignum_to_real(b);
	prot(b);
	neg = Real_negative_flag(a) != Real_negative_flag(b);
	ea = Real_exponent(a);
	eb = Real_exponent(b);
	ma = new_atom(T_INTEGER, Real_mantissa(a));
	pref(1) = ma;
	mb = new_atom(T_INTEGER, Real_mantissa(b));
	pref(0) = mb;
	e = ea + eb;
	m = s9_bignum_multiply(ma, mb);
	r = S9_make_quick_real(neg? REAL_NEGATIVE: 0, e, cdr(m));
	r = real_normalize(r);
	unprot(4);
	return r;
}

cell s9_real_divide(cell a, cell b) {
	cell	r, m, e, ma, mb, ea, eb, neg, div2;
	int	nd, dd;

	Tmp = b;
	prot(a);
	prot(b);
	Tmp = NIL;
	if (integer_p(a))
		a = s9_bignum_to_real(a);
	prot(a);
	if (integer_p(b))
		b = s9_bignum_to_real(b);
	prot(b);
	if (Real_zero_p(b)) {
		unprot(4);
		return UNDEFINED;
	}
	if (Real_zero_p(a)) {
		r = S9_make_quick_real(0, 0, cdr(Zero));
		unprot(4);
		return r;
	}
	neg = Real_negative_flag(a) != Real_negative_flag(b);
	ea = Real_exponent(a);
	eb = Real_exponent(b);
	ma = new_atom(T_INTEGER, Real_mantissa(a));
	pref(1) = ma;
	mb = new_atom(T_INTEGER, Real_mantissa(b));
	pref(0) = mb;
	if (bignum_zero_p(mb)) {
		unprot(4);
		return UNDEFINED;
	}
	nd = count_digits(cdr(ma));
	dd = S9_MANTISSA_SIZE + count_digits(cdr(mb));
	while (nd < dd) {
		ma = s9_bignum_shift_left(ma, 0);
		pref(1) = ma;
		nd++;
		ea--;
	}
	e = ea - eb;
	m = s9_bignum_divide(ma, mb);
	prot(m);
	div2 = s9_bignum_abs(mb);
	div2 = s9_bignum_divide(div2, Two);
	div2 = car(div2);
	if (s9_bignum_less_p(div2, cdr(m))) {
		m = s9_bignum_add(car(m), One);
	}
	else {
		m = car(m);
	}
	r = S9_make_quick_real(neg? REAL_NEGATIVE: 0, e, cdr(m));
	r = real_normalize(r);
	unprot(5);
	return r;
}

cell s9_real_sqrt(cell x) {
	cell	n0, n1;
	int	r;

	if (s9_real_negative_p(x))
		return UNDEFINED;
	if (s9_real_zero_p(x))
		return Zero;
	prot(x);
	n0 = x;
	prot(n0);
	while (1) {
		n1 = s9_real_divide(x, n0);
		if (n1 == UNDEFINED)
			break;
		n1 = s9_real_add(n1, n0);
		n1 = s9_real_divide(n1, Two);
		prot(n1);
		r = s9_real_approx_p(n0, n1);
		n0 = unprot(1);
		if (r) {
			break;
		}
		pref(0) = n0;
	}
	unprot(2);
	return n1;
}

/*
 * Real power algorithm from
 * http://stackoverflow.com/questions/3518973
 * Thanks, Tom Sirgedas!
 */

static cell rpower(cell x, cell y, cell prec) {
	cell	n, nprec;

	if (Abort_flag)
		return Zero;
	if (s9_real_equal_p(y, One))
		return x;
	if (!s9_real_less_p(y, Ten)) {
		prot(x);
		n = s9_real_divide(y, Two);
		pref(0) = n;
		nprec = s9_real_divide(prec, Two);
		prot(nprec);
		n = rpower(x, n, nprec);
		if (n == UNDEFINED || Abort_flag) {
			unprot(2);
			return UNDEFINED;
		}
		unprot(1);
		pref(0) = n;
		n = s9_real_multiply(n, n);
		unprot(1);
		return n;
	}
	if (!s9_real_less_p(y, One)) {
		y = s9_real_subtract(y, One);
		prot(y);
		n = rpower(x, y, prec);
		if (n == UNDEFINED || Abort_flag) {
			unprot(1);
			return UNDEFINED;
		}
		unprot(1);
		n = s9_real_multiply(x, n);
		return n;
	}
	if (!s9_real_less_p(prec, One))
		return s9_real_sqrt(x);
	y = s9_real_multiply(y, Two);
	prot(y);
	nprec = s9_real_multiply(prec, Two);
	prot(nprec);
	n = rpower(x, y, nprec);
	if (n == UNDEFINED || Abort_flag) {
		unprot(2);
		return UNDEFINED;
	}
	unprot(2);
	return s9_real_sqrt(n);
}

static cell npower(cell x, cell y) {
	cell	n;
	int	even;

	if (Abort_flag)
		return Zero;
	if (s9_real_zero_p(y))
		return One;
	if (s9_real_equal_p(y, One))
		return x; 
	prot(x);
	n = s9_bignum_divide(y, Two);
	even = bignum_zero_p(cdr(n));
	pref(0) = n;
	n = npower(x, car(n));
	if (Abort_flag) {
		unprot(1);
		return Zero;
	}
	pref(0) = n;
	n = s9_real_multiply(n, n);
	pref(0) = n;
	if (!even) {
		n = s9_real_multiply(x, n);
		pref(0) = n;
	}
	unprot(1);
	return n;
}

cell s9_real_power(cell x, cell y) {
	Tmp = x;
	prot(y);
	prot(x);
	Tmp = NIL;
	if (integer_p(y)) {
		x = npower(x, y);
		if (bignum_negative_p(y))
			x = s9_real_divide(One, x);
		unprot(2);
		return x;
	}
	if (s9_real_negative_p(y)) {
		y = s9_real_abs(y);
		prot(y);
		x = rpower(x, y, Epsilon);
		unprot(3);
		if (x == UNDEFINED)
			return x;
		return s9_real_divide(One, x);
	}
	x = rpower(x, y, Epsilon);
	unprot(2);
	return x;
}

/* type: 0=trunc, 1=floor, 2=ceil */
static cell rround(cell x, int type) {
	cell	n, m, e;

	e = s9_real_exponent(x);
	if (e >= 0)
		return x;
	prot(x);
	m = new_atom(T_INTEGER, Real_mantissa(x));
	prot(m);
	while (e < 0) {
		m = s9_bignum_shift_right(m);
		m = car(m);
		pref(0) = m;
		e++;
	}
	if (	(type == 1 && Real_negative_p(x)) ||
		(type == 2 && Real_positive_p(x))
	) {
		m = s9_bignum_add(m, One);
	}
	n = S9_make_real(Real_flags(x), e, cdr(m));
	unprot(2);
	return n;
}

cell s9_real_trunc(cell x) { return rround(x, 0); }
cell s9_real_floor(cell x) { return rround(x, 1); }
cell s9_real_ceil (cell x) { return rround(x, 2); }

cell s9_real_to_bignum(cell r) {
	cell	n;
	int	neg;

	if (Real_exponent(r) >= 0) {
		prot(r);
		neg = Real_negative_p(r);
		n = scale_mantissa(r, 0, 0);
		if (n == UNDEFINED) {
			unprot(1);
			return UNDEFINED;
		}
		n = new_atom(T_INTEGER, Real_mantissa(n));
		if (neg)
			n = s9_bignum_negate(n);
		unprot(1);
		return n;
	}
	return UNDEFINED;
}

cell s9_real_integer_p(cell x) {
	if (integer_p(x))
		return 1;
	if (real_p(x) && s9_real_to_bignum(x) != UNDEFINED)
		return 1;
	return 0;
}

/*
 * String/number conversion
 */

static int exponent_char_p(int c) {
	return c && strchr(Exponent_chars, c) != NULL;
}

int s9_integer_string_p(char *s) {
	if (*s == '-' || *s == '+')
		s++;
	if (!*s)
		return 0;
	while (isdigit(*s))
		s++;
	return *s == 0;
}

int s9_string_numeric_p(char *s) {
	int	i;
	int	got_point = 0,
		got_digit = 0;

	i = 0;
	if (s[0] == '+' || s[0] == '-')
		i = 1;
	if (!s[i])
		return 0;
	while (s[i]) {
		if (isdigit(s[i])) {
			got_digit = 1;
			i++;
		}
		else if (s[i] == '.' && !got_point) {
			got_point = 1;
			i++;
		}
		else {
			break;
		}
	}
	if (!got_digit)
		return 0;
	if (s[i] && strchr(Exponent_chars, s[i]))
		return s9_integer_string_p(&s[i+1]);
	return s[i] == 0;
}

cell s9_string_to_bignum(char *s) {
	cell	n, v, str;
	int	k, j, sign;

	sign = 1;
	if (s[0] == '-') {
		s++;
		sign = -1;
	}
	else if (s[0] == '+') {
		s++;
	}
	str = s9_make_string(s, strlen(s));
	prot(str);
	s = string(str);
	k = (int) strlen(s);
	n = NIL;
	while (k) {
		j = k <= S9_DIGITS_PER_CELL? k: S9_DIGITS_PER_CELL;
		v = s9_asctol(&s[k-j]);
		s[k-j] = 0;
		k -= j;
		n = new_atom(v, n);
		s = string(str);
	}
	unprot(1);
	car(n) = sign * car(n);
	return new_atom(T_INTEGER, n);
}

cell s9_string_to_real(char *s) {
	cell	mantissa, n;
	cell	exponent;
	int	found_dp;
	int	neg = 0;
	int	i, j, v;

	mantissa = Zero;
	prot(mantissa);
	exponent = 0;
	i = 0;
	if (s[i] == '+') {
		i++;
	}
	else if (s[i] == '-') {
		neg = 1;
		i++;
	}
	found_dp = 0;
	while (isdigit((int) s[i]) || s[i] == '#' || s[i] == '.') {
		if (s[i] == '.') {
			i++;
			found_dp = 1;
			continue;
		}
		if (found_dp)
			exponent--;
		mantissa = s9_bignum_shift_left(mantissa, 0);
		pref(0) = mantissa;
		if (s[i] == '#')
			v = 5;
		else
			v = s[i]-'0';
		mantissa = s9_bignum_add(mantissa, s9_make_integer(v));
		pref(0) = mantissa;
		i++;
	}
	j = 0;
	for (n = cdr(mantissa); n != NIL; n = cdr(n))
		j++;
	if (exponent_char_p(s[i])) {
		i++;
		if (!isdigit(s[i]) && s[i] != '-' && s[i] != '+') {
			unprot(1);
			return UNDEFINED;
		}
		n = s9_string_to_bignum(&s[i]);
		if (cddr(n) != NIL) {
			unprot(1);
			return UNDEFINED;
		}
		exponent += cadr(n);
	}
	unprot(1);
	n = S9_make_quick_real((neg? REAL_NEGATIVE: 0),
			exponent, cdr(mantissa));
	return real_normalize(n);
}

cell s9_string_to_number(char *s) {
	if (s9_integer_string_p(s))
		return s9_string_to_bignum(s);
	else
		return s9_string_to_real(s);
}

void s9_print_bignum(cell n) {
	int	first;
	char	buf[S9_DIGITS_PER_CELL+2];

	n = cdr(n);
	first = 1;
	while (n != NIL) {
		s9_prints(ntoa(buf, car(n), first? 0: S9_DIGITS_PER_CELL));
		n = cdr(n);
		first = 0;
	}
}

void s9_print_expanded_real(cell n) {
	char	buf[S9_DIGITS_PER_CELL+3];
	int	k, first;
	int	dp_offset, old_offset;
	cell	m, e;
	int	n_digits, neg;

	m = Real_mantissa(n);
	e = Real_exponent(n);
	neg = Real_negative_p(n);
	n_digits = count_digits(m);
	dp_offset = e+n_digits;
	if (neg)
		s9_prints("-");
	if (dp_offset <= 0)
		s9_prints("0");
	if (dp_offset < 0)
		s9_prints(".");
	while (dp_offset < 0) {
		s9_prints("0");
		dp_offset++;
	}
	dp_offset = e+n_digits;
	first = 1;
	while (m != NIL) {
		ntoa(buf, labs(car(m)), first? 0: S9_DIGITS_PER_CELL);
		k = strlen(buf);
		old_offset = dp_offset;
		dp_offset -= k;
		if (dp_offset < 0 && old_offset >= 0) {
			memmove(&buf[k+dp_offset+1], &buf[k+dp_offset],
				-dp_offset+1);
			buf[k+dp_offset] = '.';
		}
		s9_prints(buf);
		m = cdr(m);
		first = 0;
	}
	if (dp_offset >= 0) {
		while (dp_offset > 0) {
			s9_prints("0");
			dp_offset--;
		}
		s9_prints(".0");
	}
}

void s9_print_sci_real(cell n) {
	int	n_digits;
	cell	m, e;
	char	buf[S9_DIGITS_PER_CELL+2];
	char	es[2];

	m = Real_mantissa(n);
	e = Real_exponent(n);
	n_digits = count_digits(m);
	if (Real_negative_flag(n))
		s9_prints("-");
	ntoa(buf, car(m), 0);
	s9_blockwrite(buf, 1);
	s9_prints(".");
	s9_prints(buf[1] || cdr(m) != NIL? &buf[1]: "0");
	m = cdr(m);
	while (m != NIL) {
		s9_prints(ntoa(buf, car(m), S9_DIGITS_PER_CELL));
		m = cdr(m);
	}
	es[0] = Exponent_chars[0];
	es[1] = 0;
	s9_prints(es);
	if (e+n_digits-1 >= 0)
		s9_prints("+");
	s9_prints(ntoa(buf, e+n_digits-1, 0));
}

void s9_print_real(cell n) {
	int	n_digits;
	cell	m, e;

	m = Real_mantissa(n);
	e = Real_exponent(n);
	n_digits = count_digits(m);
	if (e+n_digits > -S9_MANTISSA_SIZE && e+n_digits <= S9_MANTISSA_SIZE) {
		s9_print_expanded_real(n);
		return;
	}
	s9_print_sci_real(n);
}

cell s9_bignum_to_int(cell x, int *of) {
	int	a, b, s;

	*of = 0;
	if (small_int_p(x)) return small_int_value(x);
	if (NIL == cdddr(x)) {
		if ((size_t) S9_INT_SEG_LIMIT > (size_t) INT_MAX)
			s9_fatal("bignum_to_int(): multi-segment integers "
				"unsupported in 64-bit mode");
		a = cadr(x);
		b = caddr(x);
		if (a > INT_MAX / S9_INT_SEG_LIMIT) {
			*of = 1;
			return 0;
		}
		if (a < INT_MIN / S9_INT_SEG_LIMIT) {
			*of = 1;
			return 0;
		}
		s = a<0? -1: 1;
		a = abs(a) * S9_INT_SEG_LIMIT;
		if (b > INT_MAX - a) {
			*of = 1;
			return 0;
		}
		return s*(a+b);
	}
	*of = 1;
	return 0;
}

cell s9_int_to_bignum(int v) {
	cell	n;

	if (v >= 0 && (long) v < S9_INT_SEG_LIMIT)
		return s9_make_integer(v);
	if (v < 0 && (long) -v < S9_INT_SEG_LIMIT)
		return s9_make_integer(v);
	if ((size_t) S9_INT_SEG_LIMIT > (size_t) INT_MAX)
		s9_fatal("int_to_bignum(): multi-segment integers "
			"unsupported in 64-bit mode");
	n = new_atom(abs(v) % S9_INT_SEG_LIMIT, NIL);
	n = new_atom(v / S9_INT_SEG_LIMIT, n);
	return new_atom(T_INTEGER, n);
}

cell s9_bignum_to_string(cell x) {
	int	n;
	cell	s;
	int	ioe;

	prot(x);
	n = count_digits(cdr(x));
	if (bignum_negative_p(x))
		n++;
	s = s9_make_string("", n);
	Str_outport = string(s);
	Str_outport_len = n+1;
	ioe = IO_error;
	IO_error = 0;
	s9_print_bignum(x);
	n = IO_error;
	IO_error = ioe;
	Str_outport = NULL;
	Str_outport_len = 0;
	unprot(1);
	if (n) {
		return UNDEFINED;
	}
	return s;
}

cell s9_real_to_string(cell x, int mode) {
	#define Z S9_MANTISSA_SIZE+S9_DIGITS_PER_CELL+10
	char	buf[Z];
	int	ioe, n;

	Str_outport = buf;
	Str_outport_len = Z;
	ioe = IO_error;
	IO_error = 0;
	switch (mode) {
	case 0:	s9_print_real(x); break;
	case 1:	s9_print_sci_real(x); break;
	case 2:	s9_print_expanded_real(x); break;
	default:
		Str_outport = NULL;
		Str_outport_len = 0;
		return UNDEFINED;
		break;
	}
	Str_outport = NULL;
	Str_outport_len = 0;
	n = IO_error;
	IO_error = ioe;
	if (n) {
		return UNDEFINED;
	}
	return s9_make_string(buf, strlen(buf));
}

/*
 * I/O
 */

void s9_close_port(int port) {
	if (port < 0 || port >= S9_MAX_PORTS)
		return;
	if (Ports[port] == NULL) {
		Port_flags[port] = 0;
		return;
	}
	fclose(Ports[port]); /* already closed? don't care */
	Ports[port] = NULL;
	Port_flags[port] = 0;
}

int s9_new_port(void) {
	int	i, tries;

	for (tries=0; tries<2; tries++) {
		for (i=0; i<S9_MAX_PORTS; i++) {
			if (Ports[i] == NULL)
				return i;
		}
		if (tries == 0)
			s9_gc();
	}
	return -1;
}

int s9_open_input_port(char *path) {
	int	i = s9_new_port();

	if (i < 0)
		return -1;
	Ports[i] = fopen(path, "r");
	if (Ports[i] == NULL)
		return -1;
	return i;
}

int s9_open_output_port(char *path, int append) {
	int	i = s9_new_port();

	if (i < 0)
		return -1;
	Ports[i] = fopen(path, append? "a": "w");
	if (Ports[i] == NULL)
		return -1;
	return i;
}

int s9_port_eof(int p) {
	if (p < 0 || p >= S9_MAX_PORTS)
		return -1;
	return feof(Ports[p]);
}

int s9_error_port(void) {
	return Error_port;
}

int s9_input_port(void) {
	return Str_inport? -1: Input_port;
}

int s9_output_port(void) {
	return Output_port;
}

cell s9_set_input_port(cell port) {
	cell	p = Input_port;

	Input_port = port;
	return p;
}

cell s9_set_output_port(cell port) {
	cell	p = Output_port;

	Output_port = port;
	return p;
}

void s9_reset_std_ports(void) {
	clearerr(stdin);
	clearerr(stdout);
	clearerr(stderr);
	Input_port = 0;
	Output_port = 1;
	Error_port = 2;
}

int s9_lock_port(int port) {
	if (port < 0 || port >= S9_MAX_PORTS)
		return -1;
	Port_flags[port] |= S9_LOCK_TAG;
	return 0;
}

int s9_unlock_port(int port) {
	if (port < 0 || port >= S9_MAX_PORTS)
		return -1;
	Port_flags[port] &= ~S9_LOCK_TAG;
	return 0;
}

/*
 * Primitives
 */

static char *expected(int n, cell who, char *what) {
	static char	msg[100];
	S9_PRIM		*p;

	p = &Primitives[cadr(who)];
	sprintf(msg, "%s: expected %s in argument #%d", p->name, what, n);
	return msg;
}

static char *wrongargs(char *name, char *what) {
	static char	buf[100];

	sprintf(buf, "%s: too %s arguments", name, what);
	return buf;
}

char *s9_typecheck(cell f) {
	S9_PRIM	*p;
	int	na, i, k;

	k = narg();
	p = prim_info(f);
	if (k < p->min_args)
		return wrongargs(p->name, "few");
	if (k > p->max_args && p->max_args >= 0)
		return wrongargs(p->name, "many");
	na = p->max_args < 0? p->min_args: p->max_args;
	if (na > k)
		na = k;
	else if (na > 3)
		na = 3;
	for (i=1; i<=na; i++) {
		switch (p->arg_types[i-1]) {
		case T_ANY:
			break;
		case T_BOOLEAN:
			if (!boolean_p(parg(i)))
				return expected(i, f, "boolean");
			break;
		case T_CHAR:
			if (!char_p(parg(i)))
				return expected(i, f, "char");
			break;
		case T_INPUT_PORT:
			if (!input_port_p(parg(i)))
				return expected(i, f, "input-port");
			break;
		case T_INTEGER:
			if (!integer_p(parg(i)))
				return expected(i, f, "integer");
			break;
		case T_OUTPUT_PORT:
			if (!output_port_p(parg(i)))
				return expected(i, f, "output-port");
			break;
		case T_PAIR:
			if (atom_p(parg(i)))
				return expected(i, f, "pair");
			break;
		case T_LIST:
			if (parg(i) != NIL && atom_p(parg(i)))
				return expected(i, f, "list");
			break;
		case T_FUNCTION:
			if (	!function_p(parg(i)) &&
				!primitive_p(parg(i)) &&
				!continuation_p(parg(i))
			)
				return expected(i, f, "function");
			break;
		case T_REAL:
			if (!integer_p(parg(i)) && !real_p(parg(i)))
				return expected(i, f, "number");
			break;
		case T_STRING:
			if (!string_p(parg(i)))
				return expected(i, f, "string");
			break;
		case T_SYMBOL:
			if (!symbol_p(parg(i)))
				return expected(i, f, "symbol");
			break;
		case T_VECTOR:
			if (!vector_p(parg(i)))
				return expected(i, f, "vector");
			break;
		}
	}
	return NULL;
}

cell s9_apply_prim(cell f) {
	S9_PRIM	*p;

	p = prim_info(f);
	return (*p->handler)();
}

/*
 * Image I/O
 */

struct magic {
	char	id[16];			/* "magic#"	*/
	char	version[8];		/* "yyyymmdd"	*/
	char	cell_size[1];		/* size + '0'	*/
	char    mantissa_size[1];	/* size + '0'	*/
	char	byte_order[8];		/* e.g. "4321"	*/
	char	prim_slots[8];		/* see code	*/
	char	pad[6];
};

static char *xfwrite(void *buf, int siz, int n, FILE *f) {
	if (fwrite(buf, siz, n, f) != n) {
		return "image file write error";
	}
	return NULL;
}

char *s9_dump_image(char *path, char *magic) {
	FILE		*f;
	cell		n, **v;
	int		i;
	struct magic	m;
	char		*s;

	f = fopen(path, "wb");
	if (f == NULL) {
		return "cannot create image file";
	}
	memset(&m, '_', sizeof(m));
	strncpy(m.id, magic, sizeof(m.id));
	strncpy(m.version, S9_VERSION, sizeof(m.version));
	m.cell_size[0] = sizeof(cell)+'0';
	m.mantissa_size[0] = S9_MANTISSA_SEGMENTS+'0';
#ifdef S9_BITS_PER_WORD_64
	n = 0x3132333435363738L;
#else
	n = 0x31323334L;
#endif
	memcpy(m.byte_order, &n, sizeof(n)>8? 8: sizeof(n));
	n = Last_prim;
	memcpy(m.prim_slots, &n, sizeof(n)>8? 8: sizeof(n));
	if ((s = xfwrite(&m, sizeof(m), 1, f)) != NULL) {
		fclose(f);
		return s;
	}
	i = Cons_pool_size;
	if ((s = xfwrite(&i, sizeof(int), 1, f)) != NULL) {
		fclose(f);
		return s;
	}
	i = Vec_pool_size;
	if ((s = xfwrite(&i, sizeof(int), 1, f)) != NULL) {
		fclose(f);
		return s;
	}
	if (	(s = xfwrite(&Free_list, sizeof(cell), 1, f)) != NULL ||
		(s = xfwrite(&Free_vecs, sizeof(cell), 1, f)) != NULL ||
		(s = xfwrite(&Symbols, sizeof(cell), 1, f)) != NULL ||
		(s = xfwrite(&Symhash, sizeof(cell), 1, f)) != NULL
	) {
		fclose(f);
		return s;
	}
	i = 0;
	v = Image_vars;
	while (v[i]) {
		if ((s = xfwrite(v[i], sizeof(cell), 1, f)) != NULL) {
			fclose(f);
			return s;
		}
		i++;
	}
	if (	fwrite(Car, 1, Cons_pool_size*sizeof(cell), f)
		 != Cons_pool_size*sizeof(cell) ||
		fwrite(Cdr, 1, Cons_pool_size*sizeof(cell), f)
		 != Cons_pool_size*sizeof(cell) ||
		fwrite(Tag, 1, Cons_pool_size, f) != Cons_pool_size ||
		fwrite(Vectors, 1, Vec_pool_size*sizeof(cell), f)
		 != Vec_pool_size*sizeof(cell)
	) {
		fclose(f);
		return "image dump failed";
	}
	fclose(f);
	return NULL;
}

static char *xfread(void *buf, int siz, int n, FILE *f) {
	if (fread(buf, siz, n, f) != n) {
		return "image file read error";
	}
	return NULL;
}

char *s9_load_image(char *path, char *magic) {
	FILE		*f;
	cell		n, **v;
	int		i;
	struct magic	m;
	int		image_nodes, image_vcells;
	char		*s;

	f = fopen(path, "rb");
	if (f == NULL)
		return "could not open file";
	if ((s = xfread(&m, sizeof(m), 1, f)) != NULL)
		return s;
	if (memcmp(m.id, magic, 16)) {
		fclose(f);
		return "magic match failed";
	}
	if (memcmp(m.version, S9_VERSION, sizeof(m.version))) {
		fclose(f);
		return "wrong image version";
	}
	if (m.cell_size[0]-'0' != sizeof(cell)) {
		fclose(f);
		return "wrong cell size";
	}
	if (m.mantissa_size[0]-'0' != S9_MANTISSA_SEGMENTS) {
		fclose(f);
		return "wrong mantissa size";
	}
	memcpy(&n, m.byte_order, sizeof(cell));
#ifdef S9_BITS_PER_WORD_64
	if (n != 0x3132333435363738L) {
#else
	if (n != 0x31323334L) {
#endif
		fclose(f);
		return "wrong byte order";
	}
	memcpy(&n, m.prim_slots, sizeof(cell));
	if (n != Last_prim) {
		fclose(f);
		return "wrong number of primitives";
	}
	memset(Tag, 0, Cons_pool_size);
	if ((s = xfread(&image_nodes, sizeof(int), 1, f)) != NULL)
		return s;
	if ((s = xfread(&image_vcells, sizeof(int), 1, f)) != NULL)
		return s;
	while (image_nodes > Cons_pool_size) {
		if (	Node_limit &&
			Cons_pool_size + Cons_segment_size > Node_limit
		) {
			fclose(f);
			return "image cons pool too large";
		}
		new_cons_segment();
	}
	while (image_vcells > Vec_pool_size) {
		if (	Vector_limit &&
			Vec_pool_size + Vec_segment_size > Vector_limit
		) {
			fclose(f);
			return "image vector pool too large";
		}
		new_vec_segment();
	}
	if (	(s = xfread(&Free_list, sizeof(cell), 1, f)) != NULL ||
		(s = xfread(&Free_vecs, sizeof(cell), 1, f)) != NULL ||
		(s = xfread(&Symbols, sizeof(cell), 1, f)) != NULL ||
		(s = xfread(&Symhash, sizeof(cell), 1, f)) != NULL
	) {
		fclose(f);
		return s;
	}
	v = Image_vars;
	i = 0;
	while (v[i]) {
		if ((s = xfread(v[i], sizeof(cell), 1, f)) != NULL)
			return s;
		i++;
	}
	if (	(fread(Car, 1, image_nodes*sizeof(cell), f)
		  != image_nodes*sizeof(cell) ||
		 fread(Cdr, 1, image_nodes*sizeof(cell), f)
		  != image_nodes*sizeof(cell) ||
		 fread(Tag, 1, image_nodes, f) != image_nodes ||
		 fread(Vectors, 1, image_vcells*sizeof(cell), f)
		  != image_vcells*sizeof(cell) ||
		 fgetc(f) != EOF)
	) {
		fclose(f);
		return "wrong file size";
	}
	fclose(f);
	return NULL;
}

/*
 * Initialization
 */

void s9_exponent_chars(char *s) {
	Exponent_chars = s;
}

/*
void s9_image_vars(cell **v) {
	Image_vars = v;
}

void s9_add_image_vars(cell **v) {
	int	i, n, m;
	cell	**nv;

	if (Image_vars != NULL) {
		for (n=0; Image_vars[n] != NULL; n++)
			;
		for (m=0; v[m] != NULL; m++)
			;
		nv = malloc((n+m+1) * sizeof(cell *));
		if (nv == NULL)
			s9_fatal("add_image_vars(): out of memory");
		n = 0;
		for (i = 0; Image_vars[i] != NULL; i++)
			nv[n++] = Image_vars[i];
		for (i = 0; v[i] != NULL; i++)
			nv[n++] = v[i];
		nv[n] = NULL;
		v = nv;
	}
	Image_vars = v;
}
*/

static void resetpools(void) {
	Cons_segment_size = S9_INITIAL_SEGMENT_SIZE;
	Vec_segment_size = S9_INITIAL_SEGMENT_SIZE;
	Cons_pool_size = 0,
	Vec_pool_size = 0;
	Car = NULL,
	Cdr = NULL;
	Tag = NULL;
	Free_list = NIL;
	Vectors = NULL;
	Free_vecs = 0;
	Primitives = NULL;
	Max_prims = 0;
}

void s9_init(cell **extroots, cell *stack, int *stkptr) {
	int	i;

#ifdef S9_BITS_PER_WORD_64
	if (sizeof(cell) != 8)
		s9_fatal("improper 64-bit build");
#endif
	GC_ext_roots = extroots;
	GC_stack = stack;
	GC_stkptr = stkptr;
	for (i=2; i<S9_MAX_PORTS; i++)
		Ports[i] = NULL;
	Ports[0] = stdin;
	Ports[1] = stdout;
	Ports[2] = stderr;
	Port_flags[0] = S9_LOCK_TAG;
	Port_flags[1] = S9_LOCK_TAG;
	Port_flags[2] = S9_LOCK_TAG;
	Input_port = 0;
	Output_port = 1;
	Error_port = 2;
	Str_outport = NULL;
	Str_outport_len = 0;
	Str_inport = NULL;
	Abort_flag = 0;
	resetpools();
	Node_limit = S9_NODE_LIMIT * 1024L;
	Vector_limit = S9_VECTOR_LIMIT * 1024L;
	Stack = NIL,
	Tmp_car = NIL;
	Tmp_cdr = NIL;
	Tmp = NIL;
	Symbols = NIL;
	Symhash = NIL;
	Printer_limit = 0;
	IO_error = 0;
	Exponent_chars = "eE";
	Run_stats = 0;
	Cons_stats = 0;
	Mem_error_handler = NULL;
	new_cons_segment();
	new_vec_segment();
	s9_gc();
	Protect = s9_make_vector(PROT_STACK_LEN);
	Protp = -1;
	Zero = make_init_integer(0);
	One = make_init_integer(1);
	Two = make_init_integer(2);
	Ten = make_init_integer(10);
	Nullvec = s9_new_vec(T_VECTOR, 0);
	Nullstr = s9_new_vec(T_STRING, 1);
	Blank = new_atom(T_CHAR, new_atom(' ', NIL));
	Epsilon = S9_make_quick_real(0, -S9_MANTISSA_SIZE, cdr(One));
}

void s9_fini() {
	int	i;

	for (i=2; i<S9_MAX_PORTS; i++) {
		if (Ports[i] != NULL)
			fclose(Ports[i]);
		Ports[i] = NULL;
	}
	if (Car) free(Car);
	if (Cdr) free(Cdr);
	if (Tag) free(Tag);
	if (Vectors) free(Vectors);
	if (Primitives) free(Primitives);
	resetpools();
}
