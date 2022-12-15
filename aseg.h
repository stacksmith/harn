/* ==============================================================

A special segment at $80000000, containing data segment above,
extending up, and code segment below, extending down.  The code 
area is mapped as executable.

Special A32 32-byte pointers:
$80000000 CFILL  fill pointer for the code segment (down!)
$80000004 DFILL  fill pointer for the data segment (up)
  ==============================================================*/
// segment header at the start of each segment
//

#define CFILL_ADDR ((U32*)0x80000004L)
#define DFILL_ADDR ((U32*)0x80000000L)
#define CFILL (*CFILL_ADDR)
#define DFILL (*DFILL_ADDR)
#define DFILL_PTR ((U8*)(U64)(DFILL))
#define CFILL_PTR ((U8*)(U64)(CFILL))

#define CODE_SEG_SIZE 0x10000000
#define CODE_SEG_ADDR 0x70000000
#define DATA_SEG_SIZE 0x10000000
#define DATA_SEG_ADDR 0x80000000

// check which seg
#define SMETA_BASE 0xC0000000
#define REL_FLAG (*(U32*)(U64)(0xC000000C))

#define IN_CODE_SEG(addr) (0x40000000 == ((addr) & 0xC0000000))
#define IN_DATA_SEG(addr) (0x80000000 == ((addr) & 0xC0000000))
#define IN_META_SEG(addr) (0xC0000000 == ((addr) & 0xC0000000))
// to check if two addrs are in the same seg, for instance:
#define SEG_BITS(addr) ((addr) & 0xC0000000)

void aseg_dump();
void aseg_alloc();

void dseg_align8();
U32  dseg_append(U8* start,U64 size);
void cseg_align8();
U32  cseg_prepend(U8* start,U64 size);

void rel_mark(U32 pos,U32 kind);

