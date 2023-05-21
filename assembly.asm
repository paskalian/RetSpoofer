IFDEF RAX

.CODE

EXECUTE PROC
	; RCX = FunctionAddress, RDX = ManipAddress, R8 = VariablesAddress, R9 = VariableAmount
	mov [rsp + 08h], rcx								; USING THE SHADOW SPACE ALLOCATED FOR OUR FUNCTION VARIABLES TO STORE THEM
	mov [rsp + 10h], rdx								; USING THE SHADOW SPACE ALLOCATED FOR OUR FUNCTION VARIABLES TO STORE THEM
	mov [rsp + 18h], r8									; USING THE SHADOW SPACE ALLOCATED FOR OUR FUNCTION VARIABLES TO STORE THEM
	push 0												; IT DOESN'T ALLOW ME TO SET IT DIRECTLY SO...
	pop [rsp + 20h]										; USING THE SHADOW SPACE ALLOCATED FOR OUR FUNCTION VARIABLES TO STORE THEM
	; [rsp + 28h] CONTAINS THE CALL CONVENTION BUT WE IGNORE IT ON X64

	cmp r9, 4											; COMPARING VariableAmount WITH 4
	jbe NOEXTRA											; IF THE VariableAmount IS BELOW OR EQUAL TO 4 THEN IT MEANS THERES NO EXTRA VARIABLES SO WE JUMP TO NOEXTRA
	sub r9, 4											; OTHERWISE WE SUBTRACT 4 FROM IT TO GET THE EXTRA VARIABLE COUNT
    mov rsi, r9                                         ; COPYING THE EXTRA VARIABLE COUNT INTO RSI
	mov [rsp + 20h], r9									; USING THE SHADOW SPACE ALLOCATED FOR OUR FUNCTION VARIABLES TO STORE THEM
	test r9, 1											; CHECKING IF THE EXTRA ARGUMENT COUNT WAS DIVISABLE BY 2
    jnz EXTRA_PUSHES                                    ; IF IT IS **NOT** DIVISABLE THERE IS NO NEED FOR STACK ALIGNMENT, THE REASON FOR THIS IS BECAUSE WE HAVE AN EXTRA PUSH (ACTUAL RETURN VALUE) WHICH BALANCES THE STACK.
STACK_ALIGN:
    sub rsp, 8                                          ; STACK ALIGNMENT
EXTRA_PUSHES:
	lea rbx, [RETPLACE]									; LOAD THE ADDRESS OF RETPLACE WHICH IS THE ACTUAL RETURN VALUE INTO RBX
	push rbx											; PUSH THE ACTUAL RETURN VALUE (RBX)
EXTRA_PUSHES_LOOP:
    push qword ptr[r8 + 18h + 08h * rsi]				; PUSHING THE EXTRA ARGUMENTS ONTO THE STACK FROM LAST-TO-FIRST ORDER
    dec rsi                                             ; DECREMENTING THE EXTRA STACK VARIABLES AMOUNT SINCE ONE OF THEM IS HANDLED
    test rsi, rsi                                       ; CHECKING IF THERE ARE ANY MORE EXTRA STACK VARIABLES LEFT
    jnz EXTRA_PUSHES_LOOP                               ; IF THERE ARE ANY MORE LEFT WE JUMP BACK TO EXTRA_PUSHES, IF NOT WE GO ON BY FCALL
NOEXTRA:
    sub rsp, 20h                                        ; OPENING UP STACK SPACE FOR VARIABLES
    push rdx											; PUSHING THE GADGET ADDRESS
	mov rbx, rcx										; COPYING THE FUNCTION ADDRESS INTO RBX SINCE RCX IS ABOUT TO BE MODIFIED
	mov rcx, [r8]										; SETTING THE FIRST 4 ARGUMENTS RESPECTIVELY
	mov rdx, [r8 + 08h]									; SETTING THE FIRST 4 ARGUMENTS RESPECTIVELY
	mov r9, [r8 + 18h]									; SETTING THE FIRST 4 ARGUMENTS RESPECTIVELY, SINCE R8 ITSELF WILL BE MODIFIED R9 IS DONE FIRST
	mov r8, [r8 + 10h]									; SETTING THE FIRST 4 ARGUMENTS RESPECTIVELY
	jmp rbx												; JUMPING TO THE FUNCTION
RETPLACE:
	mov r9, [rsp + 20h]									; GETTING BACK THE STORED EXTRA ARGUMENT COUNT TO R9
    test r9, 1											; CHECKING IF THE EXTRA ARGUMENT COUNT WAS DIVISABLE BY 2
    jnz FEND                                            ; IF IT IS **NOT** DIVISABLE THERE IS NO NEED FOR STACK ALIGNMENT, THE REASON FOR THIS IS BECAUSE WE HAVE AN EXTRA PUSH (ACTUAL RETURN VALUE) WHICH BALANCES THE STACK.
STACK_ALIGN2:
    add rsp, 8                                          ; STACK ALIGNMENT
FEND:
	ret													; ACTUAL RETURN
EXECUTE ENDP

ELSE
.386
.MODEL FLAT, C

.CODE

EXECUTE PROC
	; EBP+0x8 = FunctionAddress, EBP+0xC = ManipAddress, EBP+0x10 = VariablesAddress, EBP+0x14 = VariableAmount, EBP+0x18 = CallConvention
	push ebp
	mov ebp, esp

    lea ebx, [FEND]                                     ; LOAD THE ADDRESS OF RETPLACE WHICH IS THE ACTUAL RETURN VALUE INTO RBX
    push ebx                                            ; PUSH THE ACTUAL RETURN VALUE (EBX)
    mov eax, [ebp + 10h]                                ; MOVING THE VARIABLES BASE INTO EAX
    mov esi, [ebp + 14h]                                ; MOVING THE VARIABLE AMOUNT INTO ESI
    mov ebx, [ebp + 18h]                                ; GETTING THE CALLING CONVENTION INTO EBX
    cmp ebx, 2                                          ; CHECKING THE CALLING CONVENTION AGAINST FASTCALL
    je CONV_FASTCALL                                    ; IF ITS EQUAL TO FASTCALL THEN JUMPS TO FASTCALL
CONV_CDECL:                                             ; IF ITS ANYTHING EXCEPT THEN GOES ON BY CDECL AND STDCALL
CONV_STDCALL:
    push dword ptr[eax - 04h + 04h * esi]               ; PUSHING THE EXTRA ARGUMENTS ONTO THE STACK FROM LAST-TO-FIRST ORDER, JUMPED OVER THE RETURN ADDRESS THATS WHY ITS 0x8
    dec esi                                             ; DECREMENTING THE EXTRA STACK VARIABLES AMOUNT SINCE ONE OF THEM IS HANDLED
    test esi, esi                                       ; CHECKING IF THERE ARE ANY MORE EXTRA STACK VARIABLES LEFT
    jnz CONV_STDCALL                                    ; IF THERE ARE ANY MORE LEFT WE JUMP BACK TO CONV_STDCALL, IF NOT WE GO ON BY FCALL
    jmp FCALL                                           ; DIRECT JUMP TO FCALL
CONV_FASTCALL:
    mov ecx, dword ptr[eax]                             ; SETTING THE FIRST 2 PARAMETERS RESPECTIVELY
    mov edx, dword ptr[eax + 04h]                       ; SETTING THE FIRST 2 PARAMETERS RESPECTIVELY
    cmp esi, 2                                          ; COMPARING THE ARGUMENT COUNT AGAINST 2
    jbe FCALL                                           ; IF THE ARGUMENT COUNT IS NOT BIGGER THAN 2 OR EQUAL TO IT THEN THERE IS NO EXTRA ARGUMENTS SO WE JUMP OVER
    sub esi, 2                                          ; IF THERE ARE EXTRA ARGUMENTS WE SUBTRACT 2 FROM THE ARGUMENT COUNT TO GET THE AMOUNT OF EXTRA STACK VARIABLES
CONV_FASTCALL_PUSH:
    push dword ptr[eax + 04h + 04h * esi]               ; PUSHING THE EXTRA ARGUMENTS ONTO THE STACK FROM LAST-TO-FIRST ORDER
    dec esi                                             ; DECREMENTING THE EXTRA STACK VARIABLES AMOUNT SINCE ONE OF THEM IS HANDLED
    test esi, esi                                       ; CHECKING IF THERE ARE ANY MORE EXTRA STACK VARIABLES LEFT
    jnz CONV_FASTCALL_PUSH  
FCALL:
    push dword ptr[ebp + 0Ch]
    jmp dword ptr[ebp + 08h]
FEND:
	mov esp, ebp
	pop ebp
	ret
EXECUTE ENDP

ENDIF
END