/* Load a single elf object file and fixup */

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <elf.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <errno.h>

#include "global.h"
#include "asmutil.h"
#include "seg.h"

/* ==============================================================

A fillable area for code or data

  ==============================================================*/
/*typedef struct sSeg {
  U8* base; // ptr of allocation/mapping
  U8* fillptr;
  U8* end;
  char name[8];
} sSeg;
*/
void mseg_dump(){
  printf("Segment %08X %08x\n",META_SEG_ADDR,MFILL);
}


/* -------------------------------------------------------------
   seg_alloc

---------------------------------------------------------------*/
void mseg_alloc(){
  // allocate data area
  seg_mmap(PTR(void*,META_SEG_ADDR), META_SEG_SIZE,
	   PROT_READ|PROT_WRITE, "meta");
  seg_mmap(PTR(void*,(META_SEG_ADDR/8)), META_SEG_SIZE/8,
	   PROT_READ | PROT_WRITE,"meta-rel");

  MFILL = META_SEG_ADDR + 16;
  SRCH_LIST = 0;
  REL_FLAG = 1;
  
  rel_mark(THE_U32(MFILL_ADDR), 2); // fill  
  rel_mark(THE_U32(SRCH_LIST_ADDR), 2); // srch_list  
}
/* -------------------------------------------------------------
Metaseg has some system variables (each 32 bits long)
0 fill      A32  fill pointer
4 top       --   top of segment
8 srchlist  A32  link to search path
C rel_flag  --   relocation control

---------------------------------------------------------------*/

/*
void seg_serialize(sSeg* psg,FILE* f){
  // save data area

  U32 size = psg->fill - (U32)(U64)(psg);  // byte size of segment data
  size_t wr1 = fwrite(psg,1,size,f);
  U8* prel = (U8*)(((U64)psg)>>3);
 
  size_t wr2 = fwrite(prel,1,size>>3,f);
  printf("wrote: %lx and %lx\n",wr1,wr2);
}
// TODO: should wipe rel first; otherwise, deserializing a shorter segment
// than current will leave old garbage.
void seg_deserialize(sSeg* psg,FILE*f){
  fread(psg,1,8,f);
  U32 size = psg->fill - (U32)(U64)psg;
  printf("size %x\n",size);
  size_t rd1 = fread(psg+1,1,size-8,f);
  U8* prel = (U8*)(((U64)psg)>>3);
  size_t rd2 = fread (prel,1,size>>3,f);
  printf("read: %lx %lx\n",rd1+8,rd2);
}
*/
void mseg_align8(){
  MFILL  = (MFILL + 7) & 0xFFFFFFF8;
}


/* -------------------------------------------------------------
seg_append  Append a run of bytes to the segment
            size   number of bytes to append
            start  address from which to copy.  
                   if 0, fill with 0 bytes.
---------------------------------------------------------------*/
U8* mseg_append(U8* start,U32 size){
  U32 end = MFILL + size;
  U8* dest = PTR(U8*,MFILL);
  MFILL = end;
  if(start)
    memcpy(dest,start,size);
  else
    memset(dest,0,size);
  return dest;
}


/*----------------------------------------------------------------------------
 RELOCATIONS

 We observe that:
 * code segment may have Off32 or Abs64 references;
 * data segment may have Abs64 refs or Abs32 refs (not for C);
 * meta segment only has Abs32 refs.

----------------------------------------------------------------------------*/


/* ------------------------------------------------------------- 
seg_rel_mark

We support two kinds of refs:
1 = link A32
2 = artptr A32

*/

#include "util.h"
/*
U32 cnt_refs(sSeg*pseg,void*target){
  seg_align(pseg,8);
  //  U8* relbase = pseg->prel;
  U32 bottom = (U32)(U64)(pseg->prel);
  U32 off = seg_off(pseg);
  U32 i=0;
  U64 result;
  hd(pseg->base,16);
  //  printf("Entering loop %p %d %d\n",relbase,off,off>>3);
  while((result = bits_next_ref(off,bottom))){
    //    printf(".1.\n");
    //    printf("ref %d at $%08X",(U32)(result>>32),(U32)result);
    off = (U32) result;
    U8* ref = (U8*)(U64)off;  //position of reference
    void* tgt = (result&3)-1 ? 
      *(void**)ref :
      (void*) ((*(S32*)ref) + (ref+4));
    if(tgt==target)
      i++;
    //    printf("at %p, %p\n",ref,tgt);

  }
  return i;
}
*/
/*
U32 seg_reref(sSeg*pseg,U64 old,U64 new){
  seg_align(pseg,8);
  //  U8* relbase = pseg->prel;
  U8* segbase = pseg->base;
  U32 off = (seg_pos(pseg));
  U32 bottom = (U32)(U64)(pseg->base);
  U64 diff = new-old;
  U64 result;
  U32 i=0;
  hd(segbase,16);
  hd(pseg->prel,16);
  //  printf("Entering loop %x %x\n",off,bottom);
  while((result = bits_next_ref(off,bottom))){
    //printf(".1. %016lx\n",result);
    //printf("ref %d at $%08X\n",(U32)(result>>32),(U32)result);
    off = (U32) result;
    U8* ref = (U8*)(U64)off;  //position of reference
    if((result&3)-1) {
      if(old == (*(U64*)ref)) {
	printf("64-bit fixup at %p, by %08lX\n",ref,diff);
	i++;
	(*(U64*)ref) += diff;
      }
    } else {
      if(old == ((U64)((*(S32*)ref) + (ref+4)))){
	printf("32-bit fixup at %p, by %08X\n",ref,(U32)diff);
	i++;
	(*(U32*)ref) += (U32)diff;
      }
    }
  }
  return i;
}
*/
U32 seg_reref(sSeg*psg,U32 old,U32 new){
  printf("top:%08X bot:%08X old:%08X new:%08X \n",
	 MFILL,
	 X32(psg),
	 old,new);
  
  //                top           bottom 
  return bits_reref(MFILL, X32(psg), old, new);	 
}
/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/

sCons* mcons(U32 car, U32 cdr){
  sCons* ret;
  U32* p = PTR(U32*,MFILL);
  ret = (sCons*)p;
  MFILL += 8;
  rel_mark(THE_U32(p),2); // cdr is A32
  *p++ = cdr;
  rel_mark(THE_U32(p),2); // cdr is A32
  *p++ = car;
  return (sCons*)ret;
}
  

sCons* minsert(sCons* after, sCons* it){
  it->cdr = after->cdr;     // make it link to whatever we linked to
  after->cdr = THE_U32(it); // we link to it.
  return after;
}


/*
U64 mapc(mapcfun fun,U32 list){
  U64 ret;
  while(list){
    if((ret = (*fun)(list)))
      return ret;
    list = PTR(sCons*,list)->cdr;
  }
  return 0; 
}
*/
/*----------------------------------------------------------------------------
  sym_delete                  delete an unlinked symbol, whose artifacts are
                              free from refs


----------------------------------------------------------------------------*/
U32 temp(U32 dropzone, U32 fix, U32 artzone, U32 artend, U32 artsize){
  U32 mfill = MFILL; // MFILL will be altered - keep original copy.
  // prepare meta segment references for upcoming drop.
  U32 ret = bits_fix_meta(mfill, dropzone, fix,
			  artzone,artend,artsize);
  //  dest: hole start   src:hole-end   bytes from hole_end to top
  bits_drop(dropzone-fix, dropzone,  mfill-dropzone);
  return ret;
}
