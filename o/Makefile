TOP = ..
TOPSRC=$(TOP)

CC=gcc
#CCX=~/local/bin/tcc
#-fno-strict-aliasing -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation
# -fomit-frame-pointer line... do not generate .eh_frame
# -fcfprotection=none - do not compile bullshit target noop
# -fno-stack-protector  do not check stack and call __stack_chk_fail
CFLAGS=-Wall -Os \
-D_FORTIFY_SOURCE=0 \
-fvisibility=hidden \
-ffunction-sections \
-fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables \
-falign-functions=1 \
-nostartfiles           \
-fcf-protection=none    \
-fno-stack-protector \
-fno-mudflap \
-aux-info info.txt 


# YYY-symbolic -fno-inline  

all: test2.o twoA.o twoB.o

test2.o: test2.c
	$(CC) -c $^ $(CFLAGS) 

test3.o: test3.c
	$(CC) -c $^ $(CFLAGS) 

test4.o: test4.c
	$(CC) -c $^ $(CFLAGS) 

test10.o: test10.c
	$(CC) -c $@ $^ $(CFLAGS) -I$(TOPSRC)


