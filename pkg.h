// smSymb.h

// smSymbs are symbols stored in the meta segment.


void pk_dump(sCons* pk);
sCons* pk_new(char*name);
void pk_push_sym(sCons* pkg, sSym* sym);
//void pk_wipe_last_sym(sSym* pkg);
sSym* pk_find_hash(sCons* pk,U32 hash);
sSym* pk_find_name(sCons*pk, char*name);
sCons* pk_find_prev_hash(sCons*pk, U32 hash);

void pk_dump_protos(sCons*pk,FILE* f);
sSym* pk_unlink_sym(sCons*pk, sSym* sym);

//sSym* pks_find_hash(sCons*pk, U32 hash,sCons** in);
//sSym* pks_find_name(sCons*pk, char*name,sCons** in);

sSym* pks_find_hash0(sCons*pk, U32 hash);
sSym* pks_find_name0(sCons*pk, char*name);

//sCons* pks_find_prev_hash(sCons*pk, U32 hash,sCons**in);
sCons* pks_find_prev_hash0(sCons*pk, U32 hash);
void pks_dump_protos();



U64 find_global(char* name);

sCons* pk_from_libtxt(char*name, char*path);
void pk_rebind(sCons* pk,char*dllpath);


void srch_list_push(sCons* pk);

void pk_incorporate(sSym* new); 
