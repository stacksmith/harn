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


	global  pkg_find_hash
;;; edi - pkg to srrch         on fail, eax is 0; on match, eax = sym.
;;; esi   hash               
;;;                     This is a monolithic non-composable loop..
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
%endif
%if 0
/* -------------------------------------------------------------------------

  The package subsystem consists of two levels, or dimensions of objects:

  * HORIZONTAL Top level is a linked list of PKG headers.  
    PKGs belong to this list.   PKGs do not have associated artifacts, and
    package names are used exclusively for manipulating PKGs ; they are 
    not visible during searches.

  * VERTICAL.  Each PKG starts a linked list of actual symbols.  Each 
    package contains related symbols, and the entire package is searchanble.

  The searches generally span the entire symbol space, traversing from one 
  PKG to then next sequentially, and transparently.

  So we wanto to be able to 
pkg_   * search and operate on symbols of single  package ;
pkgs_  * search and operate on all symbols in the entire PKG list
plst   * work on PKGs objects only

 symbol, allowing us to unlink it.  

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
.loop: 	add	eax,[edx+4]     ;advance, and test for 0
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
plst_walk   a layer above, we want to traverse top list visiting each 
            package head, and operating on it specifically

ENTER WITH META_SEG_ADDR!!!! immediately indirects through 8, AKA SRCH_LIST                                                      
 -------------------------------------------------------------------------*/
%endif
	global plst_walk_U32
	global plst_walk
plst_walk_U32:	
plst_walk:
;;; rdi = ptr    ;              ; rdi = ptr
;;; rsi = param    ; callback :	; rsi = param
;;; rdx = fun                   ; rdx = prev 
	push	rbp             ; param stays in rsi!
	mov	rbp,rdx		;rbp = fun to call
	mov	edx,edi         ; META_SEG_ADDR will be first prev@
	xor	eax,eax
.loop: 	add	eax,[edx+8]     ;advance, and test for 0
	jz     .done		; at the end, return 0
	xchg	eax,edi         ;edi=ptr, ready for call
	push	rdi             ;preserve it
	push    rsi             ;preserve param
	call	rbp		;;call fun
	test	rax,rax         ;chec result of call
	pop     rsi
        pop	rdx		;edx serves as prev
	jz	.loop
.done: 	pop	rbp
	ret
	
%if 0	
/* -------------------------------------------------------------------------
pkgs_walk   operate on all symbols in the entire space by iterating on
            PKLST, and doing a pkg_walk on every encountered PKG

	    Use the same procs pkg_walk, since it dispatches ther.
ENTER WITH META_SEG_ADDR!!!! immediately indirects through 8, AKA SRCH_LIST   
We don't actually invoke the fun - we just keep passing it to pkg_walk
Likewise, prev is not necessary

;;; rdi = ptr - a package pointer                    ; edi = 
;;; rsi = parameter to pass around  in callback:   --; rsi   
;;; rdx = fun to pass to pkg_walk ;                  ; edx   
	*/
%endif
	global pkgs_walk
	global pkgs_walk_U32	
pkgs_walk_U32:	
pkgs_walk:
	xor     eax,eax
.loop:	add	eax,[edi+8]     ;next package
	jz      .done           ;if eax is not zero
	xchg	eax,edi         ;edi = next package
	push    rsi             ;save param
	push    rdx             ;save fun
	push	rdi		;save package
	call	pkg_walk
	test	rax,rax         ;test result of call
	pop     rdi
	pop	rdx
	pop	rsi
	jz	.loop           ;rax is maybe result?

.done:	ret


