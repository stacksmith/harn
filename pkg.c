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

extern sSeg scode;
extern sSeg sdata;

/* aux_proto   for functions, extract the prototype saved on the last line 
               of file the compiler generated with the -aux-info switch.
*/
char* aux_proto(char* fname){
  FILE* f = fopen(fname,"r");
  if(!f){
    fprintf(stderr,"aux_proto: unable to open %s\n",fname);
    exit(1);
  }
  char* buf=(char*)malloc(1024+1);
  // load the end of the file, tucked to the top of buf
  fseek(f,0,SEEK_END);
  long pos = ftell(f);
  //  char* beg =buf;
  printf("pos: %ld\n",pos);
  if(pos>=1024){
    fseek(f,pos-1024,SEEK_SET);
    fread(buf+1,1,1024,f);
    *buf = 0;
    //beg = buf+1;
  } else {
    fseek(f,0,SEEK_SET);
    fread(buf+1+1024-pos,1,pos,f);
    *(buf+1024-pos) = 0;
    //beg = buf+1+1024-pos;
  }
  // now scan backwards
  char* start = buf+1024-4; // back up past ' */<cr>'
  while(*start != '/') start--; 
  *--start = 0;             // chop off K&R comment
  while(*start != '/') start--; // back up to end of first comment
  start+=2;                     // but skip the / and space
  char* ret = (char*)malloc(1+strlen(start));
  strcpy(ret,start);
  free(buf);
  return ret;
}


siSymb* siSymb_new(char* name,U32 data,U32 size){
  siSymb* p = (siSymb*)malloc(sizeof(siSymb));
  size_t lname = strlen(name);
  p->name = (char*)malloc(lname+1);
  strcpy(p->name,name);
  p->hash = string_hash(name);
  p->data = data;
  p->size = size;
  p->proto = 0;
  return p;
}

void siSymb_dump(siSymb* p){
  printf("%08X %08x %s\n",p->data, p->size, p->name);
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
/*
static sDataSize nullds = {0,0};

sDataSize pkg_find_hash(sPkg* pkg,U32 hash){
  siSymb* p = pkg->data;
  while(p){
    if(p->hash == hash)
      return p->ds;
    p = p->next;
  }
  return nullds;
}
sDataSize pkg_find_name(sPkg* pkg,char* name){
  return pkg_find_hash(pkg,string_hash(name));
}
*/
siSymb* pkg_symb_of_hash(sPkg* pkg,U32 hash){
  siSymb* p = pkg->data;
  while(p){
    if(p->hash == hash)
      return p;
    p = p->next;
  }
  return p;
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
printf("Ingesting dll %s, names in %s\n",dllpath,namespath);
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



/* pkg_ingest_elf_sec   Ingest data from an elf section into segment
   
   Also, set the section address in the ELF section header (for resolution)
 */
U32 pkg_ingest_elf_sec(sPkg*pkg,sElf*pelf,U32 isec,sSeg*pseg){
  Elf64_Shdr* shdr = pelf->shdr + isec;
  U32 size = shdr->sh_size;
  U8* src;
  switch(shdr->sh_type){
  case SHT_PROGBITS: src = pelf->buf + shdr->sh_offset; break;
  case SHT_NOBITS: src = 0; break;
  default:
    printf("pkg_ingest_elf_sec: can't ingest section %d\n",isec);
    exit(1);
  }
  U8* addr = seg_append(pseg,src,size);
  shdr->sh_addr = (U64)addr;      // let elf know section address
  printf("ingesting %d bytes from %p\n",size,src);
  return (U32)(U64)addr;
}


U32 pkg_func_from_elf(sPkg* pkg,sElf*pelf,Elf64_Sym* psym){
  seg_align(&scode,8); // our alignment requirement
  U32 codesec = psym->st_shndx;
  U32 faddr = pkg_ingest_elf_sec(pkg,pelf,codesec,&scode);
  char* fname = psym->st_name + pelf->str_sym;

  // there maybe rodata
  U32 strsec = elf_find_section(pelf,".rodata.str1.1");
  if(strsec) {
    printf("pkg_func_from_elf: found .rodata.str1.1\n");
    pkg_ingest_elf_sec(pkg,pelf,strsec,&scode);
  }
  // ok, now calculate size and add to package

  U32 size = seg_pos(&scode) - faddr;
  siSymb* symb = pkg_add(pkg,fname,faddr,size);

  
  symb->proto = aux_proto("o/info.txt");
  return codesec;
  
}
/*
U32 pkg_data_from_elf(sPkg* pkg,sElf*pelf,Elf64_Sym* psym){
  seg_align(&sdata,8); // our alignment requirement
  // get data symbol
  char* name = psym->st_name + pelf->str_sym;
  U32 isec =  psym->st_shndx;
  U32 addr = pkg_ingest_elf_sec(pkg,pelf,sec,&sdata);
  Elf64_Shdr* psec = pelf->shdr + isec;
  
  
  
  
  return 0;
}
*/
// within the elf file, resolve actual addresses for symbols
U32 pkg_elf_resolve(sPkg* pkg,sElf*pelf){
  U64 proc(char* name){
    siSymb* symb = pkgs_symb_of_name(name);
    if(symb)
      return symb->data;
    else
      return 0;
  }
  U32 unresolved = elf_resolve_symbols(pelf,proc);
  if(unresolved)
    printf("%d unresolved\n",unresolved);
  //  unit_ingest_elf2(pu,pelf);
  return unresolved;
}

void pkg_load_elf_func(sPkg* pkg,sElf* pelf,Elf64_Sym* psym){
  // ingest function from ELF; get section 
  U32 codesec =  pkg_func_from_elf(pkg,pelf,psym);
  // resolve elf symbols
  pkg_elf_resolve(pkg,pelf);
  // resolve relocation section, if codesec+1 is a rel section
  void relproc(U32 p,U32 kind){
    printf("Code.Reference at: %08X, %d\n",p,kind);
    seg_rel_mark(&scode,p,kind);
  }
  elf_process_rel_section(pelf,(pelf->shdr)+codesec+1,relproc); 
}
/* pkg_load_elf_data

A data object is more complicated, as it may exist in a single .bss section, 
or span multiple ELF sections.  GCC puts pointer object into a separate 
section with a rel section; strings go into a .rodata.str.1, etc.  A safe 
strategy is to first ingest the section with prime data first (we need it to 
take the base address), then all sections referenced by the symbols, 
and any associated rel section

 */
void pkg_load_elf_data(sPkg* pkg,sElf* pelf,Elf64_Sym* psym){
  seg_align(&sdata,8); // our alignment requirement
  U32 sec = psym->st_shndx;
  U32 addr = pkg_ingest_elf_sec(pkg,pelf,sec,&sdata);
  char* name = psym->st_name + pelf->str_sym;

  printf("pkg_load_elf_data: for %s\n",name);

  // Now, walk the symbols and ingest any referenced sections.
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

  // resolve elf symbols; should be possible now.
  pkg_elf_resolve(pkg,pelf);
  // resolve relocation section, if codesec+1 is a rel section
  //  elf_process_rel_section(pelf,(pelf->shdr)+codesec+1);
  void relproc(U32 p,U32 kind){
    printf("data.Reference at: %08X, %d\n",p,kind);
    seg_rel_mark(&sdata,p,kind);
  }
  elf_apply_rels(pelf,relproc);
  // ok, now calculate size and add to package
  U32 size = seg_pos(&sdata) - addr;
  printf("Addr, size %x,%x\n",addr,size);

  pkg_add(pkg,name,addr,size);
 
}


void pkg_load_elf(sPkg* pkg,char* path){
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
  switch(ELF64_ST_TYPE(psym->st_info)){
  case STT_FUNC:
    pkg_load_elf_func(pkg,pelf,psym);
    break;
  case STT_OBJECT:
    pkg_load_elf_data(pkg,pelf,psym);
    break;
  default:
    printf("pkg_load_elf: global symbol is not FUNC or OBJECT\n");
    exit(1);
  }
    
  free(pelf);
}

extern sPkg* pkgs;
void pkgs_add(sPkg* pkg){
  if(pkg->next){
    printf("pkgs_add: pkg is not free!\n");
    exit(1);
  }
  pkg->next = pkgs;
  pkgs = pkg;
}

void pkgs_list(){
  sPkg* p = pkgs;
  printf("Packages:\n");
  while(p){
    printf(" %s\n",p->name);
    p=p->next;
  }
}


siSymb* pkgs_symb_of_name(char* name){
  sPkg* pkg = pkgs;
  U32 hash = string_hash(name);
  while(pkg){
    siSymb* symb = pkg_symb_of_hash(pkg,hash);
    if(symb)
      return symb;
    pkg=pkg->next;
  }
  return 0;
}
