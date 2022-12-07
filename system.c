#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "global.h"
#include "util.h"
#include "elf.h"
#include "elfdump.h" 
#include "seg.h"
#include "unit.h"
#include "lib.h"
#include "system.h"

extern sSystem sys;
extern sSeg scode;
extern sSeg sdata;

typedef U32 (*pfsys_iter)(sUnit* pu);

void sys_units_iter(pfsys_iter proc){
  sUnit** pulist = sys.units;
  sUnit* pu;
  while((pu=*pulist++)){
    if ((*proc)(pu)){
      return;
    }
  } 
}
void sys_dump(){
  seg_dump(&scode);
  seg_dump(&sdata);
  printf("%d Units: ",sys.nUnits);
  U32 proc(sUnit* pu){
    //printf("%s ",unit_name(pu));
    unit_dump(pu);
    return 0;
  }
  sys_units_iter(&proc);
  puts(".");
}


void sys_init(){
  size_t size = (64 * sizeof(sUnit*));
  sys.units = (sUnit**)malloc(size);
  memset(sys.units,0,size);
  sys.nUnits = 0;
}

void sys_add(sUnit* pu){
  sys.units[sys.nUnits++] = pu;
}

sUnit* sys_find_hash(U32 hash,U32* pi){
  sUnit** pulist = sys.units;
  sUnit* pu;
  U32 index;
  while((pu=*pulist++)){
    if((index = unit_find_hash(pu,hash))){
      *pi=index;
      return pu;
    }
  }
  return 0;
}

U64 sys_symbol_address(char* name){
  U32 i;
  sUnit* pu = sys_find_hash(string_hash(name),&i);
  if(!pu){
    return 0;
  }
//      printf("Undefined %s found: %lx\n",name,i);
  return pu->dats[i].off; // set elf sym value
}

sUnit* sys_load_elf(char* path){
  sElf* pelf = elf_new();
  sUnit* pu = unit_ingest_elf1(pelf,path);
  
  U32 unresolved = unit_elf_resolve(pelf,&sys_symbol_address);
  if(unresolved){
    fprintf(stderr,"Undefined symbol '%s'\n",path);
    exit(1);
  }    
  
  unit_ingest_elf2(pu,pelf);
  
  sys_add(pu);
  
  //elf_build_hashlist(pelf);
  //Elf64_Sym* psym = elf_find(pelf,string_hash("puts"));
  //  sym_dump(pelf,psym);
  
  free(pelf);
  return pu;
}

/*
// using elvs
void sys_load_two(char* path1, char* path2){
  sElvs elvs;
  char* paths[3]={path1,path2,0};
  elvs_init(&elvs,2,paths);

  // resolve each unit to its own symbols and globals
  U32 un = elvs_resolve_symbols(&elvs);
  // reslove each unit against all Elf symbols
  printf("unresolveds: %d \n",un);
  un = elvs_resolve_undefs(&elvs);
    
  printf("unresolveds: %d\n ",un);

  elvs_step2(&elvs);

  elvs_dump(&elvs);
  elvs_delete(&elvs);
}
*/
/*
void sys_load_mult(U32 cnt,char** paths){
  sElvs elvs;
  elvs_init(&elvs,cnt,paths);
  
  // resolve file-locals and system globals
  U32 un = elvs_resolve_symbols(&elvs);
  printf("1.unresolveds: %d \n",un);
  // reslove each unit against all Elf symbols
  un = elvs_resolve_undefs(&elvs);
  printf("2.unresolveds: %d\n ",un);
  elvs_step2(&elvs);
  //  elvs_dump(&elvs);
  elvs_delete(&elvs);
}
  */
