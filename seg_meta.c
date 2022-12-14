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
#include "seg_asm.h"
#include "seg_meta.h"

/* ==============================================================

A fillable area for code or data

  ==============================================================*/

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
  GNS = 0;
  REL_FLAG = 1;

  rel_mark(THE_U32(MFILL_ADDR), 2); // fill  
  rel_mark(THE_U32(GNS_ADDR), 2); // srch_list  

}
/* -------------------------------------------------------------
Metaseg has some system variables (each 32 bits long)
0 fill      A32  fill pointer
4 top       --   top of segment ???? don't need it really...
8 srchlist  A32  link to search path !!!! DO NOT MOVE!  
C rel_flag  --   relocation control

Note: PLST_WALK uses the base of metaseg as a fake HEAD, since
package heads use the art pointer at offset 8 of every head...
DO NOT MOVE SRCHLIST, and DO NOT USE PREV for anything but 
---------------------------------------------------------------*/

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

 This purges the symbol from the metadata segment and compacts it.  Any
 dereferenced pointers to symbols are stale!

 Additionally, the symbols have been adjusted for the upcoming artifact 
 purge, and any symbol->artifact references are not reliable until the
 final artifact drop



----------------------------------------------------------------------------*/
/*
U32 mseg_delete(U32 hole, U32 holesize,
		U32 artzone,  U32 artfix){
  //  U32 temp(U32 dropzone, U32 fix, U32 artzone, U32 artend, U32 artfix)
  U32 mfill = MFILL; // MFILL will be altered - keep original copy.
  U32 dropzone = hole+holesize; // end of hole
  // prepare meta segment references for upcoming drop.
  U32 ret = bits_fix_meta(mfill, dropzone, holesize,
			  artzone,artfix);
  
  bits_drop(hole, dropzone,  mfill-dropzone); // like memcpy
  return ret;
}
*/
