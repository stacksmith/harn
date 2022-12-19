#define pass 2
#if (pass==1)
U32 ass = 18;
#endif

#if (pass==2)
  extern U32 ass;
void tester(){
  printf("ass+2 is %d\n",ass+2);
}
#endif

#if (pass==3)
U32 ass = 99;
#endif



