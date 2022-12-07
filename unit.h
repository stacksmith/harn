// symtab.h

/* A unit is an occupied range of bytes in code and data segs, along with
   symbols that describe the data contained within.

   The symbols are optimized for fast searches, and are stored in 3 separate 
   arrays:
   * An array of 32-bit hashes for a cache-optimized linear search;
   * An array of per-symbol structures, with
   ** position in seg
   ** type
   ** string-table offset
   * A string table containing null-terminated strings;


 Symbols are maintained in 
 * A string-list with 0-terminated symbol names;
 * A hash-list with 32-bit hashes of each name;
 * A symdata-list, with matching data:
 ** type  obj,func,file                    4
 ** vis   local,global                     1
 ** pad
 ** str   offset to string table          16
 ** doff  data offset in current unit     32
 */

typedef struct sSym {
  union {
    U64 value;
    struct {
      U32 seg:1;    //0=data, 1=code
      U32 type:15;   //TODO: visibility, etc
      U32 ostr:16;  //string table offset
      U32 off;
    };
  };
} sSym;

/* maybe, fit into a U64:
  4 flags type,visibility
 16 ostr  string table index (64K max)
 20 size  1MB max item size
 24 off   16MB+1MB max total limit

#define SYM_OTHER 0
#define SYM_DATA 1
#define SYM_FUNC 2
#define SYM_LOCAL  0
#define SYM_GLOBAL 1
*/
  
typedef struct sUnit {
      // segment data
  U32 oCode;       // code segment offset 
  U32 szCode;      // size of code
  U32 oData;       // data segment offset
  U32 szData;
  // symbol data
  char*  strings;  // TODO: for now malloc'ed...
  U32*   hashes;   // TODO: for now malloc'ed...
  sSym*  dats;     // TODO: for now malloc'ed...
  U32    nSyms;
  U32    nGlobs;   // global symbols come first;
} sUnit;

  
void unit_dump(sUnit* pu);
char* unit_name(sUnit* pu);
void unit_sections_from_elf(sUnit*pu,sElf* pelf);
void unit_symbols_from_elf(sUnit*pu,sElf* pelf);
sUnit* unit_ingest_elf1(sElf* pelf,char* path);
void   unit_ingest_elf2(sUnit* pu,sElf* pelf);
U32 unit_elf_resolve(sElf*pelf,pfresolver resolver);

U32    unit_find_hash(sUnit*pu,U32 hash);


//void unit_lib(sUnit*pu, char* name,U32 num, void**funs, char**names); 
void unit_lib(sUnit*pu,void* dlhan, U32 num,char*namebuf);
