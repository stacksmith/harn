// smSymb.h

// smSymbs are symbols stored in the meta segment.


void pk_dump(sSym* pk);
sSym* pk_new(char*name,char* extra);
void pk_push_sym(sSym* pkg, sSym* sym);
//void pk_wipe_last_sym(sSym* pkg);
sSym* pk_find_hash(sSym* pk,U32 hash);
sSym* pk_find_name(sSym*pk, char*name);
sSym* pk_find_prev_hash(sSym*pk, U32 hash);

void pk_dump_protos(sSym*pk,FILE* f);
sSym* pk_unlink_sym(sSym*pk, sSym* sym);

//sSym* pks_find_hash(sSym*pk, U32 hash,sSym** in);
//sSym* pks_find_name(sSym*pk, char*name,sSym** in);

sSym* pks_find_hash(sSym*pk, U32 hash);
sSym* pks_find_name(sSym*pk, char*name);

//sSym* pks_find_prev_hash(sSym*pk, U32 hash,sSym**in);
sSym* pks_find_prev_hash(sSym*pk, U32 hash);
void pks_dump_protos();



U64 find_global(char* name);

sSym* pk_from_libtxt(char*name, char* dlpath,char*path);
void pk_rebind(sSym* pk);


void srch_list_push(sSym* pk);
void srch_list_pkgs_ls();
sSym* srch_list_find_pkg(U32 hash);
sSym* srch_list_prev_pkg(sSym* pkg);

void pk_incorporate(sSym* new); 


// assembly-coded
sSym* pkg_find_hash(sSym* pkg, U32 hash);

typedef U8 (*pkgfun2)(sSym* sym,U64 parm,sSym* prev);

sSym* pkg_walk(sSym*pkg, U64 parm,void* fun);
sSym* pkg_walk_U32(sSym*pkg, U32 parm,void* fun);

sSym* pkgs_walk(sSym* pkgk, U64 parm, void* fun);
/*
typedef sSym* (*apkgfun)(sSym* sym,U64 p0, U64 p1, U64 p2);
U64 apkg_walk(sSym* pkg,U64 parm0, U64 parm1, U64 parm2, apkgfun fun);
sSym* pk_find_hash1(sSym* pk,U64 hash,U64 parm1, U64 parm2);


;;sSym* apkg_walk2(sSym* pkg,U64 parm0,pkgfun2 fun);
*/
