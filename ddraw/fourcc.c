// DXGL
// Copyright (C) 2018-2019 William Feely

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

#include <windows.h>
#include <mmsystem.h>
#include "fourcc.h"

static const int START_FOURCC = __LINE__;
const DWORD dxglfourcc[] =
{
	MAKEFOURCC('U','Y','V','Y'),
	MAKEFOURCC('U','Y','N','V'),
	MAKEFOURCC('Y','4','2','2'),
	MAKEFOURCC('Y','U','Y','2'),
	MAKEFOURCC('Y','U','Y','V'),
	MAKEFOURCC('Y','U','N','V'),
	MAKEFOURCC('Y','V','Y','U'),
	MAKEFOURCC('A','Y','U','V'),
	MAKEFOURCC('Y','8',' ',' '),
	MAKEFOURCC('Y','8','0','0'),
	MAKEFOURCC('G','R','E','Y'),
	MAKEFOURCC('Y','1','6',' '),
	MAKEFOURCC('R','G','B','G'),
	MAKEFOURCC('G','R','G','B')
};
static const int END_FOURCC = __LINE__ - 4;

int GetNumFOURCC()
{
	return END_FOURCC - START_FOURCC;
}