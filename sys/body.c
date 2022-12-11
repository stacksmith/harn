#define pass 4
#if (pass==1)
U32 ass = 1;
#endif

#if (pass==2)
extern U32 ass;
U32* p = &ass;
#endif
 
#if (pass==3)
U32 ass = 99;
#endif 

#if (pass==4)
extern U32* p;
void tester(){
  printf("%d\n",*p);
}
#endif





