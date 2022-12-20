#define pass 2
#if (pass==1)
U32 ass = 1;
#endif

#if (pass==2)
  extern U32 ass;
void tester(){
  printf("ass is %d\n",ass);
}
#endif

#if (pass==3)
U32 ass = 99;
#endif








