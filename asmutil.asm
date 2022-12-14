        section .data		; Data section, initialized variables

        section .text           ; Code section.

;;; rdi = base address
;;; rsi = bit to set
	
	global bit_set1
	global bit_set2
	global bits_set
	global bit_test
	global bits_cnt
	global bits_next_ref	;
	global bits_reref	; replace old ref to new ref
	global bits_fixdown     ; replace refs above hole down by ecx
	global bits_hole	; delete a piece of a segment and rel
;;; edi=addr
bit_set2:	
	xor	eax,eax
	bts 	[rax],rdi
	inc 	edi
;;; edi=addr
bit_set1:
	xor	eax,eax
	bts 	[rax],rdi
	ret

;;; rdi = addr
;;; rsi = cnt
bits_set:
	xor	eax,eax
.loop:	bts 	[rax],rdi
	inc 	rdi
	dec 	rsi
	jnz	.loop
	ret
	
;;; rdi = addr
bit_test:
	xor	eax,eax
	bt	[rax],rdi
	sbb	eax,eax
	ret
;;; rdi = addr
bits_cnt:
	dec	rdi		;previous bit must be 0!
	xor	esi,esi		;esi = base (0)
	bt  	[rsi],rdi
	sbb	eax,eax		;-1 if error
	js      .x              ;if set, we are done, -1=error!
.loop:	
	inc	rdi		;next bit.
	inc	eax		;+1, one ahead
	bt	[rdi],rsi
	jc	.loop
	dec	eax            	;we were at +1, so fix it
.x:	ret

	
;;; edi = addr of top+1,   	;used as bit 
;;; esi = addr of bottom
;;; return: high=cnt low=bits at reference
bits_next_ref:
	xor	eax,eax		;eax = bit counter
	xor	edx,edx		;edx = base(0);
	
.loop:	dec	edi
	cmp	edi,esi
	je      .done          	;if offset is 0, exit with 0
	bt	[rdx],rdi
	jnc	.loop
 
	;; got a 1!  now count
.loop1:	inc	eax
	dec     edi             ; previous bit
	bt	[rdx],rdi
	jc	.loop1
	shl	rax,32		; put count into high dword
	inc	edi 		; point at actual ref
	or	rax,rdi		; put offset into low dword;
.done:
	ret
	
;;; edi = addr of top+1,   	;used as bit 
;;; esi = addr of bottom
;;; edx = old
;;; ecx = new
;;;
%if 0
bits_reref:

	push	rbx
	sub	rcx,rdx         ;compute fixup difference, S64
.loop0:	xor	eax,eax		;eax = bit counter
	xor	ebx,ebx		;edx = base(0);
.loop:	dec	edi		;scan bits down, until
	cmp	edi,esi        	;the very bottom
	je      .done          	;if offset is 0, exit with 0
	bt	[rbx],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits

 	;; got a 1!  now count
.loop1:	inc	eax
	dec     edi             ; previous bit
	bt	[rbx],rdi
	jc	.loop1
	inc	edi 		; point at actual ref
	dec	eax
	jnz	.large
;;; 32-bit offset relocation
	mov	ebx,[rdi]	;load 32-bit offset
	lea	ebx,[rbx+rdi+4] ;convert to pointer
	cmp	ebx,edx
	jne	.loop0		;go back to loop, but reset ebx
	add	[rdi],ecx	;fixup
	jmp	.loop0
;;; 64-bit relocation; edi = address  edx = old
.large: cmp	[rdi],rdx       ;pointing at old?
	jne	.loop0		;no match, go back to loop
	add	[rdi],rcx       ;fixup
	jmp	.loop0

.done:	pop	rbx
	ret
%endif


;;; edi = addr of top+1,   	;work pointer
;;; esi = addr of bottom        ;limit 
;;; edx = old
;;; ecx = new	 
bits_reref:
	 push	rbx             ;ebx = count of fixups
	 xor	ebx,ebx
	sub	rcx,rdx         ;compute fixup difference, S64
	inc     edi		;we will immediately decrement twice...
.loop0:	xor	eax,eax		;eax = base(0);
.loop1:	dec     edi				
.loop:	dec	edi		;scan bits down, until
	cmp	edi,esi        	;the very bottom
	jb      .done          	;if offset is 0, exit with 0
	bt	[rax],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits

 	;; got a 0-1 transition. !  now count
	dec     edi
	bt	[rax],rdi
	jc	.notone
	;a single bit was set.  Off32.
.one:	inc    	edi		;point at first 1 bit again
	mov	eax,[rdi]	;load 32-bit offset
	lea	eax,[rax+rdi+4] ;convert to abs32
	cmp	eax,edx
	jne	.loop0		;go back to loop; skip this 1 and prev 0
	add	[rdi],ecx	;fixup
	dec	edi            	;
	 inc	ebx
	jmp	.loop0
.done:
	mov	eax,ebx 	;return number of fixes
	pop	rbx
	ret
	
.notone: dec	edi
	bt	[rax],rdi	
	jc      .three
	
.two:	;; 64-bit relocation; edi = address  edx = old
	inc	edi
	cmp	[rdi],rdx       ;pointing at old?
	jne	.loop1		;no match, go back to loop (rax is 0);
	add	[rdi],rcx       ;fixup
	 inc	 ebx
	jmp	.loop1
	
.three:;; 32-bit absolute; assuming next bit is 0; pointing at it.
	cmp	[rdi],edx
	jne	.loop0
	add	[rdi],ecx
	 inc    ebx
	jmp	.loop0

;;; FIXDOWN: upon deletion, we create a hole at edx, of ecx bytes.
;;; once the hole is compacted, we need to drop all references to above
;;; the hole by ecx...
;;; edi = addr of top+1,   	;work pointer
;;; esi = addr of bottom        ;limit 
;;; edx = hole
;;; ecx = size of hole (fixup amout)
bits_fixdown:
	 push	rbx             ;ebx = count of fixups
	xor	ebx,ebx
	inc     edi		;we will immediately decrement twice...
.loop0:	xor	eax,eax		;eax = base(0);
.loop1:	dec     edi				
.loop:	dec	edi		;scan bits down, until
	cmp	edi,esi        	;the very bottom
	jb      .done          	;if offset is 0, exit with 0
	bt	[rax],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits

 	;; got a 0-1 transition. !  now count
	dec     edi             ;second bit below top 1
	bt	[rax],rdi
	jc	.notone
;;; Relative 32-bit offset is tricky: we need to check if the reference
;;; crosses the hole.  Refs that are local to either side of the hole are
;;; unchanged..
.one:	inc    	edi		;point at first 1 bit again
	cmp     edi,edx         ;is the reference site below or above?
	mov	eax,[rdi]	;load 32-bit offset
	lea	eax,[rax+rdi+4] ;convert to abs32
	jb      .below
.above:				;reference site is above...
	cmp	eax,edx		;is target below?
	jae	.loop0		;no, it's above, so no fixup here.
	add	[rdi],ecx	;fixup: make neg offset less negative!
	dec	edi            	;
 	inc	ebx
	jmp	.loop0
.below:				;reference site is below hole;
	cmp	eax,edx         ;is target above?
	jb	.loop0		;no, it's below too, so no fixup.
	sub	[rdi],ecx	;fixup: make pos offset less positive.
	dec	edi            	;
 	inc	ebx
	jmp	.loop0
.done:
	mov	eax,ebx 	;return number of fixes
	pop	rbx
	ret
	
.notone: dec	edi             ;point at third-down bit
	bt	[rax],rdi	
	jc      .three
	
.two:	;; 64-bit relocation; edi = address  edx = old
	inc	edi
	cmp	[rdi],rdx       ;pointing above hole?
	jc	.loop1		;below hole, skip fixup
	sub	[rdi],rcx       ;fixup
 	inc	 ebx		;
	jmp	.loop1
;;; 	// ;; BROKEN! skip refs to segs other than hole segment
.three:;; 32-bit absolute       ;pointing at third 1 bit,
	cmp	[rdi],edx	;compare ref target with hole
	jc	.loop0          ;below hole, skip fixup
	cmp	[rdi],esi	
	jc      .loop0          ;if outside of segment, skip
	sub     [rdi],ecx
	inc    ebx
	jmp	.loop0 


;;; edi = start of hole
;;; esi = end of hole
;;; edx = ?
;;; ecx = byte count from end of hole to top of segment
bits_hole:	
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
	pop	rcx
	pop	rsi
	pop	rdi

	shr	edi,3
	shr	esi,3
	shr	ecx,3


	rep 	movsb		;edi points at new reltop, esi, old


	mov	ecx,edx         ;hole size
	shr	ecx,3      
	xor	eax,eax         ;fill top with 0s
	rep     stosb
.x:
	mov	eax,edx         ;return hole size
	ret
