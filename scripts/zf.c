#!/bin/cc -run
/*
Copyright 2000 Ico Doornekamp <zforth@zevv.nl>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <ctype.h>
#include <string.h>
#include <setjmp.h>

#ifndef zforth_h
#define zforth_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#ifndef zfconf
#define zfconf

/* Set to 1 to add tracing support for debugging and inspection. Requires the
 * zf_host_trace() function to be implemented. Adds about one kB to .text and
 * .rodata, dramatically reduces speed, but is very useful. Make sure to enable
 * tracing at run time when calling zf_init() or by setting the 'trace' user
 * variable to 1 */

#define ZF_ENABLE_TRACE 1


/* Set to 1 to add boundary checks to stack operations. Increases .text size
 * by approx 100 bytes */

#define ZF_ENABLE_BOUNDARY_CHECKS 1


/* Set to 1 to enable bootstrapping of the forth dictionary by adding the
 * primitives and user veriables. On small embedded systems you may choose to
 * leave this out and start by loading a cross-compiled dictionary instead.
 * Enabling adds a few hundred bytes to the .text and .rodata segments */
 
#define ZF_ENABLE_BOOTSTRAP 1


/* Set to 1 to enable typed access to memory. This allows memory read and write 
 * of signed and unsigned memory of 8, 16 and 32 bits width, as well as the zf_cell 
 * type. This adds a few hundred bytes of .text. Check the memaccess.zf file for
 * examples how to use these operations */

#define ZF_ENABLE_TYPED_MEM_ACCESS 1


/* Type to use for the basic cell, data stack and return stack. Choose a signed
 * integer type that suits your needs, or 'float' or 'double' if you need
 * floating point numbers */

typedef float zf_cell;
#define ZF_CELL_FMT "%.14g"
#define ZF_SCAN_FMT "%f"

/* zf_int use for bitops, some arch int type width is less than register width,
   it will cause sign fill, so we need manual specify it */
typedef int zf_int;

/* The type to use for pointers and adresses. 'unsigned int' is usually a good
 * choice for best performance and smallest code size */

typedef unsigned int zf_addr;
#define ZF_ADDR_FMT "%04x"


/* Memory region sizes: dictionary size is given in bytes, stack sizes are
 * number of elements of type zf_cell */

#define ZF_DICT_SIZE 4096
#define ZF_DSTACK_SIZE 32
#define ZF_RSTACK_SIZE 32

#endif

/* Abort reasons */

typedef enum {
	ZF_OK,
	ZF_ABORT_INTERNAL_ERROR,
	ZF_ABORT_OUTSIDE_MEM,
	ZF_ABORT_DSTACK_UNDERRUN,
	ZF_ABORT_DSTACK_OVERRUN,
	ZF_ABORT_RSTACK_UNDERRUN,
	ZF_ABORT_RSTACK_OVERRUN,
	ZF_ABORT_NOT_A_WORD,
	ZF_ABORT_COMPILE_ONLY_WORD,
	ZF_ABORT_INVALID_SIZE,
	ZF_ABORT_DIVISION_BY_ZERO,
	ZF_ABORT_INVALID_USERVAR,
	ZF_ABORT_EXTERNAL
} zf_result;

typedef enum {
	ZF_INPUT_INTERPRET,
	ZF_INPUT_PASS_CHAR,
	ZF_INPUT_PASS_WORD
} zf_input_state;

typedef enum {
	ZF_SYSCALL_EMIT,
	ZF_SYSCALL_PRINT,
	ZF_SYSCALL_TELL,
	ZF_SYSCALL_USER = 128
} zf_syscall_id;

typedef enum {
    ZF_USERVAR_HERE = 0,
    ZF_USERVAR_LATEST,
    ZF_USERVAR_TRACE,
    ZF_USERVAR_COMPILING,
    ZF_USERVAR_POSTPONE,
    ZF_USERVAR_DSP,
    ZF_USERVAR_RSP,

    ZF_USERVAR_COUNT
} zf_uservar_id;


/* ZForth API functions */


void zf_init(int trace);
void zf_bootstrap(void);
void *zf_dump(size_t *len);
zf_result zf_eval(const char *buf);
void zf_abort(zf_result reason);

void zf_push(zf_cell v);
zf_cell zf_pop(void);
zf_cell zf_pick(zf_addr n);

zf_result zf_uservar_set(zf_uservar_id uv, zf_cell v);
zf_result zf_uservar_get(zf_uservar_id uv, zf_cell *v);

/* Host provides these functions */

zf_input_state zf_host_sys(zf_syscall_id id, const char *last_word);
void zf_host_trace(const char *fmt, va_list va);
zf_cell zf_host_parse_num(const char *buf);

#ifdef __cplusplus
}
#endif

#endif


/* Flags and length encoded in words */

#define ZF_FLAG_IMMEDIATE (1<<6)
#define ZF_FLAG_PRIM      (1<<5)
#define ZF_FLAG_LEN(v)    (v & 0x1f)


/* This macro is used to perform boundary checks. If ZF_ENABLE_BOUNDARY_CHECKS
 * is set to 0, the boundary check code will not be compiled in to reduce size */

#if ZF_ENABLE_BOUNDARY_CHECKS
#define CHECK(exp, abort) if(!(exp)) zf_abort(abort);
#else
#define CHECK(exp, abort)
#endif

typedef enum {
	ZF_MEM_SIZE_VAR = 0,      /* Variable size encoding, 1, 2 or 1+sizeof(zf_cell) bytes */
	ZF_MEM_SIZE_CELL = 1,     /* sizeof(zf_cell) bytes */
	ZF_MEM_SIZE_U8 = 2,
	ZF_MEM_SIZE_U16 = 3,
	ZF_MEM_SIZE_U32 = 4,
	ZF_MEM_SIZE_S8 = 5,
	ZF_MEM_SIZE_S16 = 6,
	ZF_MEM_SIZE_S32 = 7,
	ZF_MEM_SIZE_VAR_MAX = 64, /* Variable size encoding, 1+sizeof(zf_cell) bytes */
} zf_mem_size;

/* Define all primitives, make sure the two tables below always match.  The
 * names are defined as a \0 separated list, terminated by double \0. This
 * saves space on the pointers compared to an array of strings. Immediates are
 * prefixed by an underscore, which is later stripped of when putting the name
 * in the dictionary. */

#define _(s) s "\0"

typedef enum {
	PRIM_EXIT,    PRIM_LIT,       PRIM_LTZ,  PRIM_COL,     PRIM_SEMICOL,  PRIM_ADD,
	PRIM_SUB,     PRIM_MUL,       PRIM_DIV,  PRIM_MOD,     PRIM_DROP,     PRIM_DUP,
	PRIM_PICKR,   PRIM_IMMEDIATE, PRIM_PEEK, PRIM_POKE,    PRIM_SWAP,     PRIM_ROT,
	PRIM_JMP,     PRIM_JMP0,      PRIM_TICK, PRIM_COMMENT, PRIM_PUSHR,    PRIM_POPR,
	PRIM_EQUAL,   PRIM_SYS,       PRIM_PICK, PRIM_COMMA,   PRIM_KEY,      PRIM_LITS,
	PRIM_LEN,     PRIM_AND,       PRIM_OR,   PRIM_XOR,     PRIM_SHL,      PRIM_SHR,
	PRIM_COUNT
} zf_prim;

static const char prim_names[] =
	_("exit")    _("lit")        _("<0")    _(":")     _("_;")        _("+")
	_("-")       _("*")          _("/")     _("%")     _("drop")      _("dup")
	_("pickr")   _("_immediate") _("@@")    _("!!")    _("swap")      _("rot")
	_("jmp")     _("jmp0")       _("'")     _("_(")    _(">r")        _("r>")
	_("=")       _("sys")        _("pick")  _(",,")    _("key")       _("lits")
	_("##")      _("&")          _("|")     _("^")     _("<<")        _(">>");


/* Stacks and dictionary memory */

static zf_cell rstack[ZF_RSTACK_SIZE];
static zf_cell dstack[ZF_DSTACK_SIZE];
static uint8_t dict[ZF_DICT_SIZE];

/* State and stack and interpreter pointers */

static zf_input_state input_state;
static zf_addr ip;

/* setjmp env for handling aborts */

static jmp_buf jmpbuf;

/* User variables are variables which are shared between forth and C. From
 * forth these can be accessed with @ and ! at pseudo-indices in low memory, in
 * C they are stored in an array of zf_addr with friendly reference names
 * through some macros */

#define HERE      uservar[ZF_USERVAR_HERE]      /* compilation pointer in dictionary */
#define LATEST    uservar[ZF_USERVAR_LATEST]    /* pointer to last compiled word */
#define TRACE     uservar[ZF_USERVAR_TRACE]     /* trace enable flag */
#define COMPILING uservar[ZF_USERVAR_COMPILING] /* compiling flag */
#define POSTPONE  uservar[ZF_USERVAR_POSTPONE]  /* flag to indicate next imm word should be compiled */
#define DSP       uservar[ZF_USERVAR_DSP]       /* data stack pointer */
#define RSP	      uservar[ZF_USERVAR_RSP]       /* return stack pointer */

static const char uservar_names[] =
	_("h")   _("latest") _("trace")  _("compiling")  _("_postpone")  _("dsp")
	_("rsp");

static zf_addr *uservar = (zf_addr *)dict;


/* Prototypes */

static void do_prim(zf_prim prim, const char *input);
static zf_addr dict_get_cell(zf_addr addr, zf_cell *v);
static void dict_get_bytes(zf_addr addr, void *buf, size_t len);


/* Tracing functions. If disabled, the trace() function is replaced by an empty
 * macro, allowing the compiler to optimize away the function calls to
 * op_name() */

#if ZF_ENABLE_TRACE

static void do_trace(const char *fmt, ...)
{
	if(TRACE) {
		va_list va;
		va_start(va, fmt);
		zf_host_trace(fmt, va);
		va_end(va);
	}
}

#define trace(...) if(TRACE) do_trace(__VA_ARGS__)

static const char *op_name(zf_addr addr)
{
	zf_addr w = LATEST;
	static char name[32];

	while(TRACE && w) {
		zf_addr xt, p = w;
		zf_cell d, link, op2;
		int lenflags;

		p += dict_get_cell(p, &d);
		lenflags = d;
		p += dict_get_cell(p, &link);
		xt = p + ZF_FLAG_LEN(lenflags);
		dict_get_cell(xt, &op2);

		if(((lenflags & ZF_FLAG_PRIM) && addr == (zf_addr)op2) || addr == w || addr == xt) {
			int l = ZF_FLAG_LEN(lenflags);
			dict_get_bytes(p, name, l);
			name[l] = '\0';
			return name;
		}

		w = link;
	}
	return "?";
}

#else
static void trace(const char *fmt, ...) { }
static const char *op_name(zf_addr addr) { return NULL; }
#endif


/*
 * Handle abort by unwinding the C stack and sending control back into
 * zf_eval()
 */

void zf_abort(zf_result reason)
{
	longjmp(jmpbuf, reason);
}



/*
 * Stack operations. 
 */

void zf_push(zf_cell v)
{
	CHECK(DSP < ZF_DSTACK_SIZE, ZF_ABORT_DSTACK_OVERRUN);
	trace("»" ZF_CELL_FMT " ", v);
	dstack[DSP++] = v;
}


zf_cell zf_pop(void)
{
	zf_cell v;
	CHECK(DSP > 0, ZF_ABORT_DSTACK_UNDERRUN);
	v = dstack[--DSP];
	trace("«" ZF_CELL_FMT " ", v);
	return v;
}


zf_cell zf_pick(zf_addr n)
{
	CHECK(n < DSP, ZF_ABORT_DSTACK_UNDERRUN);
	return dstack[DSP-n-1];
}


static void zf_pushr(zf_cell v)
{
	CHECK(RSP < ZF_RSTACK_SIZE, ZF_ABORT_RSTACK_OVERRUN);
	trace("r»" ZF_CELL_FMT " ", v);
	rstack[RSP++] = v;
}


static zf_cell zf_popr(void)
{
	zf_cell v;
	CHECK(RSP > 0, ZF_ABORT_RSTACK_UNDERRUN);
	v = rstack[--RSP];
	trace("r«" ZF_CELL_FMT " ", v);
	return v;
}

zf_cell zf_pickr(zf_addr n)
{
	CHECK(n < RSP, ZF_ABORT_RSTACK_UNDERRUN);
	return rstack[RSP-n-1];
}



/*
 * All access to dictionary memory is done through these functions.
 */

static zf_addr dict_put_bytes(zf_addr addr, const void *buf, size_t len)
{
	const uint8_t *p = (const uint8_t *)buf;
	size_t i = len;
	CHECK(addr < ZF_DICT_SIZE-len, ZF_ABORT_OUTSIDE_MEM);
	while(i--) dict[addr++] = *p++;
	return len;
}


static void dict_get_bytes(zf_addr addr, void *buf, size_t len)
{
	uint8_t *p = (uint8_t *)buf;
	CHECK(addr < ZF_DICT_SIZE-len, ZF_ABORT_OUTSIDE_MEM);
	while(len--) *p++ = dict[addr++];
}


/*
 * zf_cells are encoded in the dictionary with a variable length:
 *
 * encode:
 *
 *    integer   0 ..   127  0xxxxxxx
 *    integer 128 .. 16383  10xxxxxx xxxxxxxx
 *    else                  11111111 <raw copy of zf_cell>
 */

#if ZF_ENABLE_TYPED_MEM_ACCESS
#define GET(s, t) if(size == s) { t v ## t; dict_get_bytes(addr, &v ## t, sizeof(t)); *v = v ## t; return sizeof(t); };
#define PUT(s, t, val) if(size == s) { t v ## t = val; return dict_put_bytes(addr, &v ## t, sizeof(t)); }
#else
#define GET(s, t)
#define PUT(s, t, val)
#endif

static zf_addr dict_put_cell_typed(zf_addr addr, zf_cell v, zf_mem_size size)
{
	unsigned int vi = v;
	uint8_t t[2];

	trace("\n+" ZF_ADDR_FMT " " ZF_ADDR_FMT, addr, (zf_addr)v);

	if(size == ZF_MEM_SIZE_VAR) {
		if((v - vi) == 0) {
			if(vi < 128) {
				trace(" ¹");
				t[0] = vi;
				return dict_put_bytes(addr, t, 1);
			}
			if(vi < 16384) {
				trace(" ²");
				t[0] = (vi >> 8) | 0x80;
				t[1] = vi;
				return dict_put_bytes(addr, t, sizeof(t));
			}
		}
	}

	if(size == ZF_MEM_SIZE_VAR || size == ZF_MEM_SIZE_VAR_MAX) {
		trace(" ⁵");
		t[0] = 0xff;
		return dict_put_bytes(addr+0, t, 1) + 
		       dict_put_bytes(addr+1, &v, sizeof(v));
	} 
	
	PUT(ZF_MEM_SIZE_CELL, zf_cell, v);
	PUT(ZF_MEM_SIZE_U8, uint8_t, vi);
	PUT(ZF_MEM_SIZE_U16, uint16_t, vi);
	PUT(ZF_MEM_SIZE_U32, uint32_t, vi);
	PUT(ZF_MEM_SIZE_S8, int8_t, vi);
	PUT(ZF_MEM_SIZE_S16, int16_t, vi);
	PUT(ZF_MEM_SIZE_S32, int32_t, vi);

	zf_abort(ZF_ABORT_INVALID_SIZE);
	return 0;
}


static zf_addr dict_get_cell_typed(zf_addr addr, zf_cell *v, zf_mem_size size)
{
	uint8_t t[2];
	dict_get_bytes(addr, t, sizeof(t));

	if(size == ZF_MEM_SIZE_VAR) {
		if(t[0] & 0x80) {
			if(t[0] == 0xff) {
				dict_get_bytes(addr+1, v, sizeof(*v));
				return 1 + sizeof(*v);
			} else {
				*v = ((t[0] & 0x3f) << 8) + t[1];
				return 2;
			}
		} else {
			*v = t[0];
			return 1;
		}
	} 
	
	GET(ZF_MEM_SIZE_CELL, zf_cell);
	GET(ZF_MEM_SIZE_U8, uint8_t);
	GET(ZF_MEM_SIZE_U16, uint16_t);
	GET(ZF_MEM_SIZE_U32, uint32_t);
	GET(ZF_MEM_SIZE_S8, int8_t);
	GET(ZF_MEM_SIZE_S16, int16_t);
	GET(ZF_MEM_SIZE_S32, int32_t);

	zf_abort(ZF_ABORT_INVALID_SIZE);
	return 0;
}


/*
 * Shortcut functions for cell access with variable cell size
 */

static zf_addr dict_put_cell(zf_addr addr, zf_cell v)
{
	return dict_put_cell_typed(addr, v, ZF_MEM_SIZE_VAR);
}


static zf_addr dict_get_cell(zf_addr addr, zf_cell *v)
{
	return dict_get_cell_typed(addr, v, ZF_MEM_SIZE_VAR);
}


/*
 * Generic dictionary adding, these functions all add at the HERE pointer and
 * increase the pointer
 */

static void dict_add_cell_typed(zf_cell v, zf_mem_size size)
{
	HERE += dict_put_cell_typed(HERE, v, size);
	trace(" ");
}


static void dict_add_cell(zf_cell v)
{
	dict_add_cell_typed(v, ZF_MEM_SIZE_VAR);
}


static void dict_add_op(zf_addr op)
{
	dict_add_cell(op);
	trace("+%s ", op_name(op));
}


static void dict_add_lit(zf_cell v)
{
	dict_add_op(PRIM_LIT);
	dict_add_cell(v);
}


static void dict_add_str(const char *s)
{
	size_t l;
	trace("\n+" ZF_ADDR_FMT " " ZF_ADDR_FMT " s '%s'", HERE, 0, s);
	l = strlen(s);
	HERE += dict_put_bytes(HERE, s, l);
}


/*
 * Create new word, adjusting HERE and LATEST accordingly
 */

static void create(const char *name, int flags)
{
	zf_addr here_prev;
	trace("\n=== create '%s'", name);
	here_prev = HERE;
	dict_add_cell((strlen(name)) | flags);
	dict_add_cell(LATEST);
	dict_add_str(name);
	LATEST = here_prev;
	trace("\n===");
}


/*
 * Find word in dictionary, returning address and execution token
 */

static int find_word(const char *name, zf_addr *word, zf_addr *code)
{
	zf_addr w = LATEST;
	size_t namelen = strlen(name);

	while(w) {
		zf_cell link, d;
		zf_addr p = w;
		size_t len;
		p += dict_get_cell(p, &d);
		p += dict_get_cell(p, &link);
		len = ZF_FLAG_LEN((int)d);
		if(len == namelen) {
			const char *name2 = (const char *)&dict[p];
			if(memcmp(name, name2, len) == 0) {
				*word = w;
				*code = p + len;
				return 1;
			}
		}
		w = link;
	}

	return 0;
}


/*
 * Set 'immediate' flag in last compiled word
 */

static void make_immediate(void)
{
	zf_cell lenflags;
	dict_get_cell(LATEST, &lenflags);
	dict_put_cell(LATEST, (int)lenflags | ZF_FLAG_IMMEDIATE);
}


/*
 * Inner interpreter
 */

static void run(const char *input)
{
	while(ip != 0) {
		zf_cell d;
		zf_addr i, ip_org = ip;
		zf_addr l = dict_get_cell(ip, &d);
		zf_addr code = d;

		trace("\n "ZF_ADDR_FMT " " ZF_ADDR_FMT " ", ip, code);
		for(i=0; i<RSP; i++) trace("┊  ");
		
		ip += l;

		if(code <= PRIM_COUNT) {
			do_prim((zf_prim)code, input);

			/* If the prim requests input, restore IP so that the
			 * next time around we call the same prim again */

			if(input_state != ZF_INPUT_INTERPRET) {
				ip = ip_org;
				break;
			}

		} else {
			trace("%s/" ZF_ADDR_FMT " ", op_name(code), code);
			zf_pushr(ip);
			ip = code;
		}

		input = NULL;
	} 
}


/*
 * Execute bytecode from given address
 */

static void execute(zf_addr addr)
{
	ip = addr;
	RSP = 0;
	zf_pushr(0);

	trace("\n[%s/" ZF_ADDR_FMT "] ", op_name(ip), ip);
	run(NULL);

}


static zf_addr peek(zf_addr addr, zf_cell *val, int len)
{
	if(addr < ZF_USERVAR_COUNT) {
		*val = uservar[addr];
		return 1;
	} else {
		return dict_get_cell_typed(addr, val, (zf_mem_size)len);
	}

}


/*
 * Run primitive opcode
 */

static void do_prim(zf_prim op, const char *input)
{
	zf_cell d1, d2, d3;
	zf_addr addr, len;

	trace("(%s) ", op_name(op));

	switch(op) {

		case PRIM_COL:
			if(input == NULL) {
				input_state = ZF_INPUT_PASS_WORD;
			} else {
				create(input, 0);
				COMPILING = 1;
			}
			break;

		case PRIM_LTZ:
			zf_push(zf_pop() < 0);
			break;

		case PRIM_SEMICOL:
			dict_add_op(PRIM_EXIT);
			trace("\n===");
			COMPILING = 0;
			break;

		case PRIM_LIT:
			ip += dict_get_cell(ip, &d1);
			zf_push(d1);
			break;

		case PRIM_EXIT:
			ip = zf_popr();
			break;
		
		case PRIM_LEN:
			len = zf_pop();
			addr = zf_pop();
			zf_push(peek(addr, &d1, len));
			break;

		case PRIM_PEEK:
			len = zf_pop();
			addr = zf_pop();
			peek(addr, &d1, len);
			zf_push(d1);
			break;

		case PRIM_POKE:
			d2 = zf_pop();
			addr = zf_pop();
			d1 = zf_pop();
			if(addr < ZF_USERVAR_COUNT) {
				uservar[addr] = d1;
				break;
			}
			dict_put_cell_typed(addr, d1, (zf_mem_size)d2);
			break;

		case PRIM_SWAP:
			d1 = zf_pop(); d2 = zf_pop();
			zf_push(d1); zf_push(d2);
			break;

		case PRIM_ROT:
			d1 = zf_pop(); d2 = zf_pop(); d3 = zf_pop();
			zf_push(d2); zf_push(d1); zf_push(d3);
			break;

		case PRIM_DROP:
			zf_pop();
			break;

		case PRIM_DUP:
			d1 = zf_pop();
			zf_push(d1); zf_push(d1);
			break;

		case PRIM_ADD:
			d1 = zf_pop(); d2 = zf_pop();
			zf_push(d1 + d2);
			break;

		case PRIM_SYS:
			d1 = zf_pop();
			input_state = zf_host_sys((zf_syscall_id)d1, input);
			if(input_state != ZF_INPUT_INTERPRET) {
				zf_push(d1); /* re-push id to resume */
			}
			break;

		case PRIM_PICK:
			addr = zf_pop();
			zf_push(zf_pick(addr));
			break;
		
		case PRIM_PICKR:
			addr = zf_pop();
			zf_push(zf_pickr(addr));
			break;

		case PRIM_SUB:
			d1 = zf_pop(); d2 = zf_pop();
			zf_push(d2 - d1);
			break;

		case PRIM_MUL:
			zf_push(zf_pop() * zf_pop());
			break;

		case PRIM_DIV:
			if((d2 = zf_pop()) == 0) {
				zf_abort(ZF_ABORT_DIVISION_BY_ZERO);
			}
			d1 = zf_pop();
			zf_push(d1 / d2);
			break;

		case PRIM_MOD:
			if((int)(d2 = zf_pop()) == 0) {
				zf_abort(ZF_ABORT_DIVISION_BY_ZERO);
			}
			d1 = zf_pop();
			zf_push((int)d1 % (int)d2);
			break;

		case PRIM_IMMEDIATE:
			make_immediate();
			break;

		case PRIM_JMP:
			ip += dict_get_cell(ip, &d1);
			trace("ip " ZF_ADDR_FMT "=>" ZF_ADDR_FMT, ip, (zf_addr)d1);
			ip = d1;
			break;

		case PRIM_JMP0:
			ip += dict_get_cell(ip, &d1);
			if(zf_pop() == 0) {
				trace("ip " ZF_ADDR_FMT "=>" ZF_ADDR_FMT, ip, (zf_addr)d1);
				ip = d1;
			}
			break;

		case PRIM_TICK:
			if (COMPILING) {
				ip += dict_get_cell(ip, &d1);
				trace("%s/", op_name(d1));
				zf_push(d1);
			}
			else {
				if (input) {
					if (find_word(input,&addr,&len)) zf_push(len);
					else zf_abort(ZF_ABORT_INTERNAL_ERROR);
				}
				else input_state = ZF_INPUT_PASS_WORD;
			}
					
			break;

		case PRIM_COMMA:
			d2 = zf_pop();
			d1 = zf_pop();
			dict_add_cell_typed(d1, (zf_mem_size)d2);
			break;

		case PRIM_COMMENT:
			if(!input || input[0] != ')') {
				input_state = ZF_INPUT_PASS_CHAR;
			}
			break;

		case PRIM_PUSHR:
			zf_pushr(zf_pop());
			break;

		case PRIM_POPR:
			zf_push(zf_popr());
			break;

		case PRIM_EQUAL:
			zf_push(zf_pop() == zf_pop());
			break;

		case PRIM_KEY:
			if(input == NULL) {
				input_state = ZF_INPUT_PASS_CHAR;
			} else {
				zf_push(input[0]);
			}
			break;

		case PRIM_LITS:
			ip += dict_get_cell(ip, &d1);
			zf_push(ip);
			zf_push(d1);
			ip += d1;
			break;
		
		case PRIM_AND:
			zf_push((zf_int)zf_pop() & (zf_int)zf_pop());
			break;

		case PRIM_OR:
			zf_push((zf_int)zf_pop() | (zf_int)zf_pop());
			break;

		case PRIM_XOR:
			zf_push((zf_int)zf_pop() ^ (zf_int)zf_pop());
			break;

		case PRIM_SHL:
			d1 = zf_pop();
			zf_push((zf_int)zf_pop() << (zf_int)d1);
			break;

		case PRIM_SHR:
			d1 = zf_pop();
			zf_push((zf_int)zf_pop() >> (zf_int)d1);
			break;

		default:
			zf_abort(ZF_ABORT_INTERNAL_ERROR);
			break;
	}
}


/*
 * Handle incoming word. Compile or interpreted the word, or pass it to a
 * deferred primitive if it requested a word from the input stream.
 */

static void handle_word(const char *buf)
{
	zf_addr w, c = 0;
	int found;

	/* If a word was requested by an earlier operation, resume with the new
	 * word */

	if(input_state == ZF_INPUT_PASS_WORD) {
		input_state = ZF_INPUT_INTERPRET;
		run(buf);
		return;
	}

	/* Look up the word in the dictionary */

	found = find_word(buf, &w, &c);

	if(found) {

		/* Word found: compile or execute, depending on flags and state */

		zf_cell d;
		int flags;
		dict_get_cell(w, &d);
		flags = d;

		if(COMPILING && (POSTPONE || !(flags & ZF_FLAG_IMMEDIATE))) {
			if(flags & ZF_FLAG_PRIM) {
				dict_get_cell(c, &d);
				dict_add_op(d);
			} else {
				dict_add_op(c);
			}
			POSTPONE = 0;
		} else {
			execute(c);
		}
	} else {

		/* Word not found: try to convert to a number and compile or push, depending
		 * on state */

		zf_cell v = zf_host_parse_num(buf);

		if(COMPILING) {
			dict_add_lit(v);
		} else {
			zf_push(v);
		}
	}
}


/*
 * Handle one character. Split into words to pass to handle_word(), or pass the
 * char to a deferred prim if it requested a character from the input stream
 */

static void handle_char(char c)
{
	static char buf[32];
	static size_t len = 0;

	if(input_state == ZF_INPUT_PASS_CHAR) {

		input_state = ZF_INPUT_INTERPRET;
		run(&c);

	} else if(c != '\0' && !isspace(c)) {

		if(len < sizeof(buf)-1) {
			buf[len++] = c;
			buf[len] = '\0';
		}

	} else {

		if(len > 0) {
			len = 0;
			if (buf[0] == '#' && buf[1] == '!') return;
			handle_word(buf);
		}
	}
}


/*
 * Initialisation
 */

void zf_init(int enable_trace)
{
	HERE = ZF_USERVAR_COUNT * sizeof(zf_addr);
	TRACE = enable_trace;
	LATEST = 0;
	DSP = 0;
	RSP = 0;
	COMPILING = 0;
}


#if ZF_ENABLE_BOOTSTRAP

/*
 * Functions for bootstrapping the dictionary by adding all primitive ops and the
 * user variables.
 */

static void add_prim(const char *name, zf_prim op)
{
	int imm = 0;

	if(name[0] == '_') {
		name ++;
		imm = 1;
	}

	create(name, ZF_FLAG_PRIM);
	dict_add_op(op);
	dict_add_op(PRIM_EXIT);
	if(imm) make_immediate();
}

static void add_uservar(const char *name, zf_addr addr)
{
	create(name, 0);
	dict_add_lit(addr);
	dict_add_op(PRIM_EXIT);
}


void zf_bootstrap(void)
{

	/* Add primitives and user variables to dictionary */

	zf_addr i = 0;
	const char *p;
	for(p=prim_names; *p; p+=strlen(p)+1) {
		add_prim(p, (zf_prim)i++);
	} 

	i = 0;
	for(p=uservar_names; *p; p+=strlen(p)+1) {
		add_uservar(p, i++);
	}
}

#else 
void zf_bootstrap(void) {}
#endif


/*
 * Eval forth string
 */

zf_result zf_eval(const char *buf)
{
	zf_result r = (zf_result)setjmp(jmpbuf);

	if(r == ZF_OK) {
		for(;;) {
			handle_char(*buf);
			if(*buf == '\0') {
				return ZF_OK;
			}
			buf ++;
		}
	} else {
		COMPILING = 0;
		RSP = 0;
		DSP = 0;
		return r;
	}
}


void *zf_dump(size_t *len)
{
	if(len) *len = sizeof(dict);
	return dict;
}

zf_result zf_uservar_set(zf_uservar_id uv, zf_cell v)
{
	zf_result result = ZF_ABORT_INVALID_USERVAR;

	if (uv < ZF_USERVAR_COUNT) {
		uservar[uv] = v;
		result = ZF_OK;
	}

	return result;
}

zf_result zf_uservar_get(zf_uservar_id uv, zf_cell *v)
{
	zf_result result = ZF_ABORT_INVALID_USERVAR;

	if (uv < ZF_USERVAR_COUNT) {
		if (v != NULL) {
			*v = uservar[uv];
		}
		result = ZF_OK;
	}

	return result;
}

/*
 * End
 */


#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif



/*
 * Evaluate buffer with code, check return value and report errors
 */

zf_result do_eval(const char *src, int line, const char *buf)
{
	const char *msg = NULL;

	zf_result rv = zf_eval(buf);

	switch(rv)
	{
		case ZF_OK: break;
		case ZF_ABORT_INTERNAL_ERROR: msg = "internal error"; break;
		case ZF_ABORT_OUTSIDE_MEM: msg = "outside memory"; break;
		case ZF_ABORT_DSTACK_OVERRUN: msg = "dstack overrun"; break;
		case ZF_ABORT_DSTACK_UNDERRUN: msg = "dstack underrun"; break;
		case ZF_ABORT_RSTACK_OVERRUN: msg = "rstack overrun"; break;
		case ZF_ABORT_RSTACK_UNDERRUN: msg = "rstack underrun"; break;
		case ZF_ABORT_NOT_A_WORD: msg = "not a word"; break;
		case ZF_ABORT_COMPILE_ONLY_WORD: msg = "compile-only word"; break;
		case ZF_ABORT_INVALID_SIZE: msg = "invalid size"; break;
		case ZF_ABORT_DIVISION_BY_ZERO: msg = "division by zero"; break;
		default: msg = "unknown error";
	}

	if(msg) {
		fprintf(stderr, "\033[31m");
		if(src) fprintf(stderr, "%s:%d: ", src, line);
		fprintf(stderr, "%s\033[0m\n", msg);
	}

	return rv;
}


/*
 * Load given forth file
 */

void include(const char *fname)
{
	char buf[256];

	FILE *f = fopen(fname, "rb");
	int line = 1;
	if(f) {
		while(fgets(buf, sizeof(buf), f)) {
			do_eval(fname, line++, buf);
		}
		fclose(f);
	} else {
		fprintf(stderr, "error opening file '%s': %s\n", fname, strerror(errno));
	}
}


/*
 * Save dictionary
 */

static void save(const char *fname)
{
	size_t len;
	void *p = zf_dump(&len);
	FILE *f = fopen(fname, "wb");
	if(f) {
		fwrite(p, 1, len, f);
		fclose(f);
	}
}


/*
 * Load dictionary
 */

static void load(const char *fname)
{
	size_t len;
	void *p = zf_dump(&len);
	FILE *f = fopen(fname, "rb");
	if(f) {
		fread(p, 1, len, f);
		fclose(f);
	} else {
		perror("read");
	}
}


/*
 * Sys callback function
 */

zf_input_state zf_host_sys(zf_syscall_id id, const char *input)
{
	switch((int)id) {


		/* The core system callbacks */

		case ZF_SYSCALL_EMIT:
			putchar((char)zf_pop());
			fflush(stdout);
			break;

		case ZF_SYSCALL_PRINT:
			printf(ZF_CELL_FMT " ", zf_pop());
			break;

		case ZF_SYSCALL_TELL: {
			zf_cell len = zf_pop();
			zf_cell addr = zf_pop();
			if(addr >= ZF_DICT_SIZE - len) {
				zf_abort(ZF_ABORT_OUTSIDE_MEM);
			}
			void *buf = (uint8_t *)zf_dump(NULL) + (int)addr;
			(void)fwrite(buf, 1, len, stdout);
			fflush(stdout); }
			break;


		/* Application specific callbacks */

		case ZF_SYSCALL_USER + 0:
			printf("\n");
			exit(0);
			break;

		case ZF_SYSCALL_USER + 1:
			zf_push(sin(zf_pop()));
			break;

		case ZF_SYSCALL_USER + 2:
			if(input == NULL) {
				return ZF_INPUT_PASS_WORD;
			}
			include(input);
			break;
		
		case ZF_SYSCALL_USER + 3:
			save("zforth.save");
			break;

		default:
			printf("unhandled syscall %d\n", id);
			break;
	}

	return ZF_INPUT_INTERPRET;
}


/*
 * Tracing output
 */

void zf_host_trace(const char *fmt, va_list va)
{
	fprintf(stderr, "\033[1;30m");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\033[0m");
}


/*
 * Parse number
 */

zf_cell zf_host_parse_num(const char *buf)
{
	zf_cell v;
	int n = 0;
	int r = sscanf(buf, ZF_SCAN_FMT"%n", &v, &n);
	if(r != 1 || buf[n] != '\0') {
		zf_abort(ZF_ABORT_NOT_A_WORD);
	}
	return v;
}


void usage(void)
{
	fprintf(stderr, 
		"usage: zfort [options] [src ...]\n"
		"\n"
		"Options:\n"
		"   -h         show help\n"
		"   -t         enable tracing\n"
		"   -l FILE    load dictionary from FILE\n"
		"   -q         quiet\n"
	);
}


/*
 * Main
 */

int main(int argc, char **argv)
{
	int i;
	int c;
	int trace = 0;
	int line = 0;
	int quiet = 0;
	const char *fname_load = NULL;

	/* Parse command line options */

	while((c = getopt(argc, argv, "hl:tq")) != -1) {
		switch(c) {
			case 't':
				trace = 1;
				break;
			case 'l':
				fname_load = optarg;
				break;
			case 'h':
				usage();
				exit(0);
			case 'q':
				quiet = 1;
				break;
		}
	}
	
	argc -= optind;
	argv += optind;


	/* Initialize zforth */

	zf_init(trace);


	/* Load dict from disk if requested, otherwise bootstrap fort
	 * dictionary */

	if(fname_load) {
		load(fname_load);
	} else {
		zf_bootstrap();
	}


	/* Include files from command line */

	for(i=0; i<argc; i++) {
		include(argv[i]);
	}

	if(!quiet) {
		zf_cell here;
		zf_uservar_get(ZF_USERVAR_HERE, &here);
		printf("Welcome to zForth, %d bytes used\n", (int)here);
	}

	/* Interactive interpreter: read a line using readline library,
	 * and pass to zf_eval() for evaluation*/

#ifdef USE_READLINE

	read_history(".zforth.hist");

	for(;;) {

		char *buf = readline("");
		if(buf == NULL) break;

		if(strlen(buf) > 0) {

			do_eval("stdin", ++line, buf);
			printf("\n");

			add_history(buf);
			write_history(".zforth.hist");

		}

		free(buf);
	}
#else
	for(;;) {
		char buf[4096];
		if(fgets(buf, sizeof(buf), stdin)) {
			do_eval("stdin", ++line, buf);
			printf("\n");
		} else {
			break;
		}
	}
#endif

	return 0;
}


/*
 * End
 */

