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
#ifndef _CFGMGR_H
#define _CFGMGR_H


typedef struct
{
	DWORD scaler;
	bool colormode;
	DWORD scalingfilter;
	DWORD texfilter;
	DWORD anisotropic;
	DWORD msaa;
	DWORD aspect;
	bool highres;
	DWORD vsync;
	TCHAR shaderfile[MAX_PATH+1];
	DWORD SortModes;
	bool AllColorDepths;
	bool ExtraModes;
	DWORD TextureFormat;
	DWORD TexUpload;
} DXGLCFG;

void ReadSettings(HKEY hKey, DXGLCFG *cfg, DXGLCFG *mask, bool global, bool dll, LPTSTR dir);
void WriteSettings(HKEY hKey, const DXGLCFG *cfg, const DXGLCFG *mask, bool global);
void GetCurrentConfig(DXGLCFG *cfg);
void GetGlobalConfig(DXGLCFG *cfg);
void SetGlobalConfig(const DXGLCFG *cfg);
void GetConfig(DXGLCFG *cfg, DXGLCFG *mask, LPCTSTR name);
void SetConfig(const DXGLCFG *cfg, const DXGLCFG *mask, LPCTSTR name);
void GetDirFromPath(LPTSTR path);
LPTSTR MakeNewConfig(LPTSTR path);
#endif //_CFGMGR_H