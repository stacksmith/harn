rm -f $1.o
gcc -Wall -Os \
    -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
    -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables \
    -falign-functions=1 \
    -nostartfiles           \
    -fcf-protection=none    \
    -fno-stack-protector \
    -fno-mudflap \
    -aux-info info.txt \
    -Werror-implicit-function-declaration \
    -c $1.c -o $1.o


