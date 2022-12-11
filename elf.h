#include <elf.h>
// elf
typedef struct sElf{
  union {
    U8* buf;               // mapped
    Elf64_Ehdr* ehdr;
  };
  Elf64_Shdr* shdr;        // array of section headers
  U32         shnum;               // number of sections
  Elf64_Shdr* sh_symtab;
  Elf64_Sym*  psym;
  U32         symnum;
  char*       str_sym;           //string for symbols

  // for this application, a unique global symbol, 
  Elf64_Sym*  unique;            
  
  
  // U32* sechashes;  // a table of name hashes matching section names
  S64 map_size; // for unmapping buf
} sElf;


typedef struct __attribute__ ((__packed__)) sRela {
  U64	r_offset;		/* Address */
  U32   r_type;
  U32   r_sym;
  U64   r_addend;
} sRela;
  

typedef struct sPiece {
  union {
    U8* addr;
    U64 pos;
  };
  union {
    U64 cnt;
    U64 size;
  };
} sPiece;

#define ELF_SHSTRTAB(pelf) (&((pelf)->shdr[(pelf)->ehdr->e_shstrndx]))
#define ELF_SYM_NAME(pelf,psym) ((pelf)->str_sym + (psym)->st_name)


sElf* elf_new();
sElf* elf_load(char* path);            // calls elf_new!
void  elf_delete(sElf* pelf);
U32 elf_find_section(sElf*pelf,char*name);

//void elf_syms(sElf* pelf);

// process symbol
typedef U32 (*pfElfSymProc)(Elf64_Sym*psym,U32 i);

U32 elf_process_symbols(sElf* pelf, pfElfSymProc proc);
U32 elf_resolve_symbols(sElf* pelf,pfresolver lookup);
//U32 elf_resolve_undefs(sElf* pelf,pfresolver lookup);

typedef void (*pfRelProc)(U32 addr,U32 type);
void elf_process_rel_section(sElf* pelf, Elf64_Shdr* shrel,pfRelProc proc);
void elf_apply_rels(sElf* pefl,pfRelProc proc);

S32 elf_find_global_symbol(sElf* pelf);
Elf64_Sym* elf_unique_global_symbol(sElf* pelf);
