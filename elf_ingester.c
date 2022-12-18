#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy
#include <elf.h>
#include "global.h"
#include "elf.h"
#include "elfdump.h"
#include "util.h"
//#include "seg.h"
#include "aseg.h"
#include "src.h"
#include "sym.h"
#include "pkg.h"

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



U32 ing_elf_code_sec(sElf*pelf,U32 isec){
  Elf64_Shdr* shdr = pelf->shdr + isec;
  U8* src = elf_section_data(pelf,shdr);
  U32 size = shdr->sh_size;
  U32 addr = cseg_append(src,size);
  shdr->sh_addr = addr;   
#ifdef DEBUG
  printf("ingesting code %d bytes from %p\n",size,src);
#endif
  return addr;
}

/*----------------------------------------------------------------------------
 pkg_func_from_elf     ingest function sections from elf into code segment
                       create symb and add to package
                       get prototype via auxinfo
 return: symb
-----------------------------------------------------------------------------*/
U64 ing_elf_func(sElf*pelf){
  U32 end = *CFILL_ADDR;
  // there maybe rodata.  Bring it in first as we are going backwards.
  cseg_align8(); // align data seg for new data object
  U32 strsec = elf_find_section(pelf,".rodata.str1.1");
  if(strsec) {
#ifdef DEBUG
    printf("pkg_func_from_elf: found .rodata.str1.1\n");
#endif
    ing_elf_code_sec(pelf,strsec); //TODO
  }
  //now bring in the unique function  
  Elf64_Sym* psym = pelf->unique;
  U32 codesec = psym->st_shndx;
  U32 addr = ing_elf_code_sec(pelf,codesec); //TODO
  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  if(unresolved)
    return 0;
    // Now that all ELF symbols are resolved, process the relocations.
  // for every reference, mark bits in our rel table.
  void relproc(U32 p,U32 kind){
    #ifdef DEBUG
    printf("Code.Reference at: %08X, %d\n",p,kind);
    #endif
    rel_mark(p,kind);
  }
  // codesec+1 may be its relocations... If not, no harm done.
  elf_process_rel_section(pelf,(pelf->shdr)+codesec+1,relproc);
  
  cseg_align8();
  return (((U64)(end-addr))<<32) | addr;
  
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

U32 ing_elf_data_sec(sElf*pelf,U32 isec){
  Elf64_Shdr* shdr = pelf->shdr + isec;
  U32 size = shdr->sh_size;
  U8* src = elf_section_data(pelf,shdr);
  U32 addr = dseg_append(src,size);
  shdr->sh_addr = (U64)addr;   
#ifdef DEBUG
  printf("ingesting data %d bytes from %p\n",size,src);
#endif
  return addr;
}

U64 ing_elf_data(sElf* pelf){
  Elf64_Sym* psym = pelf->unique;
  // First, ingest the main data object and keep its address.
  dseg_align8(); // align data seg for new data object
  U32 sec = psym->st_shndx;
  U32 addr = ing_elf_data_sec(pelf,sec);
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
	U32 addr = ing_elf_data_sec(pelf,refsec);
	psec->sh_addr = addr;
      }
    }
    return 0; //all symbols
  }
  elf_process_symbols(pelf,proc);
  // Now, resolve elf symbols; should be possible now.
  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  if(unresolved)
    return 0;
  U64 size = DFILL - addr;
  
  // Now, process all relocations, fixing up the references, and setting
  // the bits in the data segment's rel table.
  void relproc(U32 p,U32 kind){
    #ifdef DEBUG
    printf("data.Reference at: %08X, %d\n",p,kind);
    #endif
    rel_mark(p,kind);
  }
  elf_apply_rels(pelf,relproc);
  dseg_align8();
  return (size<<32) | addr;

}




// create a sSym, with artifact bounds, source and proto set...
sSym* ing_elf_sym(sElf* pelf,U64 bounds){
  Elf64_Sym* psym=pelf->unique;
  char* name = psym->st_name + pelf->str_sym;
  U32 len;
  U32   src = src_from_body(&len);
  char* proto = 0;
  if(ELF64_ST_TYPE(psym->st_info) == STT_FUNC){
    proto = aux_proto();
  }
  sSym* ret = sym_new(name, bounds&0xFFFFFFFF, bounds>>32, src, proto);
  free(proto);
  return ret;
}

sSym* ing_elf(sElf* pelf){
  U64 bounds;
  switch(ELF64_ST_TYPE(pelf->unique->st_info)){
  case STT_FUNC:
    bounds = ing_elf_func(pelf);
    break;
  case STT_OBJECT:
    bounds = ing_elf_data(pelf);
    break;
  default:
    return 0;
  }
  return ing_elf_sym(pelf,bounds);
}
