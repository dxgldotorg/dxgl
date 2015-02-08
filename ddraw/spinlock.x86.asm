; DXGL
; Copyright (C) 2015 William Feely

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

.model flat, c
.code
extern C smp:dword
Sleep PROTO STDCALL arg1:DWORD

_enterspinlock proc uses eax ecx esi, spinlock:ptr dword
	mov eax, smp
	mov esi, spinlock
	test eax, eax
	jz nosmp
smpproc:
	mov ecx, 1000
	jmp spin_main
nosmp:
	xor ecx, ecx
spin_main:
	mov eax, 1
	xchg eax, [esi]
	test eax, eax
	jnz spintest
	ret
spintest:
	test ecx, ecx
	jz sleeploop
	dec ecx
	jmp spin_main
sleeploop:
	push ecx
	invoke Sleep, 0
	pop ecx
	jmp spin_main
_enterspinlock endp

_exitspinlock proc uses eax, spinlock:ptr dword
	mov esi, spinlock
	xor eax, eax
	xchg eax, [esi]
	ret
_exitspinlock endp

end