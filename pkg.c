// symtab.
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy
#include <elf.h>
#include <dlfcn.h>

#include "global.h"
#include "elf.h"
#include "elfdump.h"
#include "util.h"
#include "seg.h"
#include "pkg.h"
#include "src.h"
#include "pkgs.h"

extern sSeg scode;
extern sSeg sdata;

siSymb* siSymb_new(char* name,U32 data,U32 size){
  siSymb* p = (siSymb*)malloc(sizeof(siSymb));
  size_t lname = strlen(name);
  p->name = (char*)malloc(lname+1);
  strcpy(p->name,name);
  p->hash = string_hash(name);
  p->data = data;
  p->size = size;
  p->proto = 0;
  p->src = 0;
  p->srclen = 0;
  return p;
}

void siSymb_delete(siSymb* symb){
  free(symb->name);
  free(symb->proto);
  free(symb);
}

void siSymb_dump(siSymb* p){
  printf("%08X %08x %s\n",p->data, p->size, p->name);
}

void siSymb_set_src(siSymb* symb,U32 src, U32 srclen){
  symb->src = src;
  symb->srclen = srclen;
}

void siSymb_src_to_body(siSymb* symb){
  U32 srcpos=0;
  U32 srclen=0;
  if(symb){
    srcpos = symb->src;
    srclen = symb->srclen;
  }
  src_to_body(srcpos,srclen);
}

void pkg_dump(sPkg* pkg){
  siSymb* p = pkg->data;
  while(p){
    siSymb_dump(p);
    p = p->next;
  }
}


sPkg* pkg_new(){
  sPkg* pkg = (sPkg*)malloc(sizeof(sPkg));
  pkg->next = 0;
  pkg->data = 0;
  pkg->name = 0;
  return pkg;
}
void pkg_set_name(sPkg* pkg,char*name){
  pkg->name = (char*)malloc(strlen(name)+1);
  strcpy(pkg->name,name);
}


siSymb* pkg_add(sPkg* pkg,char*name,U32 data,U32 size){
  siSymb*p = siSymb_new(name,data,size);
  // cons it in
  p->next = pkg->data;
  pkg->data = p;
  
  return p;
 
}
/*----------------------------------------------------------------------
pkg_drop_symb         destroy the last symbol, clean up

-----------------------------------------------------------------------*/
void pkg_drop_symb(sPkg* pkg){
  siSymb*symb = pkg->data;
  // clean up and return 0
  pkg->data = symb->next; //unlink the bad symbol
  U32 addr = symb->data;
  U32 size = symb->size;
  if(addr & 0x80000000){ //code?
    scode.fill = addr; // drop the code segment, eliminate code
  } else {
    sdata.fill = addr;
  }
  memset((U8*)(U64)addr,0,size); // clear it
  siSymb_delete(symb);
}

siSymb* pkg_symb_of_hash(sPkg* pkg,U32 hash){
  siSymb* p = pkg->data;
  while(p){
    if(p->hash == hash)
      return p;
    p = p->next;
  }
  return p;
}

void pkg_words(sPkg* pkg){
  siSymb* p = pkg->data;
  while(p){
    printf(" %s",p->name);
    p = p->next;
  }
  puts("");
}

siSymb* pkg_symb_of_name(sPkg* pkg,char* name){
  return pkg_symb_of_hash(pkg,string_hash(name));
}

// compile a library into pkg
static char* pkg_lib_load_names(char*path,U32*pcnt){
  char* srcbuf;
  S64 len = file_map((void**)&srcbuf,path,PROT_READ);
  if(len<0) file_map_error_msg(len,path,1);
  // copy into a buffer
  char* buf = (char*)malloc(1+len);
  char* p = buf;
  *p++ = 0;
  memcpy(p,srcbuf,len);
  //
  unmap(srcbuf,len);
  // now, replace crs with 0, and count them
  U32 i = 0;
  while((p = strchr(p,10))){
    *p++ = 0;
    i++;
  }
  *pcnt = i;
  
  return buf;	 
}

static void pkg_lib_prim(sPkg* pkg,void* dlhan, U32 num,char*namebuf){
  // a 12-byte prototype of an assembly trampoline
  static U8 buf[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0 }; // jmp rax

  char* pstr = namebuf+1; // start at lib name; will skip
  for(U32 i=1; i<num; i++){
    pstr += (strlen(pstr)+1);   // advance to next name
    seg_align(&scode,8);
    U8* addr = seg_append(&scode,buf,sizeof(buf)); // compile jump
    void* pfun = dlsym(dlhan,pstr);
    *((void**)(addr+2)) = pfun; // fixup to function address
    pkg_add(pkg,pstr,(U32)(U64)addr,sizeof(buf));
  }
  
}

void pkg_lib(sPkg* pkg,char*dllpath,char*namespath){
#ifdef DEBUG
  printf("Ingesting dll %s, names in %s\n",dllpath,namespath);
#endif
// read the names into strings, get count
  U32 symcnt;
  char* strings = pkg_lib_load_names(namespath,&symcnt);
  pkg_set_name(pkg,strings+1);
  //  printf("loaded %d names\n",symcnt);
  void* dlhan = dlopen(dllpath,RTLD_NOW);
  if(!dlhan){
    fprintf(stderr,"Unable to open dll %s\n",dllpath);
    exit(1);
  }
  pkg_lib_prim(pkg,dlhan,symcnt,strings);
  free(strings);
}
/*****************************************************************************
 
 *****************************************************************************/

/*----------------------------------------------------------------------------
   pkg_ingest_elf_sec   Ingest data from an elf section into segment
   
   Also, set the section address in the ELF section header (for resolution)
   return: section address in segment.
-----------------------------------------------------------------------------*/
U32 pkg_ingest_elf_sec(sPkg*pkg,sElf*pelf,U32 isec,sSeg*pseg){
  Elf64_Shdr* shdr = pelf->shdr + isec;
  U32 size = shdr->sh_size;
  U8* src;
  // PROGBITS sections contain data to ingest;
  // NOBITS sections are .bss, reserve cleared memory
  switch(shdr->sh_type){
  case SHT_PROGBITS: src = pelf->buf + shdr->sh_offset; break;
  case SHT_NOBITS: src = 0; break;
  default:
    printf("pkg_ingest_elf_sec: can't ingest section %d\n",isec);
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
siSymb* pkg_func_from_elf(sPkg* pkg,sElf*pelf,Elf64_Sym* psym){
  seg_align(&scode,8); // our alignment requirement

  U32 codesec = psym->st_shndx;
  U32 faddr = pkg_ingest_elf_sec(pkg,pelf,codesec,&scode);
  char* fname = psym->st_name + pelf->str_sym;

  // there maybe rodata
  U32 strsec = elf_find_section(pelf,".rodata.str1.1");
  if(strsec) {
#ifdef DEBUG
    printf("pkg_func_from_elf: found .rodata.str1.1\n");
#endif
    pkg_ingest_elf_sec(pkg,pelf,strsec,&scode);
  }
  // ok, now calculate size, make a symbol, and add to package
  U32 size = seg_pos(&scode) - faddr;
  siSymb* symb = pkg_add(pkg,fname,faddr,size);
  // attach prototype
  symb->proto = aux_proto("sys/info.txt");
  printf("....proto: %s\n",symb->proto);
  return symb;
}
/*----------------------------------------------------------------------------
pkg_elf_resolve    within the elf file, resolve actual addresses for symbols

return: number of unresolved elf symbols
----------------------------------------------------------------------------*/
U32 pkg_elf_resolve(sPkg* pkg,sElf*pelf){
  
  U64 proc(char* name){
    siSymb* symb = pkgs_symb_of_name(name); // global
    //    printf("Looking for %s; got %p\n",name,symb);
    if(symb)
      return symb->data; // will exit
    else
      return 0;
  }
  U32 unresolved = elf_resolve_symbols(pelf,proc);
  //  if(unresolved)    printf("%d unresolved\n",unresolved);
  return unresolved;
}
/*----------------------------------------------------------------------------
 pkg_load_elf_func      load a function, and try to resolve it.
                          if not resolvable, remove all traces.

 return: symb of the ingested function.
----------------------------------------------------------------------------*/

siSymb* pkg_load_elf_func(sPkg* pkg,sElf* pelf,Elf64_Sym* psym){
  // ingest function from ELF; get section 
  U32 codesec = psym->st_shndx;
  // create a symb, etc
  siSymb* ret = pkg_func_from_elf(pkg,pelf,psym);
  // resolve elf symbols
  U32 unresolved =  pkg_elf_resolve(pkg,pelf);
  // should there be unresolved symbols, erase all traces of symb in pkg
  if(unresolved){
    pkg_drop_symb(pkg);  // the last symbol is removed
    return 0;
  }
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
siSymb* pkg_load_elf_data(sPkg* pkg,sElf* pelf,Elf64_Sym* psym){
  seg_align(&sdata,8); // align data seg for new data object
  // First, ingest the main data object and keep its address.
  U32 sec = psym->st_shndx;
  U32 addr = pkg_ingest_elf_sec(pkg,pelf,sec,&sdata);
  char* name = psym->st_name + pelf->str_sym;
#ifdef DEBUG
  printf("pkg_load_elf_data: for %s\n",name);
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
	U32 addr = pkg_ingest_elf_sec(pkg,pelf,refsec,&sdata);
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

/*----------------------------------------------------------------------------
pkg_load_elf                load an ELF file, find the global symbols, and
                            load into appropriate segment.
returns: symb
----------------------------------------------------------------------------*/
siSymb* pkg_load_elf(sPkg* pkg,char* path){
  sElf* pelf = elf_new();
  elf_load(pelf,path);
  // Find the global symbol; must be only one.
  S32 i = elf_find_global_symbol(pelf);
  
  switch(i){
  case 0:
    printf("pkg_load_elf: no global symbols found\n");
    exit(1);
  case -1:
    printf("pkg_load_elf: more than one global symbol\n");
    exit(1);
  }
  // Dispatch to FUNC or OBJECT loader
  Elf64_Sym* psym = pelf->psym+i;
  siSymb* symb;
  switch(ELF64_ST_TYPE(psym->st_info)){
  case STT_FUNC:
    symb = pkg_load_elf_func(pkg,pelf,psym);
    break;
  case STT_OBJECT:
    symb = pkg_load_elf_data(pkg,pelf,psym);
    break;
  default:
    printf("pkg_load_elf: global symbol is not FUNC or OBJECT\n");
    exit(1);
  }
  // done with the elf file!
  free(pelf);
  // symb may be 0 if there are unresolved references!
  // if it's good, load source.
  if(symb){
    U32 len;
    U32 pos = src_from_body(&len);
    siSymb_set_src(symb,pos,len);
  }
  return symb;
}
/*----------------------------------------------------------------------------
 pkg_dump_protos    Dump all prototypes of this package into a header file 

----------------------------------------------------------------------------*/
void pkg_dump_protos(sPkg* pkg,FILE* f){
  siSymb* symb = pkg->data;
  while(symb){
    if(symb->proto)
      fprintf(f,"%s\n",symb->proto);
    symb = symb->next;
  }
}
