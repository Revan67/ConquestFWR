comment %
//--------------------------------------------------------------------------//
//                                                                          //
//                             COMHeap.ASM                                  //
//                                                                          //
//        $Author: Jasony $
//--------------------------------------------------------------------------//
%

.386
.model FLAT, SYSCALL


DAHEAPFLAG_NOMSGS  EQU 00000010H

;----------------------------------------------------------------------------

PUBLIC	_HEAP				; HEAP
PUBLIC	__nh_malloc			
PUBLIC  ??2@YAPAXI@Z 
PUBLIC  ??3@YAXPAX@Z 
PUBLIC	??_V@YAXPAX@Z

IFNDEF NOIMPORTS
EXTRN	__imp__HEAP_Acquire:NEAR
EXTRN	__imp__DACOM_Acquire:NEAR
EXTRN   __imp__GetModuleFileNameA@12:NEAR
ENDIF

.data

_HEAP   DD  0                              ; HEAP (pointer to IHeap interface)	
??_C@_05DMHA@IHeap?$AA@ DB 'IHeap', 00H			; `string'
defaultMsg           byte 260 dup (0)
dwUseMsgs			 DWORD -1

;----------------------------------------------------------------------------
.code

;------------------------------EXTERNS-------------------------------------
_SetDefaultHeapMsg proto


;-----------------------------DECLARATIONS--------------------------------
IFNDEF NOIMPORTS

_local_strrchr  proc private

				;; ecx is already set to the length of the filename

				std
				push	edi
				mov		edi, DWORD PTR [esp + 8]		;; string ptr
				mov		eax, DWORD PTR [esp + 12]		;; search character
				lea		edi, [edi+ecx]

				repne scasb
				lea		edi, [edi+1]
				je		@F
				xor		edi,edi
@@:
				mov		eax,edi	
Done:
				pop		edi
				cld
				ret
_local_strrchr  endp

;------------------------------------------------------

_set_name		proc  private USES EDI ESI EBX

				sub		esp, 260		;; local area
				mov		byte ptr [esp], 0
				
				mov		eax,esp		; eax -> buffer
				push	260					; 00000104H
				push	eax
				push	0			; hModule
				call	DWORD PTR __imp__GetModuleFileNameA@12


			; 213  : 	ptr = strrchr(szFileName, '\\');

				mov		ecx, eax		; ecx -> length of name
				mov		eax, esp		; eax -> buffer
				push	92					; 0000005cH
				push	eax
;;				call	_strrchr
				call	_local_strrchr
				add	esp, 8

			; 214  : 	if (ptr == 0)

				or		eax,eax
				lea		eax, [eax+1]
				jne		@F
				mov	eax, esp
@@:
			; 218  : 
			; 219  : 	SetDefaultHeapMsg (ptr);

				push	eax
				call	_SetDefaultHeapMsg

				add	esp, 4+260
				ret
_set_name		endp

;------------------------------------------------------

__heap_init     proc

                call    DWORD PTR __imp__HEAP_Acquire
				or		eax,eax
	            mov     _HEAP, eax
				je		Done

				push	eax
				mov		eax, DWORD PTR [eax]
				call	DWORD PTR [eax+21*4]		;; GetHeapFlags()
				and		eax, DAHEAPFLAG_NOMSGS			; 00000010H
				cmp		eax, 1
				sbb		eax, eax
				mov		DWORD PTR dwUseMsgs, eax

				call	_set_name
                mov     eax,1                   ;return success
Done:
				ret
__heap_init 	endp

ENDIF

;------------------------------------------------------
__heap_term	proc 
		ret
__heap_term	endp

;------------------------------------------------------

_heapSize$ = 8
_growSize$ = 12
_dwFlags$ = 16
_pHeap$ = -24
_desc$ = -20
_InitializeDAHeap PROC NEAR			; InitializeDAHeap, COMDAT

; 201  : {

	mov	eax, DWORD PTR _heapSize$[esp-4]
	mov	ecx, DWORD PTR _growSize$[esp-4]
	mov	edx, DWORD PTR _dwFlags$[esp-4]
	sub	esp, 24					; 00000018H

; 202  : 	IHeap *pHeap;
; 203  : 	DAHEAPDESC desc;

	mov	DWORD PTR _desc$[esp+32], eax
	mov	DWORD PTR _desc$[esp+40], ecx
	lea	eax, DWORD PTR _pHeap$[esp+24]
	lea	ecx, DWORD PTR _desc$[esp+24]
	mov	DWORD PTR _desc$[esp+36], edx
	push	eax
	push	ecx

	call    DWORD PTR __imp__DACOM_Acquire
	mov	DWORD PTR _desc$[esp+36], OFFSET FLAT:??_C@_05DMHA@IHeap?$AA@ ; `string'
	mov	DWORD PTR _desc$[esp+32], 20		; 00000014H

; 204  : 
; 205  : 	desc.heapSize = heapSize;
; 206  : 	desc.growSize = growSize;
; 207  : 	desc.flags = dwFlags;
; 208  : 
; 209  : 	if (HEAP->CreateInstance(&desc, (void **)&pHeap) == GR_OK)

	push	eax
	mov	eax, DWORD PTR [eax]
	call	DWORD PTR [eax+12]
	test	eax, eax
	jne	SHORT $L29930

; 210  : 	{
; 211  : 		HEAP = pHeap;

	mov	eax, DWORD PTR _pHeap$[esp+24]
	add	esp, 24					; 00000018H
	mov	DWORD PTR _HEAP, eax

		;  set dwUseMsgs to 0 if DAHEAPFLAG_NOMSGS is used

; 285  : 	if (flag & DAHEAPFLAG_NOMSGS)

	mov	eax, DWORD PTR _dwFlags$[esp-4]
	and	eax, DAHEAPFLAG_NOMSGS			; 00000010H
	cmp	eax, 1
	sbb	eax, eax
	mov DWORD PTR dwUseMsgs, eax

; 212  : 		return 1;

	mov	eax, 1
	ret	0
$L29930:

; 213  : 	}
; 214  : 	return 0;

	xor	eax, eax
	add	esp, 24					; 00000018H

; 215  : }

	ret	0
_InitializeDAHeap ENDP				; InitializeDAHeap

;------------------------------------------------------

_SetDefaultHeapMsg proc

                push    edi
                push    esi
                mov     ecx, 256/4
                mov     edi, offset defaultMsg
                mov     esi, DWORD PTR [esp+12]
                rep movsd
                pop     esi
                pop     edi
                ret

_SetDefaultHeapMsg endp

;------------------------------------------------------
__nh_malloc		proc

		mov	eax, DWORD PTR _HEAP
		mov	ecx, DWORD PTR [eax]
		push	OFFSET defaultMsg
		push	eax
		call	DWORD PTR [ecx+88]			;  HEAP->malloc_pass_through();
		ret

__nh_malloc		endp

;new
??2@YAPAXI@Z    proc

		mov	eax, DWORD PTR _HEAP
		mov	ecx, DWORD PTR [eax]
		push	OFFSET defaultMsg
		push	eax
		call	DWORD PTR [ecx+88]			;  HEAP->malloc_pass_through();
		ret

??2@YAPAXI@Z    endp

;delete
??3@YAXPAX@Z    proc
		; HEAP->FreeMemory(ptr);

		mov	eax, DWORD PTR [esp+4]
		or  eax, eax
		mov	ecx, DWORD PTR _HEAP	; HEAP
		je	@F
		push	eax
		push	ecx
		mov	eax, DWORD PTR [ecx]
		call	DWORD PTR [eax+28]
@@:
		ret	
??3@YAXPAX@Z    endp

;delete[]
??_V@YAXPAX@Z    proc
		; HEAP->FreeMemory(ptr);

		mov	eax, DWORD PTR [esp+4]
		or  eax, eax
		mov	ecx, DWORD PTR _HEAP	; HEAP
		je	@F
		push	eax
		push	ecx
		mov	eax, DWORD PTR [ecx]
		call	DWORD PTR [eax+28]
@@:
		ret	
??_V@YAXPAX@Z     endp

;------------------------------------------------------

__malloc_dbg         proc  

		mov	eax, DWORD PTR _HEAP
		mov	ecx, DWORD PTR [eax]
		push	OFFSET defaultMsg
		push	eax
		call	DWORD PTR [ecx+88]			;  HEAP->malloc_pass_through();
		ret

__malloc_dbg  	endp

_malloc         proc  

		mov	eax, DWORD PTR _HEAP
		mov	ecx, DWORD PTR [eax]
		push	OFFSET defaultMsg
		push	eax
		call	DWORD PTR [ecx+88]			;  HEAP->malloc_pass_through();
		ret

_malloc  	endp

;------------------------------------------------------

_realloc        proc 

		mov	eax, DWORD PTR _HEAP
		mov	ecx, DWORD PTR [eax]
		push	OFFSET defaultMsg
		push	eax
		call	DWORD PTR [ecx+92]			;  HEAP->realloc_pass_through();
		ret

_realloc  	endp

;---------------------------------------------------------------

_calloc         proc  

		mov	eax, DWORD PTR _HEAP
		mov	ecx, DWORD PTR [eax]
		push	OFFSET defaultMsg
		push	eax
		call	DWORD PTR [ecx+96]			;  HEAP->calloc_pass_through();
		ret

_calloc  	endp

;----------------------------------------------------------------

_free           proc  

; HEAP->FreeMemory(ptr);

		mov	eax, DWORD PTR [esp+4]
		or  eax, eax
		mov	ecx, DWORD PTR _HEAP	; HEAP
		je	@F
		push	eax
		push	ecx
		mov	eax, DWORD PTR [ecx]
		call	DWORD PTR [eax+28]
@@:
		ret	
_free 		endp

__free_dbg           proc  

; HEAP->FreeMemory(ptr);

		mov	eax, DWORD PTR [esp+4]
		or  eax, eax
		mov	ecx, DWORD PTR _HEAP	; HEAP
		je	@F
		push	eax
		push	ecx
		mov	eax, DWORD PTR [ecx]
		call	DWORD PTR [eax+28]
@@:
		ret	
__free_dbg 		endp

;---------------------------------------------------------------
public _check_heap
_check_heap	PROC

; 52   : 	HEAP->EnumerateBlocks();

	mov	eax, DWORD PTR _HEAP	; HEAP
	push	0
	push	0
	push	eax
	mov	ecx, DWORD PTR [eax]
	call	DWORD PTR [ecx+32]
	ret	

_check_heap	ENDP
;---------------------------------------------------------------
__msize	PROC	

; 52   : 	return HEAP->GetBlockSize(ptr);

	mov	eax, DWORD PTR [esp+4]
	mov	ecx, DWORD PTR _HEAP	; HEAP
	push	eax
	push	ecx
	mov	eax, DWORD PTR [ecx]
	call	DWORD PTR [eax+36]
	ret	

__msize	ENDP


__msize_dbg	PROC	

; 52   : 	return HEAP->GetBlockSize(ptr);

	mov	eax, DWORD PTR [esp+4]
	mov	ecx, DWORD PTR _HEAP	; HEAP
	push	eax
	push	ecx
	mov	eax, DWORD PTR [ecx]
	call	DWORD PTR [eax+36]
	ret	

__msize_dbg	ENDP

;---------------------------------------------------------------

		end

;---------------------------------------------------------------
;------------------END COMHeap.asm------------------------------
;---------------------------------------------------------------
