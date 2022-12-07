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
#include "unit.h"

extern sSeg scode;
extern sSeg sdata;

char* unit_name(sUnit* pu){
  return pu->strings+1;
}

void usym_dump(sUnit*pu, U32 i){
  printf("%08x ",pu->dats[i].off);
  printf("%08x ",pu->hashes[i]);//,pu->dats[i].ostr);
  printf("%s\n",pu->strings + pu->dats[i].ostr);
  
}

void unit_dump(sUnit* pu){
  printf("-----\nUnit %s\n",pu->strings+1);
  printf("code: %p, $%04X  ",scode.base + pu->oCode,pu->szCode);
  printf("data: %p, $%04X  ",sdata.base + pu->oData,pu->szData);
  printf("%d symbols, %d globals\n",pu->nSyms,pu->nGlobs);
  
  for(U32 i=1;i<pu->nSyms;i++){
    usym_dump(pu,i);
  }  
  
}



/*
Ingest sections into code or data segment, assigning base addresses

Of interest are section that need allocation (SHF_ALLOC)..
Executable (SHF_EXECINSTR) sections get placed into the code seg; the
rest go into the data seg.  
* For now, we merge all data - BSS, vars, strings, ro or not
* We section honor alignment
*/ 
void unit_sections_from_elf(sUnit*pu,sElf* pelf){
  // unit segment positions
  pu->oCode = scode.fill;
  pu->oData = sdata.fill;
 
  //printf("Of interest:\n");
  Elf64_Shdr* shdr = pelf->shdr;  
  for(U32 i=0; i< pelf->shnum; i++,shdr++){
    U64 flags = shdr->sh_flags;
    if(flags & SHF_ALLOC){
      // select code or data segment to append
      sSeg*pseg = (flags & SHF_EXECINSTR) ? &scode : &sdata;
      // src and size of append
      U8* src = (shdr->sh_type == SHT_NOBITS)?
	0 : pelf->buf + shdr->sh_offset;
      seg_align(pseg,shdr->sh_addralign);
      shdr->sh_addr = (U64)seg_append(pseg,src,shdr->sh_size);
      //      sechead_dump(pelf,i);
    }
  }
  // update unit sizes
  pu->szCode = scode.fill - pu->oCode;
  pu->szData = sdata.fill - pu->oData;
}


void unit_symbols_from_elf(sUnit*pu,sElf* pelf){

  char* buf_str = (char*)malloc(0x10000);
  U32*  buf_hashes = (U32*)malloc(0x1000 * 4);
  sSym* buf_syms = (sSym*)malloc(0x1000 * sizeof(sSym));

  // 0th entry: unit name
  char* name = pelf->str_sym+1;
  char* pstr = buf_str;
  *pstr++ = 0;
  strcpy(pstr, name);
  pstr += (1 + strlen(name));
  buf_hashes[0]=string_hash(name);
  buf_syms[0].value=0;

  // reserve 0th
  U32 cnt = 1;
  void proc(Elf64_Sym* psym,U32 i){
    // only defined global symbols make it to our table
    if(psym->st_shndx && ELF64_ST_BIND(psym->st_info)){
      char* name = pelf->str_sym + psym->st_name;
      U64 namelen = strlen(name)+1;
      strcpy(pstr,name);
      buf_hashes[cnt] = string_hash(name);
      buf_syms[cnt].ostr = pstr - buf_str; // string offset
      buf_syms[cnt].off = psym->st_value; // symbol addr
      
      pstr += namelen;
      cnt++;
    }
  }
  elf_process_symbols(pelf,proc);

  pu->nSyms = cnt;
  pu->nGlobs = cnt-1;
  pu->strings = (char*) realloc(buf_str,pstr-buf_str);
  pu->hashes  = (U32*)  realloc(buf_hashes,cnt*4);
  pu->dats =    (sSym*) realloc(buf_syms,cnt*sizeof(sSym));
}
/*
  There are 4 stages of ingestion:
  * unit_sections         - ingest unfixed data and code from ELF sections;
  * elf_resolve_symbols   - resolve ELF symbols in ELF file to addresses
  * elf_apply_rels        - apply in-system fixups using elf rels
  * unit_symbols_from_elf - create system symbols from elf file

To resolve cross-file dependencies we need to work with multiple elf files.
We can run the first two steps on each elf file, constructing system segs, 
and resolving local symbols; we wind up                                                                                      with UNDEF symbols for both globals
and interdependent symbols.  We need to resolve any interdeependencies first.
We need to
* search each elf file for defined visible symbols, and resolve elf symbols

  */

void unit_ingest_elf2(sUnit* pu,sElf* pelf){
  elf_apply_rels(pelf);
  unit_symbols_from_elf(pu,pelf);
}
sUnit* unit_ingest_elf1(sElf* pelf, char* path){
  elf_load(pelf,path);
  // seg_dump(&scode); seg_dump(&sdata);
  sUnit* pu = (sUnit*)malloc(sizeof(sUnit));
  unit_sections_from_elf(pu,pelf);
  return pu;
}

U32 unit_elf_resolve(sElf*pelf,pfresolver resolver){
  U32 unresolved = elf_resolve_symbols(pelf,resolver);
  if(unresolved)
    printf("%d unresolved\n",unresolved);
  //  unit_ingest_elf2(pu,pelf);
  return unresolved;
}
/*==========================================================================


 */

/*
 find a specified hash in a unit and return the symbol index, or 0
 remember that 0th slot is not a valid symbol!
*/
U32 unit_find_hash(sUnit*pu,U32 hash){
  U32* p = pu->hashes+1;
  for(U32 i = 1;i<pu->nSyms; i++){
    if(hash==*p++)
      return i;
  }
  return 0;
}

/* units_find_hash

ppu points at a zero-terminated list of pointers to sUnits.

returns: 0 on fail or
 * low: index+1 of matching symbol; high: index of sUnit* in list
 * parameter ppu points at sUnit* containing it


U64 units_find_hash(sUnit**ppu,U32 hash){
  sUnit* pu;
  U32 found;
  U64 i=0;
  while((pu=*ppu++)){
    found = unit_find_hash(pu,hash);
    if(found)
      return (i<<32)|found;
    i++;
  }
  return 0;
}
 */

void unit_lib(sUnit*pu,void* dlhan, U32 num,char*namebuf){
  static U8 buf[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0}; // jmp rax

  pu->oCode = scode.fill;
  pu->oData = sdata.fill;

  pu->nGlobs = num - 1;
  pu->nSyms = num;
  pu->strings = namebuf; // using the one passed to us
  pu->hashes  = (U32*)malloc((num) * 4);
  pu->dats    = (sSym*)malloc((num) * sizeof(sSym));
  
  // first pass: create per-function jump, compute strings size

  U32* phash = pu->hashes;
  char* pstr = pu->strings+1; //start at name
  sSym* psym = pu->dats;

  *phash++ = string_hash(pstr);
  pstr += (strlen(pstr)+1);
  psym->value = 0;
  psym++;
  for(U32 i=1; i<num; i++){
    U8* addr = seg_append(&scode,buf,sizeof(buf)); // compile jump
    void* pfun = dlsym(dlhan,pstr);
    *((void**)(addr+2)) = pfun; // fixup to function address
    psym->off = (U32)(U64)addr;
    psym->seg  = 1;
    psym->type = 0;
    psym->ostr = pstr - pu->strings; // get string offset
    psym++;
    *phash++ = string_hash(pstr);
    pstr += (strlen(pstr)+1);        // find next string
  }
  
  // update bounds of unit
  pu->szCode = scode.fill - pu->oCode;
  pu->szData = sdata.fill - pu->oData;

}
