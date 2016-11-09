// DXGL
// Copyright (C) 2016 William Feely

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
#ifndef __CONST_H
#define __CONST_H

#ifdef __cplusplus
extern "C" {
#endif

	extern const RECT nullrect;
	extern const DWORD valid_rop_codes[256];
	extern const DWORD rop_texture_usage[256];
	extern const DWORD supported_rops[8];
	extern const DWORD supported_rops_gl2[8];

#ifdef _M_X64
#define _PTR_MASK 0xFFFFFFFFFFFFFFFFui64
#else
#define _PTR_MASK 0xFFFFFFFF
#endif
#ifdef __cplusplus
}
#endif

#endif //__CONST_H