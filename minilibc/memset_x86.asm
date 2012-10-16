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

memset PROC dest: ptr byte, char: dword, count: dword
	jmp dword ptr memset_ptr
memset ENDP

memset_386 PROC dest: ptr byte, char: dword, count: dword
	push ecx
	push edi
	mov eax, char
	mov ecx, count
	mov edi, dest
	rep stosb
	mov eax, dest
	pop edi
	pop ecx
memset_386 ENDP
end