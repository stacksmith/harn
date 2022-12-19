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


sSym* ingest_elf(char* name);
 
//U64 ing_elf_func(sElf* pelf);

extern sSeg* psMeta;


typedef void (*fpreplfun)(char*p);
typedef U64 (*fpvoidfun)();
// bogus, careful...
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
  sSym* sym = pks_find_hash(PTR(sSym*,SRCH_LIST),hash);
  if(sym){  //
    exec_repl_sym(sym,p);
  } else {
    printf("can't find it\n");
  }
}


/*----------------------------------------------------------------------------
compile

------------------------------------------------------------------------------*/

sSym* repl_compile(char*p){
  sSym* sym =  ingest_elf("unit");
  if(sym){ // OK, this means we made it!
    printf("Ingested %s: %s %d bytes\n", SYM_NAME(sym),sym_proto(sym),sym->size);
    pk_incorporate(sym);
  } else { // no symbol
    printf("aborted.\n");
  }
  return sym;

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
  sSym* sym = pks_find_hash(PTR(sSym*,SRCH_LIST),hash);
  if(sym){
    puts("-------------------------------------------------------------");
    //    printf("In package: %s\n",SYM_NAME(pkg));
    puts("-------------------------------------------------------------");
    src_to_file(sym->src,stdout); //todo: src length...
    puts("-------------------------------------------------------------");
  }
}

sSym* pk_find_hash2(sSym* pk, U64 hash);
  

void repl_words(char* p){
  pk_dump(PTR(sSym*,SRCH_LIST));
  /*
  sSym* proc(sSym* s,U64 hash){
    return ((s->hash == hash)) ?  s : 0;
  }
  sSym* qqq = apkg_walk2( PTR(sSym*,SRCH_LIST),
			 0xa94d67e5,
			 proc);
  */
  //  sSym* qqq = pk_find_hash1( PTR(sSym*,SRCH_LIST),0xa94d67e5,0,0 );
  //   sSym* qqq = pk_find_hash2( PTR(sSym*,SRCH_LIST),0xa94d67e5 );
  //  if(qqq)    printf("helo %p %08x\n",qqq,qqq->hash);
  //else    printf("0.\n");
}

void repl_pkg(char* p){
  if(!strncmp(p,"ls",2)) { srch_list_pkgs_ls();

    return; }
  if(!strncmp(p,"use",3)) {
    p = cmd_ws(p+3);
    U32 hash = cmd_hash(&p);
    sSym* pkg = srch_list_find_pkg(hash); // find the desired package
    if(pkg){
      sSym* prev = srch_list_prev_pkg(pkg);
      if(prev){ // unlink
	sym_dump1(prev);
	prev->art = pkg->art;
	pkg->art = SRCH_LIST;
	SRCH_LIST = THE_U32(pkg);
      } // else we are already on top
      srch_list_pkgs_ls();
      return;
    }
    //   hd(p,1);
    //return;
  }
  
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
U32 ingest_run(char* name,char*p);

void repl_expr(char*p){
  FILE*f = fopen("sys/cmdbody.c","w");		
  fputs("void command_line(void){\n ",f);
  fputs(p,f);
  fputs("\n}\n",f);
  fclose(f);

  ingest_run("commandline",p);
}

void repl_dump(char*p){
  char* pend;
  U64 addr = strtol(p,&pend,16);
  //  printf("addr is %08lX\n",addr);
  if('r'== *pend)
    addr>>=3;
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
  pk_rebind((PTR(sSym*,SRCH_LIST)->art));
  //pk_rebind( ((sSym*)(U64)SRCH_LIST),"libc.so.6");
}
#endif
void repl_edit(char*p){
  sSym* sym = pks_find_name(PTR(sSym*,SRCH_LIST),p);
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
  mseg_dump();
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

    if(!strncmp("help",linebuf,4)){ repl_help(p); continue; }
    if(!strncmp("dump",linebuf,4)){ repl_dump(p); continue; }
    if(!strncmp("list",linebuf,4)){ repl_list(p);  continue; }
    if(!strncmp("pkg",linebuf,3)){ repl_pkg(p);  continue; }

	
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
