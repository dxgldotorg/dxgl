// DXGL
// Copyright (C) 2011-2018 William Feely
// Portions copyright (C) 2018 Syahmi Azhar

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	// [system]
	DWORD NoWriteRegistry;
	DWORD OverrideDefaults;
	// [display]
	DWORD scaler;
	DWORD fullmode;
	BOOL colormode;
	DWORD AddColorDepths;
	DWORD AddModes;
	DWORD SortModes;
	DWORD CustomResolutionX;
	DWORD CustomResolutionY;
	DWORD CustomRefresh;
	float DisplayMultiplierX;
	float DisplayMultiplierY;
	// [scaling]
	DWORD scalingfilter;
	DWORD BltScale;
	// Removed for DXGL 0.5.13 release
	// DWORD BltThreshold;
	DWORD primaryscale;
	float primaryscalex;
	float primaryscaley;
	float aspect;
	DWORD DPIScale;
	// [postprocess]
	DWORD postfilter;
	float postsizex;
	float postsizey;
	BOOL EnableShader;
	TCHAR shaderfile[MAX_PATH + 1];
	// [d3d]
	DWORD texfilter;
	DWORD anisotropic;
	DWORD msaa;
	DWORD aspect3d;
	DWORD LowColorRendering;
	DWORD EnableDithering;
	// [advanced]
	DWORD vsync;
	DWORD TextureFormat;
	DWORD TexUpload;
	BOOL SingleBufferDevice;
	DWORD WindowPosition;
	BOOL RememberWindowSize;
	BOOL RememberWindowPosition;
	BOOL NoResizeWindow;
	DWORD WindowX;
	DWORD WindowY;
	DWORD WindowWidth;
	DWORD WindowHeight;
	BOOL WindowMaximized;
	// [debug]
	BOOL DebugNoExtFramebuffer;
	BOOL DebugNoArbFramebuffer;
	BOOL DebugNoES2Compatibility;
	BOOL DebugNoExtDirectStateAccess;
	BOOL DebugNoArbDirectStateAccess;
	BOOL DebugNoSamplerObjects;
	BOOL DebugNoGpuShader4;
	BOOL DebugNoGLSL130;
	BOOL DebugUploadAfterUnlock;
	BOOL DebugBlendDestColorKey;
	BOOL DebugNoMouseHooks;
	DWORD DebugMaxGLVersionMajor;
	DWORD DebugMaxGLVersionMinor;
	// [hacks]
	BOOL HackCrop640480to640400;
	BOOL HackAutoScale512448to640480;
	BOOL HackNoTVRefresh;
	BOOL HackSetCursor;
	// internal
	BOOL Windows8Detected;
	BOOL ParsedAddColorDepths;
	BOOL ParsedAddModes;
	TCHAR regkey[MAX_PATH + 80];
} DXGLCFG;

typedef struct
{
	BOOL NoOverwrite;
	char sha256[65];
	char sha256comp[65];
	BOOL NoUninstall;
} app_ini_options;

typedef struct
{
	TCHAR InstallPath[MAX_PATH + 1];
	TCHAR InstallPathLowercase[MAX_PATH + 1];
	TCHAR EXEFile[MAX_PATH + 1];
	TCHAR OldKey[MAX_PATH + 1];
	TCHAR crc32[9];
	BOOL nocrc;
	SHA256_HASH PathHash;
	TCHAR PathHashString[65];
	BOOL exe_found;
} CFGREG;

void ReadSettings(HKEY hKey, DXGLCFG *cfg, DXGLCFG *mask, BOOL global, BOOL dll, LPTSTR dir);
void WriteSettings(HKEY hKey, const DXGLCFG *cfg, const DXGLCFG *mask);
void GetCurrentConfig(DXGLCFG *cfg, BOOL initial);
void GetGlobalConfig(DXGLCFG *cfg, BOOL initial);
void GetGlobalConfigWithMask(DXGLCFG *cfg, DXGLCFG *mask, BOOL initial);
void SetGlobalConfig(const DXGLCFG *cfg, const DXGLCFG *mask);
void GetConfig(DXGLCFG *cfg, DXGLCFG *mask, LPCTSTR name);
void SetConfig(const DXGLCFG *cfg, const DXGLCFG *mask, LPCTSTR name);
void GetDirFromPath(LPTSTR path);
void UpgradeDXGLTestToDXGLCfg();
void UpgradeConfig();
void ReadAppINIOptions(LPCTSTR path, app_ini_options *options);
void SaveWindowSettings(const DXGLCFG *cfg);
BOOL CheckProfileExists(LPTSTR path);
LPTSTR MakeNewConfig(LPTSTR path);

#ifdef __cplusplus
}
#endif

#endif //_CFGMGR_H