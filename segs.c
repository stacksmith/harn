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

/*==============================================================
segs_reref          replace all references to 'from' with 'to'.

==============================================================*/

extern sSeg* psCode;
extern sSeg* psData;
extern sSeg* psMeta;
// Reref data and code segments.  Leave the metadata alone.
U32 segs_reref(U32 old, U32 new){
  U32 ret = 0;
  ret += seg_reref(psCode, old, new); // in code seg
  ret += seg_reref(psData, old, new); // in data seg
  //  ret += seg_reref(psMeta, old, new); // in meta seg
  return ret;
}
