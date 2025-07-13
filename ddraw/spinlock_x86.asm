; DXGL
; Copyright (C) 2024 William Feely

; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.

; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.

; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


IFDEF ASM32
.model flat, C
ENDIF ;ASM32

IFDEF ASMX86

IFDEF ASM32
Sleep PROTO STDCALL dwMilliseconds:DWORD
ELSE
Sleep PROTO dwMilliseconds:DWORD
ENDIF

.code

IFDEF ASM32
EnterSpinlock PROC spinlock:DWORD, max_lock:DWORD
LOCAL counter:dword
LOCAL pointer:dword
	mov ebx, spinlock
	mov ecx, max_lock
	test ecx, ecx
	jz SpinLockNoLimit
SpinlockLimit:
	mov eax, 1
	xchg eax, [ebx]
	test eax, eax
	jnz DecrementCounter
	ret
DecrementCounter:
	mov edx, ecx
	dec ecx
	test edx,edx
	jnz SpinlockLimit
	mov ecx, 0
	mov counter, ecx
	mov pointer, ebx
	push 0
	call Sleep
	mov ebx, pointer
	mov ecx, counter
	jmp SpinlockLimit
SpinlockNoLimit:
	mov eax, 1
	xchg eax, [ebx]
	test eax, eax
	jnz SpinlockNoLimit
	ret
EnterSpinlock ENDP
ELSE
EnterSpinlock PROC ;spinlock:rcx, max_lock:edx
LOCAL counter:dword
LOCAL pointer:qword
	mov r10d, edx
	test r10d, r10d
	jz SpinLockNoLimit
SpinlockLimit:
	mov eax, 1
	xchg eax, [rcx]
	test eax, eax
	jnz DecrementCounter
	ret
DecrementCounter:
	mov r11d, r10d
	dec r10d
	test r11d,r11d
	jnz SpinlockLimit
	mov r10d, 0
	mov counter, r10d
	mov pointer, rcx
	mov rcx, 0
	sub rsp,32
	call Sleep
	add rsp,32
	mov rcx, pointer
	mov r10d, counter
	jmp SpinlockLimit
SpinlockNoLimit:
	mov eax, 1
	xchg eax, [rcx]
	test eax, eax
	jnz SpinlockNoLimit
	ret
EnterSpinlock ENDP
ENDIF

IFDEF ASM32
LeaveSpinlock PROC spinlock:DWORD
	mov ebx, spinlock
	xor eax,eax
	xchg eax, [ebx]
	ret
LeaveSpinlock ENDP
ELSE
LeaveSpinlock PROC ;spinlock:rcx
	xor eax,eax
	xchg eax, [rcx]
	ret
LeaveSpinlock ENDP
ENDIF

ENDIF ;ASMX86

END