/*---------------------------------------------------------------------------
  ELF_INGESTER

This subsystem is responsible for ingesting compiled unit from an ELF object
file.

Used primarily by the REPL 'cc' command which calls 'ingest_elf(name)', and
the REPL expression runner, calls 'ingets_run(name,p)'

This is a fairly high level subsystem, capable of creating new symbols...

  ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy
#include <elf.h>
#include "global.h"
#include "elf.h"
#include "elfdump.h"
#include "util.h"
#include "seg_art.h"
#include "seg_meta.h"
#include "src.h"
#include "sym.h"
#include "pkg.h"

// in case of error,  
U32 elf_error(sElf* pelf,char* msg1){
  printf("ing_elf: %s\n",msg1);
  return 0x100; //error number?
}
/*
U32 ing_elf_resolve(sElf*pelf){
  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  //  if(unresolved)    printf("%d unresolved\n",unresolved);
  return unresolved;
}
*/
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
  shdr->sh_addr = addr;               // set section address for elf
  
#ifdef DEBUG
  printf("ingesting code %d bytes from %p\n",size,src);
#endif
  return addr;
}

/*----------------------------------------------------------------------------
 pkg_func_from_elf     ingest function sections from elf into code segment
                       create symb and add to package
                       get prototype via auxinfo
 return: 0, or error
-----------------------------------------------------------------------------*/

U32 ing_elf_func(sElf*pelf){
  pelf->ing_start = CFILL;
  //now bring in the unique function
  Elf64_Sym* psym = pelf->unique;
  U32 codesec = psym->st_shndx; // from the func symbol, get its section
  ing_elf_code_sec(pelf,codesec);  // ingest section! get addr.
  // there maybe rodata.  Bring it in too
  U32 strsec = elf_find_section(pelf,".rodata.str1.1");
  if(strsec) {
#ifdef DEBUG
    printf("pkg_func_from_elf: found .rodata.str1.1\n");
#endif
    ing_elf_code_sec(pelf,strsec); //TODO
  }
  // set ingested size in sElf


  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  if(unresolved)
    return unresolved; // >0 is error!
  
  // Now that all ELF symbols are resolved, process the relocations.
  // Mark all absolute relocations; offsets inside unit are not marked.
  U32 start = pelf->ing_start;
  U32 end = CFILL;
  void relproc(U32 p,U32 kind,U64 target){
    #ifdef DEBUG
    printf("Code.Reference at: %08X, %d; target %08lX\n",p,kind,target);
    #endif
    if(kind == 1){
      if ((target < start) || (target > end))
	rel_mark(p,kind);
    } else {
      // all absolute relocations are marked
      rel_mark(p,kind);
    }
    
  }
  // codesec+1 may be its relocations... If not, no harm done.
  // normally returns 0, but static data will cause error.
  return elf_process_rel_section(pelf,(pelf->shdr)+codesec+1,relproc);
    
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

U32 ing_elf_data(sElf* pelf){
  pelf->ing_start = DFILL;
  Elf64_Sym* psym = pelf->unique;
  // First, ingest the main data object and keep its address.
  dseg_align8(); // align data seg for new data object
  U32 sec = psym->st_shndx;
  ing_elf_data_sec(pelf,sec);

#ifdef DEBUG
  printf("ing_elf_data: for %s\n",pelf->unique->st_name + pelf->str_sym);
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
  //done ingesting...  
 
  
  // Now, resolve elf symbols; should be possible now.
  U32 unresolved = elf_resolve_symbols(pelf,find_global);
  if(unresolved)
    return unresolved;
  // Now, process all relocations, fixing up the references, and setting
  // the bits in the data segment's rel table for out-of-unit fixups.
  U32 start = pelf->ing_start;
  U32 end = CFILL;
 
  void relproc(U32 p,U32 kind,U64 target){
#ifdef DEBUG
    printf("data.Reference at: %08X, %d %08lX\n",p,kind,target);
#endif
    if(kind==1) {// relative relocations? TODO: this never happens in data
      if ((target < start) || (target > end))
	rel_mark(p,kind);
    } else
      // all absolute relocations are marked.
      rel_mark(p,kind);
  };
  elf_apply_rels(pelf,relproc);
  
  return 0;

}

/*----------------------------------------------------------------------------
  ing_elf                              ingest an elf file, return a new
                                       unlinked sSym

This is a high-level routine which dispatches to ELF function or ELF data 
ingestor which pulls code or data into a proper segment, returning bounds.
We then create an unlinked symbol in the meta directory



------------------------------------------------------------------------------*/

// return 0 or error
U32 ing_elf(sElf* pelf,U32 need_function){
  cseg_align8();
  dseg_align8();
  
  if(!pelf->unique)
    return elf_error(pelf,"ELF file has no unique global symbol");

  //char* name = pelf->unique->st_name + pelf->str_sym;
  U32 unresolved = 0;
  
  switch(ELF64_ST_TYPE(pelf->unique->st_info)){
  case STT_FUNC:
    unresolved =ing_elf_func(pelf);
    break;
  case STT_OBJECT:
    if(need_function)
      return elf_error(pelf,"Need a function!");
    unresolved = ing_elf_data(pelf);
    break;
  default:
    return elf_error(pelf," not func or object...");
  }
  return unresolved; // hopefully, 0...
}

/*----------------------------------------------------------------------------
  rebuild                              invoke a shell to rebuild a file and
                                       generate an elf file...  And load it

 
------------------------------------------------------------------------------*/

sElf* ing_rebuild(char* name){
  pkgs_dump_protos();
  char buf[256];
  sprintf(buf,"cd sys; ./build.sh %s",name);
   //puts(buf);
  int ret = system(buf);
    if(ret){
    printf("OS shellout to compiler failed! Build Error %d\n",ret);
    return 0;
  }
  sprintf(buf,"sys/%s.o",name);

  sElf* pelf = elf_load(buf);
  
  return pelf;
}

/*----------------------------------------------------------------------------
  ingest_elf                           ingest an elf file, and return
                                       artifact address.

Handle all errors, wiping the system clean if at all possible (which it
should be...)

------------------------------------------------------------------------------*/

sElf* ingest_elf_prim(char* name){
  sElf* pelf = ing_rebuild(name);
  U32   err;
  if(pelf) {
    err  = ing_elf(pelf,0);    //0 is good; otherwise error
    if(err){
      puts("ingets_elf_prim: err!\n");
      U32 addr = pelf->ing_start;  //if addr is set, possibly ingested
      aseg_wipe(addr); //works if it's 0, same as fill, or whatever...
      addr = 0;
      elf_delete(pelf);
      return 0;
    }
  }
  // return a pelf with ing_start, or 0 if could not build or ingest.
  return pelf;
}

sSym* ingest_elf(char* name){
  sElf* pelf = ingest_elf_prim(name);
  sSym* sym = 0;
  if(pelf){
    U32 addr = pelf->ing_start;
    char* name = pelf->unique->st_name + pelf->str_sym;
    sym = sym_for_artifact(name, addr);
    elf_delete(pelf);
  }
  return sym;
}
typedef void (*fpreplfun)();


U64 ingest_run(char* name){
  U64 ret = 0;
  cseg_align8();    // TODO: redundant... make up your mind...
  REL_FLAG = 0;     // turn off relocation, as we shall erase this ASAP
  sElf* pelf = ingest_elf_prim("commandline");
  if(pelf){   // no harm... just get out...
    U32 addr = pelf->ing_start;
    elf_delete(pelf);
    if(CFILL > addr) {// compiled something..
      fpreplfun entry = PTR(fpreplfun,addr);
      printf("about to call %p\n",entry);  
      hd(entry,4);
      (*entry)();
      U32 end = CFILL;
      CFILL = addr;
      memset(PTR(U8*,addr),0,end-addr);
    }
  }
  REL_FLAG = 1;
  return ret;
}

#ifdef STRESSTEST

sSym* ingest_elf_test(char* name){
  sElf* pelf = elf_load("sys/unit.o");
  sSym* sym = 0;
  if(pelf){
    U32 err  = ing_elf(pelf,0);    //0 is good; otherwise error
    if(!err){
      U32 addr = pelf->ing_start;
      printf("ADDR: %08X\n",pelf->ing_start);
      sym = sym_for_artifact(name, addr);
      elf_delete(pelf);
    } else {
      printf("ingest_elf_test: unable to ingest\n");
    }
  } else {
    printf("ingest_elf_test: unable to load ELF object\n");
  }
  return sym;
}
#endif
