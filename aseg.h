/* ==============================================================

Artifact segments, for code and data...  While it's tempting to
make segments uniform, we want to avoid fill pointer in code seg;
and want the option of loading code and data without meta, so we 
are stuck with fill pointers in the data segment...

Code is at $40000000  read,write,execute
Data is at $80000000  read,write

Special A32 32-byte pointers:
$80000000 CFILL  fill pointer for the code segment 
$80000004 DFILL  fill pointer for the data segment 
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
#define CODE_SEG_ADDR 0x40000000
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
U32  cseg_append(U8* start,U64 size);

void rel_mark(U32 pos,U32 kind);

