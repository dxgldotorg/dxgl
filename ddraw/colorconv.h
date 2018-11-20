// DXGL
// Copyright (C) 2018 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#pragma once
#ifndef __COLORCONV_H
#define __COLORCONV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*COLORCONVPROC) (size_t count, void *dest, void *src);
extern COLORCONVPROC colorconvproc[];

void rgba8332torgba8888(size_t count, DWORD *dest, WORD *src);
void rgba8888torgba8332(size_t count, WORD *dest, DWORD *src);
void rgb565torgba8888(size_t count, DWORD *dest, WORD *src);
void rgb565torgbx8888(size_t count, DWORD *dest, WORD *src);
void rgbx8888torgb565(size_t count, WORD *dest, DWORD *src);
void rgba1555torgba8888(size_t count, DWORD *dest, WORD *src);
void rgba8888torgba1555(size_t count, WORD *dest, DWORD *src);
void rgba4444torgba8888(size_t count, DWORD *dest, WORD *src);
void rgba8888torgba4444(size_t count, WORD *dest, DWORD *src);
void uyvytorgbx8888(size_t count, DWORD *dest, DWORD *src);

#ifdef __cplusplus
}
#endif

#endif //__COLORCONV_H