#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"
#include "src.h"
#include "seg.h"
#include "aseg.h" 

#include "sym.h"
#include "asmutil.h"

sSym* sym_new(char* name, U32 art, U32 size, U32 src,char* proto){
  U32 namelen = strlen(name);
  U32 protolen = proto ? strlen(proto) : 0;
  U32 octs = (SYM_NAME_OFF + namelen + protolen + 2 + 7) >> 3;
  U8* bptr = mseg_append(0,octs*8);
  U32 uptr = (U32)(U64)bptr;
  sSym* p = (sSym*)bptr;
  
  rel_mark(uptr+0,2); // next pointer is a 32-bit pointer
  rel_mark(uptr+8,2); // data pointer is a 32-bit pointer

  mseg_align8();
  p->hash = string_hash(name);
  p->art = art;
  p->size = size;
  p->src  = src;
  p->octs = octs;
  p->namelen = namelen+1;
  strcpy(bptr+SYM_NAME_OFF, name);

  if(proto){
    strcpy(bptr+SYM_NAME_OFF+1+namelen, proto);
  }
  mseg_align8();
  return p;
}
/*----------------------------------------------------------------------------
  sym_delete                  delete an unlinked symbol, whose artifacts are
                              free from refs


----------------------------------------------------------------------------*/
#include "aseg.h"
/*
U32 sym_delete(sSym* sym){
  printf("sym_delete %p\n",sym);
  sym_dump1(sym);
  // before the symbol disappears, read the data stored therein...
  U32 symU32 = THE_U32(sym);
  U32 symsize = sym->octs << 3;  // size of the hole, in bytes
  U32 symend = symU32 + symsize; // alignment inherent
  // artifact bounds
  U32 art = sym->art;
  U32 artsize = 0xFFFFFFF8 & (sym->size + 7);

  U32 artzone = art+artsize;    //start of art dropzone
  U32 artend;                   // end of art dropzone segment
  U32 otherend;                 // end of other segment
  U32 mtop = MFILL;             
  if(IN_DATA_SEG(art)){
    artend =  DFILL;
    otherend = CFILL;
  } else {
    artend = CFILL;
    otherend = DFILL;
  }
  printf("will delete %08x to %08x\n",symU32,symend);  
  printf("  art: %08x, %08x\n",art,artsize);
  hd(sym,4);
  printf("bits_fix_meta(top:%08x, hole: %08x, fix:%08x,\n artz:%08x,artend:%08x,artfix:%08x)\n",  mtop,symend,symsize,  artzone,artend,artsize);
 U32 ret = bits_fix_meta(mtop,
			 symend, symsize,
			 artzone,artend,artsize);
   printf("%08X fixups\n",ret);
   hd(sym,4);
  
 ///printf("symsize %08x; end %08x\n",symsize, symend);
   printf("bits_drop(%08x,%08x,%08x)\n",	 symU32, symend,  mtop - symend);
  // drop in meta, eliminating the symbol
  bits_drop(symU32, symend,  mtop - symend);
  printf("drop got %08X\n",ret);
  // printf("after drop before fix, prev->next is %08x\n",prev->next);
  // printf("bits_fix_meta: %08X fixups\n",ret);

  // printf("Requesting drop %08X, %08X\n",art,artsize);

  printf("aseg_chomp(%08x,%08x,%08x, %08x)\n",	artzone, artend, artsize, otherend);
  ret += aseg_delete(artzone, artend, artsize, otherend);
  return ret;
}
*/
U32 sym_del(sSym* sym){
  U32 artfix  = ROUND8(sym->size);
  U32 artzone = sym->art + artfix;    //start of art dropzone

  U32 artend;                   // end of art dropzone segment
  U32 otherend;                 // end of other segment
  if(IN_DATA_SEG(artzone)){
    artend =  DFILL;
    otherend = CFILL;
  } else {
    artend = CFILL;
    otherend = DFILL;
  }
  //                       
  U32 ret = mseg_delete(THE_U32(sym), SYM_BYTES(sym),
			artzone, artend, artfix);
  //  ret += aseg_delete(artzone, artend, artfix, otherend);
  ret += aseg_delete1(artzone, artfix);
  return ret;
}

/*
void sym_wipe_last(sSym* sym){
  U32 bytes = sym->octs * 8;
  U32 addr = sym->data;
  U32 size = sym->size;
  if(IN_CODE_SEG(addr)){
    psCode->fill = addr; // drop the code segment, eliminate code
  } else {
    psData->fill = addr;
  }
  memset((U8*)(U64)addr,0,size); // clear it
  memset((U8*)(U64)(addr/8),0,(size+7)/8); // clear rel

  psMeta->fill = (U32)(U64)sym;  
  memset(sym,0,bytes);
  memset((U8*)(U64)(psMeta->fill/8),0,sym->octs); // clear rel
}
*/
char* sym_name(sSym* sym){
  return ((char*)sym)+SYM_NAME_OFF;
}
char* sym_proto(sSym* sym){
  char* pname = ((char*)sym)+SYM_NAME_OFF;
  return (pname + sym->namelen);
}

void sym_dump1(sSym* sym){
  printf("%p: %s:\t %08X %04X %s\n",sym,SYM_NAME(sym),sym->art, sym->size,sym_proto(sym));
}
/*----------------------------------------------------------------------------
  sym_for_artifact            create a sSym for an artifact in aseg...

This is called by ing_elf after ingesting the data.  Some assumptions:
* sElf structure is still alive and contains the name (delete later!)
* gcc invoked with '-aux-info info.txt'; last line is func prototype
* compiled source for this artifact is in 'code.c'; will be absorbed.

----------------------------------------------------------------------------*/


sSym* sym_wrap(char* name,U32 art, U32 size){
  U32 srclen; //not used!
  U32 src = src_from_body(&srclen);  // store source and get file offset
  // if function, extract proto
  char* proto = 0;
  if(IN_CODE_SEG(art))
    proto = aux_proto();    // allocated buffer with prototype from aux-info
  // create a new symbol (proto is copied);
  sSym* ret = sym_new(name, art, size, src, proto);
  free(proto);
  return ret;
}

