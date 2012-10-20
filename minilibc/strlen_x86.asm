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

strlen PROC string:ptr byte
	push ecx
	push ebx
	push esi
	mov eax, string
	and eax, 3
	jnz aligned_loop
unaligned_loop:
	mov ecx, 0
	mov esi, string
	test byte ptr [esi],0
	jz return
	dec eax
	inc esi
	inc ecx
	jz aligned_loop
	jmp unaligned_loop
aligned_loop:
	mov eax, dword ptr [esi]
	mov ebx, eax
	and ebx, 0FFh
	jz return
	inc ecx
	mov ebx, eax
	and ebx, 0FF00h
	jz return
	inc ecx
	mov ebx, eax
	and ebx, 0FF0000h
	jz return
	inc ecx
	mov ebx, eax
	and ebx, 0FF000000h
	jz return
	inc ecx
	add esi, 4
	jmp aligned_loop
return:
	mov eax, ecx
	pop esi
	pop ebx
	pop ecx
	ret
strlen ENDP

end