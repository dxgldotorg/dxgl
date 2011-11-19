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

void GenScreen0(DDSURFACEDESC ddsd, unsigned char *buffer);  // Palette test
void GenScreen1(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface); // Gradients
void GenScreen2(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen3(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen4(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen5(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen6(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen7(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen8(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen9(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen10(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);


#endif //_SURFACEGEN_H