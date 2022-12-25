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
#include "seg_common.h"
#include "seg_asm.h"   // for bits_
//#include "aseg.h"

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

/* -------------------------------------------------------------
   seg_alloc

---------------------------------------------------------------*/

void* seg_mmap(void* addr, U64 size, U64 prot, char* which){
  void* ret  = mmap(addr, size, prot,
		    0x20 | MAP_SHARED | MAP_FIXED, //MAP_ANONYMOUS
		    0,0);
  if(MAP_FAILED == ret){
    fprintf(stderr,"Error allocating %s seg at %p: %d\n",
	    which,addr,errno);
    exit(1);
  }
  return ret;
}

/* ------------------------------------------------------------- */
void rel_mark(U32 pos,U32 kind){
  if(REL_FLAG) {
    bits_set(pos, kind);
  }
}

U32 seg_serialize(U32 fillptr,FILE*f){
  U32 addr = fillptr & 0xC0000000;
  U32 size = fillptr & 0x3FFFFFFF;  // byte size
  size_t wr1 = fwrite(PTR(U8*,addr),1,size,f);
  size_t wr2 = fwrite(PTR(U8*,(addr>>3)),1,(size>>3),f);
  printf("wrote: %lx and %lx\n",wr1,wr2);
  return(U32)(wr1+wr2);
}

/*----------------------------------------------------------------------------
Deserializing is made more difficult by not knowing the size of the segment.
So the first step is to maybe load a few bytes of a segment to get to the 
fillptr...  This amount in bytes is 'already'.
----------------------------------------------------------------------------*/

U32 seg_deserialize_start(U32 addr,U32 bytes,FILE*f){
  return fread(PTR(U8*,addr),1,bytes,f);
}

U32 seg_deserialize(U32 fillptr,FILE*f,U32 already){
  U32 addr = fillptr & 0xC0000000;
  U32 size = fillptr & 0x3FFFFFFF;  // byte size
  // read according to fillptr, but accounting already-read data
  size_t rd1 = fread(PTR(U8*,(addr+already)),1,(size-already),f);
  size_t rd2 = fread(PTR(U8*,(addr>>3)),1,(size>>3),f);
  printf("read: %lx and %lx\n",rd1+already,rd2);
  return(U32)(rd1+rd2);
}
