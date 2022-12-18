#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"
#include "seg.h"
#include "sym.h"
#include "asmutil.h"
extern sSeg* psCode;
extern sSeg* psData;
extern sSeg* psMeta;

struct siSymb;
typedef struct siSymb {
  struct siSymb* next;
  char* name;                 // malloc'ed
  union {
    sDataSize ds;
    struct {
      U32 data;
      U32 size;
    };
  };
  char* proto;                // malloc'ed
  U32 src;                    // file
  U32 srclen;
  U32 hash;
  
} siSymb;

sSym* sym_new(char* name, U32 art, U32 size, U32 src,char* proto){
  U32 namelen = strlen(name);
  U32 protolen = proto ? strlen(proto) : 0;
  U32 octs = (SYM_NAME_OFF + namelen + protolen + 2 + 7) >> 3;
  U8* bptr = seg_append(psMeta,0,octs*8);
  U32 uptr = (U32)(U64)bptr;
  sSym* p = (sSym*)bptr;
  
  seg_rel_mark(psMeta,uptr+0,2); // next pointer is a 32-bit pointer
  seg_rel_mark(psMeta,uptr+8,2); // data pointer is a 32-bit pointer

  seg_align(psMeta,8);
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
  seg_align(psMeta,8);
  return p;
}
/*----------------------------------------------------------------------------
  sym_delete                  delete a symbol.

In order to unlink sym, we need the symbol that comes before it in the search
known as prev!

----------------------------------------------------------------------------*/
#include "aseg.h"
U32 sym_delete(sSym* prev){ 
  printf("sym.delete.. prev: %p, next %08x\n",prev,prev->next);
  sSym* sym = U32_SYM(prev->next);  //symbol we are actually deleting
  prev->next = sym->next;    //unlink symbol
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

   //   hd(sym,4);
   //printf("bits_fix_meta(top:%08x, hole: %08x, fix:%08x,\n artz:%08x,artend:%08x,artfix:%08x)\n",  mtop,symend,symsize,  artzone,artend,artsize);
 U32 ret = bits_fix_meta(mtop,
			 symend, symsize,
			 artzone,artend,artsize);
 //  printf("%08X fixups\n",ret);
 //  hd(sym,4);
  
  //printf("symsize %08x; end %08x\n",symsize, symend);
 //  printf("bits_drop(%08x,%08x,%08x,%08x)\n",	 symU32, symend, 0, mtop - symend);
  // drop in meta, eliminating the symbol
  bits_drop(symU32, symend, 0, mtop - symend);
  //printf("drop got %08X\n",ret);
  // printf("after drop before fix, prev->next is %08x\n",prev->next);
  // hd(sym,4);
  // printf("bits_fix_meta: %08X fixups\n",ret);

  // printf("Requesting drop %08X, %08X\n",art,artsize);
  //rintf("aseg_chomp(%08x,%08x,%08x, %08x)\n",	artzone, artend, artsize, otherend);
 ret += aseg_chomp(artzone, artend, artsize, otherend);
 printf(" %08X fixups\n",ret);
 
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

----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
create a symbol from ELF sisymb   ;TODO: wtf?
----------------------------------------------------------------------------*/
sSym* sym_from_siSymb(void* v){
  siSymb*s = (siSymb*)v;
  return sym_new(s->name,s->data,s->size,s->src,s->proto);
}

