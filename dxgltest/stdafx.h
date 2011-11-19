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
#ifndef _STDAFX_H
#define _STDAFX_H

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <math.h>

// wxWidgets headers
#include <wx/wx.h>
#include <wx/notebook.h>
#ifdef __WXMSW__
    #include <wx/msw/msvcrt.h>
#endif 
#include <wx/spinctrl.h>
// DirectX/DXGL headers
#include <ddraw.h>

#endif //_STDAFX_H