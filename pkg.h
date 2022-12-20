// 

// 
sSym* pkg_find_hash_proc(sSym* s,U32 hash,sSym*prev);
sSym* pkg_find_hash(sSym* pk,U32 hash);
sSym* pkgs_find_hash(sSym*pk, U32 hash);
sSym* plst_find_hash(sSym* pk,U32 hash); 


sSym* pkg_find_name(sSym*pk, char*name);
sSym* pkgs_find_name(sSym*pk, char*name);

U64 find_global(char* name);

U32   pkg_find_prev_hash_proc(sSym* s,U32 hash, U32 prev);
sSym* pkg_find_prev_hash(sSym*pk, U32 hash);
sSym* pks_find_prev_hash(sSym*pk, U32 hash);
sSym* plst_find_prev_hash(sSym* pk,U32 hash);

sSym* pkg_new(char*name,char* extra);
void  pkg_push_sym(sSym* pkg, sSym* sym);

sSym* pkg_unlink(sSym* prev);
void srch_list_push(sSym* pk);


void  pkg_dump(sSym* pk);

U32   pkg_dump_protos_proc(sSym* s,FILE* f);
void  pkgs_dump_protos();
void  pkg_dump_protos(sSym*pk,FILE* f);
//sSym* pkg_unlink_sym(sSym*pk, sSym* sym);


sSym* pkg_from_libtxt(char*name, char* dlpath,char*path);
void pkg_rebind(sSym* pk);




void srch_list_pkgs_ls();
sSym* srch_list_find_pkg(U32 hash);
sSym* srch_list_prev_pkg(sSym* pkg);

void pkg_incorporate(sSym* new); 


// assembly-coded


typedef U8 (*pkgfun2)(sSym* sym,U64 parm,sSym* prev);

sSym* pkg_walk(sSym*pkg, U64 parm,void* fun);
sSym* pkg_walk_U32(sSym*pkg, U32 parm,void* fun);


sSym* pkgs_walk(sSym* pkgk, U64 parm, void* fun);
sSym* pkgs_walk_U32(sSym* pkgk, U32 parm, void* fun);

sSym* plst_walk(sSym* pkgk, U64 parm, void* fun);
sSym* plst_walk_U32(sSym* pkgk, U32 parm, void* fun);
/*
typedef sSym* (*apkgfun)(sSym* sym,U64 p0, U64 p1, U64 p2);
U64 apkg_walk(sSym* pkg,U64 parm0, U64 parm1, U64 parm2, apkgfun fun);
sSym* pk_find_hash1(sSym* pk,U64 hash,U64 parm1, U64 parm2);


;;sSym* apkg_walk2(sSym* pkg,U64 parm0,pkgfun2 fun);
*/
