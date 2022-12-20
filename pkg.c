#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"

#include "seg.h"
#include "aseg.h"
#include "sym.h"
#include "pkg.h"

/*----------------------------------------------------------------------------

 Physically symbols are grouped into packages,

A package is a group of related symbols, kept in a linked list. 

sym          a named symbol corresponding to an artifact in code or data area;

head         a special symbol used to hold a package, a linked list of 
             symbols which are attached to this head.  The head is not 
             a part of the package, but rather contains a package.

pkg          a conceptual idea of the contents of the head, or a bunch of
             related symbols, or a group of symbols anchored to the head.

plist        a separately linked grouping of heads creating a combined 
             namespace.




----------------------------------------------------------------------------*/

// Some procs used by the pkg_walk and pkgs_walk



/*----------------------------------------------------------------------------

  S
  Symbols are accessible via walkers or iterators set up to visit groupings.
  Walkers are started with a 'proc', which are called for every item in the
  group.  Different procs are developed for particulaar operations (although
  procs can often be reused).  Upon completion of its task on a single item,
  a proc returns 0 to continue, or any other value to exit, in which case
  the walker stops and returns that value.

  There are curently three walkers

  * a package walker iterates on all symbols within a package;
  * a namespace walker visits all packages 
  * a package is searchable via its head an `pkg_walk` iterator
  * the entire namespace is 



  ----------------------------------------------------------------------------*// 
sSym* pkg_find_hash_proc(sSym* s,U32 hash,sSym*prev){
  return (s->hash == hash) ? s : 0;
}

sSym* pkg_find_hash(sSym* s,U32 hash){
  return pkg_walk_U32(s,hash,pkg_find_hash_proc);
}

sSym* pkgs_find_hash(sSym* pk,U32 hash){
  return pkgs_walk_U32(pk,hash,pkg_find_hash_proc);
}

sSym* plst_find_hash(sSym* pk,U32 hash){
  return plst_walk_U32(pk,hash,pkg_find_hash_proc);
}



/*----------------------------------------------------------------------------
  .._find_prev_hash

----------------------------------------------------------------------------*/


sSym* pkg_find_name(sSym*s, char*name){
  return pkg_find_hash(s,string_hash(name));
}

sSym* pkgs_find_name(sSym*pk, char*name){
  return pkgs_find_hash(pk,string_hash(name));
}


U64 find_global(char*name){
  sSym* sym = pkgs_find_name(PTR(sSym*,META_SEG_ADDR),name);
  if(sym)
    return sym->art;
  else
    return 0;
}






/*----------------------------------------------------------------------------
  .._find_prev_hash

----------------------------------------------------------------------------*/
U32 pkg_find_prev_hash_proc(sSym* s,U32 hash, U32 prev){
  return (s->hash == hash) ?  prev : 0;
}

sSym* pkg_find_prev_hash(sSym* s,U32 hash){
  return pkg_walk_U32(s,hash,pkg_find_prev_hash_proc);
}

sSym* pkgs_find_prev_hash(sSym* pk,U32 hash){
  return pkgs_walk_U32(pk,hash,pkg_find_prev_hash_proc);
}


sSym* plst_find_prev_hash(sSym* pk,U32 hash){
  return plst_walk_U32(pk,hash,pkg_find_prev_hash_proc);
}


/*----------------------------------------------------------------------------
  package insertion and suck

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

// insert a package into srch_list
void srch_list_push(sSym* pk){
  pk->art = SRCH_LIST;  // next package;
  SRCH_LIST = THE_U32(pk);
}


void srch_list_pkgs_ls(void){
  U32 proc(sSym* pk){
     printf("%s ",SYM_NAME(pk));
    return 0;
  }
  plst_walk_U32(WALK_SRCH_LIST, 0,proc);
  printf("\n"); 
}


  
void pkg_dump(sSym*s){ 
  pkg_walk(s,0,sym_dump1);
}

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
  FILE* f = fopen("sys/headers.h","w");
  sSym* pk = WALK_SRCH_LIST;
  pkgs_walk (pk, (U64)(f), pkg_dump_protos_proc);
  fclose(f);
}


/*----------------------------------------------------------------------------
 pkgs  global namespace operations on all visible packages.  Use same procs
       as pkg_walk

----------------------------------------------------------------------------*/










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
  sSym* prev = pkgs_find_prev_hash(WALK_SRCH_LIST, new->hash);
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
