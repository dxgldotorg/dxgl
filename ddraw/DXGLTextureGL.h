// DXGL
// Copyright (C) 2024 William Feely

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

#ifndef _DXGLTEXTUREGL_H
#define _DXGLTEXTUREGL_H
#ifdef __cplusplus
extern "C" {
#endif


HRESULT DXGLTextureGL_ValidatePixelFormat(LPDDPIXELFORMAT pixelformat, DWORD *texformat);
HRESULT DXGLTextureGL_Create(LPDDSURFACEDESC2 ddsd, DWORD bpp, glExtensions *ext, DXGLTexture *out);

void DXGLTextureGL__FinishCreate(DXGLTexture *This, LPDXGLRENDERERGL renderer, glExtensions *ext, glUtil *util);
void DXGLTextureGL__Lock(DXGLTexture *This, glExtensions *ext, GLuint miplevel, BYTE **ptr);
void DXGLTextureGL__Unlock(DXGLTexture *This, glExtensions *ext, GLuint miplevel);

// Pixel format constants
#define DXGLPIXELFORMATGL_INVALID			-1
#define DXGLPIXELFORMATGL_PAL1				0
#define DXGLPIXELFORMATGL_PAL2				1
#define DXGLPIXELFORMATGL_PAL4				2
#define DXGLPIXELFORMATGL_PAL8				3
#define DXGLPIXELFORMATGL_RGB332			4
#define DXGLPIXELFORMATGL_RGB555			5
#define DXGLPIXELFORMATGL_RGB565			6
#define DXGLPIXELFORMATGL_RGB888			7
#define DXGLPIXELFORMATGL_RGB888_REV		8
#define DXGLPIXELFORMATGL_RGBX8888			9
#define DXGLPIXELFORMATGL_RGBX8888_REV		10
#define DXGLPIXELFORMATGL_RGBA8332			11
#define DXGLPIXELFORMATGL_RGBA4444			12
#define DXGLPIXELFORMATGL_RGBA1555			13
#define DXGLPIXELFORMATGL_RGBA8888			14
#define DXGLPIXELFORMATGL_LUM8				15
#define DXGLPIXELFORMATGL_ALPHA8			16
#define DXGLPIXELFORMATGL_LUM_ALPHA88		17
#define DXGLPIXELFORMATGL_Z16				18
#define DXGLPIXELFORMATGL_Z24				19
#define DXGLPIXELFORMATGL_X8_Z24			20
#define DXGLPIXELFORMATGL_X8_Z24_REV		21
#define DXGLPIXELFORMATGL_Z32				22
#define DXGLPIXELFORMATGL_S8_Z32			23
#define DXGLPIXELFORMATGL_S8_Z32_REV		24
#define DXGLPIXELFORMATGL_FOURCC_Y8			25
#define DXGLPIXELFORMATGL_FOURCC_Y800		26
#define DXGLPIXELFORMATGL_FOURCC_GREY		27
#define DXGLPIXELFORMATGL_FOURCC_Y16		28
#define DXGLPIXELFORMATGL_FOURCC_UYVY		29
#define DXGLPIXELFORMATGL_FOURCC_UYNV		30
#define DXGLPIXELFORMATGL_FOURCC_Y422		31
#define DXGLPIXELFORMATGL_FOURCC_YUY2		32
#define DXGLPIXELFORMATGL_FOURCC_YUYV		33
#define DXGLPIXELFORMATGL_FOURCC_YUNV		34
#define DXGLPIXELFORMATGL_FOURCC_YVYU		35
#define DXGLPIXELFORMATGL_FOURCC_RGBG		36
#define DXGLPIXELFORMATGL_FOURCC_GRGB		37
#define DXGLPIXELFORMATGL_FOURCC_AYUV		38



#ifdef __cplusplus
}
#endif


#endif //_DXGLTEXTUREGL_H
