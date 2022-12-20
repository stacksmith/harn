        section .data		; Data section, initialized variables

        section .text           ; Code section.

%if 0
/* -------------------------------------------------------------------------
 A small diversion... I find it hard to resist coding in assembly on 
 occasions.  The C type system is just so in the way...


 Let's start with traversing the metaspace, and iterating on symbols in
 packages.  A symbol starts with:
  0 U32 hash
  4 A32 cdr         The most common operation is searching for hashes...
  8 A32 art         To that end, the has is placed at offset 0, and cdr
  C U32 size        is at 4.  This allows us to grab a U64 with both, and
 10 U32 fsrc        quickly check the low dword for a hash match, then
 14 U8  octs        >>32 to get the cdr and check for zero.  Cool.
 15 U8  namelen
 16 ..name,etc.
-------------------------------------------------------------------------*/
%endif
	global  pkg_find_hash
;;; edi - pkg to srrch         on fail, eax is 0; on match, eax = sym.
;;; esi   hash
;;; 
pkg_find_hash:
        ;;  we always start with a package pointer, TODO: skip it..
	mov	edi,[rdi+4]     ;load package's cdr...
.loop:	mov	rax,[rdi]	;rdi is address, rax is |next|hash|
	cmp	eax,esi         ;hash check in low dword
	xchg	rax,rdi         ;rax is address, rdi is its next/hash
	je      .done           ;on match, rax is symbol matched.
	shr	rdi,32	        ;otherwise, drop next into low DWORD
	jnz	.loop           ;z? eax is pointing at the last sym
	xor	eax,eax	        ;and not a match, so there.
.done:  ret

%if 0
/* -------------------------------------------------------------------------
pkg_walk - a generic walker for traversing packages ; calls a a c function
U32 pkgfun2(cSym* s, cSym*prev, U64 param) ; along these lines...
	;; 		 

Be careful declaring the called function!  It is tempting to use a void*
in the walker setup, and it will generally work, but note that gcc has 
given me hours of grief because I called a function return a U8 flag --
which made it leterally 6 bytes to compare hash!  But the walker exits on
rax currently, and the function set a byte, giving me intermittent failures
depending on who screwed with rax...

Anyway, this is a wonderful walker, and the calling function gets the Sym
pointer, the previous pointer if it wants to unlink, and a parameter which
is sent to pkg_walk at setup time, and can be anything.  The canonical 
usage is walking a package looking for a hash sent as a parameter, with
a callback:

sSym* proc1(sSym* s,sSym*prev,U  hash){
  return (s->hash == hash) ? (U64)(prev) : 0;
}
q = pkg_walk(proc1,q,0xA52bCAF9) ;

This will find the symbol with a matching hash and return the previous 
symbol, allowing us to unlink it.  

By the way, with wrapper functions like these, make sure parameter lists
match as much as possible, otherwise functions get really big shuffling
registers.   Moved parameters pf pkg_walk to keep pkg and parm as 1 and2 ;
now will move the invoked proc parameters to match:
(pkg,parm,prev) because next is pretty rare, really...
-------------------------------------------------------*/
%endif
global pkg_walk
global pkg_walk_U32
pkg_walk_U32:	
;;; rdi = ptr    ;              ; rdi = ptr
;;; rsi = param    ; callback :	; rsi = param
;;; rdx = fun                   ; rdx = prev 
pkg_walk:	                
	push	rbp             ; param stays in rsi!
	mov	rbp,rdx		;rbp = fun to call
	mov	edx,edi         ; ptr (edx = old, edi = current)
	xor	eax,eax
.loop: 
	add	eax,[edx+4]     ;advance, and test for 0
	jz     .done		; at the end, return 0
	xchg	eax,edi         ;edi=ptr, ready for call
	push	rdi             ;preserve it
	push    rsi             ;preserve param
	call	rbp		;;call fun
	pop     rsi
        pop	rdx		;edx serves as prev
	test	rax,rax
	jz	.loop

.done: 	pop	rbp
	ret
%if 0	
/* -------------------------------------------------------------------------
pkgs_walk   Let's take this to the next logical level: a packages
            walker, which walks through packages, invoking pkg_walk on each
 and passing it a package, a parameter, and whatnot.  

So, pkgs_walk will keep an internal packages pointer, param, and invoke 
    pkg_walk with the first symbol, param, and proc....  If the pkg_walk
    returns 0, we move to the next package ; any other value will be
    returned,  Note: packages are linked via the art pointer!

;;; rdi = ptr - a package pointer
;;; rsi = parameter to pass around
;;; rdx = fun to pass to pkg_walk       

;; Unlike pkg_walk, we do not skip the first (package) symbol, but send it
;; directly to the walker.  For now, we won't bother with the prev - pkg
;; reordering within the srch_list is not done often...
	*/
%endif
	
global pkgs_walk
	
pkgs_walk:
	xchg	eax,edi
.loop:	xchg	eax,edi         ;edi = next package
	push    rsi             ;save param
	push    rdx             ;save fun
	push	rdi		;save package
	call	pkg_walk
	pop     rdi
	pop	rdx
	pop	rsi
;;; check result
	test	rax,rax	
	jnz	.done           ;rax is maybe result?
	add	eax,[edi+8]     ;next package
	jnz     .loop           ;if eax is not zero
.done:	ret
