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




/* -------------------------------------------------------------
   elf_load   Load an ELF object file (via mapping)
 -------------------------------------------------------------*/
S64 elf_load(sElf* pelf,char* path){
  S64 len = file_map((void**)&pelf->buf,path,PROT_READ|PROT_WRITE);
  if(len<0) file_map_error_msg(len,path,1);
  pelf->map_size = len; // for unmapping
  //  printf("loaded elf %ld\n",len);  

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

  // build the hashtable
  //  pelf->hashes = 0;//
  return len;
}
sElf* elf_new(){
  sElf* p = (sElf*)malloc(sizeof(sElf));
  p->buf = 0;
  return p;
}
void elf_delete(sElf* pelf){
  if(munmap(pelf->buf,pelf->map_size)){
    fprintf(stderr,"Error unmapping an elf file\n");
    exit(1);
  }
  // if we allocated a hashlist, free it.
  //  if(pelf->hashes)
  //  free(pelf->hashes);
  
  free(pelf);
}

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
// Process each symbol by calling proc.  If proc returns
// non-zero, exit with the index that bounced us.  If all
// symbols are processed, return 1
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
 */
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


U32 elf_resolve_symbols(sElf* pelf,pfresolver lookup){
  // resolve ELF symbols to actual addresses
  U32 nUndef = 0;
  U32 proc(Elf64_Sym* psym,U32 i){
    U32 shi = psym->st_shndx; // get section we are referring to
    if(shi){ //for defined symbols, add section base
      psym->st_value += pelf->shdr[shi].sh_addr;
    } else { //for undefined symbols, find!
      psym->st_value = (*lookup)(pelf->str_sym + psym->st_name);
      // count undefined symbols
      if(!psym->st_value)
	nUndef++;
    }
    //          sym_dump(pelf,psym);
    return 0;
  }
  elf_process_symbols(pelf,proc);
  return nUndef;
}
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
void process_rel(sElf* pelf, Elf64_Rela* prel, Elf64_Shdr* shto){
  U64 base = shto->sh_addr; // base address of image being fixed-up
  U64 p = base + prel->r_offset;
  Elf64_Sym* psym = &pelf->psym[ELF64_R_SYM(prel->r_info)];
  U64 s = psym->st_value;
  U64 a = prel->r_addend;
  switch(ELF64_R_TYPE(prel->r_info)){
  case R_X86_64_PC32:  //data access
  case R_X86_64_PLT32: //calls
    U32 fixup = (U32)(s+a-p);
    *((U32*)p) = fixup;
    //printf("Fixup: P:%lx A:%ld S:%lx S+A-P: %08x\n",p,a,s,fixup);        
    break;
  case R_X86_64_64: //data, pointer
    *((U64*)p) = s + a;
    //printf("Fixup: P:%lx A:%ld S:%lx S+A: %016lx\n",p,a,s,s+a);
    break;
  default:
    printf("Unknown relocation type\n");
    rel_dump(pelf,prel);
    break;
  }
}


void elf_process_rel_section(sElf* pelf, Elf64_Shdr* shrel){
  printf("elf_process_rel_section: processing section %p\n",shrel);
  if(SHT_RELA == shrel->sh_type){
    U32 relnum = shrel->sh_size / shrel->sh_entsize;
    Elf64_Rela* prel = (Elf64_Rela*)(pelf->buf + shrel->sh_offset);
    // applying relocations to this section
    Elf64_Shdr* shto = &pelf->shdr[shrel->sh_info];
    printf("Applying relocations against %lX, %ld\n",shto->sh_addr,shto->sh_size);
    for(U32 i=0; i<relnum; i++,prel++){
      process_rel(pelf,prel,shto);
    }
  } else {
    printf("elf_process_rel_section: no relocations\n");
  }
}

void elf_apply_rels(sElf* pelf){
  //  printf("Rel sections:\n");
  Elf64_Shdr* shdr = pelf->shdr;  
  for(U32 i=0; i< pelf->shnum; i++,shdr++){
    if(SHT_RELA == shdr->sh_type){
      //  sechead_dump(pelf,i);
      elf_process_rel_section(pelf,shdr);
    }
  }
}  

