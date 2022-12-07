// symtab.h

/* A package is a collection of symbolic names, each resolving to a memory interval in data and code.

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

// a primitive way to keep symbols

typedef struct sDataSize {
  U32 data;
  U32 size;
} sDataSize;

struct siSymb;
typedef struct siSymb {
  struct siSymb* next;
  char* name;
  union {
    sDataSize ds;
    struct {
      U32 data;
      U32 size;
    };
  };
  U32 hash;
} siSymb;

typedef struct sPkg {
  siSymb* data;
} sPkg;

void pkg_dump(sPkg* pkg);

void pkg_init(sPkg* pkg,char* name);
void pkg_add(sPkg* pkg,char*name,U32 data,U32 size);
sDataSize pkg_find_name(sPkg* pkg,char*name);
sDataSize pkg_find_hash(sPkg* pkg,U32 hash);

/*
void pkgunit_dump(sUnit* pu);
char* unit_name(sUnit* pu);
void unit_sections_from_elf(sUnit*pu,sElf* pelf);
void unit_symbols_from_elf(sUnit*pu,sElf* pelf);
sUnit* unit_ingest_elf1(sElf* pelf,char* path);
void   unit_ingest_elf2(sUnit* pu,sElf* pelf);
U32 unit_elf_resolve(sElf*pelf,pfresolver resolver);

U32    unit_find_hash(sUnit*pu,U32 hash);


//void unit_lib(sUnit*pu, char* name,U32 num, void**funs, char**names); 
void unit_lib(sUnit*pu,void* dlhan, U32 num,char*namebuf);
*/
