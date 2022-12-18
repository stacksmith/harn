#define pass 1
#if (pass==2)
void bar(){
  foo();
}
#endif
#if ((pass==1) || (pass==3))
void foo(){
#if (pass==1)
   puts("old");
#else
   puts("newest of them all");
#endif
}
#endif

