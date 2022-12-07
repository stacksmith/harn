TOP = ..

CC=gcc

CFLAGS= -Wall -O2 -std=c99 -fno-strict-aliasing -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation 

all: harn

tsv2names: tsv2names.c
	$(CC) -o $@ $^ $(CFLAGS) 	

harn: harn.c elf.c elfdump.c seg.c util.c unit.c lib.c pkg.c system.c 
	$(CC) -o $@ $^ $(CFLAGS) 

clean:
	rm harn harn.o *.o *~
 
