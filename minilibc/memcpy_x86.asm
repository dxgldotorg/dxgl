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

MEMCPY_ASM = 1

INCLUDE common.inc

memcpy PROC dest: ptr byte, src: ptr byte, count: dword
	jmp dword ptr memcpy_ptr
memcpy ENDP

memcpy_386 PROC dest: ptr byte, src: ptr byte, count: dword
	push esi
	push edi
	push ecx
	mov esi, src
	mov edi, dest
	mov ecx, count
	rep movsb
	mov eax, dest
	pop ecx
	pop edi
	pop esi
	ret
memcpy_386 ENDP

end
