// DXGL
// Copyright (C) 2011 William Feely

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
#ifndef _SURFACEGEN_H
#define _SURFACEGEN_H

void DrawPalette(DDSURFACEDESC2 ddsd, unsigned char *buffer);  // Palette test
void DrawDitheredColor(DDSURFACEDESC2 *ddsd, unsigned char *buffer, DWORD color, BOOL invert);  // Draw a dithered color over the surface
void DrawGradients(DDSURFACEDESC2 ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWPALETTE palette, int type, DWORD color); // Gradients
void DrawGDIPatterns(DDSURFACEDESC2 ddsd, HDC hDC, int type); // GDI pattern test
void DrawROPPatternSurface(MultiDirectDrawSurface *surface, int bpp, int ddver); // ROP pattern test - pattern surface
void DrawROPPatterns(MultiDirectDrawSurface *primary, DDSPRITE *sprites, int backbuffers, int ddver, int bpp, DWORD *ropcaps,
	HWND hwnd, LPDIRECTDRAWPALETTE palette); // ROP pattern test
void DrawRotatedBlt(MultiDirectDrawSurface *primary, DDSPRITE *sprites);
void DrawColorKeyCompPatterns(DDSURFACEDESC2 ddsd, unsigned char *buffer, int bpp, int index);
void DrawFormatTestHUD(MultiDirectDrawSurface *surface, int srcformat, int destformat, int showhud, int testpattern, int testmethod, int x, int y);

#endif //_SURFACEGEN_H