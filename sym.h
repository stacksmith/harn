// smSymb.h

// smSymbs are symbols stored in the meta segment.


typedef struct sSym {
  U32 next;               // fixed up upon local moves only
  U32 hash;
  union {
    U64 bounds;
    struct {
      U32 data;               // local, code, or data  data, must fix
      U32 size;
    };
  };
  U32 src;                // file offset
  U8  octs;               // total size in blocks of 8.
  U8  namelen;
} sSym;

#define SYM_NAME_OFF 22
#define SYM_NAME(sym)  (((char*)(sym))+SYM_NAME_OFF)
#define U32_SYM(usym) ((sSym*)(U64)(usym))
// name
// 0
// prototype
// 0
//smSymb* smSymb_new(char* name,U32 data,U32 size);
//void    smSymb_set_src(smSymb* symb,U32 src, U32 srclen);
//void    smSymb_set_proto(smSymb* symb,char* proto);
sSym* sym_new(char* name, U32 data, U32 size, U32 src,char* proto);
char* sym_proto(sSym* sym);
void sym_dump1(sSym* sym);
sSym* sym_from_siSymb(void* symb);

void pk_dump(sSym* pk);
sSym* pk_new(char*name);
void pk_push_sym(sSym* pkg, sSym* sym);
void pk_wipe_last_sym(sSym* pkg);
sSym* pk_find_hash(sSym* pk,U32 hash);
sSym* pk_find_name(sSym*pk, char*name);

sSym* pks_find_hash(sSym*pk, U32 hash);
sSym* pks_find_name(sSym*pk, char*name);
void pk_dump_protos(sSym* pk,FILE* f);
void pks_dump_protos();

U64 find_global(char* name);

sSym* pk_from_libtxt(char*name, char*path);
void pk_rebind(sSym* pk,char*dllpath);

void srch_list_push(sSym* pk);
