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

strcpy PROC dest:ptr byte, src:ptr byte
	push src
	call strlen
	add esp, 4
	push eax
	push src
	push dest
	call memcpy
	add esp, 12
	mov eax, dest
	ret
strcpy ENDP

end