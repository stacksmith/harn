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

bit_set2:
	bts [rdi],rsi
	inc rsi
bit_set1:
	bts [rdi],rsi
	ret

;;; rdi = base
;;; rsi = bit
;;; rdx = cnt
bits_set:
	bts [rdi],rsi
	inc rsi
	dec rdx
	jnz bits_set
	ret
	
	
bit_test:
	xor	eax,eax
	bt	[rdi],rsi
	sbb	eax,eax
	ret
	
bits_cnt:
	dec	rsi		;previous bit must be 0!
	bt  	[rdi],rsi
	sbb	eax,eax		;-1 if set, 0 if clear
	js      .x
	dec	eax
.loop:	
	inc	rsi
	inc	eax
	bt	[rdi],rsi
	jc	.loop
.x:	ret

	
;;; edi = base
;;; esi = bits
;;; return: high=cnt low=bits at reference
bits_next_ref:
	xor	eax,eax
	
.loop:	sub	esi,1
	jz      .done          	;if offset is 0, exit with 0
	bt	[rdi],rsi
	jnc	.loop
 
	;; got a 1!  now count
.loop1:	inc	eax
	dec     esi             ; previous bit
	bt	[rdi],rsi
	jc	.loop1
	shl	rax,32		; put count into high dword
	inc	esi 		;
	or	rax,rsi		; put offset into low dword;
.done:
	ret
	



	
