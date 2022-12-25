#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"

#include "seg_meta.h"
#include "seg_art.h"
#include "sym.h"
#include "pkg.h"

/*----------------------------------------------------------------------------

 Physically symbols are grouped into packages,

A package is a group of related symbols, kept in a linked list. 

sym          a named symbol corresponding to an artifact in code or data area;
             representation: sSym

head         a special symbol used to hold a package, a linked list of 
             symbols which are attached to this head.  The head is not 
             a part of the package, but rather contains a package.
	     repr: sSym with art -> next head...

pkg          a conceptual idea of the contents of the head, or a bunch of
             related symbols, or a group of symbols anchored to the head.
	     repr: sSym; rather its cdr list of sSyms

ns           a namespace, or a group of packages.
             repr: a linked list of heads (via art)

GNS          global namespace, a namespace pointed to by the GNS variable.
             repr: global pointer GNS


Typically, there are three kinds of things we do:
* Operate on symbols of a package;
* Operate on all symbols of a namespace;
* Operate on packages of a namespace.

These access patterns are abstracted into three iterators, or walkers, which
call a proc on every visited symbol (and if it returns a value, abort walk 
and return the value.  See pkgasm.asm
----------------------------------------------------------------------------*/
  

sSym* pkg_new(char*name,char* opt){
  return sym_new(name,0,0,0,opt);
}

// insert symbol into package
void pkg_push_sym(sSym* pk, sSym* sym){
  sym->cdr = pk->cdr;
  pk->cdr = THE_U32(sym);
}
// unlink prev's next, and return it
sSym* pkg_unlink(sSym* prev){
  sSym* it = PTR(sSym*,prev->art);
  if(it) 
    prev->art = it->art;
  return it;                           //may be 0 if prev was last
}

void pkg_dump(sSym*s){ 
  pkg_walk(s,0,sym_dump1);
}
/*----------------------------------------------------------------------------
  ns_pkg_push             pre-pend pkg into global namespace

----------------------------------------------------------------------------*/
void ns_pkg_push(sSym* pk){
  pk->art = GNS;  // next package;
  GNS = THE_U32(pk);
}
/*----------------------------------------------------------------------------
  ns_pkg_ls               show all packages in namespace

----------------------------------------------------------------------------*/
void ns_pkgs_ls(void){
  U32 proc(sSym* pk){
     printf("%s ",SYM_NAME(pk));
    return 0;
  }
  plst_walk_U32(GNS_AS_SYM, 0,proc);
  printf("\n"); 
}






/*----------------------------------------------------------------------------
  find_hash                         Locate a has within a package or ns...

----------------------------------------------------------------------------*/
sSym* pkg_find_hash_proc(sSym* s,U32 hash,sSym*prev){
  return (s->hash == hash) ? s : 0;
}

sSym* pkg_find_hash(sSym* s,U32 hash){
  return pkg_walk_U32(s,hash,pkg_find_hash_proc);
}

sSym* ns_find_hash(sSym* pk,U32 hash){
  return pkgs_walk_U32(pk,hash,pkg_find_hash_proc);
}
// this one finds the package name in namespace!
sSym* ns_find_pkg_hash(sSym* pk,U32 hash){
  return plst_walk_U32(pk,hash,pkg_find_hash_proc);
}
/*----------------------------------------------------------------------------
  

----------------------------------------------------------------------------*/


sSym* pkg_find_name(sSym*s, char*name){
  return pkg_find_hash(s,string_hash(name));
}

sSym* ns_find_name(sSym*pk, char*name){
  return ns_find_hash(pk,string_hash(name));
}


U64 find_global(char*name){
  sSym* sym = ns_find_name(PTR(sSym*,META_SEG_ADDR),name);
  if(sym)
    return sym->art;
  else
    return 0;
}






/*----------------------------------------------------------------------------
  find_prev_hash                  Find the symbol whse link points to
                                  matching symbol
----------------------------------------------------------------------------*/
U32 pkg_find_prev_hash_proc(sSym* s,U32 hash, U32 prev){
  return (s->hash == hash) ?  prev : 0;
}

sSym* pkg_find_prev_hash(sSym* s,U32 hash){
  return pkg_walk_U32(s,hash,pkg_find_prev_hash_proc);
}

sSym* ns_find_prev_hash(sSym* pk,U32 hash){
  return pkgs_walk_U32(pk,hash,pkg_find_prev_hash_proc);
}

// this one finds the previous pkg whose art points at matching pkg!
sSym* ns_find_prev_pkg_hash(sSym* pk,U32 hash){
  return plst_walk_U32(pk,hash,pkg_find_prev_hash_proc);
}




/*----------------------------------------------------------------------------
  dump_protos                 if there is a prototype, dump it.
   
This is used to make a prototype file prior to invoking the C compiler...
----------------------------------------------------------------------------*/

U32 pkg_dump_protos_proc(sSym* s,FILE* f){
  char* proto = sym_proto(s);
  if(*proto)
    fprintf(f,"%s\n",proto);
  return 0;
}

void pkg_dump_protos(sSym* s,FILE* f){
  pkg_walk(s, (U64)(f), pkg_dump_protos_proc);
}

void pkgs_dump_protos(){
  FILE* f = fopen("sys/headers.h","w");;
  pkgs_walk (GNS_AS_SYM, (U64)(f), pkg_dump_protos_proc);
  fclose(f);
}










char* next_line(char* p){
  p = strchr(p,10);
  if(p) 
    *p++ = 0;
  return p;
}
// using a text file at path to initialize names of dl symbols...
// the library package is not actually linked to the system -
// pk_rebind must be invoked to bind to the current addresses after
// a load...
sSym* pkg_from_libtxt(char* name,char* dlpath,char*path){
  // a 12-byte prototype of an assembly trampoline
  U8 ljump[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0 }; // jmp rax
  
  char* buf = filebuf_malloc(path);
  if(!buf)
    return 0;
  sSym* pk = pkg_new(name,dlpath); 
  char* pc = buf;
  char* next;
  while((next = next_line(pc))){
    U32 addr = cseg_append(ljump,sizeof(ljump));
    cseg_align8();
    rel_mark( addr+2, 3); // mark the 64-bit address
    sSym* sy = sym_new(pc,addr,sizeof(ljump),0,0);
    pkg_push_sym(pk,sy) ;
    pc = next;
  }
  filebuf_free(buf);
  return pk;
  
}
void pkg_rebind(sSym* s){

  printf("rebinding package %p\n",s);
  char* dllpath = sym_proto(s);
  void* dlhan = dlopen(dllpath,RTLD_NOW);
  if(!dlhan){
    fprintf(stderr,"Unable to open dll %s\n",dllpath);
    exit(1);
  }
  
  while((s = PTR(sSym*,s->cdr))){
    void* pfun = dlsym(dlhan,SYM_NAME(s));
    if(!pfun)
      printf("pk_rebind: could not bind to %s\n",SYM_NAME(s));
    *((void**)(U64)(s->art+2)) = pfun; // fixup to function address
    dlclose(dlhan);
  }
}


/*----------------------------------------------------------------------------
  pk_incorporate            Given a new, unlinked symbol, make sure it is
                             a valid new symbol, or a valid replacement of
                             an old one (in which case, replace!)

This is particularly tricky, as:

----------------------------------------------------------------------------*/
void pkg_incorporate(sSym* new){
  // search for previous symbos of same name.  New is not linked!  Get prev.
  sSym* prev = ns_find_prev_hash(GNS_AS_SYM, new->hash);
  //  printf("pkg_inc prev %p   new: %p\n",prev,new);
  if(!prev) { // no previous versions; simply link it into 
     pkg_push_sym(TOP_PKG,new);   
  } else {  // so,   <prev>---<old>---, and we want to replace old...
    sSym* old = PTR(sSym*,prev->cdr);  
    //printf("prev: %p, old: %p\n",prev,old);
    // replacing symbol must be in same seg
    if( (SEG_BITS(old->art)) == (SEG_BITS(new->art))){
      new->cdr = old->cdr;     // point our cdr at whatever old one was
      prev->cdr = THE_U32(new); // and set prev to us, unlinking old
      U32 fixups = aseg_reref(old->art,new->art); // fix old refs to us
  //  printf("%d old refs fixed from %08X to %08X\n",fixups, old->art, new->art);
      //      hd(PTR(void*,old->art),4);
      fixups += sym_del(old);        // get rid of odl
      printf("%d fixups\n",fixups);
    } else {
      printf("!!! SEGMENT MISMATCH! TODO: FIX!!!!\n");
      //segment mismatch..
    }
  }

}
