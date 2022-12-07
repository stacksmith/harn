/* Load a single self-contained ELF object file and link */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "global.h"
#include "elf.h"
#include "util.h"
static char* str_SHT[]= {
  "NULL    ",            /* Section header table entry unused */
  "PROGBITS",/* Program data */
  "SYMTAB  ",	/* Symbol data */
  "STRTAB  ",		/* String table */
  "RELA    ",	  		/* Relocation entries with addends */
  "HASH    "	  ,		/* Symbol hash table */
  "DYNAMIC "	  ,		/* Dynamic linking information */
  "NOTE    "	  ,		/* Notes */
  "NOBITS  "	  ,		/* Program space with no data (bss) */
  "REL     "	,		/* Relocation entries, no addends */
  "SHLIB   "	  ,		/* Reserved */
  "DYNSYM  "	  ,		/* Dynamic linker symbol table */
  "INIT_ARRAY   "     ,	/* Array of constructors */
  "FINI_ARRAY   ",		/* Array of destructors */
  "PREINIT_ARRAY",		/* Array of pre-constructors */
  "GROUP        ",		/* Section group */
  "SYMTAB_SHNDX "		/* Extended section indices */
};
#include <ctype.h>

char* get_str(sElf* pelf, Elf64_Shdr* sh_str, int str_idx){
  if(str_idx){
    if(str_idx > sh_str->sh_size-1){
      fprintf(stderr,"sym_str: string index too high\n");
      exit(1);
    } 
    return (char*)(pelf->buf + sh_str->sh_offset + str_idx);
  }else{
    return "-0-";
  }
}

char* section_name(sElf* pelf,U32 idx){
  Elf64_Shdr* sh_shstrtab = ELF_SHSTRTAB(pelf);
  if(idx<SHN_LORESERVE){
    if(!idx) return "-UNDEF-";
    return get_str(pelf,sh_shstrtab,pelf->shdr[idx].sh_name);
  } else {
    switch(idx){
    case SHN_ABS: return "-ABS-";
    case SHN_COMMON: return "-COMON-";
    case SHN_XINDEX: return "-XINDEX-";
    default: return "-UNK-";
    }
  }
}

void sechead_dump(sElf* pelf, U32 sh_idx){
  Elf64_Shdr* psh = &pelf->shdr[sh_idx];
  int sht = psh->sh_type;
  printf("%08lx %4ld ",psh->sh_addr,psh->sh_size);
  if(sht<SHT_NUM)
    printf("%s ",str_SHT[sht]);
  else
    printf("shtype %X ",sht);
  printf("%04lX ",psh->sh_flags);
  printf("%5ld ",psh->sh_offset);
  printf("%02ld",psh->sh_addralign);
  printf(" %s ",section_name(pelf,sh_idx));
  printf("\n");
}
void secheads_dump(sElf* pelf){
    printf(" #  Address  size sh_type  flag off   al  name\n");
  // sections
  for(int i=0;i<pelf->ehdr->e_shnum;i++){
    printf("%02d: ",i);
    sechead_dump(pelf,i);
  }
}
//=========================================================================
// Symbol table
//
char* sym_info_bind_str(U32 n){
  switch(ELF64_ST_BIND(n)){
  case 0: return "LOCAL";
  case 1: return "GLOBL";
  case 2: return "WEAK ";
  case 10: return "GNU_UNIQUE";
  default: return "BAD_BIND!";
  }
}

char* sym_info_type_str(U32 n){
  switch(ELF64_ST_TYPE(n)){
  case 0: return "NOTYPE";
  case 1: return "OBJECT";
  case 2: return "FUNC  ";
  case 3: return "SECTN ";
  case 4: return "FILE  ";
  case 5: return "COMMON";
  case 6: return "TLS   ";
  case 10: return "GNU_IFUNC";
  default: return "BAD_STYPE!";
  }
}
char* sym_other_vis_str(U32 n){
  switch(ELF64_ST_VISIBILITY(n)){
  case 0: return "vDEFAULT ";
  case 1: return "vINTERNAL";
  case 2: return "vHIDDEN  ";
  case 3: return "vPROTECT ";
  }
  return 0;
}

void sym_dump(sElf* pelf,Elf64_Sym* psym){
  //  char* name = get_str(pelf, sh_str, psym->st_name);
  char* name = pelf->str_sym + psym->st_name;
  printf("%08lX %4lX ",psym->st_value,psym->st_size);
  printf("%s %s %s ",
	 sym_info_type_str(psym->st_info),
	 sym_info_bind_str(psym->st_info),
	 sym_other_vis_str(psym->st_other));
  char* secname = section_name(pelf, psym->st_shndx);
  int pad = 20-strlen(secname);
  if(pad<0) pad = 0;
  printf("%s ",secname);
  for(int i=0;i<pad;i++) printf(" ");
  printf("%s\n",name);
  
}


void symtab_dump(sElf*pelf){
  printf("Symbol table at %ld, size %ld\n",pelf->sh_symtab->sh_offset,
	 pelf->sh_symtab->sh_size);
  // compute the total count of symbols
  int sym_cnt = (int)pelf->sh_symtab->sh_size / pelf->sh_symtab->sh_entsize;
  printf("entries: %d\n",sym_cnt);
  printf("Corresponding string table section %d\n",pelf->sh_symtab->sh_link);

  Elf64_Sym* syms = pelf->psym;
  for(int i=0;i<pelf->symnum;i++){
    printf("%3d: ",i);
    sym_dump(pelf,&syms[i]);
  }
}
char* reltype_str(U32 i){
  switch(i){
  case 0: return "NONE";
  case 1: return "64_64";
  case 2: return "PC32";
  case 3: return "GOT32";
  case 4: return "PLT32";
  default:
    fprintf(stderr,"reltype_str: implement rel type %d\n",i);
    exit(1);
  }
  return 0;
}


void rel_dump(sElf*pelf,Elf64_Rela* p){
  // string table for symbols
  Elf64_Shdr* sh_str = &pelf->shdr[pelf->sh_symtab->sh_link];
  Elf64_Sym* syms = pelf->psym;
  char* symname = get_str(pelf,sh_str,syms[ELF64_R_SYM(p->r_info)].st_name);
  printf("%08lx %s %02ld , type:%s\n",
	 p->r_offset,
	 symname,
 	 p->r_addend,

	 //	 ELF64_R_SYM(p->r_info),
	 reltype_str((U32)ELF64_R_TYPE(p->r_info))
	 );
}
void reltab_dump(sElf* pelf,U32 rtab_isec){
  Elf64_Shdr* sh_reltab = &pelf->shdr[rtab_isec];
  Elf64_Rela*  reltab = (Elf64_Rela*) (pelf->buf + sh_reltab->sh_offset);
  U32 cnt = sh_reltab->sh_size / sh_reltab->sh_entsize;
  U32 i_sym_sh = sh_reltab->sh_link;
  U32 i_bin_sh = sh_reltab->sh_info;
  printf("%d relocations %ld %ld\n",cnt,
	 sh_reltab->sh_size, sh_reltab->sh_entsize
	 );

  printf("Assoc. symtab idx %d; apply to section %d\n",i_sym_sh,i_bin_sh);
  for(int i=0;i<cnt;i++){
    rel_dump(pelf,&reltab[i]);
  }
}

void elf_dump(sElf* pelf){
  secheads_dump(pelf);
  symtab_dump(pelf);
 

}

