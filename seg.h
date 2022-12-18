/* ==============================================================

A fillable area for code or data

  ==============================================================*/
#include "seg_common.h"

typedef struct sSeg {
  U32 fill;
  U32 end;
  U32 srchlist;
  U32 rel_flag; 
} sSeg;

// check which seg

void mseg_dump();
void mseg_alloc();
//void seg_serialize(sSeg* psg,FILE* f);
//void seg_deserialize(sSeg* psg,FILE* f);


void mseg_align8();
U8*  mseg_append(U8* start,U32 size);



U32 seg_reref(sSeg*psg,U32 old,U32 new);

sCons* mseg_cons(U32 car, U32 cdr);
