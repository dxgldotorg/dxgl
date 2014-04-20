// DXGL
// Copyright (C) 2011-2012 William Feely

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
#ifndef _COMMON_H
#define _COMMON_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <MMSystem.h>
#include "include/winedef.h"
#include <ddraw.h>
#define D3D_OVERLOADS
#include "include/d3d.h"
#include "GL/gl.h"
#include "include/GL/glext.h"
#include "include/GL/wglext.h"
#include "glExtensions.h"
#include "ShaderManager.h"

extern const GUID device_template;


#ifdef _DEBUG
#define DEBUGBREAK DebugBreak();
#define _CRTDBG_MAP_ALLOC
#else
#define DEBUGBREAK
#endif
#include <stdio.h>
#include <stdlib.h>
#ifdef _DEBUG
#include <crtdbg.h>
#define new DEBUGNEW
#define DEBUGNEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
#define FIXME(x)\
{\
	OutputDebugStringA("FIXME: ");\
	OutputDebugStringA(x);\
	DEBUGBREAK\
}
#define DEBUG(x)\
{\
	OutputDebugStringA("DEBUG: ");\
	OutputDebugStringA(x);\
}
#define STR2(x) #x
#define STR(x) STR2(x)
#ifdef _DEBUG
#define ERR(error) \
{\
	OutputDebugStringA("ERROR: ");\
	OutputDebugStringA(STR(__FILE__));\
	OutputDebugStringA(": Line ");\
	OutputDebugStringA(STR(__LINE__));\
	OutputDebugStringA(": ");\
	OutputDebugStringA(STR(error));\
	OutputDebugStringA("\n");\
	DEBUGBREAK\
	return error;\
}
#else
#define ERR(error) return error;
#endif

static inline int NextMultipleOf8(int number){return ((number+7) & (~7));}
static inline int NextMultipleOf4(int number){return ((number+3) & (~3));}
static inline int NextMultipleOf2(int number){return ((number+1) & (~1));}
static inline void dwordto4float(DWORD in, GLfloat *out)
{
	out[0] = (GLfloat)((in>>16) & 0xff) / 255.0f;
	out[1] = (GLfloat)((in>>8) & 0xff) / 255.0f;
	out[2] = (GLfloat)(in& 0xff) / 255.0f;
	out[3] = (GLfloat)((in>>24) & 0xff) / 255.0f;
}

static inline void dwordto4int(DWORD in, GLint *out)
{
	out[0] = (GLint)((in>>16) & 0xff);
	out[1] = (GLint)((in>>8) & 0xff);
	out[2] = (GLint)(in& 0xff);
	out[3] = (GLint)((in>>24) & 0xff);
}

#ifdef _M_X64
#define NextMultipleOfWord NextMultipleOf8
#else
#define NextMultipleOfWord NextMultipleOf4
#endif

#include "trace.h"

#include "../cfgmgr/cfgmgr.h"
#endif //_COMMON_H
