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
#include <tchar.h>
using namespace std;
#ifdef _UNICODE
typedef wstring tstring;
#else
typedef string tstring;
#endif


TCHAR regkeyglobal[] = _T("Software\\DXGL\\Global");
TCHAR regkeybase[] = _T("Software\\DXGL\\");

DXGLCFG defaultmask;

void GetDirFromPath(LPTSTR path)
{
	int len = _tcslen(path);
	for(int i = len; i > 0; i--)
	{
		if((path[i] == '\\') || (path[i] == '/'))
		{
			path[i] = 0;
			break;
		}
	}
}

int FindStringInMultiSz(LPTSTR multisz, LPCTSTR comp)
{
	LPTSTR str = multisz;
	while(str[0] != 0)
	{
		if(!_tcscmp(str,comp)) return 1;
		str += (_tcslen(str) + 1);
	}
	return 0;
}

void AddStringToMultiSz(LPTSTR multisz, LPCTSTR string)
{
	LPTSTR str = multisz;
	while(str[0] != 0)
	{
		str += (_tcslen(str) + 1);
	}
	_tcscpy(str,string);
}


bool ReadBool(HKEY hKey, bool original, bool &mask, LPCTSTR value)
{
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		mask = true;
		if(dwOut) return true;
		else return false;
	}
	else
	{
		mask = false;
		return original;
	}
}

DWORD ReadDWORD(HKEY hKey, DWORD original, DWORD &mask, LPCTSTR value)
{
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		mask = 1;
		return dwOut;
	}
	else
	{
		mask = 0;
		return original;
	}
}

void ReadPath(HKEY hKey, TCHAR *path, TCHAR *mask, LPCTSTR value)
{
	DWORD sizeout = MAX_PATH*sizeof(TCHAR);
	DWORD regsz = REG_SZ;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regsz,(LPBYTE)path,&sizeout);
	if(error == ERROR_SUCCESS) mask[0] = 0xFF;
	else
	{
		mask[0] = 0;
		path[0] = 0;
	}
}

void ReadSettings(HKEY hKey, DXGLCFG *cfg, DXGLCFG *mask, bool global, bool dll, LPTSTR dir)
{
	DXGLCFG *cfgmask;
	if(mask) cfgmask = mask;
	else cfgmask = &defaultmask;
	TCHAR path[MAX_PATH+1];
	LONG error;
	DWORD regmultisz = REG_MULTI_SZ;
	DWORD sizeout=4;
	cfg->scaler = ReadDWORD(hKey,cfg->scaler,cfgmask->scaler,_T("ScalingMode"));
	cfg->colormode = ReadBool(hKey,cfg->colormode,cfgmask->colormode,_T("ChangeColorDepth"));
	cfg->scalingfilter = ReadDWORD(hKey,cfg->scalingfilter,cfgmask->scalingfilter,_T("ScalingFilter"));
	cfg->texfilter = ReadDWORD(hKey,cfg->texfilter,cfgmask->texfilter,_T("TextureFilter"));
	cfg->anisotropic = ReadDWORD(hKey,cfg->anisotropic,cfgmask->anisotropic,_T("AnisotropicFiltering"));
	cfg->msaa = ReadDWORD(hKey,cfg->msaa,cfgmask->msaa,_T("Antialiasing"));
	cfg->aspect = ReadDWORD(hKey,cfg->aspect,cfgmask->aspect,_T("AdjustAspectRatio"));
	cfg->highres = ReadBool(hKey,cfg->highres,cfgmask->highres,_T("AdjustPrimaryResolution"));
	ReadPath(hKey,cfg->shaderfile,cfgmask->shaderfile,_T("ShaderFile"));
	cfg->SortModes = ReadDWORD(hKey,cfg->SortModes,cfgmask->SortModes,_T("SortModes"));
	cfg->AllColorDepths = ReadBool(hKey,cfg->AllColorDepths,cfgmask->AllColorDepths,_T("AllColorDepths"));
	cfg->ExtraModes = ReadBool(hKey,cfg->ExtraModes,cfgmask->ExtraModes,_T("ExtraModes"));
	cfg->vsync = ReadDWORD(hKey,cfg->vsync,cfgmask->vsync,_T("VSync"));
	cfg->audio3d = ReadBool(hKey,cfg->audio3d,cfgmask->audio3d,_T("Use3DAudio"));
	cfg->audioglobal = ReadBool(hKey,cfg->audioglobal,cfgmask->audioglobal,_T("GlobalAudio"));
	cfg->inputglobal = ReadBool(hKey,cfg->inputglobal,cfgmask->inputglobal,_T("GlobalInput"));
	if(!global && dll)
	{
		LPTSTR paths;
		sizeout = 0;
		if(!dir) GetModuleFileName(NULL,path,MAX_PATH);
		else _tcsncpy(path,dir,MAX_PATH+1);
		GetDirFromPath(path);
		error = RegQueryValueEx(hKey,_T("InstallPaths"),NULL,&regmultisz,NULL,&sizeout);
		if(error == ERROR_FILE_NOT_FOUND)
		{
			sizeout = (_tcslen(path)*2)+4;
			paths = (LPTSTR)malloc(sizeout*sizeof(TCHAR));
			ZeroMemory(paths,sizeout*sizeof(TCHAR));
			_tcscpy(paths,path);
			error = RegSetValueEx(hKey,_T("InstallPaths"),0,REG_MULTI_SZ,(LPBYTE)paths,sizeout);
			free(paths);
		}
		else
		{
			paths = (LPTSTR)malloc(sizeout+(MAX_PATH*2)+4);
			ZeroMemory(paths,sizeout+(MAX_PATH*2)+4);
			error = RegQueryValueEx(hKey,_T("InstallPaths"),NULL,&regmultisz,(LPBYTE)paths,&sizeout);
			if(!FindStringInMultiSz(paths,path))
			{
				AddStringToMultiSz(paths,path);
				sizeout += (MAX_PATH*2)+4;
				error = RegSetValueEx(hKey,_T("InstallPaths"),0,REG_MULTI_SZ,(LPBYTE)paths,sizeout);
			}
			free(paths);
		}
	}
}

void WriteBool(HKEY hKey, bool value, bool mask, LPCTSTR name)
{
	const DWORD one = 1;
	const DWORD zero = 0;
	if(mask)
	{
		if(value) RegSetValueEx(hKey,name,0,REG_DWORD,(BYTE*)&one,4);
		else RegSetValueEx(hKey,name,0,REG_DWORD,(BYTE*)&zero,4);
	}
	else RegDeleteValue(hKey,name);
}

void WriteDWORD(HKEY hKey, DWORD value, DWORD mask, LPCTSTR name)
{
	if(mask) RegSetValueEx(hKey,name,0,REG_DWORD,(BYTE*)&value,4);
	else RegDeleteValue(hKey,name);
}
void WritePath(HKEY hKey, const TCHAR *path, const TCHAR *mask, LPCTSTR name)
{
	if(mask[0]) RegSetValueEx(hKey,name,0,REG_SZ,(BYTE*)path,(_tcslen(path)+1)*sizeof(TCHAR));
	else RegDeleteValue(hKey,name);
}

void WriteSettings(HKEY hKey, const DXGLCFG *cfg, const DXGLCFG *mask, bool global)
{
	const DXGLCFG *cfgmask;
	if(mask) cfgmask = mask;
	else cfgmask = &defaultmask;
	memset(&defaultmask,1,sizeof(DXGLCFG));
	WriteDWORD(hKey,cfg->scaler,cfgmask->scaler,_T("ScalingMode"));
	WriteBool(hKey,cfg->colormode,cfgmask->colormode,_T("ChangeColorDepth"));
	WriteDWORD(hKey,cfg->scalingfilter,cfgmask->scalingfilter,_T("ScalingFilter"));
	WriteDWORD(hKey,cfg->texfilter,cfgmask->texfilter,_T("TextureFilter"));
	WriteDWORD(hKey,cfg->anisotropic,cfgmask->anisotropic,_T("AnisotropicFiltering"));
	WriteDWORD(hKey,cfg->msaa,cfgmask->msaa,_T("Antialiasing"));
	WriteDWORD(hKey,cfg->aspect,cfgmask->aspect,_T("AdjustAspectRatio"));
	WriteBool(hKey,cfg->highres,cfgmask->highres,_T("AdjustPrimaryResolution"));
	WritePath(hKey,cfg->shaderfile,cfgmask->shaderfile,_T("ShaderFile"));
	WriteDWORD(hKey,cfg->SortModes,cfgmask->SortModes,_T("SortModes"));
	WriteBool(hKey,cfg->AllColorDepths,cfgmask->AllColorDepths,_T("AllColorDepths"));
	WriteBool(hKey,cfg->ExtraModes,cfgmask->ExtraModes,_T("ExtraModes"));
	WriteDWORD(hKey,cfg->vsync,cfgmask->vsync,_T("VSync"));
}

tstring newregname;

LPTSTR MakeNewConfig(LPTSTR path)
{
	HKEY hKey;
	DXGLCFG tmp;
	TCHAR crcstr[10];
	unsigned long crc;
	FILE *file = _tfopen(path,_T("rb"));
	if(file != NULL) Crc32_ComputeFile(file,&crc);
	else crc = 0;
	_itot(crc,crcstr,16);
	if(file) fclose(file);
	tstring regkey = regkeybase;
	int i;
	TCHAR filename[MAX_PATH+1];
	_tcsncpy(filename,path,MAX_PATH);
	for(i = _tcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
	i++;
	regkey.append(&filename[i]);
	regkey.append(_T("-"));
	regkey.append(crcstr);
	newregname = &filename[i];
	newregname.append(_T("-"));
	newregname.append(crcstr);
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,&tmp,NULL,false,true,path);
	RegCloseKey(hKey);
	return (LPTSTR)newregname.c_str();
}

bool IsInstalledDXGLTest(LPCTSTR path)
{
	LRESULT err;
	TCHAR dir[MAX_PATH+1];
	TCHAR cmp[MAX_PATH+1];
	_tcsncpy(dir,path,MAX_PATH);
	GetDirFromPath(dir);
	HKEY hKey;
	DWORD sizeout = MAX_PATH*sizeof(TCHAR);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\DXGL"),0,KEY_READ,&hKey) != ERROR_SUCCESS)
		return false;
	err = RegQueryValueEx(hKey,_T("InstallDir"),NULL,NULL,(LPBYTE)cmp,&sizeout);
	if(err != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);
	if(!_tcsnicmp(dir,cmp,MAX_PATH)) return true;
	else return false;
}

void GetCurrentConfig(DXGLCFG *cfg)
{
	HKEY hKey;
	unsigned long crc;
	tstring regkey;
	FILE *file;
	TCHAR filename[MAX_PATH+1];
	TCHAR crcstr[10];
	GetModuleFileName(NULL,filename,MAX_PATH);
	if(IsInstalledDXGLTest(filename))
	{
		regkey = regkeybase;
		regkey.append(_T("DXGLTestApp"));
	}
	else
	{
		file = _tfopen(filename,_T("rb"));
		if(file != NULL) Crc32_ComputeFile(file,&crc);
		else crc = 0;
		_itot(crc,crcstr,16);
		if(file) fclose(file);
		regkey = regkeybase;
		int i;
		for(i = _tcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
		i++;
		regkey.append(&filename[i]);
		regkey.append(_T("-"));
		regkey.append(crcstr);
	}
	GetGlobalConfig(cfg);
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,cfg,NULL,false,true,NULL);
	RegCloseKey(hKey);
}
void GetGlobalConfig(DXGLCFG *cfg)
{
	HKEY hKey;
	ZeroMemory(cfg,sizeof(DXGLCFG));
	RegCreateKeyEx(HKEY_CURRENT_USER,regkeyglobal,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,cfg,NULL,true,false,NULL);
	RegCloseKey(hKey);
}

void GetConfig(DXGLCFG *cfg, DXGLCFG *mask, LPCTSTR name)
{
	HKEY hKey;
	tstring regkey = regkeybase;
	regkey.append(name);
	ZeroMemory(cfg,sizeof(DXGLCFG));
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,cfg,mask,false,false,NULL);
	RegCloseKey(hKey);
}
void SetGlobalConfig(const DXGLCFG *cfg)
{
	HKEY hKey;
	RegCreateKeyEx(HKEY_CURRENT_USER,regkeyglobal,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	WriteSettings(hKey,cfg,NULL,true);
	RegCloseKey(hKey);
}

void SetConfig(const DXGLCFG *cfg, const DXGLCFG *mask, LPCTSTR name)
{
	HKEY hKey;
	tstring regkey = regkeybase;
	regkey.append(name);
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey.c_str(),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	WriteSettings(hKey,cfg,mask,false);
	RegCloseKey(hKey);
}
