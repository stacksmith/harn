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
sDataSize ing_elf_func(sElf*pelf){
  cseg_align8(); // align data seg for new data object
  //now bring in the unique function  
  Elf64_Sym* psym = pelf->unique;
  U32 codesec = psym->st_shndx;
  U32 addr = ing_elf_code_sec(pelf,codesec); 
  // there maybe rodata.  Bring it in.
  U32 strsec = elf_find_section(pelf,".rodata.str1.1");
  if(strsec) {
#ifdef DEBUG
    printf("pkg_func_from_elf: found .rodata.str1.1\n");
#endif
    ing_elf_code_sec(pelf,strsec); //TODO
  }
  U32 end = CFILL;
  cseg_align8();  

  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  if(unresolved)
    return (sDataSize){val:0};
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
  
  return (sDataSize){data:addr,size:end-addr};
  
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

sDataSize ing_elf_data(sElf* pelf){
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
    return 0;
  }
  elf_process_symbols(pelf,proc);
  // Now, resolve elf symbols; should be possible now.
  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  if(unresolved)
    return (sDataSize){val:0};
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
  return (sDataSize){data:addr,size};

}

/*----------------------------------------------------------------------------
  ing_elf                              ingest an elf file, return a new
                                       unlinked sSym

This is a high-level routine which dispatches to ELF function or ELF data 
ingestor which pulls code or data into a proper segment, returning bounds.
We then create an unlinked symbol in the meta directory



------------------------------------------------------------------------------*/
extern sSym*sym_wrap(char* name,U32 art, U32 size);

sSym* ing_elf(sElf* pelf){
  sDataSize bounds;
  switch(ELF64_ST_TYPE(pelf->unique->st_info)){
  case STT_FUNC:
    bounds = ing_elf_func(pelf);
    break;
  case STT_OBJECT:
    bounds = ing_elf_data(pelf);
    break;
  default:
    elf_delete(pelf);
    return 0;
  }
  char* name = pelf->unique->st_name + pelf->str_sym;
  sSym* sym = sym_for_artifact(name, bounds.data, bounds.size);
  elf_delete(pelf);
  return sym;
}
