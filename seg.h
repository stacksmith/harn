/* ==============================================================

A fillable area for code or data

  ==============================================================*/
typedef struct sSeg {
  U8* base; // ptr of allocation/mapping
  U8* prel;  // allocated rel table
  U32 fill;
  U32 end;
  char name[8];
} sSeg;
// check which seg 
#define IN_CODE_SEG(addr) ((addr) & 0x80000000)
#define IN_DATA_SEG(addr) ((addr) & 0x40000000)
// to check if two addrs are in the same seg, for instance:
#define SEG_BITS(addr) ((addr) & 0xC0000000)

void seg_dump(sSeg* pseg);
int  seg_alloc(sSeg* pseg,char*name,U64 req_size, void* req_addr, U32 prot);
U32  seg_pos(sSeg* pseg);
U32  seg_off(sSeg* pseg);

void seg_align(sSeg*pseg, U64 align);
U8*  seg_append(sSeg* pseg,U8* start,U64 size);

void seg_rel_mark(sSeg*pseg,U32 pos,U32 kind);

U32 cnt_refs(sSeg*pseg,void* target);
//U32 seg_reref(sSeg*pseg,U64 old,U64 new);
void seg_reref(sSeg*pseg,U32 old,U32 new);
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
