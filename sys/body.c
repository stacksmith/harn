//sPt* mypt = &((sPt){9,10});

extern sPt* mypt;

void bad(){
  puts("OK");
  printf("mypoint is %d %d\n",mypt->x, mypt->y);
}



