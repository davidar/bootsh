#!/bin/sbang
#!/bin/cc -I/src/cosmopolitan -run

#define _COSMO_SOURCE

#undef __INT64_TYPE__
#undef __INTPTR_TYPE__
#undef __PTRDIFF_TYPE__
#undef __SIZE_TYPE__
#undef __UINTPTR_TYPE__
#define __APPLE__
#include "libc/integral/normalize.inc"
#undef __APPLE__

#include "libc/fmt/itoa.h"

#include "libc/calls/tinyprint.c"

#include "libc/elf/findelfsectionbyname.c"
#include "libc/elf/getelfprogramheaderaddress.c"
#include "libc/elf/getelfsectionaddress.c"
#include "libc/elf/getelfsectionheaderaddress.c"
#include "libc/elf/getelfsectionname.c"
#include "libc/elf/getelfsectionnamestringtable.c"
#include "libc/elf/getelfsegmentaddress.c"
#include "libc/elf/getelfstring.c"
#include "libc/elf/getelfstringtable.c"
#include "libc/elf/getelfsymbols.c"
#include "libc/elf/getelfsymboltable.c"
#include "libc/elf/iself64binary.c"
#include "libc/elf/iselfsymbolcontent.c"

#include "libc/fmt/formatoctal64.c"

#include "libc/intrin/formatint64.c"
#include "libc/intrin/formatoctal32.c"

#include "libc/str/endswith.c"
// #include "libc/str/strlcpy.c"

#include "tool/build/lib/getargs.c"

static __inline uint16_t __bswap_16(uint16_t __x)
{
	return __x<<8 | __x>>8;
}

static __inline uint32_t __bswap_32(uint32_t __x)
{
	return __x>>24 | __x>>8&0xff00 | __x<<8&0xff0000 | __x<<24;
}

static __inline uint64_t __bswap_64(uint64_t __x)
{
	return __bswap_32(__x)+0ULL<<32 | __bswap_32(__x>>32);
}

#define __builtin_bswap16(x) __bswap_16(x)
#define __builtin_bswap32(x) __bswap_32(x)
#define __builtin_bswap64(x) __bswap_64(x)

#include "tool/build/ar.c"

char *program_invocation_name = "ar";

const errno_t EXDEV = 18;
const errno_t ENOSYS = 38;
const errno_t ENOTSUP = 95;
const errno_t EOPNOTSUPP = 95;

const struct MagnumStr kErrnoDocs[] = {};

int _pthread_block_cancelation(void) { return 0; }
void _pthread_allow_cancelation(int _) {}
