#include <assert.h>
#include <byteswap.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#define ARMAG  "!<arch>\n"
#define SARMAG 8
#define ARFMAG "`\n"

struct ar_hdr {
  char ar_name[16];
  char ar_date[12];
  char ar_uid[6];
  char ar_gid[6];
  char ar_mode[8];
  char ar_size[10];
  char ar_fmag[2];
};

#define ROUNDUP(X, K)   (((X) + (K) - 1) & -(K))

#define ARRAYLEN(A) \
  ((sizeof(A) / sizeof(*(A))) / ((unsigned)!(sizeof(A) % sizeof(*(A)))))

struct GetArgs {
  size_t i, j;
  char **args;
  char *path;
  char *map;
  size_t mapsize;
};

void getargs_init(struct GetArgs *, char **);
const char *getargs_next(struct GetArgs *);
void getargs_destroy(struct GetArgs *);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __SWAPBE16(x) (x)
#define __SWAPBE32(x) (x)
#define __SWAPBE64(x) (x)
#else
#define __SWAPBE16(x) bswap_16(x)
#define __SWAPBE32(x) bswap_32(x)
#define __SWAPBE64(x) bswap_64(x)
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __SWAPLE16(x) (x)
#define __SWAPLE32(x) (x)
#define __SWAPLE64(x) (x)
#else
#define __SWAPLE16(x) bswap_16(x)
#define __SWAPLE32(x) bswap_32(x)
#define __SWAPLE64(x) bswap_64(x)
#endif

#define READ16LE(P)                    \
  (__extension__({                     \
    uint16_t __x;                      \
    __builtin_memcpy(&__x, P, 16 / 8); \
    __SWAPLE16(__x);                   \
  }))

#define READ16BE(P)                    \
  (__extension__({                     \
    uint16_t __x;                      \
    __builtin_memcpy(&__x, P, 16 / 8); \
    __SWAPBE16(__x);                   \
  }))

#define READ32LE(P)                    \
  (__extension__({                     \
    uint32_t __x;                      \
    __builtin_memcpy(&__x, P, 32 / 8); \
    __SWAPLE32(__x);                   \
  }))

#define READ32BE(P)                    \
  (__extension__({                     \
    uint32_t __x;                      \
    __builtin_memcpy(&__x, P, 32 / 8); \
    __SWAPBE32(__x);                   \
  }))

#define READ64LE(P)                    \
  (__extension__({                     \
    uint64_t __x;                      \
    __builtin_memcpy(&__x, P, 64 / 8); \
    __SWAPLE32(__x);                   \
  }))

#define READ64BE(P)                    \
  (__extension__({                     \
    uint64_t __x;                      \
    __builtin_memcpy(&__x, P, 64 / 8); \
    __SWAPBE64(__x);                   \
  }))

#define WRITE16LE(P, X)                  \
  (__extension__({                       \
    __typeof__(&(P)[0]) __p = (P);       \
    uint16_t __x = __SWAPLE16(X);        \
    __builtin_memcpy(__p, &__x, 16 / 8); \
    __p + 16 / 8;                        \
  }))

#define WRITE16BE(P, X)                  \
  (__extension__({                       \
    __typeof__(&(P)[0]) __p = (P);       \
    uint16_t __x = __SWAPBE16(X);        \
    __builtin_memcpy(__p, &__x, 16 / 8); \
    __p + 16 / 8;                        \
  }))

#define WRITE32LE(P, X)                  \
  (__extension__({                       \
    __typeof__(&(P)[0]) __p = (P);       \
    uint32_t __x = __SWAPLE32(X);        \
    __builtin_memcpy(__p, &__x, 32 / 8); \
    __p + 32 / 8;                        \
  }))

#define WRITE32BE(P, X)                  \
  (__extension__({                       \
    __typeof__(&(P)[0]) __p = (P);       \
    uint32_t __x = __SWAPBE32(X);        \
    __builtin_memcpy(__p, &__x, 32 / 8); \
    __p + 32 / 8;                        \
  }))

#define WRITE64LE(P, X)                  \
  (__extension__({                       \
    __typeof__(&(P)[0]) __p = (P);       \
    uint64_t __x = __SWAPLE64(X);        \
    __builtin_memcpy(__p, &__x, 64 / 8); \
    __p + 64 / 8;                        \
  }))

#define WRITE64BE(P, X)                  \
  (__extension__({                       \
    __typeof__(&(P)[0]) __p = (P);       \
    uint64_t __x = __SWAPBE64(X);        \
    __builtin_memcpy(__p, &__x, 64 / 8); \
    __p + 64 / 8;                        \
  }))
