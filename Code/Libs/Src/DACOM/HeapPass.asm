comment %
//--------------------------------------------------------------------------//
//                                                                          //
//                               HeapPass.asm                               //
//                                                                          //
//--------------------------------------------------------------------------//
%

.386
.model FLAT, SYSCALL


;----------------------------------------------------------------------------

.data

;----------------------------------------------------------------------------
.code

EXTRN	__imp__calloc:NEAR
EXTRN	__imp__malloc:NEAR
EXTRN	__imp__realloc:NEAR

_msg$ = 12
_this$ = 8
?malloc_pass_through@BaseHeap@@UAGPAXPBD@Z PROC NEAR	; BaseHeap::malloc_pass_through, COMDAT

; 594  : 	void * result;
; 595  : 	
; 596  : 	if ((result = AllocateMemory(12, msg)) != 0 && (dwFlags & DAHEAPFLAG_NOMSGS)==0)

	mov	ecx, DWORD PTR _msg$[esp-4]
	push	esi
	push	edi
	mov	edi, DWORD PTR _this$[esp+4]
	mov	esi, DWORD PTR [esp+24]  ; esi = numBytes
	push	ecx
	push	esi					; numBytes
	mov	eax, DWORD PTR [edi]
	push	edi
	call	DWORD PTR [eax+16]	; AllocateMemory()
	mov	esi, eax
	test	esi, esi
	je	SHORT $L43464
	test	BYTE PTR [edi+12], 16			; 00000010H
	jne	SHORT $L43464

; 597  : 		SetBlockOwner(result, addr);

	mov eax, DWORD PTR [esp+20]				; return address
	mov	edx, DWORD PTR [edi]
	push	eax					; addr
	push	esi
	push	edi
	call	DWORD PTR [edx+68]
$L43464:

; 598  : 	
; 599  : 	return result;

	mov	eax, esi
	pop	edi
	pop	esi

; 600  : }

	ret	8
?malloc_pass_through@BaseHeap@@UAGPAXPBD@Z ENDP		; BaseHeap::malloc_pass_through
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;	COMDAT ?realloc_pass_through@BaseHeap@@UAGPAXPBD@Z

_msg$ = 12
_this$ = 8
?realloc_pass_through@BaseHeap@@UAGPAXPBD@Z PROC NEAR	; BaseHeap::realloc_pass_through, COMDAT

; 605  : 	void * result;
; 606  : 	
; 607  : 	if ((result = ReallocateMemory(0, 12, msg)) != 0 && (dwFlags & DAHEAPFLAG_NOMSGS)==0)

	mov	ecx, DWORD PTR _msg$[esp-4]
	push	esi
	push	edi
	mov	edi, DWORD PTR _this$[esp+4]
	mov eax, DWORD PTR [esp+28]				; numbytes
	mov esi, DWORD PTR [esp+24]				; allocatedBlock
	push	ecx
	push	eax					; numBytes
	mov	eax, DWORD PTR [edi]
	push	esi					; allocatedBlock
	push	edi
	call	DWORD PTR [eax+24]		; ReallocateMemory()
	mov	esi, eax
	test	esi, esi
	je	SHORT $L43468
	test	BYTE PTR [edi+12], 16			; 00000010H
	jne	SHORT $L43468

; 608  : 		SetBlockOwner(result, retAddr);

	mov eax, DWORD PTR [esp+20]	; retAddr
	mov	edx, DWORD PTR [edi]
	push	eax					; retAddr
	push	esi
	push	edi
	call	DWORD PTR [edx+68]
$L43468:

; 609  : 	
; 610  : 	return result;

	mov	eax, esi
	pop	edi
	pop	esi

; 611  : }

	ret	8
?realloc_pass_through@BaseHeap@@UAGPAXPBD@Z ENDP	; BaseHeap::realloc_pass_through

_msg$ = 12
_this$ = 8
?calloc_pass_through@BaseHeap@@UAGPAXPBD@Z PROC NEAR	; BaseHeap::calloc_pass_through, COMDAT

; 616  : 	void * result;
; 617  : 	
; 618  : 	if ((result = ClearAllocateMemory(12, msg, 10)) != 0 && (dwFlags & DAHEAPFLAG_NOMSGS)==0)

	mov		ecx, DWORD PTR _msg$[esp-4]
	push	esi
	push	edi
	mov		edx, DWORD PTR [esp+24]			; numBytesPerElement
	mov		edi, DWORD PTR _this$[esp+4]
	imul	edx, DWORD PTR [esp+28]			; numElements  -> edx = total bytes to allocate
	push	0					; fill character
	push	ecx					; msg
	mov		eax, DWORD PTR [edi]
	push	edx					; number of bytes
	push	edi
	call	DWORD PTR [eax+20]		; ClearAllocateMemory()
	mov		esi, eax
	test	esi, esi
	je	SHORT $L43472
	test	BYTE PTR [edi+12], 16			; 00000010H
	jne	SHORT $L43472

; 619  : 		SetBlockOwner(result, retAddr);

	mov eax, DWORD PTR [esp+20]	; retAddr
	mov	edx, DWORD PTR [edi]
	push	eax					; retAddr
	push	esi
	push	edi
	call	DWORD PTR [edx+68]
$L43472:

; 620  : 	
; 621  : 	return result;

	mov	eax, esi
	pop	edi
	pop	esi

; 622  : }

	ret	8
?calloc_pass_through@BaseHeap@@UAGPAXPBD@Z ENDP		; BaseHeap::calloc_pass_through


;---------------------------------------------------------------------------------------
; ----------- Now do the pass-through code for the MSHeap ------------------------------
;---------------------------------------------------------------------------------------
;
?malloc_pass_through@MSHeap@@UAGPAXPBD@Z PROC NEAR	; MSHeap::malloc_pass_through, COMDAT

	add		esp, 12				; remove 3 dword params from the stack
	jmp		DWORD PTR __imp__malloc

?malloc_pass_through@MSHeap@@UAGPAXPBD@Z ENDP		; MSHeap::malloc_pass_through
;---------------------------------------------------------------------------------------
;
?realloc_pass_through@MSHeap@@UAGPAXPBD@Z PROC NEAR	; MSHeap::realloc_pass_through, COMDAT

	add		esp, 12				; remove 3 dword params from the stack
	jmp		DWORD PTR __imp__realloc

?realloc_pass_through@MSHeap@@UAGPAXPBD@Z ENDP		; MSHeap::realloc_pass_through
;---------------------------------------------------------------------------------------
;
?calloc_pass_through@MSHeap@@UAGPAXPBD@Z PROC NEAR	; MSHeap::calloc_pass_through, COMDAT

	add		esp, 12				; remove 3 dword params from the stack
	jmp		DWORD PTR __imp__calloc

?calloc_pass_through@MSHeap@@UAGPAXPBD@Z ENDP		; MSHeap::calloc_pass_through



		end

;------------------------------------------------------------------
;------------------END HeapPass.asm--------------------------------
;------------------------------------------------------------------
