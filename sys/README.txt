# What's all this stuff?

`harn` invokes the compiler using the `build.sh`, which takes a single name parameter; the input file is <name>.c and the output is <name>.o.

In practice, there are two names in use: `commandline` and `unit`.  The system uses unit for normal compilation, and commandline to compile expressions in REPL.  The system does not change toplevel commandline.c and unit.c; these include some headers, and the body file, into which the system writes source as needed.

Currently,
commandline.c       toplevel command-line expression compiler file
cmdbody.c           the expression is inserted here into a function
unit.c              toplevel compiler file
body.c              the function or variable being compiled is inserted here
headers.h           system-generated headers exposing available symbols to body
types.h             static definition of types
info.txt            gcc-generated file; for function units, last line has declaration


