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
#ifndef _TESTS_H
#define _TESTS_H

typedef struct
{
	MultiDirectDrawSurface *surface;
	DDSURFACEDESC2 ddsd;
	float width;
	float height;
	float x;
	float y;
	float xvelocity;
	float yvelocity;
	DWORD bltflags;
	RECT rect;
} DDSPRITE;

void RunTest2D(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	double fps, bool fullscreen, bool resizable);

void RunTest3D(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	int filter,	int msaa, double fps, bool fullscreen, bool resizable);

#endif //_TESTS_H
