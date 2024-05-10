/*
 * muxzcat.c: tiny .xz and .lzma decompression filter
 * by pts@fazekas.hu at Wed Jan 30 15:15:23 CET 2019
 *
 * Compile with any of:
 *
 *   $ gcc -ansi -s -O2 -W -Wall -Wextra -o muxzcat muxzcat.c
 *   $ g++ -ansi -s -O2 -W -Wall -Wextra -o muxzcat muxzcat.c
 *   $ xtiny   gcc -DCONFIG_SIZE_OPT -DCONFIG_PROB32 -ansi -s -Os -W -Wall -Wextra -o muxzcat muxzcat.c
 *   $ xstatic gcc -DCONFIG_SIZE_OPT -DCONFIG_PROB32 -ansi -s -Os -W -Wall -Wextra -o muxzcat muxzcat.c
 *   $ tcc -W -Wall -Wextra -Werror -s -O2 -o muxzcat muxzcat.c
 *   $ i686-w64-mingw32-gcc   -s -Os -W -Wall -Wextra -o muxzcat.exe muxzcat.c
 *   $ x86-64-w64-mingw32-gcc -s -Os -W -Wall -Wextra -o muxzcat.exe muxzcat.c
 *
 * Run with any of:
 *
 *   $ ./muxzcat <input.xz >output.bin
 *   $ ./muxzcat <input.lzma >output.bin
 *
 *   Error is indicated as a non-zero exit status.
 *
 * https://github.com/pts/muxzcat
 *
 * This is free software, GNU GPL >=2.0. There is NO WARRANTY. Use at your risk.
 *
 * muxzcat.c is size-optimized for Linux i386 (also runs on amd64) with
 * `xtiny gcc': the final statically linked executable is 7376 bytes, and with
 * upxbc (`upxbc --elftiny -f -o muxzcat.upx muxzcat') it can be compressed
 * to 4678 bytes.
 *
 * Limitations of muxzcat.c:
 *
 * * In worst case it keeps 2 times the compression dictionary size in
 *   dynamic memory (also multiply it by 3 for realloc overhead), and it
 *   needs 130 KiB of memory on top of it: readBuf is about 64 KiB,
 *   CLzmaDec.prob is about 28 KiB, the rest is decompressBuf (containing
 *   the entire uncompressed data) and a small constant overhead.
 * * It doesn't support dictionary sizes larger than 1610612736 (~1.61 GB).
 *   The default for xz (-6) is 8 MiB.
 *   (This is not a problem in practice, because even the ouput of `xz -9e'
 *   uses only 64 MiB dictionary size.)
 * * It doesn't support LZMA files with an explicit uncompressed size larger
 *   than 4294967294. (lzma(1) in Xz creates LZMA files with unspecified
 *   uncompressed size, and that works without upper limit.) There is no
 *   upper limit for LZMA2 (.xz) files.
 * * For .xz it supports only LZMA2 (no other filters such as BCJ).
 * * For .lzma it doesn't work with files with 5 <= lc + lp <= 12.
 * * It doesn't verify checksums (e.g. CRC-32 and CRC-64).
 * * It extracts the first stream only, and it ignores the index.
 *
 * LZMA algorithm implementation based on
 * https://github.com/pts/pts-tiny-7z-sfx/commit/b9a101b076672879f861d472665afaa6caa6fec1
 * , which is based on 7z922.tar.bz2.
 *
 * Can use: -DCONFIG_DEBUG
 * Can use: -DCONFIG_PROB32  (Increases memory requirements by 28 KiB, decreases code size by 108 bytes on i386, makes it faster.)
 * Can use: -DCONFIG_SIZE_OPT  (Decreases code size by 416 bytes on i386, makes execution 0.206% slower.)
 *
 * $ xtiny gcc-4.8 -DCONFIG_SIZE_OPT -DCONFIG_PROB32 -ansi -s -Os -W -Wall -Wextra -Werror=implicit-function-declaration -o muxzcat muxzcat.c
 * -rwxr-xr-x 1 pts pts 7316 Jan 31 18:03 muxzcat
 *
 * Examples:
 *
 *   # Smallest possible dictionary:
 *   $ xz --lzma2=preset=8,dict=4096 <ta8.tar >ta4k.tar.xz
 *
 * TODO(pts): Make memory usage smaller: use global.dic as a ring buffer? See branch memory-optimization. It's buggy.
 */

#ifdef __TINYC__  /* tcc https://bellard.org/tcc/ , pts-tcc https://github.com/pts/pts-tcc */

#define NULL ((void*)0)

typedef int int32_t;
typedef unsigned uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef int size_t;
typedef unsigned ssize_t;

void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
void *realloc(void *ptr, size_t size);

#define MyRealloc(ptr, old_size, new_size) realloc(ptr, new_size)

#else
#ifdef __XTINY__  /* xtiny https://github.com/pts/pts-xtiny */

#include <xtiny.h>

void *MyRealloc(void *ptr, size_t old_size, size_t new_size) {
  if (ptr) {
    ptr = (void*)mremap(ptr, old_size, new_size, MREMAP_MAYMOVE, 0);
  } else {
    ptr = (void*)mmap2(0, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }
  return ptr == (void*)-1 ? 0 : ptr;
}

#else  /* Not __XTINY__. */
#ifdef __KERNEL32TINY__  /* _WIN32 with kernel32.dll only. */

#include <windows.h>

#define memcmp MyMemcmp

typedef int int32_t;
typedef unsigned uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

void *MyRealloc(void *ptr, size_t old_size, size_t new_size) {
  if (ptr) {
    /* !! TODO(pts): Use GlobalAlloc and GlobalReAlloc instead: https://stackoverflow.com/questions/34326835/localalloc-vs-globalalloc-vs-malloc-vs-new
     * Or can we use VirtualAlloc with the same address to realloc?
     * TODO(pts): Test this.
     */
    if (new_size > old_size) {
      void *ptr2 = VirtualAlloc(NULL, new_size, MEM_COMMIT, PAGE_READWRITE);
      if (!ptr2) return NULL;  /* VirtualFree(ptr); could be used to save memory. */
      memcpy(ptr2, ptr, old_size);
      VirtualFree(ptr, 0, MEM_RELEASE);
      ptr = ptr2;
    }
  } else {
    /* Wine allows 768 MiB, but not 1 GiB VirtualAlloc. */
    ptr = VirtualAlloc(NULL, new_size, MEM_COMMIT, PAGE_READWRITE);
  }
  return ptr;
}

#else  /* Not _WIN32. */

#include <string.h>  /* memcpy(), memmove() */
#include <unistd.h>  /* read(), write() */
#if defined(MSDOS) || defined(_WIN32)
#  include <fcntl.h>  /* setmode() */
#endif
#include <stdint.h>
#include <stdlib.h>  /* realloc() */

#define MyRealloc(ptr, old_size, new_size) realloc(ptr, new_size)

#endif  /* Not _WIN32 */
#endif  /* Not __XTINY__ */
#endif  /* Not __TINYC__ */

typedef int32_t  Int32;
typedef uint32_t UInt32;
typedef int16_t  Int16;
typedef uint16_t UInt16;
typedef uint8_t  Byte;

/* This fails to compile if any condition after the : is false. */
struct IntegerTypeAsserts {
  int ByteIsInteger : (Byte)1 / 2 == 0;
  int ByteIs8Bits : sizeof(Byte) == 1;
  int ByteIsUnsigned : (Byte)-1 > 0;
  int UInt16IsInteger : (UInt16)1 / 2 == 0;
  int UInt16Is16Bits : sizeof(UInt16) == 2;
  int UInt16IsUnsigned : (UInt16)-1 > 0;
  int UInt32IsInteger : (UInt32)1 / 2 == 0;
  int UInt32Is32Bits : sizeof(UInt32) == 4;
  int UInt32IsUnsigned : (UInt32)-1 > 0;
};

/* --- */

#ifdef CONFIG_DEBUG
/* This is guaranteed to work with Linux and gcc only. For example, %lld in
 * printf doesn't work with MinGW.
 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#define DEBUGF(...) fprintf(stderr, "DEBUG: " __VA_ARGS__)
#define ASSERT(condition) assert(condition)
#else
#define DEBUGF(...)
/* Just check that it compiles. */
#define ASSERT(condition) do {} while (0 && (condition))
#endif

/* TODO(pts): Use__attribute__((sysv_abi, regparm(1))) for muxzcat.upx only. */
#define STATIC static /*__attribute__((sysv_abi, regparm(3)))*/

#if 0  /* __i386 is is always defined, just playing it safe. */
static __inline__ void MemmoveOverlap(void *dest, void *src, UInt32 n) {
  __asm__ __volatile__ ("rep movsb\n\t"
                        : "+D" (dest), "+S" (src), "+c" (n)
                        :
                        : "memory" );
}
#else
/* gcc-4.8 -Os is smart enough to generate rep movsb for this C
 * implementation, there is no size difference. In fact, it generates
 * better code with this than the inline assembly.
 */
STATIC void MemmoveOverlap(void *dest, const void *src, UInt32 n) {
  char *destCp = (char*)dest;
  char *srcCp = (char*)src;
  for (; n > 0; --n) {
    *destCp++ = *srcCp++;
  }
}
#endif

/* --- */

#define SZ_OK 0

#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2  /* Out of memory. */
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
/*#define SZ_ERROR_OUTPUT_EOF 7*/
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_FINISHED_WITH_MARK 15            /* LzmaDec_DecodeToDic stream was finished with end mark. */
#define SZ_ERROR_NOT_FINISHED 16                  /* LzmaDec_DecodeToDic stream was not finished, i.e. dicfLimit reached while there is input to decompress */
#define SZ_ERROR_NEEDS_MORE_INPUT 17              /* LzmaDec_DecodeToDic, you must provide more input bytes */
/*#define SZ_MAYBE_FINISHED_WITHOUT_MARK SZ_OK*/  /* LzmaDec_DecodeToDic, there is probability that stream was finished without end mark */
#define SZ_ERROR_CHUNK_NOT_CONSUMED 18
#define SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL 17      /* LzmaDec_DecodeToDic, more input needed, but existing input was partially processed */

typedef UInt32 SRes;

#ifndef RINOK
#define RINOK(x) { SRes __result__ = (x); if (__result__ != 0) return __result__; }
#endif

typedef Byte Bool;
#define True 1
#define False 0

#define LZMA_REQUIRED_INPUT_MAX 20

/* CONFIG_PROB32 can increase the speed on some CPUs,
   but memory usage for CLzmaDec::probs will be doubled in that case
   CONFIG_PROB32 increases memory usage by 28268 bytes.
   */
#ifdef CONFIG_PROB32
#define CLzmaProb UInt32
#else
#define CLzmaProb UInt16
#endif

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768
#define LZMA2_LCLP_MAX 4

#define MAX_DIC_SIZE 1610612736  /* ~1.61 GB. 2 GiB is user virtual memory limit for many 32-bit systems. */
#define MAX_DIC_SIZE_PROP 37

#define MAX_MATCH_SIZE 273
#define MAX_DICF_SIZE (MAX_DIC_SIZE + MAX_MATCH_SIZE)  /* Maximum number of bytes in global.dicf. */

/* For LZMA streams, lc <= 8, lp <= 4, lc + lp <= 8 + 4 == 12.
 * For LZMA2 streams, lc + lp <= 4.
 * Minimum value: 1846.
 * Maximum value for LZMA streams: 1846 + (768 << (8 + 4)) == 3147574.
 * Maximum value for LZMA2 streams: 1846 + (768 << 4) == 14134.
 * Memory usage of prob: sizeof(CLzmaProb) * value == (2 or 4) * value bytes.
 */
#define LzmaProps_GetNumProbs(p) ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))
/* 14134 */
#define Lzma2Props_GetMaxNumProbs() ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << LZMA2_LCLP_MAX))

typedef struct {
  /* lc, lp and pb would fit into a byte, but i386 code is shorter as UInt32.
   *
   * Constraints:
   *
   * * (0 <= lc <= 8) by LZMA.
   * * 0 <= lc <= 4 by LZMA2 and muxzcat-LZMA and muzxcat-LZMA2.
   * * 0 <= lp <= 4.
   * * 0 <= pb <= 4.
   * * (0 <= lc + lp == 8 + 4 <= 12) by LZMA.
   * * 0 <= lc + lp <= 4 by LZMA2 and muxzcat-LZMA and muxzcat-LZMA2.
   */
  UInt32 lc, lp, pb; /* Configured in prop byte. */
  /* Maximum lookback delta.
   * More optimized implementations (but not this version of muxzcat) need
   * that many bytes of storage for the dictionary. muxzcat uses more,
   * because it keeps the entire decompression output in memory, for
   * the simplicity of the implementation.
   * Configured in dicSizeProp byte. Maximum LZMA and LZMA2 supports is 0xffffffff,
   * maximum we support is MAX_DIC_SIZE == 1610612736.
   */
  UInt32 dicSize;
  const Byte *buf;
  UInt32 range, code;
  UInt32 dicfPos;  /* The next decompression output byte will be written to dicf + dicfPos. */
  UInt32 dicfLimit;  /* It's OK to write this many decompression output bytes to dic. GrowDic(dicfPos + len) must be called before writing len bytes at dicfPos. */
  UInt32 writtenPos;  /* Decompression output bytes dicf[:writtenPos] are already written to the output file. writtenPos <= dicfPos. */
  UInt32 discardedSize;  /* Number of decompression output bytes discarded. */
  UInt32 writeRemaining;  /* Maximum number of remaining bytes to write, or ~(UInt32)0 for unlimited. */
  UInt32 allocCapacity;  /* Number of bytes allocated in dic. */
  UInt32 processedPos;  /* Decompression output byte count since the last call to LzmaDec_InitDicAndState(True, ...); */
  UInt32 checkDicSize;
  UInt32 state;
  UInt32 reps[4];
  UInt32 remainLen;
  UInt32 tempBufSize;
  CLzmaProb probs[Lzma2Props_GetMaxNumProbs()];
  Bool needFlush;
  Bool needInitLzma;
  Bool needInitDic;
  Bool needInitState;
  Bool needInitProp;
  Byte tempBuf[LZMA_REQUIRED_INPUT_MAX];
  /* Contains the decompresison output, and used as the lookback dictionary.
   * allocCapacity bytes are allocated, it's OK to grow it up to dicfLimit.
   */
  Byte *dicf;
} CLzmaDec;

static CLzmaDec global;

/* --- */

#ifdef __KERNEL32TINY__
static HANDLE hstdin, hstdout;

/* TODO(pts): Optimize this for equality; optimize for 7 bytes. */
STATIC Int32 MyMemcmp(const void *s1, const void *s2, UInt32 n) {
  while (n-- != 0) {
    const Int32 d = *(unsigned char*)s1++ - *(unsigned char*)s2++;
    if (d != 0) return d;
  }
  return 0;
}
#endif  /* __KERNEL32TINY__ */

/* Writes uncompressed data (global.dicf[global.writtenPos : global.dicfPos] to stdout. */
STATIC SRes Flush(void) {
  const UInt32 flushSize1 = global.dicfPos - global.writtenPos;
  const UInt32 flushSize = flushSize1 > global.writeRemaining ? global.writeRemaining : flushSize1;
  const Byte *p = global.dicf + global.writtenPos;
  const Byte *q = p + flushSize;
  DEBUGF("FLUSH WRITE %d %d dicfPos=%d\n", flushSize1, flushSize, global.dicfPos);
  while (p != q) {
#ifdef __KERNEL32TINY__
    DWORD got;
    /* EOF or error on input. */
    if (!WriteFile(hstdout, p, q - p, &got, 0) || got == 0) {
      return SZ_ERROR_WRITE;
    }
#else
    const Int32 got = write(1, p, q - p);
    if (got <= 0) return SZ_ERROR_WRITE;
#endif  /* Not __KERNEL32TINY__. */
    p += got;
    global.writtenPos += got;
    if (global.writeRemaining != ~(UInt32)0) global.writeRemaining -= got;
  }
  global.writtenPos = global.dicfPos;  /* Ignore truncated output bytes. */
  return SZ_OK;
}

STATIC SRes FlushDiscardOldFromStartOfDic(void) {
  if (global.dicfPos > global.dicSize) {
    const UInt32 delta = global.dicfPos - global.dicSize;
    if (delta + MAX_MATCH_SIZE >= global.dicSize) {
      DEBUGF("DISCARD OLD delta=%d dicSize=%d\n", delta, global.dicSize);
      RINOK(Flush());
      MemmoveOverlap(global.dicf, global.dicf + delta, global.dicSize);
      global.dicfPos -= delta;
      global.dicfLimit -= delta;
      global.writtenPos -= delta;
      global.discardedSize += delta;
    }
  }
  return SZ_OK;
}

STATIC SRes GrowCapacity(UInt32 newCapacity) {
  if (newCapacity > global.allocCapacity) {
    DEBUGF("GROWCAPACITY allocCapacity/old=%d newCapacity=%d\n", global.allocCapacity, newCapacity);
    if (newCapacity > MAX_DICF_SIZE) return SZ_ERROR_MEM;
    /* Possible memory leak if realloc fails, returning NULL. */
    global.dicf = (Byte*)MyRealloc(global.dicf, global.allocCapacity, newCapacity);
    if (!global.dicf) return SZ_ERROR_MEM;
    global.allocCapacity = newCapacity;
  }
  return SZ_OK;
}

STATIC SRes FlushDiscardGrowDic(UInt32 dicfPosDelta) {
  UInt32 minCapacity = global.dicfPos + dicfPosDelta;
  if (minCapacity > global.allocCapacity) {
    RINOK(FlushDiscardOldFromStartOfDic());
    minCapacity = global.dicfPos + dicfPosDelta;
    if (minCapacity > global.allocCapacity) {
      UInt32 newCapacity = 65536;
      while (newCapacity < minCapacity) {
        if (newCapacity > global.dicSize) {  /* No overflow. */
          newCapacity = global.dicSize << 1;
          if (newCapacity < minCapacity) newCapacity = minCapacity;
          break;
        }
        newCapacity <<= 1;
      }
      DEBUGF("GROWDIC allocCapacity/old=%d minCapacity=%d newCapacity=%d\n", global.allocCapacity, minCapacity, newCapacity);
      RINOK(GrowCapacity(newCapacity));
    }
  }
  return SZ_OK;
}

/* --- */

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_INIT_SIZE 5

#define NORMALIZE if (range < kTopValue) { range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0(p) ttt = *(p); NORMALIZE; bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1) IF_BIT_0(p) \
  { UPDATE_0(p); i = (i + i); A0; } else \
  { UPDATE_1(p); i = (i + i) + 1; A1; }
#define GET_BIT(p, i) GET_BIT2(p, i, ; , ;)

#define TREE_GET_BIT(probs, i) { GET_BIT((probs + i), i); }
#define TREE_DECODE(probs, limit, i) \
  { i = 1; do { TREE_GET_BIT(probs, i); } while (i < limit); i -= limit; }

#ifdef CONFIG_SIZE_OPT
#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)
#else
#define TREE_6_DECODE(probs, i) \
  { i = 1; \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  i -= 0x40; }
#endif

#define NORMALIZE_CHECK if (range < kTopValue) { if (buf >= bufLimit) return DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0_CHECK(p) ttt = *(p); NORMALIZE_CHECK; bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK range -= bound; code -= bound;
#define GET_BIT2_CHECK(p, i, A0, A1) IF_BIT_0_CHECK(p) \
  { UPDATE_0_CHECK; i = (i + i); A0; } else \
  { UPDATE_1_CHECK; i = (i + i) + 1; A1; }
#define GET_BIT_CHECK(p, i) GET_BIT2_CHECK(p, i, ; , ;)
#define TREE_DECODE_CHECK(probs, limit, i) \
  { i = 1; do { GET_BIT_CHECK(probs + i, i) } while (i < limit); i -= limit; }


#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenChoice 0
#define LenChoice2 (LenChoice + 1)
#define LenLow (LenChoice2 + 1)
#define LenMid (LenLow + (kNumPosStatesMax << kLenNumLowBits))
#define LenHigh (LenMid + (kNumPosStatesMax << kLenNumMidBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)


#define kNumStates 12
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)

#define IsMatch 0
#define IsRep (IsMatch + (kNumStates << kNumPosBitsMax))
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define IsRep0Long (IsRepG2 + kNumStates)
#define PosSlot (IsRep0Long + (kNumStates << kNumPosBitsMax))
#define SpecPos (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define Align (SpecPos + kNumFullDistances - kEndPosModelIndex)
#define LenCoder (Align + kAlignTableSize)
#define RepLenCoder (LenCoder + kNumLenProbs)
#define Literal (RepLenCoder + kNumLenProbs)

#if Literal != LZMA_BASE_SIZE
#error StopCompilingDueBUG
#endif

#define LZMA_DIC_MIN (1 << 12)

STATIC SRes LzmaDec_DecodeReal(UInt32 limit, const Byte *bufLimit)
{
  CLzmaProb *probs = global.probs;

  UInt32 state = global.state;
  UInt32 rep0 = global.reps[0], rep1 = global.reps[1], rep2 = global.reps[2], rep3 = global.reps[3];
  UInt32 pbMask = ((UInt32)1 << (global.pb)) - 1;
  UInt32 lpMask = ((UInt32)1 << (global.lp)) - 1;
  UInt32 lc = global.lc;

  Byte *dicl = global.dicf;
  UInt32 diclLimit = global.dicfLimit;
  UInt32 diclPos = global.dicfPos;

  UInt32 processedPos = global.processedPos;
  UInt32 checkDicSize = global.checkDicSize;
  UInt32 len = 0;

  const Byte *buf = global.buf;
  UInt32 range = global.range;
  UInt32 code = global.code;

  do
  {
    CLzmaProb *prob;
    UInt32 bound;
    UInt32 ttt;
    UInt32 posState = processedPos & pbMask;

    prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
    IF_BIT_0(prob)
    {
      UInt32 symbol;
      UPDATE_0(prob);
      prob = probs + Literal;
      if (checkDicSize != 0 || processedPos != 0)
        prob += (LZMA_LIT_SIZE * (((processedPos & lpMask) << lc) +
        (dicl[(diclPos == 0 ? diclLimit : diclPos) - 1] >> (8 - lc))));

      if (state < kNumLitStates)
      {
        state -= (state < 4) ? state : 3;
        symbol = 1;
        do { GET_BIT(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        UInt32 matchByte = dicl[(diclPos - rep0) + ((diclPos < rep0) ? diclLimit : 0)];
        UInt32 offs = 0x100;
        state -= (state < 10) ? 3 : 6;
        symbol = 1;
        do
        {
          UInt32 bit;
          CLzmaProb *probLit;
          matchByte <<= 1;
          bit = (matchByte & offs);
          probLit = prob + offs + bit + symbol;
          GET_BIT2(probLit, symbol, offs &= ~bit, offs &= bit)
        }
        while (symbol < 0x100);
      }
      if (diclPos >= global.allocCapacity) {
        global.dicfPos = diclPos;
        RINOK(FlushDiscardGrowDic(1));
        dicl = global.dicf; diclLimit = global.dicfLimit; diclPos = global.dicfPos;
      }
      dicl[diclPos++] = (Byte)symbol;
      processedPos++;
      continue;
    }
    else
    {
      UPDATE_1(prob);
      prob = probs + IsRep + state;
      IF_BIT_0(prob)
      {
        UPDATE_0(prob);
        state += kNumStates;
        prob = probs + LenCoder;
      }
      else
      {
        UPDATE_1(prob);
        if (checkDicSize == 0 && processedPos == 0)
          return SZ_ERROR_DATA;
        prob = probs + IsRepG0 + state;
        IF_BIT_0(prob)
        {
          UPDATE_0(prob);
          prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob);
            if (diclPos >= global.allocCapacity) {
              global.dicfPos = diclPos;
              RINOK(FlushDiscardGrowDic(1));
              dicl = global.dicf; diclLimit = global.dicfLimit; diclPos = global.dicfPos;
            }
            dicl[diclPos] = dicl[(diclPos - rep0) + ((diclPos < rep0) ? diclLimit : 0)];
            diclPos++;
            processedPos++;
            state = state < kNumLitStates ? 9 : 11;
            continue;
          }
          UPDATE_1(prob);
        }
        else
        {
          UInt32 distance;
          UPDATE_1(prob);
          prob = probs + IsRepG1 + state;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob);
            distance = rep1;
          }
          else
          {
            UPDATE_1(prob);
            prob = probs + IsRepG2 + state;
            IF_BIT_0(prob)
            {
              UPDATE_0(prob);
              distance = rep2;
            }
            else
            {
              UPDATE_1(prob);
              distance = rep3;
              rep3 = rep2;
            }
            rep2 = rep1;
          }
          rep1 = rep0;
          rep0 = distance;
        }
        state = state < kNumLitStates ? 8 : 11;
        prob = probs + RepLenCoder;
      }
      {
        UInt32 limit, offset;
        CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0(probLen)
        {
          UPDATE_0(probLen);
          probLen = prob + LenLow + (posState << kLenNumLowBits);
          offset = 0;
          limit = (1 << kLenNumLowBits);
        }
        else
        {
          UPDATE_1(probLen);
          probLen = prob + LenChoice2;
          IF_BIT_0(probLen)
          {
            UPDATE_0(probLen);
            probLen = prob + LenMid + (posState << kLenNumMidBits);
            offset = kLenNumLowSymbols;
            limit = (1 << kLenNumMidBits);
          }
          else
          {
            UPDATE_1(probLen);
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols + kLenNumMidSymbols;
            limit = (1 << kLenNumHighBits);
          }
        }
        TREE_DECODE(probLen, limit, len);
        len += offset;
      }

      if (state >= kNumStates)
      {
        UInt32 distance;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
        TREE_6_DECODE(prob, distance);
        if (distance >= kStartPosModelIndex)
        {
          UInt32 posSlot = (UInt32)distance;
          int numDirectBits = (int)(((distance >> 1) - 1));
          distance = (2 | (distance & 1));
          if (posSlot < kEndPosModelIndex)
          {
            distance <<= numDirectBits;
            prob = probs + SpecPos + distance - posSlot - 1;
            {
              UInt32 mask = 1;
              UInt32 i = 1;
              do
              {
                GET_BIT2(prob + i, i, ; , distance |= mask);
                mask <<= 1;
              }
              while (--numDirectBits != 0);
            }
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE
              range >>= 1;

              {
                UInt32 t;
                code -= range;
                t = (0 - ((UInt32)code >> 31));
                distance = (distance << 1) + (t + 1);
                code += range & t;
              }
              /*
              distance <<= 1;
              if (code >= range)
              {
                code -= range;
                distance |= 1;
              }
              */
            }
            while (--numDirectBits != 0);
            prob = probs + Align;
            distance <<= kNumAlignBits;
            {
              UInt32 i = 1;
              GET_BIT2(prob + i, i, ; , distance |= 1);
              GET_BIT2(prob + i, i, ; , distance |= 2);
              GET_BIT2(prob + i, i, ; , distance |= 4);
              GET_BIT2(prob + i, i, ; , distance |= 8);
            }
            if (distance == (UInt32)0xFFFFFFFF)
            {
              len += kMatchSpecLenStart;
              state -= kNumStates;
              break;
            }
          }
        }
        rep3 = rep2;
        rep2 = rep1;
        rep1 = rep0;
        rep0 = distance + 1;
        if (checkDicSize == 0)
        {
          if (distance >= processedPos)
            return SZ_ERROR_DATA;
        }
        else if (distance >= checkDicSize)
          return SZ_ERROR_DATA;
        state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
      }

      len += kMatchMinLen;
      ASSERT(len <= MAX_MATCH_SIZE);

      if (limit == diclPos)
        return SZ_ERROR_DATA;
      {
        UInt32 rem = limit - diclPos;
        UInt32 curLen = ((rem < len) ? (UInt32)rem : len);
        UInt32 pos = (diclPos - rep0) + ((diclPos < rep0) ? diclLimit : 0);
        processedPos += curLen;
        len -= curLen;  /* TODO(pts): ASSERT(len == curLen);, simplify buffering code. */
        if (diclPos + curLen > global.allocCapacity) {  /* + cannot overflow. */
          global.dicfPos = diclPos;
          RINOK(FlushDiscardGrowDic(curLen));
          pos += global.dicfPos - diclPos;
          dicl = global.dicf; diclLimit = global.dicfLimit; diclPos = global.dicfPos;
        }
        if (pos + curLen <= diclLimit)
        {
          ASSERT(diclPos > pos);
          ASSERT(curLen > 0);
          MemmoveOverlap(dicl + diclPos, dicl + pos, curLen);
          diclPos += curLen;
        }
        else
        {
          do
          {
            dicl[diclPos++] = dicl[pos];
            if (++pos == diclLimit)
              pos = 0;
          }
          while (--curLen != 0);
        }
      }
    }
  }
  while (diclPos < limit && buf < bufLimit);
  NORMALIZE;
  global.buf = buf;
  global.range = range;
  global.code = code;
  global.remainLen = len;
  global.dicfPos = diclPos;
  global.processedPos = processedPos;
  global.reps[0] = rep0;
  global.reps[1] = rep1;
  global.reps[2] = rep2;
  global.reps[3] = rep3;
  global.state = state;

  return SZ_OK;
}

STATIC SRes LzmaDec_WriteRem(UInt32 limit)
{
  if (global.remainLen != 0 && global.remainLen < kMatchSpecLenStart)
  {
    Byte *dicl = global.dicf;
    UInt32 diclPos = global.dicfPos;
    UInt32 diclLimit = global.dicfLimit;
    UInt32 len = global.remainLen;
    UInt32 rep0 = global.reps[0];
    if (limit - diclPos < len)
      len = (UInt32)(limit - diclPos);
    if (diclPos + len > global.allocCapacity) {  /* + cannot overflow, see below. */
      RINOK(FlushDiscardGrowDic(len));
      dicl = global.dicf; diclLimit = global.dicfLimit; diclPos = global.dicfPos;
    }

    if (global.checkDicSize == 0 && global.dicSize - global.processedPos <= len)
      global.checkDicSize = global.dicSize;

    global.processedPos += len;
    global.remainLen -= len;
    while (len != 0)
    {
      len--;
      dicl[diclPos] = dicl[(diclPos - rep0) + ((diclPos < rep0) ? diclLimit : 0)];
      diclPos++;
    }
    global.dicfPos = diclPos;
  }
  return SZ_OK;
}

STATIC SRes LzmaDec_DecodeReal2(UInt32 limit, const Byte *bufLimit)
{
  do
  {
    UInt32 limit2 = limit;
    if (global.checkDicSize == 0)
    {
      UInt32 rem = global.dicSize - global.processedPos;
      if (limit - global.dicfPos > rem)
        limit2 = global.dicfPos + rem;
    }
    RINOK(LzmaDec_DecodeReal(limit2, bufLimit));
    if (global.processedPos >= global.dicSize)
      global.checkDicSize = global.dicSize;
    RINOK(LzmaDec_WriteRem(limit));
  }
  while (global.dicfPos < limit && global.buf < bufLimit && global.remainLen < kMatchSpecLenStart);

  if (global.remainLen > kMatchSpecLenStart)
  {
    global.remainLen = kMatchSpecLenStart;
  }
  return SZ_OK;
}

typedef enum
{
  DUMMY_ERROR, /* unexpected end of input stream */
  DUMMY_LIT,
  DUMMY_MATCH,
  DUMMY_REP
} ELzmaDummy;

STATIC ELzmaDummy LzmaDec_TryDummy(const Byte *buf, UInt32 inSize)
{
  UInt32 range = global.range;
  UInt32 code = global.code;
  const Byte *bufLimit = buf + inSize;
  const CLzmaProb *probs = global.probs;
  UInt32 state = global.state;
  ELzmaDummy res;

  {
    const CLzmaProb *prob;
    UInt32 bound;
    UInt32 ttt;
    UInt32 posState = (global.processedPos) & ((1 << global.pb) - 1);

    prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
    IF_BIT_0_CHECK(prob)
    {
      UPDATE_0_CHECK

      /* if (bufLimit - buf >= 7) return DUMMY_LIT; */

      prob = probs + Literal;
      if (global.checkDicSize != 0 || global.processedPos != 0)
        prob += (LZMA_LIT_SIZE *
          ((((global.processedPos) & ((1 << (global.lp)) - 1)) << global.lc) +
          (global.dicf[(global.dicfPos == 0 ? global.dicfLimit : global.dicfPos) - 1] >> (8 - global.lc))));

      if (state < kNumLitStates)
      {
        UInt32 symbol = 1;
        do { GET_BIT_CHECK(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        UInt32 matchByte = global.dicf[global.dicfPos - global.reps[0] +
            ((global.dicfPos < global.reps[0]) ? global.dicfLimit : 0)];
        UInt32 offs = 0x100;
        UInt32 symbol = 1;
        do
        {
          UInt32 bit;
          const CLzmaProb *probLit;
          matchByte <<= 1;
          bit = (matchByte & offs);
          probLit = prob + offs + bit + symbol;
          GET_BIT2_CHECK(probLit, symbol, offs &= ~bit, offs &= bit)
        }
        while (symbol < 0x100);
      }
      res = DUMMY_LIT;
    }
    else
    {
      UInt32 len;
      UPDATE_1_CHECK;

      prob = probs + IsRep + state;
      IF_BIT_0_CHECK(prob)
      {
        UPDATE_0_CHECK;
        state = 0;
        prob = probs + LenCoder;
        res = DUMMY_MATCH;
      }
      else
      {
        UPDATE_1_CHECK;
        res = DUMMY_REP;
        prob = probs + IsRepG0 + state;
        IF_BIT_0_CHECK(prob)
        {
          UPDATE_0_CHECK;
          prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK;
            NORMALIZE_CHECK;
            return DUMMY_REP;
          }
          else
          {
            UPDATE_1_CHECK;
          }
        }
        else
        {
          UPDATE_1_CHECK;
          prob = probs + IsRepG1 + state;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK;
          }
          else
          {
            UPDATE_1_CHECK;
            prob = probs + IsRepG2 + state;
            IF_BIT_0_CHECK(prob)
            {
              UPDATE_0_CHECK;
            }
            else
            {
              UPDATE_1_CHECK;
            }
          }
        }
        state = kNumStates;
        prob = probs + RepLenCoder;
      }
      {
        UInt32 limit, offset;
        const CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0_CHECK(probLen)
        {
          UPDATE_0_CHECK;
          probLen = prob + LenLow + (posState << kLenNumLowBits);
          offset = 0;
          limit = 1 << kLenNumLowBits;
        }
        else
        {
          UPDATE_1_CHECK;
          probLen = prob + LenChoice2;
          IF_BIT_0_CHECK(probLen)
          {
            UPDATE_0_CHECK;
            probLen = prob + LenMid + (posState << kLenNumMidBits);
            offset = kLenNumLowSymbols;
            limit = 1 << kLenNumMidBits;
          }
          else
          {
            UPDATE_1_CHECK;
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols + kLenNumMidSymbols;
            limit = 1 << kLenNumHighBits;
          }
        }
        TREE_DECODE_CHECK(probLen, limit, len);
        len += offset;
      }

      if (state < 4)
      {
        UInt32 posSlot;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) <<
            kNumPosSlotBits);
        TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot);
        if (posSlot >= kStartPosModelIndex)
        {
          UInt32 numDirectBits = ((posSlot >> 1) - 1);

          /* if (bufLimit - buf >= 8) return DUMMY_MATCH; */

          if (posSlot < kEndPosModelIndex)
          {
            prob = probs + SpecPos + ((2 | (posSlot & 1)) << numDirectBits) - posSlot - 1;
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE_CHECK
              range >>= 1;
              code -= range & (((code - range) >> 31) - 1);
              /* if (code >= range) code -= range; */
            }
            while (--numDirectBits != 0);
            prob = probs + Align;
            numDirectBits = kNumAlignBits;
          }
          {
            UInt32 i = 1;
            do
            {
              GET_BIT_CHECK(prob + i, i);
            }
            while (--numDirectBits != 0);
          }
        }
      }
    }
  }
  NORMALIZE_CHECK;
  return res;
}


STATIC void LzmaDec_InitRc(const Byte *data)
{
  global.code = ((UInt32)data[1] << 24) | ((UInt32)data[2] << 16) | ((UInt32)data[3] << 8) | ((UInt32)data[4]);
  global.range = 0xFFFFFFFF;
  global.needFlush = False;
}

STATIC void LzmaDec_InitDicAndState(Bool initDic, Bool initState)
{
  global.needFlush = True;
  global.remainLen = 0;
  global.tempBufSize = 0;

  if (initDic)
  {
    global.processedPos = 0;
    global.checkDicSize = 0;
    global.needInitLzma = True;
  }
  if (initState)
    global.needInitLzma = True;
}

STATIC void LzmaDec_InitStateReal(void)
{
  UInt32 numProbs = Literal + ((UInt32)LZMA_LIT_SIZE << (global.lc + global.lp));
  UInt32 i;
  CLzmaProb *probs = global.probs;
  for (i = 0; i < numProbs; i++)
    probs[i] = kBitModelTotal >> 1;
  global.reps[0] = global.reps[1] = global.reps[2] = global.reps[3] = 1;
  global.state = 0;
  global.needInitLzma = False;
}

STATIC SRes LzmaDec_DecodeToDic(const Byte *src, UInt32 srcLen) {
  const UInt32 srcLen0 = srcLen;
  UInt32 inSize = srcLen;
  srcLen = 0;
  RINOK(LzmaDec_WriteRem(global.dicfLimit));

  while (global.remainLen != kMatchSpecLenStart)
  {
      Bool checkEndMarkNow;

      if (global.needFlush)
      {
        for (; inSize > 0 && global.tempBufSize < RC_INIT_SIZE; srcLen++, inSize--)
          global.tempBuf[global.tempBufSize++] = *src++;
        if (global.tempBufSize < RC_INIT_SIZE)
        {
         on_needs_more_input:
          if (srcLen != srcLen0) return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL;
          return SZ_ERROR_NEEDS_MORE_INPUT;
        }
        if (global.tempBuf[0] != 0)
          return SZ_ERROR_DATA;

        LzmaDec_InitRc(global.tempBuf);
        global.tempBufSize = 0;
      }

      checkEndMarkNow = False;
      if (global.dicfPos >= global.dicfLimit)
      {
        if (global.remainLen == 0 && global.code == 0)
        {
          if (srcLen != srcLen0) return SZ_ERROR_CHUNK_NOT_CONSUMED;
          return SZ_OK /* MAYBE_FINISHED_WITHOUT_MARK */;
        }
        if (global.remainLen != 0)
        {
          return SZ_ERROR_NOT_FINISHED;
        }
        checkEndMarkNow = True;
      }

      if (global.needInitLzma) {
        LzmaDec_InitStateReal();
      }

      if (global.tempBufSize == 0)
      {
        UInt32 processed;
        const Byte *bufLimit;
        if (inSize < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          SRes dummyRes = LzmaDec_TryDummy(src, inSize);
          if (dummyRes == DUMMY_ERROR)
          {
            memcpy(global.tempBuf, src, inSize);
            global.tempBufSize = (UInt32)inSize;
            srcLen += inSize;
            goto on_needs_more_input;
          }
          if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
          {
            return SZ_ERROR_NOT_FINISHED;
          }
          bufLimit = src;
        }
        else
          bufLimit = src + inSize - LZMA_REQUIRED_INPUT_MAX;
        global.buf = src;
        if (LzmaDec_DecodeReal2(global.dicfLimit, bufLimit) != 0)
          return SZ_ERROR_DATA;
        processed = (UInt32)(global.buf - src);
        srcLen += processed;
        src += processed;
        inSize -= processed;
      }
      else
      {
        UInt32 rem = global.tempBufSize, lookAhead = 0;
        while (rem < LZMA_REQUIRED_INPUT_MAX && lookAhead < inSize)
          global.tempBuf[rem++] = src[lookAhead++];
        global.tempBufSize = rem;
        if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          SRes dummyRes = LzmaDec_TryDummy(global.tempBuf, rem);
          if (dummyRes == DUMMY_ERROR)
          {
            srcLen += lookAhead;
            goto on_needs_more_input;
          }
          if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
          {
            return SZ_ERROR_NOT_FINISHED;
          }
        }
        global.buf = global.tempBuf;
        if (LzmaDec_DecodeReal2(global.dicfLimit, global.buf) != 0)
          return SZ_ERROR_DATA;
        lookAhead -= (rem - (UInt32)(global.buf - global.tempBuf));
        srcLen += lookAhead;
        src += lookAhead;
        inSize -= lookAhead;
        global.tempBufSize = 0;
      }
  }
  if (global.code != 0) return SZ_ERROR_DATA;
  return SZ_ERROR_FINISHED_WITH_MARK;
}

#define LZMA2_GET_LZMA_MODE(pc) (((pc) >> 5) & 3)

/* Works if p <= 39. */
#define LZMA2_DIC_SIZE_FROM_SMALL_PROP(p) (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11))

static Byte readBuf[65536 + 12], *readCur = readBuf, *readEnd = readBuf;
#ifdef CONFIG_DEBUG
static long long readFileOfs = 0;
#endif

/* Tries to preread r bytes to the read buffer. Returns the number of bytes
 * available in the read buffer. If smaller than r, that indicates EOF.
 *
 * Doesn't try to preread more than absolutely necessary, to avoid copies in
 * the future.
 *
 * Works only if r <= sizeof(readBuf).
 */
STATIC UInt32 Preread(UInt32 r) {
  UInt32 p = readEnd - readCur;
  ASSERT(r <= sizeof(readBuf));
  if (p < r) {  /* Not enough pending available. */
    if (readBuf + sizeof(readBuf) - readCur + 0U < r) {
      /* If no room for r bytes to the end, discard bytes from the beginning. */
      DEBUGF("MEMMOVE size=%d\n", p);
      MemmoveOverlap(readBuf, readCur, p);
      readEnd = readBuf + p;
      readCur = readBuf;
    }
    while (p < r) {
      /* Instead of (r - p) we could use (readBuf + sizeof(readBuf) -
       * readEnd) to read as much as the buffer has room for.
       */
      DEBUGF("READ size=%d\n", r - p);
      {
#ifdef __KERNEL32TINY__
        DWORD got;
        /* EOF or error on input. */
        if (!ReadFile(hstdin, readEnd, r - p, &got, 0) || got == 0) break;
#else
        const Int32 got = read(0, readEnd, r - p);
        if (got <= 0) break;  /* EOF or error on input. */
#endif  /* Not __KERNEL32TINY__. */
        readEnd += got;
        p += got;
#ifdef CONFIG_DEBUG
        readFileOfs += got;
#endif
      }
    }
  }
  DEBUGF("PREREAD r=%d p=%d\n", r, p);
  return p;
}

#ifdef CONFIG_DEBUG
/* Returns the number of bytes read from stdin. */
STATIC long long GetReadPosForDebug(void) {
  return readFileOfs - (readEnd - readCur);
}
#endif

#define SZ_ERROR_BAD_MAGIC 51
#define SZ_ERROR_BAD_STREAM_FLAGS 52  /* SZ_ERROR_BAD_MAGIC is reported instead. */
#define SZ_ERROR_UNSUPPORTED_FILTER_COUNT 53
#define SZ_ERROR_BAD_BLOCK_FLAGS 54
#define SZ_ERROR_UNSUPPORTED_FILTER_ID 55
#define SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE 56
#define SZ_ERROR_BAD_PADDING 57
#define SZ_ERROR_BLOCK_HEADER_TOO_LONG 58
#define SZ_ERROR_BAD_CHUNK_CONTROL_BYTE 59
#define SZ_ERROR_BAD_CHECKSUM_TYPE 60
#define SZ_ERROR_BAD_DICTIONARY_SIZE 61
#define SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE 62
#define SZ_ERROR_FEED_CHUNK 63
/*#define SZ_ERROR_NOT_FINISHED_WITH_MARK 64*/
#define SZ_ERROR_BAD_DICPOS 65
#define SZ_ERROR_MISSING_INITPROP 67
#define SZ_ERROR_BAD_LCLPPB_PROP 68

STATIC void IgnoreVarint(void) {
  while (*readCur++ >= 0x80) {}
}

STATIC SRes IgnoreZeroBytes(UInt32 c) {
  for (; c > 0; --c) {
    if (*readCur++ != 0) return SZ_ERROR_BAD_PADDING;
  }
  return SZ_OK;
}

#if defined(__i386) || defined(_M_IX86) || defined(__i386__) || defined(__amd64) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)
/* Shortcut for little endian CPU supporting unaligned access. */
STATIC __inline__ UInt32 GetLE4(const Byte *p) {
  return *(const UInt32*)(char*)p;
}
#else
STATIC UInt32 GetLE4(Byte *p) {
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}
#endif

/* Expects global.dicSize be set already. Can be called before or after InitProp. */
STATIC void InitDecode(void) {
  /* global.lc = global.pb = global.lp = 0; */  /* needinitprop will initialize it */
  global.dicfLimit = 0;  /* We'll increment it later. */
  global.needInitDic = True;
  global.needInitState = True;
  global.needInitProp = True;
  global.writtenPos = 0;
  global.writeRemaining = ~(UInt32)0;
  global.discardedSize = 0;
  global.dicfPos = 0;
  LzmaDec_InitDicAndState(True, True);
}

STATIC SRes InitProp(Byte b) {
  UInt32 lc, lp;
  if (b >= (9 * 5 * 5)) return SZ_ERROR_BAD_LCLPPB_PROP;
  lc = b % 9;
  b /= 9;
  global.pb = b / 5;
  lp = b % 5;
  if (lc + lp > LZMA2_LCLP_MAX) return SZ_ERROR_BAD_LCLPPB_PROP;
  global.lc = lc;
  global.lp = lp;
  global.needInitProp = False;
  return SZ_OK;
}

#define FILTER_ID_LZMA2 0x21

/* Reads .xz or .lzma data from stdin, writes uncompressed bytes to stdout,
 * uses CLzmaDec.dic. It verifies some aspects of the file format (so it
 * can't be tricked to an infinite loop etc.), itdoesn't verify checksums
 * (e.g. CRC32).
 */
STATIC SRes DecompressXzOrLzma(void) {
  Byte checksumSize;
  UInt32 bhf;  /* Block header flags */
  /* 12 for the stream header + 12 for the first block header + 6 for the
   * first chunk header. empty.xz is 32 bytes.
   */
  if (Preread(12 + 12 + 6) < 12 + 12 + 6) return SZ_ERROR_INPUT_EOF;
  /* readbuf[7] is actually stream flags, should also be 0. */
  if (0 == memcmp(readCur, "\xFD""7zXZ\0", 7)) {  /* .xz */
  } else if (readCur[0] <= 225 && readCur[13] == 0 &&  /* .lzma */
        /* High 4 bytes of uncompressed size. */
        ((bhf = GetLE4(readCur + 9)) == 0 || bhf == ~(UInt32)0) &&
        (global.dicSize = GetLE4(readCur + 1)) >= LZMA_DIC_MIN) {
    /* Based on https://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt */
    /* TODO(pts): Support 8-byte uncompressed size. */
    const UInt32 us = bhf == 0 ? GetLE4(readCur + 5) : bhf /* max UInt32 */;
    UInt32 srcLen;
    if (global.dicSize > MAX_DIC_SIZE) return SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE;
    InitDecode();
    global.allocCapacity = 0;
    global.dicf = NULL;
    /* LZMA2 restricts lc + lp <= 4. LZMA requires lc + lp <= 12.
     * We apply the LZMA2 restriction here (to save memory in
     * CLzmaDec.probs), thus we are not able to extract some legitimate
     * .lzma files.
     */
    RINOK(InitProp(readCur[0]));
    readCur += 13;  /* Start decompressing the 0 byte. */
    global.dicfLimit = global.writeRemaining = us;  /* Works even if us == ~(UInt32)0. */
    if (us <= global.dicSize) {
      RINOK(GrowCapacity(us));  /* Preallocate small output buffer, for speed. */
    }
    DEBUGF("LZMA dicSize=0x%x=%d us=%d\n", global.dicSize, global.dicSize, us);
    /* Any Preread(...) amount starting from 1 works here, but higher values
     * are faster.
     */
    while (global.discardedSize + global.dicfPos != us) {
      SRes res;
      if ((srcLen = Preread(sizeof(readBuf))) == 0) {
        if (us != ~(UInt32)0) return SZ_ERROR_INPUT_EOF;
        break;
      }
      res = LzmaDec_DecodeToDic(readCur, srcLen);
      DEBUGF("LZMADEC res=%d\n", res);
      readCur += srcLen;
      if (res == SZ_ERROR_FINISHED_WITH_MARK) break;
      if (res != SZ_ERROR_NEEDS_MORE_INPUT && res != SZ_OK) return res;
    }
    RINOK(Flush());
    return SZ_OK;
  } else {
    return SZ_ERROR_BAD_MAGIC;
  }
  /* Based on https://tukaani.org/xz/xz-file-format-1.0.4.txt */
  switch (readCur[7]) {
   case 0: /* None */ checksumSize = 1; break;
   case 1: /* CRC32 */ checksumSize = 4; break;
   case 4: /* CRC64, typical xz output. */ checksumSize = 8; break;
   default: return SZ_ERROR_BAD_CHECKSUM_TYPE;
  }
  /* Also ignore the CRC32 after checksumSize. */
  readCur += 12;
  global.allocCapacity = 0;
  global.dicf = NULL;
  for (;;) {  /* Next block. */
    /* We need it modulo 4, so a Byte is enough. */
    Byte blockSizePad = 3;
    UInt32 bhs, bhs2; /* Block header size */
    Byte dicSizeProp;
    Byte* readAtBlock;
    ASSERT(readEnd - readCur >= 12);  /* At least 12 bytes preread. */
    if ((bhs = *readCur++) == 0) break;  /* Last block, index follows. */
    /* Block header size includes the bhs field above and the CRC32 below. */
    bhs = (bhs + 1) << 2;
    DEBUGF("bhs=%d\n", bhs);
    /* Typically the Preread(12 + 12 + 6) above covers it. */
    if (Preread(bhs) < bhs) return SZ_ERROR_INPUT_EOF;
    readAtBlock = readCur;
    bhf = *readCur++;
    if ((bhf & 2) != 0) return SZ_ERROR_UNSUPPORTED_FILTER_COUNT;
    DEBUGF("filter count=%d\n", (bhf & 2) + 1);
    if ((bhf & 20) != 0) return SZ_ERROR_BAD_BLOCK_FLAGS;
    if (bhf & 64) {  /* Compressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    if (bhf & 128) {  /* Uncompressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (*readCur++ != FILTER_ID_LZMA2) return SZ_ERROR_UNSUPPORTED_FILTER_ID;
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (*readCur++ != 1) return SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE;
    dicSizeProp = *readCur++;
    /* Typical large dictionary sizes:
     *
     *  * 35: 805306368 bytes == 768 MiB
     *  * 36: 1073741824 bytes == 1 GiB
     *  * 37: 1610612736 bytes, largest supported by .xz
     *  * 38: 2147483648 bytes == 2 GiB
     *  * 39: 3221225472 bytes == 3 GiB
     *  * 40: 4294967295 bytes, largest supported by .xz
     */
    DEBUGF("dicSizeProp=0x%02x\n", dicSizeProp);
    if (dicSizeProp > 40) return SZ_ERROR_BAD_DICTIONARY_SIZE;
    /* LZMA2 and .xz support it, we don't (for simpler memory management on
     * 32-bit systems).
     */
    if (dicSizeProp > MAX_DIC_SIZE_PROP) return SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE;
    global.dicSize = LZMA2_DIC_SIZE_FROM_SMALL_PROP(dicSizeProp);
    /* TODO(pts): Free dic after use, also after realloc error. */
    ASSERT(global.dicSize >= LZMA_DIC_MIN);
    DEBUGF("dicSize39=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(39));
    DEBUGF("dicSize38=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(38));
    DEBUGF("dicSize37=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(37));
    DEBUGF("dicSize36=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(36));
    DEBUGF("dicSize35=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(35));
    bhs2 = readCur - readAtBlock + 5;  /* Won't overflow. */
    DEBUGF("bhs=%d bhs2=%d\n", bhs, bhs2);
    if (bhs2 > bhs) return SZ_ERROR_BLOCK_HEADER_TOO_LONG;
    RINOK(IgnoreZeroBytes(bhs - bhs2));
    readCur += 4;  /* Ignore CRC32. */
    /* Typically it's offset 24, xz creates it by default, minimal. */
    DEBUGF("LZMA2 at %lld\n", GetReadPosForDebug());
    { /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      UInt32 us, cs;  /* Uncompressed and compressed chunk sizes. */
      InitDecode();

      for (;;) {
        Byte control;
        ASSERT(global.dicfPos == global.dicfLimit);
        /* Actually 2 bytes is enough to get to the index if everything is
         * aligned and there is no block checksum.
         */
        if (Preread(6) < 6) return SZ_ERROR_INPUT_EOF;
        control = readCur[0];
        DEBUGF("CONTROL control=0x%02x at=%lld inbuf=%d\n", control, GetReadPosForDebug(), (UInt32)(readCur - readBuf));
        if (control == 0) {
          DEBUGF("LASTFED\n");
          ++readCur;
          break;
        } else if ((Byte)(control - 3) < 0x80 - 3U) {
          return SZ_ERROR_BAD_CHUNK_CONTROL_BYTE;
        }
        us = (readCur[1] << 8) + readCur[2] + 1;
        if (control < 3) {  /* Uncompressed chunk. */
          const Bool initDic = (control == 1);
          cs = us;
          readCur += 3;
          blockSizePad -= 3;
          if (initDic) {
            global.needInitProp = global.needInitState = True;
            global.needInitDic = False;
          } else if (global.needInitDic) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(initDic, False);
        } else {  /* LZMA chunk. */
          const Byte mode = LZMA2_GET_LZMA_MODE(control);
          const Bool initDic = (mode == 3);
          const Bool initState = (mode > 0);
          const Bool isProp = (control & 64) != 0;
          us += (control & 31) << 16;
          cs = (readCur[3] << 8) + readCur[4] + 1;
          if (isProp) {
            RINOK(InitProp(readCur[5]));
            ++readCur;
            --blockSizePad;
          } else {
            if (global.needInitProp) return SZ_ERROR_MISSING_INITPROP;
          }
          readCur += 5;
          blockSizePad -= 5;
          if ((!initDic && global.needInitDic) || (!initState && global.needInitState))
            return SZ_ERROR_DATA;
          LzmaDec_InitDicAndState(initDic, initState);
          global.needInitDic = False;
          global.needInitState = False;
        }
        ASSERT(us <= (1 << 24));
        ASSERT(cs <= (1 << 16));
        ASSERT(global.dicfPos == global.dicfLimit);
        RINOK(FlushDiscardOldFromStartOfDic());
        global.dicfLimit += us;
        if (global.dicfLimit < us) return SZ_ERROR_MEM;  /* `+=' above overflowed. */
        /* Read 6 extra bytes to optimize away a read(...) system call in
         * the Prefetch(6) call in the next chunk header.
         */
        if (Preread(cs + 6) < cs) return SZ_ERROR_INPUT_EOF;
        DEBUGF("FEED us=%d cs=%d dicfPos=%d\n", us, cs, global.dicfPos);
        if (control < 3) {  /* Uncompressed chunk, at most 64 KiB. */
          DEBUGF("DECODE uncompressed\n");
          ASSERT(global.dicfPos + us == global.dicfLimit);
          FlushDiscardGrowDic(us);
          memcpy(global.dicf + global.dicfPos, readCur, us);
          global.dicfPos += us;
          if (global.checkDicSize == 0 && global.dicSize - global.processedPos <= us)
            global.checkDicSize = global.dicSize;
          global.processedPos += us;
        } else {  /* Compressed chunk. */
          DEBUGF("DECODE call\n");
          /* This call doesn't change global.dicfLimit. */
          RINOK(LzmaDec_DecodeToDic(readCur, cs));
        }
        if (global.dicfPos != global.dicfLimit) return SZ_ERROR_BAD_DICPOS;
        readCur += cs;
        blockSizePad -= cs;
        /* We can't discard decompressbuf[:global.dicfLimit] now,
         * because we need it a dictionary in which subsequent calls to
         * Lzma2Dec_DecodeToDic will look up backreferences.
         */
      }
      RINOK(Flush());
    }  /* End of LZMA2 stream. */
    DEBUGF("TELL %lld\n", GetReadPosForDebug());
    /* End of block. */
    /* 7 for padding4 and CRC32 + 12 for the next block header + 6 for the next
     * chunk header.
     */
    if (Preread(7 + 12 + 6) < 7 + 12 + 6) return SZ_ERROR_INPUT_EOF;
    DEBUGF("ALTELL %lld blockSizePad=%d\n", GetReadPosForDebug(), blockSizePad & 3);
    RINOK(IgnoreZeroBytes(blockSizePad & 3));  /* Ignore block padding. */
    DEBUGF("AMTELL %lld\n", GetReadPosForDebug());
    readCur += checksumSize;  /* Ignore CRC32, CRC64 etc. */
  }
  /* The .xz input file continues with the index, which we ignore from here. */
  return SZ_OK;
}

#ifdef __KERNEL32TINY__
void __cdecl mainCRTStartup(void) {
  hstdin = GetStdHandle(STD_INPUT_HANDLE);
  hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
  ExitProcess(DecompressXzOrLzma());
}
#else
int main(int argc, char **argv) {
  (void)argc; (void)argv;
#if defined(MSDOS) || defined(_WIN32)  /* Also MinGW. Good. */
  setmode(0, O_BINARY);
  setmode(1, O_BINARY);
#endif
#ifdef CONFIG_DEBUG
  global.allocCapacity = 0;
  global.dicSize = 0;
  {
    const SRes res = DecompressXzOrLzma();
    DEBUGF("END res=%d dicSize=%d allocCapacity=%d\n", res, global.dicSize, global.allocCapacity);
    free(global.dicf);  /* Pacify valgrind(1). */
    return res;
  }
#else
  return DecompressXzOrLzma();
#endif
}
#endif  /* Not __KERNEL32TINY__. */
