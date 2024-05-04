// These live in main.c

#define HELP_USAGE   1  // usage: line only
#define HELP_HEADER  2  // Add Toybox header line to help output
#define HELP_SEE     4  // "See COMMAND" instead of dereferencing alias
#define HELP_HTML    8  // Output HTML

struct toy_list *toy_find(char *name);
void show_help(FILE *out, int full);
void check_help(char **arg);
void toy_singleinit(struct toy_list *which, char *argv[]);
void toy_init(struct toy_list *which, char *argv[]);
void toy_exec_which(struct toy_list *which, char *argv[]);
void toy_exec(char *argv[]);

// Array of available commands

extern struct toy_list {
  char *name;
  void (*toy_main)(void);
  char *options;
  unsigned flags;
} toy_list[];

// Global context shared by all commands.

extern struct toy_context {
  struct toy_list *which;  // Which entry in toy_list is this one?
  char **argv;             // Original command line arguments
  char **optargs;          // Arguments left over from get_optflags()
  unsigned long long optflags; // Command line option flags from get_optflags()
  int optc;                // Count of optargs
  short toycount;          // Total number of commands in this build
  char exitval;            // Value error_exit feeds to exit()
  char wasroot;            // dropped setuid

  // toy_init() should not zero past here.
  sigjmp_buf *rebound;     // siglongjmp here instead of exit when do_rebound
  struct arg_list *xexit;  // atexit() functions for xexit(), set by sigatexit()
  void *stacktop;          // nested toy_exec() call count, or 0 if vforked
  int envc;                // Count of original environ entries
  int old_umask;           // Old umask preserved by TOYFLAG_UMASK
  short signal;            // generic_signal() records what signal it saw here
  int signalfd;            // and writes signal to this fd, if set
} toys;

// Two big temporary buffers: one for use by commands, one for library functions

extern char **environ, *toybox_version, toybuf[4096], libbuf[4096];

#define FLAG(x) (!!(toys.optflags&FLAG_##x))  // Return 1 if flag set, 0 if not

#define GLOBALS(...)
#define ARRAY_LEN(array) (sizeof(array)/sizeof(*array))
#define TAGGED_ARRAY(X, ...) {__VA_ARGS__}

#ifndef TOYBOX_VERSION
#ifndef TOYBOX_VENDOR
#define TOYBOX_VENDOR ""
#endif
#define TOYBOX_VERSION "0.8.11"TOYBOX_VENDOR
#endif

#include "args.c"
#include "commas.c"
#include "deflate.c"
#include "dirtree.c"
#include "env.c"
#include "lib.c"
#include "llist.c"
#include "net.c"
#include "portability.c"
#include "tty.c"
#include "utf8.c"
#include "xwrap.c"
