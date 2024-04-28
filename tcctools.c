/* -------------------------------------------------------------- */
/*
 *  TCC - Tiny C Compiler
 *
 *  tcctools.c - extra tools and and -m32/64 support
 *
 */

/* -------------------------------------------------------------- */
/*
 * tiny_impdef creates an export definition file (.def) from a dll
 * on MS-Windows. Usage: tiny_impdef library.dll [-o outputfile]"
 *
 *  Copyright (c) 2005,2007 grischka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef TCC_TARGET_PE

ST_FUNC int tcc_tool_impdef(TCCState *s1, int argc, char **argv)
{
    int ret, v, i;
    char infile[260];
    char outfile[260];

    const char *file;
    char *p, *q;
    FILE *fp, *op;

#ifdef _WIN32
    char path[260];
#endif

    infile[0] = outfile[0] = 0;
    fp = op = NULL;
    ret = 1;
    p = NULL;
    v = 0;

    for (i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if ('-' == a[0]) {
            if (0 == strcmp(a, "-v")) {
                v = 1;
            } else if (0 == strcmp(a, "-o")) {
                if (++i == argc)
                    goto usage;
                strcpy(outfile, argv[i]);
            } else
                goto usage;
        } else if (0 == infile[0])
            strcpy(infile, a);
        else
            goto usage;
    }

    if (0 == infile[0]) {
usage:
        fprintf(stderr,
            "usage: tcc -impdef library.dll [-v] [-o outputfile]\n"
            "create export definition file (.def) from dll\n"
            );
        goto the_end;
    }

    if (0 == outfile[0]) {
        strcpy(outfile, tcc_basename(infile));
        q = strrchr(outfile, '.');
        if (NULL == q)
            q = strchr(outfile, 0);
        strcpy(q, ".def");
    }

    file = infile;
#ifdef _WIN32
    if (SearchPath(NULL, file, ".dll", sizeof path, path, NULL))
        file = path;
#endif
    ret = tcc_get_dllexports(file, &p);
    if (ret || !p) {
        fprintf(stderr, "tcc: impdef: %s '%s'\n",
            ret == -1 ? "can't find file" :
            ret ==  1 ? "can't read symbols" :
            ret ==  0 ? "no symbols found in" :
            "unknown file type", file);
        ret = 1;
        goto the_end;
    }

    if (v)
        printf("-> %s\n", file);

    op = fopen(outfile, "wb");
    if (NULL == op) {
        fprintf(stderr, "tcc: impdef: could not create output file: %s\n", outfile);
        goto the_end;
    }

    fprintf(op, "LIBRARY %s\n\nEXPORTS\n", tcc_basename(file));
    for (q = p, i = 0; *q; ++i) {
        fprintf(op, "%s\n", q);
        q += strlen(q) + 1;
    }

    if (v)
        printf("<- %s (%d symbol%s)\n", outfile, i, &"s"[i<2]);

    ret = 0;

the_end:
    if (p)
        tcc_free(p);
    if (fp)
        fclose(fp);
    if (op)
        fclose(op);
    return ret;
}

#endif /* TCC_TARGET_PE */

/* -------------------------------------------------------------- */
/*
 *  TCC - Tiny C Compiler
 *
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* re-execute the i386/x86_64 cross-compilers with tcc -m32/-m64: */

#if !defined TCC_TARGET_I386 && !defined TCC_TARGET_X86_64

ST_FUNC int tcc_tool_cross(TCCState *s1, char **argv, int option)
{
    tcc_error_noabort("-m%d not implemented.", option);
    return 1;
}

#else
#ifdef _WIN32
#include <process.h>

static char *str_replace(const char *str, const char *p, const char *r)
{
    const char *s, *s0;
    char *d, *d0;
    int sl, pl, rl;

    sl = strlen(str);
    pl = strlen(p);
    rl = strlen(r);
    for (d0 = NULL;; d0 = tcc_malloc(sl + 1)) {
        for (d = d0, s = str; s0 = s, s = strstr(s, p), s; s += pl) {
            if (d) {
                memcpy(d, s0, sl = s - s0), d += sl;
                memcpy(d, r, rl), d += rl;
            } else
                sl += rl - pl;
        }
        if (d) {
            strcpy(d, s0);
            return d0;
        }
    }
}

static int execvp_win32(const char *prog, char **argv)
{
    int ret; char **p;
    /* replace all " by \" */
    for (p = argv; *p; ++p)
        if (strchr(*p, '"'))
            *p = str_replace(*p, "\"", "\\\"");
    ret = _spawnvp(P_NOWAIT, prog, (const char *const*)argv);
    if (-1 == ret)
        return ret;
    _cwait(&ret, ret, WAIT_CHILD);
    exit(ret);
}
#define execvp execvp_win32
#endif /* _WIN32 */

ST_FUNC int tcc_tool_cross(TCCState *s1, char **argv, int target)
{
    char program[4096];
    char *a0 = argv[0];
    int prefix = tcc_basename(a0) - a0;

    snprintf(program, sizeof program,
        "%.*s%s"
#ifdef TCC_TARGET_PE
        "-win32"
#endif
        "-tcc"
#ifdef _WIN32
        ".exe"
#endif
        , prefix, a0, target == 64 ? "x86_64" : "i386");

    if (strcmp(a0, program))
        execvp(argv[0] = program, argv);
    tcc_error_noabort("could not run '%s'", program);
    return 1;
}

#endif /* TCC_TARGET_I386 && TCC_TARGET_X86_64 */
/* -------------------------------------------------------------- */
/* enable commandline wildcard expansion (tcc -o x.exe *.c) */

#ifdef _WIN32
const int _CRT_glob = 1;
#ifndef _CRT_glob
const int _dowildcard = 1;
#endif
#endif

/* -------------------------------------------------------------- */
/* generate xxx.d file */

static char *escape_target_dep(const char *s) {
    char *res = tcc_malloc(strlen(s) * 2 + 1);
    int j;
    for (j = 0; *s; s++, j++) {
        if (is_space(*s)) {
            res[j++] = '\\';
        }
        res[j] = *s;
    }
    res[j] = '\0';
    return res;
}

ST_FUNC int gen_makedeps(TCCState *s1, const char *target, const char *filename)
{
    FILE *depout;
    char buf[1024];
    char **escaped_targets;
    int i, k, num_targets;

    if (!filename) {
        /* compute filename automatically: dir/file.o -> dir/file.d */
        snprintf(buf, sizeof buf, "%.*s.d",
            (int)(tcc_fileextension(target) - target), target);
        filename = buf;
    }

    if(!strcmp(filename, "-"))
        depout = fdopen(1, "w");
    else
        /* XXX return err codes instead of error() ? */
        depout = fopen(filename, "w");
    if (!depout)
        return tcc_error_noabort("could not open '%s'", filename);
    if (s1->verbose)
        printf("<- %s\n", filename);

    escaped_targets = tcc_malloc(s1->nb_target_deps * sizeof(*escaped_targets));
    num_targets = 0;
    for (i = 0; i<s1->nb_target_deps; ++i) {
        for (k = 0; k < i; ++k)
            if (0 == strcmp(s1->target_deps[i], s1->target_deps[k]))
                goto next;
        escaped_targets[num_targets++] = escape_target_dep(s1->target_deps[i]);
    next:;
    }

    fprintf(depout, "%s:", target);
    for (i = 0; i < num_targets; ++i)
        fprintf(depout, " \\\n  %s", escaped_targets[i]);
    fprintf(depout, "\n");
    if (s1->gen_phony_deps) {
        /* Skip first file, which is the c file.
         * Only works for single file give on command-line,
         * but other compilers have the same limitation */
        for (i = 1; i < num_targets; ++i)
            fprintf(depout, "%s:\n", escaped_targets[i]);
    }
    for (i = 0; i < num_targets; ++i)
        tcc_free(escaped_targets[i]);
    tcc_free(escaped_targets);
    fclose(depout);
    return 0;
}

/* -------------------------------------------------------------- */
