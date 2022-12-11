#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy
#include <elf.h>
#include "global.h"
#include "elf.h"
#include "elfdump.h"
#include "util.h"
#include "seg.h"
#include "src.h"
#include "sym.h"

extern sSeg scode;
extern sSeg sdata;

U32 ing_elf_resolve(sElf*pelf){
    U32 unresolved = elf_resolve_symbols(pelf,find_global);
  //  if(unresolved)    printf("%d unresolved\n",unresolved);
  return unresolved;
}
/*----------------------------------------------------------------------------
   pkg_ingest_elf_sec   Ingest data from an elf section into segment
   
   Also, set the section address in the ELF section header (for resolution)
   return: section address in segment.
-----------------------------------------------------------------------------*/
U32 ing_elf_sec(sElf*pelf,U32 isec,sSeg*pseg){
  Elf64_Shdr* shdr = pelf->shdr + isec;
  U32 size = shdr->sh_size;
  U8* src;
  // PROGBITS sections contain data to ingest;
  // NOBITS sections are .bss, reserve cleared memory
  switch(shdr->sh_type){
  case SHT_PROGBITS: src = pelf->buf + shdr->sh_offset; break;
  case SHT_NOBITS: src = 0; break;
  default:
    printf("ingest_elf_sec: can't ingest section %d\n",isec);
    exit(1);
  }
  // actually put it into the specified segment
  U8* addr = seg_append(pseg,src,size);
  // set section address in ELF section image, for resolution
  shdr->sh_addr = (U64)addr;   
#ifdef DEBUG
  printf("ingesting %d bytes from %p\n",size,src);
#endif
  return (U32)(U64)addr;
}
/*----------------------------------------------------------------------------
 pkg_func_from_elf     ingest function sections from elf into code segment
                       create symb and add to package
                       get prototype via auxinfo
 return: symb
-----------------------------------------------------------------------------*/
sSym* ing_elf_func(sElf*pelf,Elf64_Sym* psym){
   //  seg_align(&scode,8); // our alignment requirement
  U32 codesec = psym->st_shndx;
  U32 faddr = ing_elf_sec(pelf,codesec,&scode);

  // there maybe rodata
  U32 strsec = elf_find_section(pelf,".rodata.str1.1");
  if(strsec) {
#ifdef DEBUG
    printf("pkg_func_from_elf: found .rodata.str1.1\n");
#endif
    ing_elf_sec(pelf,strsec,&scode);
  }
  U32 fsize = seg_pos(&scode) - faddr;
  //return (size << 32) | faddr;
  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  if(unresolved)
    return 0;
    // Now that all ELF symbols are resolved, process the relocations.
  // for every reference, mark bits in our rel table.
  void relproc(U32 p,U32 kind){
    #ifdef DEBUG
    printf("Code.Reference at: %08X, %d\n",p,kind);
    #endif
    seg_rel_mark(&scode,p,kind);
  }
  // codesec+1 may be its relocations... If not, no harm done.
  elf_process_rel_section(pelf,(pelf->shdr)+codesec+1,relproc);

  char* name = psym->st_name + pelf->str_sym;
  char* proto = aux_proto("sys/info.txt");
  U32 len;
  U32   src =     src_from_body(&len);
  sSym* ret = sym_new(name, faddr, fsize, src, proto);
  return ret;
}
/*----------------------------------------------------------------------------
 pkg_load_elf_data                Load a data object into package

A data object is more complicated, as it may exist in a single .bss section, 
or span multiple ELF sections.  GCC puts pointer object into a separate 
section with a rel section; strings go into a .rodata.str.1, etc.  A safe 
strategy is to first ingest the section with prime data first (we need it to 
take the base address), then all sections referenced by the symbols, 
and any associated rel section

return: symb of the ingested data.
----------------------------------------------------------------------------*/
/*siSymb* ing_elf_data(sElf* pelf,Elf64_Sym* psym){
  seg_align(&sdata,8); // align data seg for new data object
  // First, ingest the main data object and keep its address.
  U32 sec = psym->st_shndx;
  U32 addr = ing_elf_sec(pelf,sec,&sdata);
  char* name = psym->st_name + pelf->str_sym;
#ifdef DEBUG
  printf("ing_elf_data: for %s\n",name);
#endif 
  // Now, walk the symbols and ingest any referenced sections.  All will go
  // into the data segment, above the original data object.
  U32 proc(Elf64_Sym* psym,U32 i){
    U32 refsec = psym->st_shndx;
    if(refsec && (refsec < 0xFF00)){
      //      printf("processing symbol %d, refsec %d\n",i, refsec);
      Elf64_Shdr* psec = pelf->shdr+refsec;
      if( ! psec->sh_addr) {
	//	printf("Need section %d!\n",refsec);
	U32 addr = ing_elf_sec(pelf,refsec,&sdata);
	psec->sh_addr = addr;
      }
    }
    return 0; //all symbols
  }
  elf_process_symbols(pelf,proc);
  // Now, resolve elf symbols; should be possible now.
  pkg_elf_resolve(pkg,pelf);
  // Now, process all relocations, fixing up the references, and setting
  // the bits in the data segment's rel table.

  void relproc(U32 p,U32 kind){
    #ifdef DEBUG
    printf("data.Reference at: %08X, %d\n",p,kind);
    #endif
    seg_rel_mark(&sdata,p,kind);
  }
  elf_apply_rels(pelf,relproc);
  // ok, now calculate size and add to package
  U32 size = seg_pos(&sdata) - addr;
  return pkg_add(pkg,name,addr,size);
}
*/
sSym* ing_elf(sElf* pelf){
  Elf64_Sym* psym = elf_unique_global_symbol(pelf);
  if(!psym){
    printf("ing_elf: no unique global symbol\n");
    return 0;
  }

  switch(ELF64_ST_TYPE(psym->st_info)){
  case STT_FUNC:
    return ing_elf_func(pelf,psym);
  case STT_OBJECT:
    //    symb = pkg_load_elf_data(pkg,pelf,psym);
    printf("data load not implemented\n");
    return 0;
    break;
  default:
    printf("ing_elf: global symbol is not FUNC or OBJECT\n");
    return 0;
  }
  // done with the elf file!
  // symb may be 0 if there are unresolved references!
  // if it's good, load source.
}
