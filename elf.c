#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> //malloc
#include <sys/mman.h>
#include <string.h>


#include "global.h"
#include "util.h"
#include "elf.h"
#include "elfdump.h"


sElf* elf_new(){
  sElf* p = (sElf*)malloc(sizeof(sElf));
  p->buf = 0;
  return p;
}
/* -------------------------------------------------------------
   elf_load   Load an ELF object file (via mapping)

returns: sElf*.  Cannot fail
 -------------------------------------------------------------*/
sElf* elf_load(char* path){
  sElf* pelf = elf_new();
  pelf->buf = filebuf_malloc(path);
  // section header array
  pelf->shdr = (Elf64_Shdr*)(pelf->buf + pelf->ehdr->e_shoff);
  pelf->shnum = pelf->ehdr->e_shnum;
  // scan sections backwards, looking for symbol table
  for(Elf64_Shdr* p = &pelf->shdr[pelf->ehdr->e_shnum-1];
      p > pelf->shdr; p--){
    if(p->sh_type == SHT_SYMTAB){
      pelf->sh_symtab = p;
      break;
    }
  }
  pelf->psym = (Elf64_Sym*)(pelf->buf + pelf->sh_symtab->sh_offset);
  pelf->symnum = pelf->sh_symtab->sh_size / pelf->sh_symtab->sh_entsize;
  // string table associated with symbols
  pelf->str_sym = pelf->buf + pelf->shdr[pelf->sh_symtab->sh_link].sh_offset;

  // for this application, find the unique symbol
  pelf->unique = elf_unique_global_symbol(pelf);
  pelf->ing_start = 0;
  return pelf;
}
/*-------------------------------------------------------------
   elf_delete    Free allocated memory
 
 -------------------------------------------------------------*/
void elf_delete(sElf* pelf){
  //  printf("Unmapping %p; buf %p, size %ld\n",pelf,pelf->buf,pelf->map_size);
  printf("deleted %p\n",pelf);
  //  fclose(pelf->f);
  free(pelf->buf);
  free(pelf);
}
/*-------------------------------------------------------------
 elf_find_section             find a names ELF section

Regurns: index of the section or 0
  -------------------------------------------------------------*/
U32 elf_find_section(sElf*pelf,char*name){
  char* strtab = pelf->buf + (ELF_SHSTRTAB(pelf))->sh_offset;
  U32 i;
  Elf64_Shdr* shdr = (pelf->shdr) + 1;

  for(i=1; i< pelf->shnum; i++,shdr++){
    if(!strcmp(name, (strtab + shdr->sh_name)))
      return i;
  }
  return 0;
}

U8* elf_section_data(sElf* pelf, Elf64_Shdr* shdr){
  // PROGBITS sections contain data to ingest;
  // NOBITS sections are .bss, reserve cleared memory
  switch(shdr->sh_type){
  case SHT_PROGBITS: return pelf->buf + shdr->sh_offset;
  case SHT_NOBITS:   return 0;
  default:
    printf("elf_section_data: can't ingest section %s\n",
	   pelf->buf + (ELF_SHSTRTAB(pelf))->sh_offset + shdr->sh_name);
	   
    exit(1);
  }
  return 0;
}
/*-------------------------------------------------------------
  elf_process_symbols     Call a proc on each symbol.

  Starting with the last symbol, go down to 2.
  If proc returns non-zero, exit with the index that bounced us.
  If all symbols are processed, return 0.
  -------------------------------------------------------------*/
U32 elf_process_symbols(sElf* pelf,pfElfSymProc proc){
  Elf64_Sym* psym = pelf->psym + pelf->symnum-1;
  for(U32 i=pelf->symnum-1;i>1;i--,psym--){
    if((*proc)(psym,i))
      return i; // return the symbol that bounced us
  }
  return 0; //fully processed all
}

/* 
  Count the number of func-symbols in this ELF
TODO: make more generic searches

U32 elf_func_count(sElf* pelf){
  U32 cnt = 0;
  U32 proc(Elf64_Sym* psym,U32 i){
    if( (STB_GLOBAL == ELF64_ST_BIND(psym->st_info)) &&
       (STT_FUNC == ELF64_ST_TYPE(psym->st_info)) ) 
      cnt++;
    return 0; // never bounce
  }
  elf_process_symbols(pelf,&proc);
  return cnt;
}
U32 elf_data_count(sElf* pelf){
  U32 cnt = 0;
  U32 proc(Elf64_Sym* psym,U32 i){
    if( (STB_GLOBAL == ELF64_ST_BIND(psym->st_info)) &&
       (STT_OBJECT == ELF64_ST_TYPE(psym->st_info)) ) 
      cnt++;
    return 0; // never bounce
  }
  elf_process_symbols(pelf,&proc);
  return cnt;
}

U32 elf_func_find(sElf* pelf){;
  U32 proc(Elf64_Sym* psym,U32 i){
    if( (STB_GLOBAL == ELF64_ST_BIND(psym->st_info)) &&
       (STT_FUNC == ELF64_ST_TYPE(psym->st_info)) )
      return 1;
    return 0;
  }
  return elf_process_symbols(pelf,&proc);
}

U32 elf_data_find(sElf* pelf){;
  U32 proc(Elf64_Sym* psym,U32 i){
    if( (STB_GLOBAL == ELF64_ST_BIND(psym->st_info)) &&
       (STT_OBJECT == ELF64_ST_TYPE(psym->st_info)) )
      return 1;
    return 0;
  }
  return elf_process_symbols(pelf,&proc);
}
*/
/*-------------------------------------------------------------
elf_find_global_symbol           Scan all symbols for STB_GLOBAL

return: 0 for no globals; 1 for 1 global; -1 for many globals.
 -------------------------------------------------------------*/
S32 elf_find_global_symbol(sElf* pelf){
  S32 ret = 0;
  U32 proc(Elf64_Sym* psym,U32 i){
    // if symbol is defined here, and is GLOBAL!
    if(psym->st_shndx && (STB_GLOBAL == ELF64_ST_BIND(psym->st_info)))
      ret = ret ? -1 : i ; // first time, set it to i; then, -1
    return 0; // scan all symbols
  }
  elf_process_symbols(pelf,&proc);
  return ret;
}
/*-------------------------------------------------------------
elf_unique_global_symbol      Locate the hopefully 1 global

return elf symbol, or 0 if not exactly one.
=-------------------------------------------------------------*/

Elf64_Sym* elf_unique_global_symbol(sElf* pelf){
  S32 i = elf_find_global_symbol(pelf);

  switch(i){
  case 0:
    printf("elf_unique_global_symbol: no global symbols found\n");
    return 0;
  case -1:
    printf("elf_unique_global_symbol: many global symbols\n");
    return 0;
  }
  return pelf->psym+i;  
}
/*-------------------------------------------------------------
  elf_resolve_symbols           walk elf symbols and set addr.

return: number of unresolved symbols or 0 on success
-------------------------------------------------------------*/
U32 elf_resolve_symbols(sElf* pelf,pfresolver lookup){
  // resolve ELF symbols to actual addresses
  U32 nUndef = 0;
  // For every symbol call this proc, which, for local symbols,
  // updates the ELF st_value to point at our segs, and for
  // external references, looks up harn symbols.:
  U32 proc(Elf64_Sym* psym,U32 i){
    U32 shi = psym->st_shndx; // get section we are referring to
    if(shi){ //for defined symbols, add section base
      psym->st_value += pelf->shdr[shi].sh_addr;
    } else { //for undefined symbols, find!
      psym->st_value = (*lookup)(pelf->str_sym + psym->st_name);
      if(!psym->st_value) {
	nUndef++; // keep a count of undefined symbols
	printf("Unresolved: %s\n",ELF_SYM_NAME(pelf,psym));
      }
    }
    return 0; // process all symbols
  }
  elf_process_symbols(pelf,proc);
  return nUndef; // return count of undefined symbols!
}
/* not currently used

// after initial resolution, if there are unresolved symbols,
// try to resolve them against other elf files here...
U32 elf_resolve_undefs(sElf* pelf,pfresolver lookup){
  // resolve ELF symbols to actual addresses
  U32 nUndef = 0;
  U32 proc(Elf64_Sym* psym,U32 i){
    // only process NOTYPE symbols with 0 value as undefs
    if((STT_NOTYPE == ELF64_ST_TYPE(psym->st_info)) &&
       (!psym->st_value)) {
      psym->st_value = (*lookup)(pelf->str_sym + psym->st_name);
      // count undefined symbols
      if(!psym->st_value) {
	nUndef++;
	printf("undef: %s\n",(pelf->str_sym + psym->st_name));
      }
    }
    //          sym_dump(pelf,psym);
    return 0;
  }
  elf_process_symbols(pelf,proc);
  return nUndef;
}
*/

/* proces_rel    Apply a relocation to a reference located in section shto,
                 as described by prel.

Currently handling only two kinds of relocations: 32-bit PC-relative offsets 
as found in x86-64 code, and 64-bit absolute pointers.

Note: we eliminate GOTs and PLTs, and use direct references.

   

 */

/*-------------------------------------------------------------
elf_process_rel      process a single ELF relocation, fixing up
                     data in segs and calling the proc.
-------------------------------------------------------------*/
U32 elf_process_rel(sElf* pelf, Elf64_Rela* prel, Elf64_Shdr* shto,
		     pfRelProc proc){
  U64 base = shto->sh_addr; // base address of image being fixed-up
  U64 p = base + prel->r_offset; // ref site
  Elf64_Sym* psym = &pelf->psym[ELF64_R_SYM(prel->r_info)];
  // symbol must be previously resolved.
  U64 s = psym->st_value;        // symbol address
  if(!s) {
    // One reason this happens is trying to use a static var in a func;
    if( (3 == ELF64_ST_TYPE(psym->st_info)) &&
	(!strcmp(".bss",section_name(pelf, psym->st_shndx))) ){
      printf("\
Your function contains uninitialized static data in the .bss section.\n\
Harn does not allow functions to create inaccessible data; if you need\n\
the equivalent of a static, simply create a data object\n");
      return 0xFFFF000;
    }

    printf("process_rel: symbol %ld is not initialized!\n",
	   ELF64_R_SYM(prel->r_info));
    sym_dump(pelf,psym);
    exit(1);
  }
  U64 a = prel->r_addend;
    
  switch(ELF64_R_TYPE(prel->r_info)){
  case R_X86_64_PC32:  //data access
  case R_X86_64_PLT32: //calls
    U32 fixup = (U32)(s+a-p);
    *((U32*)p) = fixup;
    (*proc)((U32)p,1,s+a);
#ifdef DEBUG
    printf("Fixup: P:%lx A:%ld S:%lx S+A-P: %08x\n",p,a,s,fixup);        
#endif
    break;
  case R_X86_64_64: //data, pointer
    *((U64*)p) = s + a;
    (*proc)((U32)p,3,s+a); 
#ifdef DEBUG
    printf("Fixup: P:%lx A:%ld S:%lx S+A: %016lx\n",p,a,s,s+a);
#endif
    break;
  default:
    printf("Unknown relocation type\n");
    rel_dump(pelf,prel);
    exit(1);
  }
  return 0;
}

/*----------------------------------------------------------------------------
  elf_process_rel_section       if the specified section is a RELA, process
                                each relocation (see above).
                                
----------------------------------------------------------------------------*/
U32 elf_process_rel_section(sElf* pelf, Elf64_Shdr* shrel,pfRelProc proc){
  if(SHT_RELA == shrel->sh_type){ // redundant, but pkg does not check!
    U32 relnum = shrel->sh_size / shrel->sh_entsize;
#ifdef DEBUG
    printf("elf_process_rel_section %p with %d rels\n",shrel,relnum);
#endif
    Elf64_Rela* prel = (Elf64_Rela*)(pelf->buf + shrel->sh_offset);
    // applying relocations to this section
    Elf64_Shdr* shto = &pelf->shdr[shrel->sh_info];
#ifdef DEBUG
    printf("Applying relocations against %lX, %ld\n",shto->sh_addr,shto->sh_size);
#endif
    for(U32 i=0; i<relnum; i++,prel++){
      U32 ret = elf_process_rel(pelf,prel,shto,proc);
      if(ret)
	return ret;
    }
  } else {
#ifdef DEBUG
    printf("elf_process_rel_section: no relocations\n");
#endif
  }
  return 0;
}
/*----------------------------------------------------------------------------
elf_apply_rels                   process each rel section in the file.
----------------------------------------------------------------------------*/
void elf_apply_rels(sElf* pelf,pfRelProc proc){
  //  printf("Rel sections:\n");
  Elf64_Shdr* shdr = pelf->shdr;  
  for(U32 i=0; i< pelf->shnum; i++,shdr++){
    if(SHT_RELA == shdr->sh_type){ 
      elf_process_rel_section(pelf,shdr,proc);
    }
  }
}  

