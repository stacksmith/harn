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
void seg_dump(sSeg* pseg){
  printf("Segment %s: %p %08x\n",pseg->name, pseg->base, pseg->fill);
}
/* -------------------------------------------------------------
   seg_alloc

---------------------------------------------------------------*/
int seg_alloc(sSeg* pseg,char*name,U64 req_size, void* req_addr, U32 prot){
  pseg->base =mmap(req_addr,
		   req_size,
		  prot,
		  //MAP_ANONYMOUS|
		  0x20|  MAP_SHARED|MAP_FIXED,
		  0,0);
  if(MAP_FAILED == pseg->base){
    fprintf(stderr,"Error allocating %s seg: %d\n",name,errno);
    exit(1);
  } else {
    memcpy(pseg->name,name,7);
    pseg->name[7]=0;
    pseg->fill = 0;
    pseg->end = (U32)req_size;
    return 0;
  }
}
/* -------------------------------------------------------------
seg_append  Append a run of bytes to the segment
            size   number of bytes to append
            start  address from which to copy.  
                   if 0, fill with 0 bytes.
---------------------------------------------------------------*/
U8* seg_append(sSeg* pseg,U8* start,U64 size){
  U32 end = pseg->fill + size;
  U8* dest = pseg->base + pseg->fill;
  if(end >= pseg->end) {
    seg_dump(pseg);
    fprintf(stderr,"seg_append failed: out of space\n");
    exit(1);
  } else {
    pseg->fill = end;
    if(start)
      memcpy(dest,start,size);
    else
      memset(dest,0,size);
  } 
  return dest;
}

void seg_align(sSeg*pseg, U64 align){
  int rem  = pseg->fill % (U32)align;
  if(rem) {
    seg_append(pseg,0,align-rem);
    printf("Inserted pad of %ld bytes\n",align-rem);
  }  
}

