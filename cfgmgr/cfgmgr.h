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
#ifndef _CFGMGR_H
#define _CFGMGR_H


typedef struct
{
	bool UseGfxSettings;
	DWORD scaler;
	bool colormode;
	DWORD scalingfilter;
	DWORD texfilter;
	DWORD anisotropic;
	DWORD msaa;
	DWORD aspect;
	DWORD highres;
	DWORD vsync;
	WCHAR shaderfile[MAX_PATH+1];
	DWORD SortModes;
	bool AllColorDepths;
	bool ExtraModes;
	bool UseAudSettings;
	bool audio3d;
	bool audioglobal;
	bool UseInputSettings;
	bool inputglobal;
} DXGLCFG;

void ReadSettings(HKEY hKey, DXGLCFG *cfg, bool global);
void WriteSettings(HKEY hKey, const DXGLCFG *cfg, bool global);
void GetCurrentConfig(DXGLCFG *cfg);
void GetGlobalConfig(DXGLCFG *cfg);
void SetGlobalConfig(const DXGLCFG *cfg);

#endif //_CFGMGR_H