/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│ vi: set et ft=c ts=2 sts=2 sw=2 fenc=utf-8                               :vi │
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2023 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/

#include "ar.h"

/**
 * Returns true if `elf` is a 64-bit elf executable.
 *
 * @param elf points to the start of the executable image
 * @param mapsize is the number of bytes past `elf` we can access
 * @return true if elf header looks legit
 */
bool IsElf64Binary(const Elf64_Ehdr *elf, size_t mapsize) {
  if (mapsize < sizeof(Elf64_Ehdr))
    return false;
  if (READ32LE(elf->e_ident) != READ32LE(ELFMAG))
    return false;
  return elf->e_ident[EI_CLASS] != ELFCLASS32;
}

/**
 * Returns section header object at `elf.section[i]`.
 *
 * @param elf points to the start of the executable image data
 * @param mapsize is the number of bytes past `elf` we can access
 * @param i is the index of the section header
 * @return pointer to section header within image, or null if
 *     1. `i` was a magic number, i.e. `i >= SHN_LORESERVE`, or
 *     2. `e_shoff` was zero (image has no section headers), or
 *     3. `e_shentsize` had fewer than the mandatory 60 bytes, or
 *     4. section header wasn't contained by `[elf,elf+mapsize)`, or
 *     5. an arithmetic overflow occurred
 */
Elf64_Shdr *GetElfSectionHeaderAddress(const Elf64_Ehdr *elf,  //
                                       size_t mapsize,         //
                                       Elf64_Half i) {         //
  Elf64_Off off;
  if (i >= SHN_LORESERVE)
    return 0;
  if (i >= elf->e_shnum)
    return 0;
  if (elf->e_shoff <= 0)
    return 0;
  if (elf->e_shoff >= mapsize)
    return 0;
  if (elf->e_shentsize < sizeof(Elf64_Shdr))
    return 0;
  if ((off = elf->e_shoff + (unsigned)i * elf->e_shentsize) > mapsize)
    return 0;
  return (Elf64_Shdr *)((char *)elf + off);
}

/**
 * Returns section name string table.
 *
 * @param elf points to the start of the executable image
 * @param mapsize is the number of bytes past `elf` we can access
 * @return double-nul terminated string list, or null on error
 */
char *GetElfSectionNameStringTable(const Elf64_Ehdr *elf, size_t mapsize) {
  return GetElfSectionAddress(
      elf, mapsize, GetElfSectionHeaderAddress(elf, mapsize, elf->e_shstrndx));
}

char *GetElfSectionName(const Elf64_Ehdr *elf, size_t mapsize,
                        const Elf64_Shdr *shdr) {
  if (!shdr)
    return 0;
  return GetElfString(elf, mapsize, GetElfSectionNameStringTable(elf, mapsize),
                      shdr->sh_name);
}

/**
 * Returns pointer to elf string table.
 *
 * @param elf points to the start of the executable image data
 * @param mapsize is the number of bytes past `elf` we can access
 * @param section_name is usually `".strtab"`, `".dynstr"`, or null
 * @return pointer to string table within `elf` image, which should
 *     normally be a sequence of NUL-terminated strings whose first
 *     string is the empty string; otherwise NULL is returned, when
 *     either: (1) `section_name` is not found, (2) it did not have
 *     the `SHT_STRTAB` section type, (3) the section size was zero
 *     noting that the ELF spec does consider that legal, or lastly
 *     (4) an overflow or boundary violation occurred
 */
char *GetElfStringTable(const Elf64_Ehdr *elf,  //
                        size_t mapsize,         //
                        const char *section_name) {
  int i;
  char *name;
  const Elf64_Shdr *shdr;
  for (i = 0; i < elf->e_shnum; ++i) {
    if ((shdr = GetElfSectionHeaderAddress(elf, mapsize, i)) &&
        shdr->sh_type == SHT_STRTAB &&
        (name = GetElfSectionName(elf, mapsize, shdr)) &&
        (!section_name || !strcmp(name, section_name))) {
      return GetElfSectionAddress(elf, mapsize, shdr);
    }
  }
  return 0;
}


/**
 * Returns pointer to the elf section header for a symbol table.
 *
 * The easiest way to get the symbol table is:
 *
 *     Elf64_Xword i, n;
 *     Elf64_Sym *st = GetElfSymbols(map, size, SHT_SYMTAB, &n);
 *     for (i = 0; st && i < n; ++i) {
 *       // st[i] holds a symbol
 *     }
 *
 * This API is more verbose than the GetElfSymbols() shortcut, however
 * calling this the long way makes tricks like the following possible:
 *
 *     Elf64_Xword i, n;
 *     Elf64_Shdr *sh = GetElfSymbolTable(map, size, SHT_SYMTAB, &n);
 *     Elf64_Sym *st = GetElfSectionAddress(map, size, sh);
 *     if (st) {
 *       for (i = sh->sh_info; i < n; ++i) {
 *         // st[i] holds a non-local symbol
 *       }
 *     }
 *
 * Our code here only cares about `STB_GLOBAL` and `STB_WEAK` symbols
 * however `SHT_SYMTAB` usually has countless `STB_LOCAL` entries too
 * that must be skipped over. The trick is that the ELF spec requires
 * local symbols be ordered before global symbols, and that the index
 * dividing the two be stored to `sh_info`. So, if we start iterating
 * there, then we've cleverly avoided possibly dozens of page faults!
 *
 * @param elf points to the start of the executable image data
 * @param mapsize is the number of bytes past `elf` we can access
 * @param section_type is usually `SHT_SYMTAB` or `SHT_DYNSYM`
 * @param out_count optionally receives number of symbols
 * @return pointer to symbol table section header, otherwise null
 */
Elf64_Shdr *GetElfSymbolTable(const Elf64_Ehdr *elf,  //
                              size_t mapsize,         //
                              int section_type,       //
                              Elf64_Xword *out_count) {
  int i;
  Elf64_Shdr *shdr;
  for (i = elf->e_shnum; i > 0; --i) {
    if ((shdr = GetElfSectionHeaderAddress(elf, mapsize, i - 1)) &&  //
        shdr->sh_entsize == sizeof(Elf64_Sym) &&                     //
        shdr->sh_type == section_type) {
      if (out_count) {
        *out_count = shdr->sh_size / sizeof(Elf64_Sym);
      }
      return shdr;
    }
  }
  return 0;
}


/**
 * Returns pointer to ELF section file content.
 *
 * This function computes `elf + sh_offset` with safety checks.
 *
 * @param elf points to the start of the executable image data
 * @param mapsize is the number of bytes of `elf` we can access
 * @param shdr is from GetElfSectionHeaderAddress(), or null
 * @return pointer to section data within image, or null if
 *     1. `shdr` was null, or
 *     2. content wasn't contained within `[elf,elf+mapsize)`, or
 *     3. an arithmetic overflow occurred
 */
void *GetElfSectionAddress(const Elf64_Ehdr *elf,     // validated
                           size_t mapsize,            // validated
                           const Elf64_Shdr *shdr) {  // foreign
  // Elf64_Off last;
  if (!shdr)
    return 0;
  if (!shdr->sh_size)
    return (void *)elf;
  if (shdr->sh_type == SHT_NOBITS)
    return 0;
  // if (ckd_add(&last, shdr->sh_offset, shdr->sh_size))
  //   return 0;
  // if (last > mapsize)
  //   return 0;
  return (char *)elf + shdr->sh_offset;
}

/**
 * Returns `strtab + i` from elf string table.
 *
 * @param elf points to the start of the executable image data
 * @param mapsize is the number of bytes past `elf` we can access
 * @param strtab is double-nul string list from GetElfStringTable()
 *     which may be null, in which case only the `!i` name is valid
 * @param i is byte index into strtab where needed string starts or
 *     zero (no name) in which case empty string is always returned
 *     as a pointer to the read-only string literal, rather than in
 *     the elf image, since the elf spec permits an empty or absent
 *     string table section
 * @return a const nul-terminated string pointer, otherwise null if
 *     1. `i` was nonzero and `strtab` was null, or
 *     2. `strtab+i` wasn't inside `[elf,elf+mapsize)`, or
 *     3. a nul byte wasn't present within `[strtab+i,elf+mapsize)`, or
 *     4. an arithmetic overflow occurred
 */
char *GetElfString(const Elf64_Ehdr *elf,  // validated
                   size_t mapsize,         // validated
                   const char *strtab,     // validated
                   Elf64_Word i) {         // foreign
  const char *e;
  if (!i)
    return "";
  e = (const char *)elf;
  if (!strtab)
    return 0;
  if (i >= mapsize)
    return 0;
  if (strtab + i >= e + mapsize)
    return 0;
  if (!memchr(strtab + i, 0, (e + mapsize) - (strtab + i)))
    return 0;
  return (char *)strtab + i;
}

/**
 * Converts unsigned 32-bit integer to octal string.
 *
 * @param p needs at least 12 bytes
 * @param z ensures it starts with zero
 * @return pointer to nul byte
 */
char *FormatOctal32(char p[static 13], uint32_t x, bool z) {
  char t;
  size_t i, a, b;
  i = 0;
  z = x && z;
  do {
    p[i++] = x % 8 + '0';
    x = x / 8;
  } while (x > 0);
  if (z)
    p[i++] = '0';
  p[i] = '\0';
  if (i) {
    for (a = 0, b = i - 1; a < b; ++a, --b) {
      t = p[a];
      p[a] = p[b];
      p[b] = t;
    }
  }
  return p + i;
}

/**
 * Converts unsigned 64-bit integer to string.
 *
 * @param p needs at least 21 bytes
 * @return pointer to nul byte
 */
char *FormatUint64(char p[static 21], uint64_t x) {
  char t;
  size_t i, a, b;
  i = 0;
  do {
    p[i++] = x % 10 + '0';
    x = x / 10;
  } while (x > 0);
  p[i] = '\0';
  if (i) {
    for (a = 0, b = i - 1; a < b; ++a, --b) {
      t = p[a];
      p[a] = p[b];
      p[b] = t;
    }
  }
  return p + i;
}

/**
 * Returns true if s has suffix.
 *
 * @param s is a NUL-terminated string
 * @param suffix is also NUL-terminated
 */
bool endswith(const char *s, const char *suffix) {
  size_t n, m;
  n = strlen(s);
  m = strlen(suffix);
  if (m > n)
    return false;
  return !memcmp(s + n - m, suffix, m);
}



/**
 * @fileoverview cosmopolitan ar
 *
 * This static archiver is superior:
 *
 * - Isn't "accidentally quadratic" like GNU ar
 * - Goes 2x faster than LLVM ar while using 100x less memory
 * - Can be built as a 52kb APE binary that works well on six OSes
 *
 * This static archiver introduces handy features:
 *
 * - Arguments may be supplied in an `@args.txt` file
 * - Directory arguments are ignored
 *
 * @see https://www.unix.com/man-page/opensolaris/3head/ar.h/
 * @see https://en.wikipedia.org/wiki/Ar_(Unix)
 */

#define VERSION                     \
  "cosmopolitan ar v2.0\n"          \
  "copyright 2023 justine tunney\n" \
  "https://github.com/jart/cosmopolitan\n"

#define HEAP_SIZE (256L * 1024 * 1024)

struct Ints {
  int *p;
  size_t i;
};

struct Args {
  char **p;
  size_t i;
};

struct Bytes {
  char *p;
  size_t i;
};

static void SortChars(char *A, long n) {
  long i, j, t;
  for (i = 1; i < n; i++) {
    t = A[i];
    j = i - 1;
    while (j >= 0 && A[j] > t) {
      A[j + 1] = A[j];
      j = j - 1;
    }
    A[j + 1] = t;
  }
}

static void Die(const char *path, const char *reason) {
  fprintf(stderr, "%s: %s\n", path, reason);
  exit(1);
}

static void SysDie(const char *path, const char *func) {
  fprintf(stderr, "%s: %s: %s\n", path, func, strerror(errno));
  exit(1);
}

static const char *USAGE =
  VERSION
  "\n"
  "USAGE\n"
  "\n"
  "  ar FLAGS ARCHIVE FILE...\n"
  "\n"
  "FLAGS\n"
  "\n"
  "  rcs        create new archive with index\n"
  "  rcsD       always deterministic\n"
  "  --help     show usage\n"
  "  --version  show program details\n"
  "\n"
  "ARGUMENTS\n"
  "\n"
  "  ARCHIVE    should be foo.a\n"
  "  FILE       should be foo.o or @args.txt\n"
  "\n";

// allocates 𝑛 bytes of memory aligned on 𝑎 from .bss
// - avoids binary bloat of mmap() and malloc()
// - dies if out of memory or overflow occurs
// - new memory is always zero-initialized
// - can't be resized; use reballoc api
// - can't be freed or reclaimed
static void *balloc(size_t n, size_t a) {
  size_t c;
  int resizable;
  uintptr_t h, p;
  static size_t used;
  static char heap[HEAP_SIZE];
  assert(a >= 1 && !(a & (a - 1)));
  h = (uintptr_t)heap;
  p = h + used;
  if ((resizable = (ssize_t)n < 0)) {
    n = ~n;
    p += sizeof(c);
  }
  p += a - 1;
  p &= -a;
  if (n <= a) {
    c = a;
  } else if (!resizable) {
    c = n;
  } else {
    c = 2ull << (__builtin_clzll(n - 1) ^ (sizeof(long long) * CHAR_BIT - 1));
  }
  if (c < a || c > HEAP_SIZE || p + c > h + HEAP_SIZE) {
    Die("ar", "out of memory");
  }
  used = p - h + c;
  if (resizable) {
    memcpy((char *)p - sizeof(c), &c, sizeof(c));
  }
  return (void *)p;
}

// reallocates 𝑛 𝑧-sized elements aligned on 𝑧 from .bss
// - avoids binary bloat of mmap() and realloc()
// - dies if out of memory or overflow occurs
// - new memory is always zero-initialized
// - abstracts multiply overflow check
// - shrinking always happens in-place
// - growing cost is always amortized
// - can't be freed or reclaimed
static void *reballoc(void *p, size_t n, size_t z) {
  size_t c;
  assert(n >= 0);
  assert(z >= 1 && !(z & (z - 1)));
  n = n * z;
  // if (ckd_mul(&n, n, z))
  //   n = HEAP_SIZE;
  if (!p)
    return balloc(~n, z);
  memcpy(&c, (char *)p - sizeof(c), sizeof(c));
  assert(c >= z && c < HEAP_SIZE && !(c & (c - 1)));
  if (n <= c)
    return p;
  return memcpy(balloc(~n, z), p, c);
}

static char *StrDup(const char *s) {
  size_t n = strlen(s) + 1;
  return memcpy(balloc(n, 1), s, n);
}

static char *StrCat(const char *a, const char *b) {
  char *p;
  size_t n, m;
  n = strlen(a);
  m = strlen(b);
  p = balloc(n + m + 1, 1);
  memcpy(p, a, n);
  memcpy(p + n, b, m + 1);
  return p;
}

static void AppendInt(struct Ints *l, int i) {
  l->p = reballoc(l->p, l->i + 2, sizeof(*l->p));
  l->p[l->i++] = i;
}

static void AppendArg(struct Args *l, char *s) {
  l->p = reballoc(l->p, l->i + 2, sizeof(*l->p));
  l->p[l->i++] = s;
}

static void AppendBytes(struct Bytes *l, const char *s, size_t n) {
  l->p = reballoc(l->p, l->i + n + 1, sizeof(*l->p));
  memcpy(l->p + l->i, s, n);
  l->i += n;
}

static int IsEqual(const char *a, const char *b) {
  return !strcmp(a, b);
}

static void MakeArHeader(struct ar_hdr *h,  //
                         const char *name,  //
                         int mode,          //
                         size_t size) {     //
  size_t n;
  char b[21];
  memset(h, ' ', sizeof(*h));
  n = strlen(name);
  if (n > ARRAYLEN(h->ar_name)) {
    Die("ar", "ar_name overflow");
  }
  memcpy(h->ar_name, name, n);
  if (!IsEqual(name, "//")) {
    h->ar_date[0] = '0';
    h->ar_uid[0] = '0';
    h->ar_gid[0] = '0';
    memcpy(h->ar_mode, b, FormatOctal32(b, mode & 0777, false) - b);
  }
  if (size > 9999999999) {
    Die("ar", "ar_size overflow");
  }
  memcpy(h->ar_size, b, FormatUint64(b, size) - b);
  memcpy(h->ar_fmag, ARFMAG, sizeof(h->ar_fmag));
}

// copies data between file descriptors until end of file
// - assumes signal handlers aren't in play
// - uses copy_file_range() if possible
// - returns number of bytes exchanged
// - dies if operation fails
static int64_t CopyFileOrDie(const char *inpath, int infd,  //
                             const char *outpath, int outfd) {
  int64_t toto;
  char buf[512];
  size_t exchanged;
  ssize_t got, wrote;
  enum { CFR, RW } mode;
  for (mode = CFR, toto = 0;; toto += exchanged) {
    if (mode == CFR) {
      got = copy_file_range(infd, 0, outfd, 0, 4194304, 0);
      if (!got)
        break;
      if (got != -1) {
        exchanged = got;
      } else if (errno == EXDEV ||       // different partitions
                 errno == ENOSYS ||      // not linux or freebsd
                 errno == ENOTSUP ||     // probably a /zip file
                 errno == EOPNOTSUPP) {  // technically the same
        exchanged = 0;
        mode = RW;
      } else {
        SysDie(inpath, "copy_file_range");
      }
    } else {
      got = read(infd, buf, sizeof(buf));
      if (!got)
        break;
      if (got == -1)
        SysDie(inpath, "read");
      wrote = write(outfd, buf, got);
      if (wrote == -1)
        SysDie(outpath, "write");
      if (wrote != got)
        Die(outpath, "posix violated");
      exchanged = wrote;
    }
  }
  return toto;
}

int ar_main(int argc, char *argv[]) {
  int fd, objectid;
  struct ar_hdr header1;
  struct ar_hdr header2;

#ifdef MODE_DBG
  ShowCrashReports();
#endif

  // handle hardcoded flags
  if (argc == 2) {
    if (IsEqual(argv[1], "-n")) {
      exit(0);
    }
    if (IsEqual(argv[1], "-h") ||  //
        IsEqual(argv[1], "-?") ||  //
        IsEqual(argv[1], "--help")) {
      puts(USAGE);
      exit(0);
    }
    if (IsEqual(argv[1], "--version")) {
      puts(VERSION);
      exit(0);
    }
  }

  // get flags and output path
  if (argc < 3) {
    fprintf(stderr, USAGE);
    exit(1);
  }
  char *flags = argv[1];
  const char *outpath = argv[2];

  // we only support one mode of operation, which is creating a new
  // deterministic archive. computing the full archive goes so fast
  // on modern systems that it isn't worth supporting the byzantine
  // standard posix ar flags intended to improve cassette tape perf
  SortChars(flags, strlen(flags));
  if (*flags == 'D')
    ++flags;
  if (!IsEqual(flags, "cr") &&    //
      !IsEqual(flags, "cru") &&   //
      !IsEqual(flags, "crsu") &&  //
      !IsEqual(flags, "crs")) {
    fprintf(stderr, "ar: flags should be rcsD\n");
    fprintf(stderr, USAGE);
    exit(1);
  }

  struct Args args = {reballoc(0, 4096, sizeof(char *))};
  struct Args names = {reballoc(0, 4096, sizeof(char *))};
  struct Ints modes = {reballoc(0, 4096, sizeof(int))};
  struct Ints sizes = {reballoc(0, 4096, sizeof(int))};
  struct Ints symnames = {reballoc(0, 16384, sizeof(int))};
  struct Bytes symbols = {reballoc(0, 131072, sizeof(char))};
  struct Bytes filenames = {reballoc(0, 16384, sizeof(char))};

  // perform analysis pass on input files
  struct GetArgs ga;
  getargs_init(&ga, argv + 3);
  for (objectid = 0;;) {
    struct stat st;
    const char *arg;
    if (!(arg = getargs_next(&ga)))
      break;
    if (endswith(arg, "/"))
      continue;
    if (endswith(arg, ".pkg"))
      continue;
    if (stat(arg, &st))
      SysDie(arg, "stat");
    if (S_ISDIR(st.st_mode))
      continue;
    if (!st.st_size)
      Die(arg, "file is empty");
    if (st.st_size > 0x7ffff000)
      Die(arg, "file too large");
    if ((fd = open(arg, O_RDONLY)) == -1)
      SysDie(arg, "open");
    AppendArg(&args, StrDup(arg));
    AppendInt(&sizes, st.st_size);
    AppendInt(&modes, st.st_mode);
    char bnbuf[PATH_MAX + 1];
    strlcpy(bnbuf, arg, sizeof(bnbuf));
    char *aname = StrCat(basename(bnbuf), "/");
    if (strlen(aname) <= sizeof(header1.ar_name)) {
      AppendArg(&names, aname);
    } else {
      char ibuf[21];
      FormatUint64(ibuf, filenames.i);
      AppendArg(&names, StrCat("/", ibuf));
      AppendBytes(&filenames, aname, strlen(aname));
      AppendBytes(&filenames, "\n", 1);
    }
    size_t mapsize = st.st_size;
    void *elf = mmap(0, mapsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (elf == MAP_FAILED)
      SysDie(arg, "mmap");
    if (!IsElf64Binary(elf, mapsize))
      Die(arg, "not an elf64 binary");
    char *strs = GetElfStringTable(elf, mapsize, ".strtab");
    if (!strs)
      Die(arg, "elf .strtab not found");
    Elf64_Xword symcount;
    Elf64_Shdr *symsec = GetElfSymbolTable(elf, mapsize, SHT_SYMTAB, &symcount);
    Elf64_Sym *syms = GetElfSectionAddress(elf, mapsize, symsec);
    if (!syms)
      Die(arg, "elf symbol table not found");
    for (Elf64_Xword j = symsec->sh_info; j < symcount; ++j) {
      if (!syms[j].st_name)
        continue;
      if (syms[j].st_shndx == SHN_UNDEF)
        continue;
      if (syms[j].st_shndx == SHN_COMMON)
        continue;
      const char *symname = GetElfString(elf, mapsize, strs, syms[j].st_name);
      if (!symname)
        Die(arg, "elf symbol name corrupted");
      AppendBytes(&symbols, symname, strlen(symname) + 1);
      AppendInt(&symnames, objectid);
    }
    if (munmap(elf, mapsize))
      SysDie(arg, "munmap");
    if (close(fd))
      SysDie(arg, "close");
    ++objectid;
  }
  getargs_destroy(&ga);

  // compute length of output archive
  size_t outsize = 0;
  struct iovec iov[8];
  int tablebufsize = 4 + symnames.i * 4;
  char *tablebuf = balloc(tablebufsize, 1);
  int *offsets = balloc(args.i * sizeof(int), sizeof(int));
  iov[0].iov_base = ARMAG;
  outsize += (iov[0].iov_len = SARMAG);
  iov[1].iov_base = &header1;
  outsize += (iov[1].iov_len = sizeof(struct ar_hdr));
  iov[2].iov_base = tablebuf;
  outsize += (iov[2].iov_len = tablebufsize);
  iov[3].iov_base = symbols.p;
  outsize += (iov[3].iov_len = symbols.i);
  iov[4].iov_base = "";
  outsize += (iov[4].iov_len = outsize & 1);
  iov[5].iov_base = &header2;
  outsize += (iov[5].iov_len = filenames.i ? sizeof(struct ar_hdr) : 0);
  iov[6].iov_base = filenames.p;
  outsize += (iov[6].iov_len = filenames.i);
  iov[7].iov_base = "\n";
  outsize += (iov[7].iov_len = filenames.i & 1);
  for (size_t i = 0; i < args.i; ++i) {
    outsize += outsize & 1;
    if (outsize > INT_MAX) {
      Die(outpath, "archive too large");
    }
    offsets[i] = outsize;
    outsize += sizeof(struct ar_hdr);
    outsize += sizes.p[i];
  }

  // serialize metadata
  MakeArHeader(&header1, "/", 0, tablebufsize + ROUNDUP(symbols.i, 2));
  MakeArHeader(&header2, "//", 0, ROUNDUP(filenames.i, 2));
  WRITE32BE(tablebuf, symnames.i);
  for (size_t i = 0; i < symnames.i; ++i) {
    WRITE32BE(tablebuf + 4 + i * 4, offsets[symnames.p[i]]);
  }

  // write output archive
  int outfd;
  if ((outfd = creat(outpath, 0644)) == -1) {
    SysDie(outpath, "creat");
  }
  if (ftruncate(outfd, outsize)) {
    SysDie(outpath, "ftruncate");
  }
  if ((outsize = writev(outfd, iov, ARRAYLEN(iov))) == -1) {
    SysDie(outpath, "writev[1]");
  }
  for (size_t i = 0; i < args.i; ++i) {
    const char *inpath = args.p[i];
    if ((fd = open(inpath, O_RDONLY)) == -1) {
      SysDie(inpath, "open");
    }
    iov[0].iov_base = "\n";
    outsize += (iov[0].iov_len = outsize & 1);
    iov[1].iov_base = &header1;
    outsize += (iov[1].iov_len = sizeof(struct ar_hdr));
    MakeArHeader(&header1, names.p[i], modes.p[i], sizes.p[i]);
    if (writev(outfd, iov, 2) == -1) {
      SysDie(outpath, "writev[2]");
    }
    outsize += sizes.p[i];
    if (CopyFileOrDie(inpath, fd, outpath, outfd) != sizes.p[i]) {
      Die(inpath, "file size changed");
    }
    if (close(fd)) {
      SysDie(inpath, "close");
    }
  }
  if (close(outfd)) {
    SysDie(outpath, "close");
  }

  return 0;
}



/**
 * @fileoverview Fast Command Line Argument Ingestion.
 *
 * The purpose of this library is to be able to have build commands with
 * huge argument lists. The way we do that is by replacing commands like
 *
 *     foo lots of args
 *
 * with this
 *
 *     echo of args >args
 *     foo lots @args
 *
 * This iterator abstracts the process of reading the special `@`
 * prefixed args. In order to do that quickly and easily, we make the
 * following assumptions:
 *
 * 1. Arguments don't have whitespace.
 * 2. Files have a trailing whitespace.
 *
 * We need (1) so GNU Make can go faster. Assume we tokenized based on
 * newlines. We would would write that in our Makefile as follows:
 *
 *     # don't do this
 *     target: thousands of args
 *         $(file >$@.args) $(foreach x,$^,$(file >>$@.args,$(x)))
 *         tool -o $@ @$@.args
 *
 * That is slow because it needs to open and close the args file
 * thousands of times. If we trade away filenames with spaces then the
 * following will only require a couple system calls:
 *
 *     # do this
 *     target: thousands of args
 *         $(file >$@.args,$^)
 *         tool -o $@ @$@.args
 *
 * We need (2) because it make the code in this file simpler and avoids
 * a malloc() dependency. Having that trailing character means argument
 * parsing from files can be a zero-copy operation.
 */

#define IsSpace(c) ((255 & (c)) <= ' ')

static void getargs_fail(const char *path, const char *reason) {
  // const char *errstr;
  // if (!(errstr = _strerdoc(errno)))
  //   errstr = "Unknown error";
  // tinyprint(2, path, ": ", reason, ": ", errstr, "\n", NULL);
  exit(1);
}

/**
 * Zeroes GetArgs object and sets its fields.
 * @param args is borrowed for the lifetime of the GetArgs object
 */
void getargs_init(struct GetArgs *ga, char **args) {
  assert(args);
  bzero(ga, sizeof(*ga));
  ga->args = args;
}

/**
 * Releases memory associated with GetArgs object and zeroes it.
 */
void getargs_destroy(struct GetArgs *ga) {
  if (ga->map) {
    if (munmap(ga->map, ga->mapsize))
      abort();
  }
  bzero(ga, sizeof(*ga));
}

/**
 * Gets next argument, e.g.
 *
 *     const char *s;
 *     while ((s = getargs_next(&ga))) {
 *       printf("%s\n", s);
 *     }
 *
 * @return NUL-terminated string; it should not be freed; it should be
 *     assumed that it stays in scope until the next getargs_next call
 */
const char *getargs_next(struct GetArgs *ga) {
  int fd;
  char *p;
  size_t k;
  ssize_t size;
  for (;;) {
    if (ga->map) {
      for (; ga->j < ga->mapsize; ++ga->j) {
        if (!IsSpace(ga->map[ga->j])) {
          break;
        }
      }
      k = 0;
#if defined(__SSE2__) && defined(__GNUC__) && !defined(__STRICT_ANSI__)
      unsigned m;
      typedef unsigned char xmm_t
          __attribute__((__vector_size__(16), __aligned__(1)));
      for (; ga->j + k + 16 <= ga->mapsize; k += 16) {
        if ((m = __builtin_ia32_pmovmskb128(
                     *(const xmm_t *)(ga->map + ga->j + k) >
                     (xmm_t){' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                             ' ', ' ', ' ', ' ', ' ', ' '}) ^
                 0xffff)) {
          k += __builtin_ctzl(m);
          break;
        }
      }
#endif
      for (; ga->j + k < ga->mapsize; ++k) {
        if (IsSpace(ga->map[ga->j + k])) {
          break;
        }
      }
      if (k && ga->j + k < ga->mapsize) {
        ga->map[ga->j + k] = 0;
        p = ga->map + ga->j;
        ga->j += ++k;
        return p;
      }
      if (munmap(ga->map, ga->mapsize))
        abort();
      ga->map = 0;
      ga->mapsize = 0;
      ga->j = 0;
    }
    if (!(p = ga->args[ga->i])) {
      return 0;
    }
    ++ga->i;
    if (*p != '@') {
      return p;
    }
    ++p;
    if ((fd = open((ga->path = p), O_RDONLY)) == -1) {
      getargs_fail(ga->path, "open");
    }
    if ((size = lseek(fd, 0, SEEK_END)) == -1) {
      getargs_fail(ga->path, "lseek");
    }
    if (size) {
      p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
      if (p == MAP_FAILED) {
        getargs_fail(ga->path, "mmap");
      }
      ga->map = p;
      ga->mapsize = size;
    }
    close(fd);
  }
}
