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

#include "stdafx.h"
#include "crc32.h"
#include "cfgmgr.h"

wchar_t regkeyglobal[] = L"Software\\DXGL\\Global";
wchar_t regkeybase[] = L"Software\\DXGL\\";

void ReadSettings(HKEY hKey, DXGLCFG *cfg, bool global)
{
	DWORD dwOut;
	LONG error;
	DWORD regdword = REG_DWORD;
	DWORD regsz = REG_SZ;
	DWORD sizeout=4;
	error = RegQueryValueExW(hKey,L"UseGraphicsSettings",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(((error == ERROR_SUCCESS) && dwOut) || global)
	{
		cfg->UseGfxSettings = true;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"ScalingMode",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->scaler = dwOut;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"ChangeColorDepth",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if((error == ERROR_SUCCESS) && dwOut) cfg->colormode = true;
		else cfg->colormode = false;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"ScalingFilter",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->scalingfilter = dwOut;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"TextureFilter",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->texfilter = dwOut;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"AnisotropicFiltering",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->anisotropic = dwOut;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"Antialiasing",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->msaa = dwOut;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"AdjustAspectRatio",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->aspect = dwOut;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"AdjustPrimaryResolution",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->highres = dwOut;
		sizeout = MAX_PATH*2;
		WCHAR file[MAX_PATH+1];
		error = RegQueryValueExW(hKey,L"ShaderFile",NULL,&regsz,(LPBYTE)file,&sizeout);
		if(error == ERROR_SUCCESS) wcsncpy(cfg->shaderfile,file,MAX_PATH);
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"SortModes",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->SortModes = dwOut;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"AllColorDepths",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS && dwOut) cfg->AllColorDepths = true;
		else cfg->AllColorDepths = false;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"ExtraModes",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS && dwOut) cfg->ExtraModes = true;
		else cfg->ExtraModes = false;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"VSync",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if(error == ERROR_SUCCESS) cfg->vsync = dwOut;
		sizeout = 4;
	}
	else cfg->UseGfxSettings = false;
	sizeout = 4;
	error = RegQueryValueExW(hKey,L"UseAudioSettings",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(((error == ERROR_SUCCESS) && dwOut) || global)
	{
		cfg->UseAudSettings = true;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"Use3DAudio",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if((error == ERROR_SUCCESS) && dwOut) cfg->audio3d = true;
		else cfg->audio3d = false;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"GlobalAudio",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if((error == ERROR_SUCCESS) && dwOut) cfg->audioglobal = true;
		else cfg->audioglobal = false;
	}
	else cfg->UseAudSettings = false;
	sizeout = 4;
	error = RegQueryValueExW(hKey,L"UseInputSettings",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(((error == ERROR_SUCCESS) && dwOut) || global)
	{
		cfg->UseInputSettings = true;
		sizeout = 4;
		error = RegQueryValueExW(hKey,L"GlobalInput",NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
		if((error == ERROR_SUCCESS) && dwOut) cfg->inputglobal = true;
		else cfg->inputglobal = false;
	}
	else cfg->UseInputSettings = false;
}
void WriteSettings(HKEY hKey, const DXGLCFG *cfg, bool global)
{
	LONG error;
	const DWORD one = 1;
	const DWORD zero = 0;
	if(cfg->UseGfxSettings || global)
	{
		if(!global) error = RegSetValueExW(hKey,L"UseGraphicsSettings",0,REG_DWORD,(BYTE*)&one,4);
		error = RegSetValueExW(hKey,L"ScalingMode",0,REG_DWORD,(BYTE*)&cfg->scaler,4);
		if(cfg->colormode) error = RegSetValueExW(hKey,L"ChangeColorDepth",0,REG_DWORD,(BYTE*)&one,4);
		else error = RegSetValueExW(hKey,L"ChangeColorDepth",0,REG_DWORD,(BYTE*)&zero,4);
		error = RegSetValueExW(hKey,L"ScalingFilter",0,REG_DWORD,(BYTE*)&cfg->scalingfilter,4);
		error = RegSetValueExW(hKey,L"TextureFilter",0,REG_DWORD,(BYTE*)&cfg->texfilter,4);
		error = RegSetValueExW(hKey,L"AnisotropicFiltering",0,REG_DWORD,(BYTE*)&cfg->anisotropic,4);
		error = RegSetValueExW(hKey,L"Antialiasing",0,REG_DWORD,(BYTE*)&cfg->msaa,4);
		error = RegSetValueExW(hKey,L"AdjustAspectRatio",0,REG_DWORD,(BYTE*)&cfg->aspect,4);
		error = RegSetValueExW(hKey,L"AdjustPrimaryResolution",0,REG_DWORD,(BYTE*)&cfg->highres,4);
		if(cfg->shaderfile[0]) error = RegSetValueExW(hKey,L"ShaderFile",0,REG_SZ,(BYTE*)cfg->shaderfile,
			wcslen(cfg->shaderfile)*2);
		else error = RegDeleteValueW(hKey,L"ShaderFile");
		error = RegSetValueExW(hKey,L"SortModes",0,REG_DWORD,(BYTE*)&cfg->SortModes,4);
		if(cfg->AllColorDepths) error = RegSetValueExW(hKey,L"AllColorDepths",0,REG_DWORD,(BYTE*)&one,4);
		else error = RegSetValueExW(hKey,L"AllColorDepths",0,REG_DWORD,(BYTE*)&zero,4);
		if(cfg->ExtraModes) error = RegSetValueExW(hKey,L"ExtraModes",0,REG_DWORD,(BYTE*)&one,4);
		else error = RegSetValueExW(hKey,L"LowResModes",0,REG_DWORD,(BYTE*)&zero,4);
		error = RegSetValueExW(hKey,L"VSync",0,REG_DWORD,(BYTE*)&cfg->vsync,4);
	}
	else if(!cfg->UseGfxSettings && !global) error = RegDeleteValueW(hKey,L"UseGraphicsSettings");
}

void GetCurrentConfig(DXGLCFG *cfg)
{
	HKEY hKey;
	unsigned long crc;
	FILE *file;
	WCHAR filename[MAX_PATH+1];
	WCHAR crcstr[10];
	GetModuleFileNameW(NULL,filename,MAX_PATH);
	file = _wfopen(filename,L"rb");
	if(file != NULL)
	{
		Crc32_ComputeFile(file,&crc);
	}
	else
	{
		crc = 0;
	}
	_itow(crc,crcstr,16);
	std::wstring regkey = regkeybase;
	int i;
	for(i = wcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
	i++;
	regkey.append(&filename[i]);
	regkey.append(L"-");
	regkey.append(crcstr);
	GetGlobalConfig(cfg);
	RegCreateKeyExW(HKEY_CURRENT_USER,regkey.c_str(),NULL,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,cfg,false);
	RegCloseKey(hKey);
}
void GetGlobalConfig(DXGLCFG *cfg)
{
	HKEY hKey;
	ZeroMemory(cfg,sizeof(DXGLCFG));
	RegCreateKeyExW(HKEY_CURRENT_USER,regkeyglobal,NULL,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,cfg,true);
	RegCloseKey(hKey);
}

void SetGlobalConfig(const DXGLCFG *cfg)
{
	HKEY hKey;
	RegCreateKeyExW(HKEY_CURRENT_USER,regkeyglobal,NULL,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	WriteSettings(hKey,cfg,true);
	RegCloseKey(hKey);
}
