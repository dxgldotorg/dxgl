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

atoi PROC string:ptr byte
	push esi
	push edx
	mov eax, 0
	mov edx, 0
	mov esi, string
scanloop:
	mov al, byte ptr [esi]
	cmp al, 0
	je return
	cmp al, '-'
	je negative
	and al, '0'
	cmp al, '0'
	je positive
	inc esi
	jmp scanloop
negative:
	inc esi
negloop:
	mov al, byte ptr [esi]
	mov ah, al
	and ah, '0'
	cmp ah, '0'
	jne return
	imul edx, 10
	mov ah, 0
	sub edx, eax
	inc esi
	jmp negloop
positive:
	mov al, byte ptr [esi]
	mov ah, al
	and ah, '0'
	cmp ah, '0'
	jne return
	imul edx, 10
	mov ah, 0
	add edx, eax
	inc esi
	jmp positive
return:
	mov eax,edx
	pop edx
	pop esi
	ret
atoi ENDP

end