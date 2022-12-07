#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> //malloc
#include <sys/mman.h>


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

  
// a loop for processing all elf symbols, calling a function on each
// void proc(Elf64_sym*)
//
/*
void elf_process_symbols(sElf* pelf,pfElfSymProc proc){
  Elf64_Sym* psym = pelf->psym + pelf->symnum-1;
  for(U32 i=2;i<pelf->symnum;i++,psym--){
    (*proc)(psym);
  }
}
*/
void elf_process_symbols(sElf* pelf,pfElfSymProc proc){
  Elf64_Sym* psym = pelf->psym + pelf->symnum-1;
  for(U32 i=pelf->symnum-1;i>1;i--,psym--){
    (*proc)(psym,i);
  }
}

U32 elf_resolve_symbols(sElf* pelf,pfresolver lookup){
  // resolve ELF symbols to actual addresses
  U32 nUndef = 0;
  void proc(Elf64_Sym* psym,U32 i){
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
  }
  elf_process_symbols(pelf,proc);
  return nUndef;
}
// after initial resolution, if there are unresolved symbols,
// try to resolve them against other elf files here...
U32 elf_resolve_undefs(sElf* pelf,pfresolver lookup){
  // resolve ELF symbols to actual addresses
  U32 nUndef = 0;
  void proc(Elf64_Sym* psym,U32 i){
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


static void process_rel_section(sElf* pelf, Elf64_Shdr* shrel){
  U32 relnum = shrel->sh_size / shrel->sh_entsize;
  Elf64_Rela* prel = (Elf64_Rela*)(pelf->buf + shrel->sh_offset);
  // applying relocations to this section
  Elf64_Shdr* shto = &pelf->shdr[shrel->sh_info];
  //  printf("Applying relocations against %lX, %ld\n",shto->sh_addr,shto->sh_size);
  for(U32 i=0; i<relnum; i++,prel++){
    process_rel(pelf,prel,shto);
  }
}

void elf_apply_rels(sElf* pelf){
  //  printf("Rel sections:\n");
  Elf64_Shdr* shdr = pelf->shdr;  
  for(U32 i=0; i< pelf->shnum; i++,shdr++){
    if(SHT_RELA == shdr->sh_type){
      //  sechead_dump(pelf,i);
      process_rel_section(pelf,shdr);
    }
  }
}  
