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

#define _CRT_SECURE_NO_WARNINGS


#define WIN32_LEAN_AND_MEAN
// Windows Header Files:
#include <windows.h>
#include <commctrl.h>
extern bool gradientavailable;
extern BOOL (WINAPI *_GradientFill)(HDC hdc, TRIVERTEX* pVertices, ULONG nVertices, void* pMesh, ULONG nMeshElements, DWORD dwMode);

// C RunTime Header Files
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <math.h>
#include <time.h>
#include <string>
#ifdef _DEBUG
#include <crtdbg.h>
#define new DEBUGNEW
#define DEBUGNEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
using namespace std;
#ifdef _UNICODE
typedef wstring tstring;
#else
typedef string tstring;
#endif
#include "Resource.h"
// DirectX/DXGL headers
#include "../ddraw/include/ddraw.h"
extern const unsigned char DefaultPalette[1024];

#ifdef _UNICODE
#define _ttof _wtof
#else
#define _ttof atof
#endif

#endif //_COMMON_H
