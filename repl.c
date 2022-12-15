#include <stdio.h>
#include <stdlib.h> //system
#include <string.h>
#include <ctype.h> // for isspace
#include "global.h"
#include "util.h"
#include "elf.h"
#include "seg.h"
#include "aseg.h"
//#include "pkgs.h"
#include "src.h"
#include "sym.h"
#include "pkg.h"

sSym* ing_elf(sElf* pelf);
U64 ing_elf_func(sElf* pelf);

extern sSeg* psMeta;


typedef void (*fpreplfun)(char*p);
typedef U64 (*fpvoidfun)();

void exec_repl_sym(sSym* sym,char*p){
  U32 addr = sym->art;
  if(IN_CODE_SEG(addr)){
    //    printf("found %s\n",funname);
    fpreplfun entry = (fpreplfun)(U64)(addr);
    (*entry)(p);
  } else {
    printf("can't run data %s\n",SYM_NAME(sym));
  }
}

void run_repl_fun(U32 hash,char*p){
  sSym* sym = pks_find_hash((sSym*)(U64)SRCH_LIST,hash,0);
  if(sym){  //
    exec_repl_sym(sym,p);
  } else {
    printf("can't find it\n");
  }
}





void edit(char* name){
  printf("Editing [%s].. type 'cc' to compile when done...\n",name);
  /*
  siSymb* symb =  pkgs_symb_of_name(name);
  // write headers for packages in use
  pkgs_dump_protos();

  siSymb_src_to_body(symb);
  */
}

sElf* rebuild(char* name,U32 need_function){
  pks_dump_protos();
  char buf[256];
  sprintf(buf,"cd sys; ./build.sh %s",name);
   //puts(buf);
  int ret = system(buf);
  if(ret){
    printf("OS shellout to compiler failed! Build Error %d\n",ret);
    return 0;
  }
  sprintf(buf,"sys/%s.o",name);
  sElf* pelf = elf_load(buf);
  if(!pelf->unique){
    printf("ing_elf: no unique global symbol\n");
    elf_delete(pelf);
    return 0;
  }
  if(need_function && (ELF64_ST_TYPE(pelf->unique->st_info) != STT_FUNC)){
    printf("repl_expr: must be a functional expression\n");
    elf_delete(pelf);
    return 0;
  }
  return pelf;
}
/*----------------------------------------------------------------------------
replace-old-symbol

We just compiled something and generated a new symbol (but haven't attached it
to the pkg).
* Remove old sym from meta segment.  GC the hole, fixing up only meta seg!
* remove old artifact from its segment. fixup code and data.
----------------------------------------------------------------------------*/

void new_symbol(sSym* new){
      // do we already have a symbol with same name?  Hold onto it.
  sSym* pkg;
  sSym* prev = pks_find_prev_hash((sSym*)(U64)SRCH_LIST,new->hash,&pkg);
  if(prev)
    printf("new_symbol: prev is %p\n",prev);
  // Now that the new object is in our segment, is there an 
  if(prev) { // older version we are replacing?
    printf("old version exists\n");
    sSym* old = (sSym*)(U64)prev->next;
    printf("prev: %p, old: %p\n",prev,old);

pk_push_sym(U32_SYM(SRCH_LIST),new);
#if 0
    // first, make ssure it's in the same seg.
    if( (SEG_BITS(old->art)) == (SEG_BITS(new->art))){
      //	// yes, both are in the same seg.  Fix old refs
      if(pkg == (sSym*)(U64)SRCH_LIST){
	// rereference data and code segments
	printf("xx\n");
	U32 fixes = segs_reref(old->art, new->art);
	printf("%d fixups\n",fixes);
	// now remove the symbol in metadata
	printf("1. prev: %p, next %08x\n",prev,prev->next);
	U32 holesize = sym_delete(prev);
	// shit is different places now!
	new = U32_SYM( PTR_U32(new) - holesize);

	pk_push_sym(U32_SYM(SRCH_LIST),new);


	
      } else {
	printf("Old version is in %s, new in %s; abandoning\n",
	       SYM_NAME(pkg), SYM_NAME((sSym*)(U64)SRCH_LIST));
	//TODO: kill segment!
      }
    } else { // No, different segs.  Nothing good can come of it.
      fprintf(stderr,"New one is in a different seg! Abandoning!\n");
      //	    sym = pkg_drop_symb(pkgs); // drop new object from topmost pkg
    }
#endif
  } else {// else a new symbol, no problem
    printf("pushing symbol\n");
    pk_push_sym((sSym*)(U64)SRCH_LIST,new);
  }
}

sSym* compile(void){
  sSym* sym=0;
  sElf* pelf = rebuild("unit",0);
  if(!pelf){
    printf("build aborted\n");
    return 0;
  }
  // load the entire ELF trainwreck
  sym = ing_elf(pelf); 

  if(sym){ // OK, this means we are done with ingestion.
    printf("Ingested %s: %s %d bytes\n",
	   SYM_NAME(sym),sym_proto(sym),sym->size);
    new_symbol(sym);

  } else {
    printf("ELF not ingested due to unresolved ELF symbols.\n");
  }
  elf_delete(pelf);
  


  return sym;
}

sSym* repl_compile(char*p){
  return compile();
}

char linebuf[1024];

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

/*----------------------------------------------------------------------------
  cmd_hash                  parse and hash a presumed command, updating pptr

A command is an identifier, terminated by a anything below ascii '0'

----------------------------------------------------------------------------*/
U32 cmd_hash(char** ppstart){
  U32 hash = FNV_OFFSET_BASIS;
  U8 c;
  char*p = *ppstart;
  while((c=*p++) && (c>='0')){
    hash = (U32)((hash ^ c) * FNV_PRIME);
  }
  *ppstart=p-1;
  return hash;
}

char* cmd_ws(char*p){
  while(isspace(*p))
    p++;
  return p;
}
//if(0xC3589F16 == hash){

void repl_list(char* p){
  U32 hash = cmd_hash(&p);
  sSym* pkg;
  sSym* sym = pks_find_hash((sSym*)(U64)SRCH_LIST,hash,&pkg);
  if(sym){
    puts("-------------------------------------------------------------");
    printf("In package: %s\n",SYM_NAME(pkg));
    puts("-------------------------------------------------------------");
    src_to_file(sym->src,stdout); //todo: src length...
    puts("-------------------------------------------------------------");
  }
}

void repl_words(char* p){
  pk_dump((sSym*)(U64)SRCH_LIST);
}
char* helpstr =
  "built-in commands:\n\
 bye<cr>           exit\n\
 cc<cr>            compile sys/unit.c; please edit sys/body.c first\n\
 dump XXXXXXXX<cr> hexdump data at hex address\n\
 edit <name><cr>   put the source of symbol into sys/body.c\n\
 help<cr>          displays this message\n\
 list <name><cr>   show the source of the symbol\n\
 load<cr>          load the image file\n\
 save<cr>          save the image in image/image.dat file\n\
 sys<cr>           show system statistics\n\
 words<cr>         show the symbol table\n\
\n\
puts(\"hello\");     execute any stdlib or your own function\n\
\n\
The entire libc is available; <stdio.h> and <stdlib.h> are always included; \n\
other headers may need to be included in each unit, add to /sys/types.h if \n\
you intend to use a header or type a lot.\n\
\n\
Each edit/compilation unit must have exactly one global symbol - one data \n\
object or one function.  Make sure to add declarations for global data; \n\
your function prototypes will be automatically generated.\n\n\
If you use emacs, keep a buffer with sys/body.c, and turn on \n\
auto-revert-mode.  Now when you type in 'edit foo<cr>', the source of foo \n\
will appear in the buffer.  After making changes, enter `cc` to compile and \n\
integrate the changes.  Or edit sys/body.c, save, and cc to compile.\n\
\n\
You can call any compiled function from the command line.\n";





void repl_help(char*p){
  puts(helpstr);
}

void repl_expr(char*p){
  FILE*f = fopen("sys/cmdbody.c","w");		
  fputs("void command_line(void){\n ",f);
  fputs(p,f);
  fputs("\n}\n",f);
  fclose(f);

  sElf* pelf = rebuild("commandline",1);
  if(!pelf) return;
  REL_FLAG = 0;
  U32 here = CFILL;
  U32 addr = (U32)ing_elf_func(pelf); // don't care about size
  REL_FLAG = 1;
  elf_delete(pelf);
  
  hd(PTR(U8*,addr),4);
  if(addr){
    fpreplfun entry = (fpreplfun)(U64)(addr);
    (*entry)(p);
    memset(entry,0,here-addr);
    CFILL = here;
  }
}

void repl_dump(char*p){
  U64 addr = strtol(p,0,16);
  if(addr<0xFFFFFFFF){
    hd((void*)addr,4);
  }
}
#if 0
void repl_save(char*p){
  FILE* f = fopen("image/image.dat","w");
  seg_serialize(psData,f);
  seg_serialize(psCode,f);
  seg_serialize(psMeta,f);
  fclose(f);
}

void repl_load(char*p){
  FILE* f = fopen("image/image.dat","r");
  seg_deserialize(psData,f);
  seg_deserialize(psCode,f);
  seg_deserialize(psMeta,f);
  fclose(f);
  //TODO: automate search for libraries to rebind upon load
  pk_rebind( (sSym*)(U64)(((sSym*)(U64)SRCH_LIST)->art),"libc.so.6");
  //pk_rebind( ((sSym*)(U64)SRCH_LIST),"libc.so.6");
}
#endif
void repl_edit(char*p){
  sSym* sym = pks_find_name((sSym*)(U64)SRCH_LIST,p,0);
  FILE*f = fopen("sys/body.c","w");
  if(sym){
    src_to_file(sym->src,f);
  }
  fclose(f);
  puts("edit and enter 'cc' when done\n");
}
/*---------------------------------------------------------------------------

  ----------------------------------------------------------------------------*/
void repl_sys(char* p){
  aseg_dump();
  seg_dump(psMeta);
  //pkgs_list();
}
    
/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/

void repl_loop(){
  pks_dump_protos();
  puts("harn - an interactive C environment.  (c) 2022 StackSmith\n\n\
      ...try 'help<cr>', or 'printf(\"Hello World!\\n\")<cr>'\n");
  while(1) {
    printf("> ");
    fgets(linebuf,1024,stdin);

    char*p=linebuf;
    U32 hash = cmd_hash(&p); // and advance to ws
    //    printf("Hash: %08X, len %d\n",hash,(U32)(p-linebuf));
    //    U32 cmdlen = (p-linebuf);
    // if terminated by a (, compile line and execute as a function
    if(strchr(p,';')) { repl_expr(linebuf); continue ; }

    // skip any whitespace to parameters, and remove the final cr
    p = cmd_ws(p);
    { size_t len = strlen(linebuf);
      *(linebuf+len-1)=0;
    }
    if(!strncmp("cc",linebuf,2))  { repl_compile(p);continue; }
    if(!strncmp("sys",linebuf,3)) { repl_sys(p);    continue; }
    // ox56299123
    // 5CB7AA8A,
    if(!strncmp("words",linebuf,4)){ repl_words(p); continue; }
    //if(!strncmp("edit",linebuf,4)){ repl_edit(p);  continue; }

    //if(!strncmp("list",linebuf,4)){ repl_list(p);  continue; }

    if(!strncmp("help",linebuf,4)){ repl_help(p); continue; }
    if(!strncmp("dump",linebuf,4)){ repl_dump(p); continue; }
    //if(!strncmp("save",linebuf,4)){ repl_save(p); continue; }
    //if(!strncmp("load",linebuf,4)){ repl_load(p); continue; }

    // user typed in the name of a function to run as void
    // 0x71F39F63
    if(!strncmp("bye",linebuf,3)) exit(0);

    // try to run it as a repl function
    run_repl_fun(hash,p);
 
  }
}
    /*
    if(!strncmp("edit",linebuf,4)){
      edit(linebuf+5);
      system("vim sys/body.c");
      compile();
      pkgs_dump_protos();
      continue;
    }
    */
