#ifndef COSMOPOLITAN_LIBC_ELF_H_
#define COSMOPOLITAN_LIBC_ELF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "struct/ehdr.h"
#include "struct/phdr.h"
#include "struct/shdr.h"
#include "struct/sym.h"

/*───────────────────────────────────────────────────────────────────────────│─╗
│ cosmopolitan § executable linkable format                                ─╬─│┼
╚────────────────────────────────────────────────────────────────────────────│*/
/* clang-format off */

bool IsElfSymbolContent(const Elf64_Sym *);
bool IsElf64Binary(const Elf64_Ehdr *, size_t);
char *GetElfStringTable(const Elf64_Ehdr *, size_t, const char *);
Elf64_Sym *GetElfSymbols(const Elf64_Ehdr *, size_t, int, Elf64_Xword *);
Elf64_Shdr *GetElfSymbolTable(const Elf64_Ehdr *, size_t, int, Elf64_Xword *);
Elf64_Phdr *GetElfProgramHeaderAddress(const Elf64_Ehdr *, size_t, Elf64_Half);
Elf64_Shdr *GetElfSectionHeaderAddress(const Elf64_Ehdr *, size_t, Elf64_Half);
Elf64_Shdr *FindElfSectionByName(const Elf64_Ehdr *, size_t, char *, const char *);
char *GetElfString(const Elf64_Ehdr *, size_t, const char *, Elf64_Word);
void *GetElfSectionAddress(const Elf64_Ehdr *, size_t, const Elf64_Shdr *);
void *GetElfSegmentAddress(const Elf64_Ehdr *, size_t, const Elf64_Phdr *);
char *GetElfSectionName(const Elf64_Ehdr *, size_t, const Elf64_Shdr *);
char *GetElfSectionNameStringTable(const Elf64_Ehdr *, size_t);

#endif /* COSMOPOLITAN_LIBC_ELF_H_ */
