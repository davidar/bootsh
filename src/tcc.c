/*
 *  TCC - Tiny C Compiler
 * 
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "../lib/tcc/tcc.h"

int ld_add_file(TCCState *s1, const char filename[]);

/*
Tiny C Compiler - Copyright (C) 2001-2006 Fabrice Bellard
Usage: tcc [options...] [-o outfile] [-c] infile(s)...
       tcc [options...] -run infile (or --) [arguments...]
General options:
  -c           compile only - generate an object file
  -o outfile   set output filename
  -run         run compiled source
  -fflag       set or reset (with 'no-' prefix) 'flag' (see tcc -hh)
  -std=c99     Conform to the ISO 1999 C standard (default).
  -std=c11     Conform to the ISO 2011 C standard.
  -Wwarning    set or reset (with 'no-' prefix) 'warning' (see tcc -hh)
  -w           disable all warnings
  -v --version show version
  -vv          show search paths or loaded files
  -h -hh       show this, show more help
  -bench       show compilation statistics
  -            use stdin pipe as infile
  @listfile    read arguments from listfile
Preprocessor options:
  -Idir        add include path 'dir'
  -Dsym[=val]  define 'sym' with value 'val'
  -Usym        undefine 'sym'
  -E           preprocess only
Linker options:
  -Ldir        add library path 'dir'
  -llib        link with dynamic or static library 'lib'
  -r           generate (relocatable) object file
  -shared      generate a shared library/dll
  -rdynamic    export all global symbols to dynamic linker
  -soname      set name for shared library to be used at runtime
  -Wl,-opt[=val]  set linker option (see tcc -hh)
Debugger options:
  -g           generate stab runtime debug info
  -gdwarf[-x]  generate dwarf runtime debug info
Misc. options:
  -x[c|a|b|n]  specify type of the next infile (C,ASM,BIN,NONE)
  -nostdinc    do not use standard system include paths
  -nostdlib    do not link with standard crt and libraries
  -Bdir        set tcc's private include/library dir
  -M[M]D       generate make dependency file [ignore system files]
  -M[M]        as above but no other output
  -MF file     specify dependency file name

Special options:
  -P -P1                        with -E: no/alternative #line output
  -dD -dM                       with -E: output #define directives
  -pthread                      same as -D_REENTRANT and -lpthread
  -On                           same as -D__OPTIMIZE__ for n > 0
  -Wp,-opt                      same as -opt
  -include file                 include 'file' above each input file
  -isystem dir                  add 'dir' to system include path
  -static                       link to static libraries (not recommended)
  -dumpversion                  print version
  -print-search-dirs            print search paths
  -dt                           with -run/-E: auto-define 'test_...' macros
Ignored options:
  -arch -C --param -pedantic -pipe -s -traditional
-W[no-]... warnings:
  all                           turn on some (*) warnings
  error[=warning]               stop after warning (any or specified)
  write-strings                 strings are const
  unsupported                   warn about ignored options, pragmas, etc.
  implicit-function-declaration warn for missing prototype (*)
  discarded-qualifiers          warn when const is dropped (*)
-f[no-]... flags:
  unsigned-char                 default char is unsigned
  signed-char                   default char is signed
  common                        use common section instead of bss
  leading-underscore            decorate extern symbols
  ms-extensions                 allow anonymous struct in struct
  dollars-in-identifiers        allow '$' in C symbols
  test-coverage                 create code coverage code
-m... target specific options:
  ms-bitfields                  use MSVC bitfield layout
  no-sse                        disable floats on x86_64
-Wl,... linker options:
  -nostdlib                     do not link with standard crt/libs
  -[no-]whole-archive           load lib(s) fully/only as needed
  -export-all-symbols           same as -rdynamic
  -export-dynamic               same as -rdynamic
  -image-base= -Ttext=          set base address of executable
  -section-alignment=           set section alignment in executable
  -rpath=                       set dynamic library search path
  -enable-new-dtags             set DT_RUNPATH instead of DT_RPATH
  -soname=                      set DT_SONAME elf tag
  -Bsymbolic                    set DT_SYMBOLIC elf tag
  -oformat=[elf32/64-* binary]  set executable output format
  -init= -fini= -Map= -as-needed -O   (ignored)
Predefined macros:
  tcc -E -dM - < /dev/null
See also the manual for more details.
*/

static void print_dirs(const char *msg, char **paths, int nb_paths)
{
    int i;
    printf("%s:\n%s", msg, nb_paths ? "" : "  -\n");
    for(i = 0; i < nb_paths; i++)
        printf("  %s\n", paths[i]);
}

static void print_search_dirs(TCCState *s)
{
    printf("install: %s\n", s->tcc_lib_path);
    /* print_dirs("programs", NULL, 0); */
    print_dirs("include", s->sysinclude_paths, s->nb_sysinclude_paths);
    print_dirs("libraries", s->library_paths, s->nb_library_paths);
    printf("libtcc1:\n  %s/%s\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);
#if !defined TCC_TARGET_PE && !defined TCC_TARGET_MACHO
    print_dirs("crt", s->crt_paths, s->nb_crt_paths);
    printf("elfinterp:\n  %s\n",  DEFAULT_ELFINTERP(s));
#endif
}

static void set_environment(TCCState *s)
{
    char * path;

    path = getenv("C_INCLUDE_PATH");
    if(path != NULL) {
        tcc_add_sysinclude_path(s, path);
    }
    path = getenv("CPATH");
    if(path != NULL) {
        tcc_add_include_path(s, path);
    }
    path = getenv("LIBRARY_PATH");
    if(path != NULL) {
        tcc_add_library_path(s, path);
    }
}

static char *default_outputfile(TCCState *s, const char *first_file)
{
    char buf[1024];
    char *ext;
    const char *name = "a";

    if (first_file && strcmp(first_file, "-"))
        name = tcc_basename(first_file);
    snprintf(buf, sizeof(buf), "%s", name);
    ext = tcc_fileextension(buf);
    if ((s->just_deps || s->output_type == TCC_OUTPUT_OBJ) && !s->option_r && *ext)
        strcpy(ext, ".o");
    else
        strcpy(buf, "a.out");
    return tcc_strdup(buf);
}

static unsigned getclock_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + (tv.tv_usec+500)/1000;
}

int tcc_main(int argc0, char **argv0)
{
    TCCState *s, *s1;
    int ret, opt, n = 0, t = 0, done, tcc_run;
    unsigned start_time = 0, end_time = 0;
    const char *first_file;
    int argc; char **argv;
    FILE *ppfp = stdout;

redo:
    argc = argc0, argv = argv0;
    s = s1 = tcc_new();
#ifdef CONFIG_TCC_SWITCHES /* predefined options */
    tcc_set_options(s, CONFIG_TCC_SWITCHES);
#endif
    opt = tcc_parse_args(s, &argc, &argv, 1);
    if (opt < 0)
        return 1;

    s->static_link = 1;

    tcc_run = s->output_type == TCC_OUTPUT_MEMORY;

    if (n == 0) {
        if (opt == OPT_V || opt == OPT_PRINT_DIRS) {
            /* initialize search dirs */
            set_environment(s);
            tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
            printf("Tiny C Compiler\n");
            print_search_dirs(s);
            return 0;
        }

        if (s->nb_files == 0) {
            tcc_error_noabort("no input files");
        } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
            if (s->outfile && 0!=strcmp("-",s->outfile)) {
                ppfp = fopen(s->outfile, "wb");
                if (!ppfp)
                    tcc_error_noabort("could not write '%s'", s->outfile);
            }
        } else if (s->output_type == TCC_OUTPUT_OBJ && !s->option_r) {
            if (s->nb_libraries)
                tcc_error_noabort("cannot specify libraries with -c");
            else if (s->nb_files > 1 && s->outfile)
                tcc_error_noabort("cannot specify output file with -c many files");
        }
        if (s->nb_errors)
            return 1;
        if (s->do_bench)
            start_time = getclock_ms();
    }

    set_environment(s);
    if (s->output_type == 0 || s->output_type == TCC_OUTPUT_MEMORY)
        s->output_type = TCC_OUTPUT_EXE;
    tcc_set_output_type(s, s->output_type);
    s->ppfp = ppfp;

    if ((s->output_type == TCC_OUTPUT_MEMORY
      || s->output_type == TCC_OUTPUT_PREPROCESS)
        && (s->dflag & 16)) { /* -dt option */
        if (t)
            s->dflag |= 32;
        s->run_test = ++t;
        if (n)
            --n;
    }

    /* compile or add each files or library */
    first_file = NULL;
    do {
        struct filespec *f = s->files[n];
        s->filetype = f->type;
        if (f->type & AFF_TYPE_LIB) {
            if (!strcmp(f->name, "crypt")
             || !strcmp(f->name, "dl")
             || !strcmp(f->name, "m")
             || !strcmp(f->name, "pthread")
             || !strcmp(f->name, "resolv")
             || !strcmp(f->name, "rt")
             || !strcmp(f->name, "util")
             || !strcmp(f->name, "xnet")) {
                // these are just stubs
                ret = 0;
            } else {
                ret = tcc_add_library_err(s, f->name);
            }
        } else {
            if (1 == s->verbose)
                printf("-> %s\n", f->name);
            if (!first_file)
                first_file = f->name;
            ret = tcc_add_file(s, f->name);
        }
        done = ret || ++n >= s->nb_files;
    } while (!done && (s->output_type != TCC_OUTPUT_OBJ || s->option_r));

    while (s->new_undef_sym) {
        s->new_undef_sym = 0;
        ld_add_file(s, "/lib/libc.a");
        ld_add_file(s, "/lib/tcc/libtcc1.a");
    }

    if (s->do_bench)
        end_time = getclock_ms();

    if (s->run_test) {
        t = 0;
    } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
        ;
    } else if (0 == ret) {
        if (tcc_run) {
            char tmpfname[] = "/tmp/.tccrunXXXXXX";
            int fd = mkstemp(tmpfname);
            s->outfile = tcc_strdup(tmpfname);
            close(fd);
            tcc_output_file(s, s->outfile);
            execve(s->outfile, argv, environ);
        } else {
            if (!s->outfile)
                s->outfile = default_outputfile(s, first_file);
            if (!s->just_deps && tcc_output_file(s, s->outfile))
                ;
        }
    }

    done = 1;
    if (t)
        done = 0; /* run more tests with -dt -run */
    else if (s->nb_errors)
        ret = 1;
    else if (n < s->nb_files)
        done = 0; /* compile more files with -c */
    else if (s->do_bench)
        tcc_print_stats(s, end_time - start_time);
    tcc_delete(s);
    if (!done)
        goto redo;
    if (ppfp && ppfp != stdout)
        fclose(ppfp);
    return ret;
}
