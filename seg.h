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

U32 mseg_delete(U32 hole, U32 holesize,
		U32 artzone,  U32 artfix);


U32 seg_reref(sSeg*psg,U32 old,U32 new);

sCons* mcons(U32 car, U32 cdr);
sCons* mins(sCons* after, sCons* it); 
typedef U64 (*mapcfun)(U32 cons); // return 0 to continue; val exits mapc
//U64 mapc(mapcfun fun,U32 list);
