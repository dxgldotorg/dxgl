// DXGL
// Copyright (C) 2012 William Feely

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

#include "common.h"
#include "fog.h"

static DWORD fogcolor = 0;
static GLfloat fogstart = 0;
static GLfloat fogend = 1;
static GLfloat fogdensity = 1;

void SetFogColor(DWORD color)
{
	if(color == fogcolor) return;
	fogcolor = color;
	GLfloat colors[4];
	colors[0] = (color >> 16) & 255;
	colors[1] = (color >> 8) & 255;
	colors[2] = color & 255;
	colors[3] = (color >> 24) & 255;
	glFogfv(GL_FOG_COLOR,colors);
}

void SetFogStart(GLfloat start)
{
	if(start == fogstart) return;
	fogstart = start;
	glFogf(GL_FOG_START,start);
}

void SetFogEnd(GLfloat end)
{
	if(end == fogend) return;
	fogend = end;
	glFogf(GL_FOG_END,end);
}

void SetFogDensity(GLfloat density)
{
	if(density == fogdensity) return;
	fogdensity = density;
	glFogf(GL_FOG_DENSITY,density);
}