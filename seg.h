/* ==============================================================

A fillable area for code or data

  ==============================================================*/
typedef struct sSeg {
  U8* base; // ptr of allocation/mapping
  U32 fill;
  U32 end;
  char name[8];
} sSeg;

void seg_dump(sSeg* pseg);
int  seg_alloc(sSeg* pseg,char*name,U64 req_size, void* req_addr, U32 prot);
void seg_align(sSeg*pseg, U64 align);
U8*  seg_append(sSeg* pseg,U8* start,U64 size);

/* ==============================================================

A unit represents a compilation unit brought into the system.

  ==============================================================*/
/*
typedef struct sUnit {
  U32 code_start;
  U32 code_size;
  
  U32 data_start;
  U32 data_size;
} sUnit;
*/
