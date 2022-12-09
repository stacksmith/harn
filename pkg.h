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
  char* proto;
  U32 hash;
  
} siSymb;

typedef struct sPkg {
  struct sPkg* next;
  siSymb* data;
  char* name;
} sPkg;

void pkg_dump(sPkg* pkg);

sPkg* pkg_new();
void pkg_set_name(sPkg* pkg,char* name);
siSymb* pkg_add(sPkg* pkg,char*name,U32 data,U32 size);
siSymb* pkg_symb_of_hash(sPkg* pkg,U32 hash);
siSymb* pkg_symb_of_name(sPkg* pkg,char* name);
void pkg_lib(sPkg* pkg,char*dllpath,char*namespath);
void pkg_load_elf(sPkg* pkg,char* path);

void pkgs_add(sPkg* pkg);
void pkgs_list();
siSymb* pkgs_symb_of_name(char* name);
