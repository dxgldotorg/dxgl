// DXGL
// Copyright (C) 2011-2016 William Feely

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
#ifndef _GLEXTENSIONS_H
#define _GLEXTENSIONS_H

#ifndef GL_DEPTH_BUFFER
#define GL_DEPTH_BUFFER 0x8223
#endif
#ifndef GL_STENCIL_BUFFER
#define GL_STENCIL_BUFFER 0x8224
#endif

#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_RGB565 0x8D62

#ifdef __cplusplus
extern "C" {
#endif

void glExtensions_Init(glExtensions *ext, HDC hdc, BOOL core);

#ifdef __cplusplus
}
#endif

#endif //_GLEXTENSIONS_H
