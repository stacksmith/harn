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
#include "aseg.h"

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
