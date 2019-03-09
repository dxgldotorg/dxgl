// DXGL
// Copyright (C) 2012-2019 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY W	ARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#pragma once
#ifndef _GLTEXTURE_H
#define _GLTEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

struct BufferObject;

extern const DDPIXELFORMAT texformats[];
extern int numtexformats;

struct glUtil;

DWORD CalculateMipLevels(DWORD width, DWORD height);

HRESULT glTexture_Create(const DDSURFACEDESC2 *ddsd, glTexture **texture, struct glRenderer *renderer, GLint bigwidth, GLint bigheight, BOOL zhasstencil, BOOL backend, GLenum targetoverride);
ULONG glTexture_AddRef(glTexture *This);
ULONG glTexture_Release(glTexture *This, BOOL backend);
HRESULT glTexture_Lock(glTexture *This, GLint level, LPRECT r, LPDDSURFACEDESC2 ddsd, DWORD flags, BOOL backend);
HRESULT glTexture_Unlock(glTexture *This, GLint level, LPRECT r, BOOL backend);
HRESULT glTexture_GetDC(glTexture *This, GLint level, HDC *hdc, glDirectDrawPalette *palette);
HRESULT glTexture_ReleaseDC(glTexture *This, GLint level, HDC hdc);
void glTexture_SetPalette(glTexture *This, glTexture *palette, BOOL backend);
void glTexture_SetStencil(glTexture *This, glTexture *stencil, BOOL backend);
void glTexture_CreateDummyColor(glTexture *This, BOOL backend);
void glTexture_DeleteDummyColor(glTexture *This, BOOL backend);
BOOL glTexture_ValidatePixelFormat(DDPIXELFORMAT *pixelformat);
void glTexture__SetFilter(glTexture *This, int level, GLint mag, GLint min, struct glRenderer *renderer);
HRESULT glTexture__SetSurfaceDesc(glTexture *This, LPDDSURFACEDESC2 ddsd);
void glTexture__Download(glTexture *This, GLint level);
void glTexture__Upload(glTexture *This, GLint level);
void glTexture__Upload2(glTexture *This, int level, int width, int height, BOOL checkerror, BOOL dorealloc, glUtil *util);
BOOL glTexture__Repair(glTexture *This, BOOL preserve);
void glTexture__SetPrimaryScale(glTexture *This, GLint bigwidth, GLint bigheight, BOOL scaling);
void glTexture__FinishCreate(glTexture *This);
void glTexture__Destroy(glTexture *This);

#ifdef __cplusplus
}
#endif

#endif //_GLTEXTURE_H
