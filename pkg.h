// smSymb.h

// smSymbs are symbols stored in the meta segment.

#define SRCH_LIST (*(U32*)(U64)(0xC0000008))

void pk_dump(sSym* pk);
sSym* pk_new(char*name);
void pk_push_sym(sSym* pkg, sSym* sym);
//void pk_wipe_last_sym(sSym* pkg);
sSym* pk_find_hash(sSym* pk,U32 hash);
sSym* pk_find_name(sSym*pk, char*name);
sSym* pk_find_prev_hash(sSym*pk, U32 hash);

void pk_dump_protos(sSym* pk,FILE* f);
sSym* pk_unlink_sym(sSym* pk, sSym* sym);

sSym* pks_find_hash(sSym*pk, U32 hash,sSym** in);
sSym* pks_find_name(sSym*pk, char*name,sSym** in);
sSym* pks_find_prev_hash(sSym*pk, U32 hash,sSym**in);
void pks_dump_protos();



U64 find_global(char* name);

sSym* pk_from_libtxt(char*name, char*path);
void pk_rebind(sSym* pk,char*dllpath);


void srch_list_push(sSym* pk);
