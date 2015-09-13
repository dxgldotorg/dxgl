// DXGL
// Copyright (C) 2012-2015 William Feely

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
#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

// Color orders:
// 0 - ABGR
// 1 - ARGB
// 2 - BGRA
// 3 - RGBA
// 4 - R or Indexed
// 5 - Luminance
// 6 - Alpha
// 7 - Luminance Alpha

extern const DDPIXELFORMAT texformats[];
extern int numtexformats;
struct glRenderer;
DWORD CalculateMipLevels(DWORD width, DWORD height);

void glTexture_Create(glExtensions *ext, glUtil *util, glTexture **texture, const DDSURFACEDESC2 *ddsd, GLsizei bigx, GLsizei bigy, HGLRC hrc, DWORD screenbpp);
void glTexture__CreateSimple(glTexture *This);
void glTexture_AddRef(glTexture *This);
void glTexture_Release(glTexture *This, BOOL backend, struct glRenderer *renderer);
void glTexture__Destroy(glTexture *texture);
void glTexture__SetPixelFormat(glTexture *This);
void glTexture__Modify(glTexture *texture, const DDSURFACEDESC2 *ddsd, GLsizei bigx, GLsizei bigy, BOOL preservedata);
BOOL glTexture__Repair(glTexture *This);
void glTexture__Upload(glTexture *This, int level, BOOL checkerror, BOOL realloc);
void glTexture__UploadSimple(glTexture *This, int level, BOOL checkerror, BOOL realloc);
void glTexture__Download(glTexture *This, int level);
void glTexture__DownloadSimple(glTexture *This, int level);
void glTexture__SetFilter(glTexture *This, int level, GLint mag, GLint min, glExtensions *ext, glUtil *util);
void glTexture__ScaleUpload(glTexture *This, int level);
void glTexture__ScaleDownload(glTexture *This, int level);
HRESULT glTexture__Lock(glTexture *This, RECT *r, DDSURFACEDESC2 *ddsd, DWORD flags, int level);
void glTexture__Unlock(glTexture *This, RECT *r, int level);
BOOL glTexture__CreateDIB(glTexture *This, int level);
void glTexture__DeleteDIB(glTexture *This, int level);
HDC glTexture__GetDC(glTexture *This, struct glRenderer *renderer, int level);
void glTexture__ReleaseDC(glTexture *This, int level);

#ifdef __cplusplus
}
#endif

#endif //_TEXTUREMANAGER_H
