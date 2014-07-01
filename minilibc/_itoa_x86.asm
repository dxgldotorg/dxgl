; DXGL
; Copyright (C) 2012 William Feely

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
INCLUDE common.inc

_itoa PROC value:dword, string:ptr byte, radix:dword
	mov eax, radix
	cmp radix, 2
	jl fail
	cmp radix, 36
	jg fail
	jmp begin
fail:
	mov eax, string
	ret
begin:
	push esi
	push edi
	push edx
	mov edi, string
	mov eax, value
	cmp value, 0
	je zero
	mov esi, itoachars
	cmp radix, 10
	jne unsigned
	test eax, 0
	js negative
positive:
	mov edx, 0
	idiv radix
	mov bl, byte ptr [esi+edx]
	mov byte ptr [edi], bl
	inc edi
	cmp eax, 0
	je return
	jmp positive
negative:
	mov byte ptr [edi], '-'
	inc edi
negloop:
	mov edx, 0
	idiv radix
	mov bl, byte ptr [esi+edx]
	mov byte ptr [edi], bl
	inc edi
	cmp eax, 0
	je return
	jmp negloop
unsigned:
	mov edx, 0
	div radix
	mov bl, byte ptr [esi+edx]
	mov byte ptr [edi], bl
	inc edi
	cmp eax, 0
	je return
	jmp unsigned
zero:
	mov byte ptr [edi], '0'
	inc edi
	mov byte ptr [edi], 0
return:
	mov eax, string
	pop edx
	pop edi
	pop esi
	ret
_itoa ENDP

end