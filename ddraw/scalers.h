// DXGL
// Copyright (C) 2011-2014 William Feely

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
#ifndef _SCALERS_H
#define _SCALERS_H

#ifdef __cplusplus
extern "C" {
#endif

void ScaleNearest8(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch);
void ScaleNearest16(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch);
void ScaleNearest24(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch);
void ScaleNearest32(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch);

#ifdef __cplusplus
}
#endif

#endif //_SCALERS_H