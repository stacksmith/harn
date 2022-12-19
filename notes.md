## 
`repl_compile` invokes the compiler build script, which incorporates the user function or data unit contained in `unit.c`.  The compiler leaves us with an ELF object file, and an info.txt file with headers (the last one will match the compiled function, if it was a function). 

`repl_compile` invokes `elf_ingester.c`:`ing_elf` to process the ELF object file, pull the data into an artifact segment (data or code as appropriate), and create a sSym, (see `sym_for_artifact`).  Everything is ready to integrate the symbol into the system.


`repl_compile` then passes the symbol to `pk.c` `pk_incorporate` for integration into the system and the search chain.  If the symbol is new (not found), `pk_incorporate` simply attaches it to the current package.

If a symbol with the same name exists, it is verified against the new symbol to make sure the new symbol is compatible - it must be in the same segment (code or data), and have a compatible signature....  Assuming success, the following takes place:
* The new symbol replaces the old one in the search chain. `pk_incorporate`
* All refs to old artifact are updated to the new one.     `aseg_reref`
* The old symbol and associated artifact are deleted.      `sym_del`

The deletion process is a rather complicated task, as it requires fixups and compaction across three segments.  On success, the number of fixups is reported, and the new variable or function is integrated into the system; the new version of the function or variable is magically connected instead of the old one.










This was a fail, the catdog segment was endless trouble!
```
It would actually make more sense to not have 3 uniform segments, but instead keep a combinded data/code segment growing in different directions (say, data down, code up).  When a hole is created, a single sweep can process it.  
```
In the end I wound up with 3 normalish segments, each a little different...

