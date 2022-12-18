        section .data		; Data section, initialized variables

        section .text           ; Code section.

;;; rdi = base address
;;; rsi = bit to set
	global bits_set	        ; (addr,cnt)
	global bit_test
	global bits_cnt
	global bits_next_ref	;
	global bits_reref	; replace old ref to new ref
	global bits_drop	; delete a piece of a segment and rel
	global bits_fix_lo      ; replace refs above hole down by ecx
	global bits_fix_hi
	global bits_fix_meta
;;;--------------------------------------------------------------------------------
;;; bits_set                    Set a zero followed by specified number of bits
;;;
;;; We make inherent assumptions that count is reasonable, and that the rel table
;;; is correctly set to 0 at the site.
;;; 
;;; rdi = addr
;;; rsi = cnt
bits_set:
	xor	eax,eax
.loop:	inc 	rdi
	bts 	[rax],rdi
	dec 	rsi
	jnz	.loop
	ret
	
;;; rdi = addr
bit_test:
	xor	eax,eax
	bt	[rax],rdi
	sbb	eax,eax
	ret

	
;;; edi = addr of top+1,   	;used as bit 
;;; esi = addr of bottom
;;; edx = ptr (address of reference)
;;; ecx = 0
;;; return: count of 1's
bits_next_ref:
	xor	eax,eax		;eax = bit counter
	xor	ecx,ecx		;edx = base(0);

.loop:	dec	edi		;next reference
	cmp	edi,esi
	js      .done          	;exit when below bottom
	
	bt	[rcx],rdi     
	jnc	.loop
 
	;; got a 1!  now count
.loop1:	inc	eax             ; count the bit
	dec     edi             ; previous bit
	bt	[rcx],rdi      
	jc	.loop1
	mov	[rdx],edi	;set caller's pointer
.done:
	ret
	
;;; edi = addr of top+1,   	;work pointer
;;; esi = addr of bottom        ;limit 
;;; edx = old
;;; ecx = new	 
bits_reref:
	 push	rbx             ;ebx = count of fixups
	 xor	ebx,ebx
	sub	ecx,edx         ;compute fixup difference, S64
	jmp	.loop0

.one:	mov	eax,[rdi]	;load 32-bit offset
	lea	eax,[rax+rdi+4] ;convert to abs32
.loopx:	cmp	rax,rdx         ;match?  Check all 64 bits...
	jne	.loop0		;go back to loop; skip this 1 and prev 0
	inc     ebx             ;increment fixup count
	add	[rdi],ecx       ;fixup
.loop0: xor	eax,eax		;Jump here to clear eaeax = base(0);
.loop:	dec     edi
	cmp	edi,esi        	;the very bottom
	js      .done          	;if offset is 0, exit with 0
	bt	[rax],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits
 	;; got a 1-0 transition.	
	dec     edi
	bt	[rax],rdi
	jnc	.one
	dec	edi
	bt	[rax],rdi
	jnc	.two
	dec	edi
	bt	[rax],rdi
	jnc	.three
;;; more than three 1's, return -1.
	sbb	ebx,ebx		;-1, since C is set...
.done:	mov	eax,ebx 	;return number of fixes
	pop	rbx
	ret	

.two:;; 0 - 1 - 1 - 0   A32
	mov	eax,[rdi]
	jmp	.loopx
	;; 0 - 1 - 1 - 1 - 0   A64 
.three:	;; 64-bit relocation; edi = address  edx = old
	mov	rax,[rdi]	;load entire 64-bit pointer
	jmp	.loopx



%if 0
--------------------------------------------------------------------------------
	DROP

When an artifact is eliminated, the resulting hole must be removed, by dropping
the entire area from the top of the hole to the end of the segment.	
--------------------------------------------------------------------------------
%endif

;;; edi = start of hole
;;; esi = end of hole
;;; edx = ?
;;; ecx = bytes to move
bits_drop:	
	push	rdi
	push	rsi
	push	rcx

	rep 	movsb		;edi points to new top; esi, old top
	mov	ecx,esi
	sub	ecx,edi	        ;ecx = size of hole
	mov     edx,ecx         ;edx = size of hole, keep it around 
	xor	eax,eax         ;fill top with 0s
	rep     stosb
;;; Now, process rel
	pop	rcx		;restore count of bytes to move
	shr	ecx,3           ;moving bits, not bytes
	pop	rsi             ;source
	shr	esi,3
	pop	rdi             ;destination
	shr	edi,3

	rep 	movsb		;edi points at new reltop, esi, old
	mov	ecx,edx         ;hole size
	shr	ecx,3      
	xor	eax,eax         ;fill top with 0s
	rep     stosb
.x:
	mov	eax,edx         ;return hole size / amount dropped by
	ret
%if 0
--------------------------------------------------------------------------------
	FIXDOWN

An artifact is removed from a segment, creating a hole at haddr of size hsize.
The hole is compacted by dropping the rest of the segment down to the hole.
The affected area, from the hole start to segment top is known as 'dropzone'.

After a hole is compacted, the fixup is done in two stages:
1) bits_fix_lo  to fix the area from bottom to dropzone.
	* All refs into dropzone are fixed-up by -holesize.
2) bits_fix_hi  to fix the dropzone itself.
	* Absolute refs into dropzone are fixed by -holesize.
	* Relative refs out of dropzone are fixed by +holesize.
--------------------------------------------------------------------------------
%endif
	
;;; fixdown_below:
;;; 
;;; Scan the area below the dropzone:
;;; * absolute refs to dropped area	-size
;;; * relative refs to dropped area	-size
;;; * absolute refs outside	        unchanged
;;; * relative refs outside             unchanged

;;; * references to  hole < n < segment end   -size;
	
;;; edi = addr of top+1,   	;segment top
;;; esi = addr of bottom        ;limit 
;;; edx = hole                  ;dropzone start
;;; ecx = size of hole (fixup amout)
bits_fix_lo:
	push	rbx             ;ebx = count of fixups
	push	rbp
	mov	ebp,edi		;ebp = segment top,dropzone end
	mov	edi,edx 	;START: HOLE!
	xor	ebx,ebx		;fixup count
	jmp	.loop0
	;; 0 - 1 - 0    R32
.one:	mov	eax,[rdi]	;load 32-bit offset
	lea	eax,[rax+rdi+4] ;convert to abs32
.loopx:	cmp	rax,rdx         ;< dropzone start? skip.
	jb	.loop0		;
	cmp	rax,rbp		;>= dropzone end? skp.
	jae	.loop0          ;
	inc     ebx             ;increment fixup count
	sub	[rdi],ecx       ;drop target
	
.loop0: xor	eax,eax		;Jump here to clear ea = base(0);
.loop:	dec     edi
	cmp	edi,esi        	;Operate down to segment bottom
	js      .done          	;END: BOTTOM OF SEG!
	bt	[rax],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits; eax is 0
 	;; got a 1-0 transition.	
	dec     edi
	bt	[rax],rdi
	jnc	.one
	dec	edi
	bt	[rax],rdi
	jnc	.two
	dec	edi
	bt	[rax],rdi
	jnc	.three
;;; more than three 1's, return -1.
	sbb	ebx,ebx		;-1, since C is set...
.done:	mov	eax,ebx 	;return number of fixes
	pop	rbp
	pop	rbx
	ret	

.two:;; 0 - 1 - 1 - 0   A32
	mov	eax,[rdi]	;rax = target... if in dropzone,
	jmp	.loopx          ;will fix up there, same as rel.
	
.three:	;; 0 - 1 - 1 - 1 - 0   A64 
	mov	rax,[rdi]	;load entire 64-bit pointer
	jmp	.loopx
;;; fixdown_high
;;; Scan the area that was dropped:
;;; * absolute refs to dropped area	-size
;;; * relative refs to dropped area	unchanged
;;; * absolute refs outside	        unchanged
;;; * relative refs outside             +size
	
bits_fix_hi:	
;;; edi = addr of top+1,   	;segment top
;;; esi = unused
;;; edx = hole
;;; ecx = size of hole (fixup amout)
	push	rbx             ;ebx = count of fixups
	push	rbp
	mov	ebp,edi		;ebp = segment top, dropzone end,
				;START at dropzone end!
	xor	ebp,ebp		;fixup count
	jmp	.loop0
	;; 0 - 1 - 0    R32     RELATIVE...
.one:	mov	eax,[rdi]	;load 32-bit offset
	lea	eax,[rax+rdi+4] ;convert to abs32
.loopx:	cmp	rax,rdx         ;target < dropzone
	jb	.fixpl		;fix by -holesize
	cmp	rax,rbp		;target < segment end? 
	jb	.loop0        
.fixpl:	inc     ebx             ;increment fixup count
	add	[rdi],ecx       ;make the offset smaller!
	
.loop0: xor	eax,eax		;Jump here to clear ea = base(0);
.loop:	dec     edi
	cmp	edi,edx        	;Operate down to dropzone bottom!
	js      .done          	;if offset is 0, exit with 0
	bt	[rax],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits
 	;; got a 1-0 transition.  Now dispatch on number of bits.
	dec     edi
	bt	[rax],rdi
	jnc	.one
	dec	edi
	bt	[rax],rdi
	jnc	.two
	dec	edi
	bt	[rax],rdi
	jnc	.three
;;; more than three 1's, return -1.
	sbb	ebx,ebx		;-1, since C is set...
.done:	mov	eax,ebx 	;return number of fixes
	pop	rbp
	pop	rbx
	ret	
.three:	;; 0 - 1 - 1 - 1 - 0   A64 
	mov	rax,[rdi]	;load entire 64-bit pointer
	jmp	.abs		;same as A32
.two:;; 0 - 1 - 1 - 0   A32
	mov	eax,[rdi]
.abs:	cmp	rax,rdx		;abs target < dropzone?
	jb	.loopx          ; no action
	cmp	rax,rbp         ;abs target > segment end?
	jae	.loopx          ; no action
	sub     [rdi],ecx       ;within drop zone, fix by -hole!
	jmp	.loop0
	

%if 0
--------------------------------------------------------------------------------
The meta segment is a special case: metadata is in control of other segments...

0100 refs mark link references - A32
0110 refs mark artifact pointers - A32

The act of deleting a header implies a deletion of its artifact, and the fixup
can take care of both the local linkage _and_ the artifact pointers.

%endif
	
bits_fix_meta:	
;;; edi = addr of top+1,   	;segment top
;;; esi = bottom                ;segment bottom
;;; edx = hole
;;; ecx = size of hole (fixup amout)
;;; r8  = arthole
;;; r9  = arthole size / fixup amount
	
	
	push	rbx             ;ebx = count of fixups
	xor	ebx,ebx		;fixup count
	xor	eax,eax
	dec	ebx		;

.loopf: inc	ebx
.loop:  dec	edi
	cmp	edi,esi
	js	.done
	bt	[rax],rdi       ;testing bits
	jnc	.loop
	
.bit:	dec	edi
	bt	[rax],rdi       ; ? 1 0
	jc	.art		
				; 0 1 0 = link ref A32
.link:	cmp	[rdi],edx	; compare reference to hole
	jb	.loop           ; below? no action.
	sub 	[rdi],ecx	; above hole? fixup by -size.
	jmp	.loopf

.done:	mov	eax,ebx		; return count of fixups
	pop	rbx
	ret
				; 0 1 1 0 = artifact ref A32
.art:	dec     edi
	cmp	[rdi],r8d	;compare art pointer to arthole
	jb	.loop
	sub	[rdi],r9d	; above arthole? fixup
	jmp	.loopf
