// DXGL
// Copyright (C) 2011-2019 William Feely
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

#define _WIN32_WINNT 0x0600
#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#include <HtmlHelp.h>
#include <CommCtrl.h>
#include <string.h>
#include <tchar.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <io.h>
#include <Uxtheme.h>
#include "include/vsstyle.h"
#include "resource.h"
#include "../cfgmgr/LibSha256.h"
#include "../cfgmgr/cfgmgr.h"
#include <gl/GL.h>
#include "../ddraw/include/GL/glext.h"
#include "dxgltest.h"
#include "common.h"
#include "../common/version.h"
#pragma warning(disable: 4996)

#ifndef SHGFI_ADDOVERLAYS
#define SHGFI_ADDOVERLAYS 0x000000020
#endif //SHGFI_ADDOVERLAYS

#ifndef BCM_SETSHIELD
#define BCM_SETSHIELD 0x160C
#endif

DXGLCFG *cfg;
DXGLCFG *cfgmask;
BOOL *dirty;
static HINSTANCE hinstance;
BOOL msaa_available = FALSE;
const char *extensions_string = NULL;
OSVERSIONINFO osver;
TCHAR hlppath[MAX_PATH+16];
HMODULE uxtheme = NULL;
HMODULE dxglcfgdll = NULL;
HTHEME hThemeDisplay = NULL;
HTHEME(WINAPI *_OpenThemeData)(HWND hwnd, LPCWSTR pszClassList) = NULL;
HRESULT(WINAPI *_CloseThemeData)(HTHEME hTheme) = NULL;
HRESULT(WINAPI *_DrawThemeBackground)(HTHEME hTheme, HDC hdc, int iPartID,
	int iStateID, const RECT *pRect, const RECT *pClipRect) = NULL;
HRESULT(WINAPI *_EnableThemeDialogTexture)(HWND hwnd, DWORD dwFlags) = NULL;
static BOOL ExtraModes_Dropdown = FALSE;
static BOOL ColorDepth_Dropdown = FALSE;
HWND hDialog = NULL;
static BOOL EditInterlock = FALSE;
static DWORD hackstabitem = 0xFFFFFFFF;
static BOOL createdialoglock = FALSE;
#ifdef _M_X64
static const TCHAR installdir[] = _T("InstallDir_x64");
static const TCHAR regglobal[] = _T("Global_x64");
static const TCHAR profilespath[] = _T("Software\\DXGL\\Profiles_x64");
static const TCHAR profilespath2[] = _T("Software\\DXGL\\Profiles_x64\\");
static const TCHAR dxglcfgname[] = _T("DXGL Config (x64)");
#else
static const TCHAR installdir[] = _T("InstallDir");
static const TCHAR regglobal[] = _T("Global");
static const TCHAR profilespath[] = _T("Software\\DXGL\\Profiles");
static const TCHAR profilespath2[] = _T("Software\\DXGL\\Profiles\\");
static const TCHAR dxglcfgname[] = _T("DXGL Config");
#endif


typedef struct
{
	LPTSTR regkey;
	LPTSTR name;
	HICON icon;
	BOOL icon_shared;
	BOOL dirty;
	DXGLCFG cfg;
	DXGLCFG mask;
	TCHAR path[MAX_PATH];
	BOOL builtin;
} app_setting;

TCHAR exe_filter[] = _T("Program Files\0*.exe\0All Files\0*.*\0\0");

app_setting *apps;
int appcount;
int maxapps;
DWORD current_app;
BOOL tristate;
TCHAR strdefault[] = _T("(global default)");
TCHAR strdefaultshort[] = _T("(default)");
HWND hTab;
HWND hTabs[9];
static int tabopen;
BOOL modelistdirty = FALSE;

static const TCHAR *colormodes[32] = {
	_T("None"),
	_T("8-bit"),
	_T("15-bit"),
	_T("8/15-bit"),
	_T("16-bit"),
	_T("8/16-bit"),
	_T("15/16-bit"),
	_T("8/15/16-bit"),
	_T("24-bit"),
	_T("8/24-bit"),
	_T("15/24-bit"),
	_T("8/15/24-bit"),
	_T("16/24-bit"),
	_T("8/16/24-bit"),
	_T("15/16/24-bit"),
	_T("8/15/16/24-bit"),
	_T("32-bit"),
	_T("8/32-bit"),
	_T("15/32-bit"),
	_T("8/15/32-bit"),
	_T("16/32-bit"),
	_T("8/16/32-bit"),
	_T("15/16/32-bit"),
	_T("8/15/16/32-bit"),
	_T("24/32-bit"),
	_T("8/24/32-bit"),
	_T("15/24/32-bit"),
	_T("8/15/24/32-bit"),
	_T("16/24/32-bit"),
	_T("8/16/24/32-bit"),
	_T("15/16/24/32-bit"),
	_T("8/15/16/24/32-bit")
};

static const TCHAR *colormodedropdown[5] = {
	_T("8-bit"),
	_T("15-bit"),
	_T("16-bit"),
	_T("24-bit"),
	_T("32-bit")
};

static const TCHAR *extramodes[8] = {
	_T("Common low resolutions"),
	_T("Uncommon low resolutions"),
	_T("Uncommon SD resolutions"),
	_T("High Definition resolutions"),
	_T("Ultra-HD resolutions"),
	_T("Ultra-HD above 4k"),
	_T("Very uncommon resolutions"),
	_T("Common SVGA resolutions")
};

DWORD AddApp(LPCTSTR path, BOOL copyfile, BOOL admin, BOOL force, HWND hwnd)
{
	BOOL installed = FALSE;
	BOOL dxgl_installdir = FALSE;
	BOOL old_dxgl = TRUE;
	BOOL backupped = FALSE;
	TCHAR command[MAX_PATH + 37];
	SHELLEXECUTEINFO shex;
	DWORD exitcode;
	app_ini_options inioptions;
	HMODULE hmod;
	HANDLE exefile;
	LPCTSTR errmsg;
	DWORD fileptr;
	DWORD bytesread;
	BYTE header[16];
	BYTE *headerptr;
	#ifdef _M_X64
	const WORD machine = 0x8664;
	#else
	const WORD machine = 0x014c;
	#endif
	exefile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (exefile == INVALID_HANDLE_VALUE)
	{
		exitcode = GetLastError();
		_tcscpy(command, _T("Failed to open file:\r\n"));
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, exitcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			command+_tcslen(command), MAX_PATH, NULL);
		MessageBox(hwnd, command, _T("Error"), MB_OK | MB_ICONSTOP);
		return exitcode;
	}
	else
	{
		if (SetFilePointer(exefile, 0x3C, NULL, FILE_BEGIN) != 0x3C)
		{
			exitcode = GetLastError();
			CloseHandle(exefile);
			_tcscpy(command, _T("Read error:\r\n"));
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, exitcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				command + _tcslen(command), MAX_PATH, NULL);
			MessageBox(hwnd, command, _T("Error"), MB_OK | MB_ICONSTOP);
			return exitcode;
		}
		if (!ReadFile(exefile, &fileptr, 4, &bytesread, NULL))
		{
			exitcode = GetLastError();
			CloseHandle(exefile);
			_tcscpy(command, _T("Read error:\r\n"));
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, exitcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				command + _tcslen(command), MAX_PATH, NULL);
			MessageBox(hwnd, command, _T("Error"), MB_OK | MB_ICONSTOP);
			return exitcode;
		}
		if (SetFilePointer(exefile, fileptr, NULL, FILE_BEGIN) != fileptr)
		{
			exitcode = GetLastError();
			CloseHandle(exefile);
			_tcscpy(command, _T("Read error:\r\n"));
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, exitcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				command + _tcslen(command), MAX_PATH, NULL);
			MessageBox(hwnd, command, _T("Error"), MB_OK | MB_ICONSTOP);
			return exitcode;
		}
		if (!ReadFile(exefile,header,16,&bytesread,NULL))
		{
			exitcode = GetLastError();
			CloseHandle(exefile);
			_tcscpy(command, _T("Read error:\r\n"));
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, exitcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				command + _tcslen(command), MAX_PATH, NULL);
			MessageBox(hwnd, command, _T("Error"), MB_OK | MB_ICONSTOP);
			return exitcode;
		}
		headerptr = header;
		if (*(DWORD*)headerptr != 0x4550)
		{
			CloseHandle(exefile);
			MessageBox(hwnd, _T("Selected program is not a Windows PE executable."),
				_T("Invalid EXE"), MB_OK | MB_ICONSTOP);
			return ERROR_INVALID_PARAMETER;
		}
		headerptr = header + 4;
		if (*(WORD*)headerptr != machine)
		{
			CloseHandle(exefile);
			#ifdef _M_X64
			MessageBox(hwnd, _T("Platform of selected program is not x64.  Please use the 32-bit version of DXGL to add 32-bit programs."),
				_T("Invalid EXE"), MB_OK | MB_ICONSTOP);
			#else
			MessageBox(hwnd, _T("Platform of selected program is not x86.  Please use the 64-bit version of DXGL to add 64-bit programs."),
				_T("Invalid EXE"), MB_OK | MB_ICONSTOP);
			#endif
			return ERROR_INVALID_PARAMETER;
		}
		CloseHandle(exefile);
	}
	if (copyfile)
	{
		DWORD sizeout = (MAX_PATH + 1) * sizeof(TCHAR);
		TCHAR installpath[MAX_PATH + 1];
		TCHAR srcpath[MAX_PATH + 1];
		TCHAR inipath[MAX_PATH + 1];
		TCHAR backuppath[MAX_PATH + 1];
		TCHAR destpath[MAX_PATH + 1];
		HKEY hKeyInstall;
		LONG error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
		if (error == ERROR_SUCCESS)
		{
			dxgl_installdir = TRUE;
			error = RegQueryValueEx(hKeyInstall, installdir, NULL, NULL, (LPBYTE)installpath, &sizeout);
			if (error == ERROR_SUCCESS) installed = TRUE;
		}
		if (hKeyInstall) RegCloseKey(hKeyInstall);
		if (!installed)
		{
			GetModuleFileName(NULL, installpath, MAX_PATH + 1);
		}
		if (dxgl_installdir) _tcscat(installpath, _T("\\"));
		else (_tcsrchr(installpath, _T('\\')))[1] = 0;
		_tcsncpy(srcpath, installpath, MAX_PATH + 1);
		_tcscat(srcpath, _T("ddraw.dll"));
		_tcsncpy(destpath, path, MAX_PATH + 1);
		(_tcsrchr(destpath, _T('\\')))[1] = 0;
		_tcscat(destpath, _T("ddraw.dll"));
		_tcsncpy(backuppath, path, MAX_PATH + 1);
		(_tcsrchr(backuppath, _T('\\')))[1] = 0;
		_tcscat(backuppath, _T("ddraw.dll.dxgl-backup"));
		_tcsncpy(inipath, path, MAX_PATH + 1);
		(_tcsrchr(inipath, _T('\\')))[1] = 0;
		// Check for DXGL ini file and existing ddraw.dll
		ReadAppINIOptions(inipath, &inioptions);
		error = CopyFile(srcpath, destpath, TRUE);
	error_loop:
		if (!error)
		{
			error = GetLastError();
			if (error == ERROR_FILE_EXISTS)
			{
				if (inioptions.NoOverwrite)
				{
					MessageBox(hwnd, _T("Cannot install DXGL.  An INI file has \
been placed in your game folder prohibiting overwriting the existing DirectDraw \
library.\r\n\r\nIf you want to install DXGL, edit the dxgl.ini file in your game \
folder and set the NoOverwite value to false.\r\n\r\n\
A profile will still be created for your game but may not be compatible with the \
DirectDraw library in your game folder."), _T("Error"), MB_OK | MB_ICONERROR);
					return 0; // Continue to install registry key anyway
				}
				if ((inioptions.sha256[0] != 0) && !memcmp(inioptions.sha256, inioptions.sha256comp, 64))
					// Detected original ddraw matches INI hash
				{
					error = CopyFile(destpath, backuppath, FALSE);
					if (!error)
					{
						error = GetLastError();
						if ((error == ERROR_ACCESS_DENIED) && !admin)
						{
							_tcscpy(command, _T(" install "));
							_tcscat(command, path);
							ZeroMemory(&shex, sizeof(SHELLEXECUTEINFO));
							shex.cbSize = sizeof(SHELLEXECUTEINFO);
							shex.lpVerb = _T("runas");
							shex.fMask = SEE_MASK_NOCLOSEPROCESS;
							_tcscat(installpath, _T("\\dxglcfg.exe"));
							shex.lpFile = installpath;
							shex.lpParameters = command;
							ShellExecuteEx(&shex);
							WaitForSingleObject(shex.hProcess, INFINITE);
							GetExitCodeProcess(shex.hProcess, &exitcode);
							return exitcode;
						}
					}
					else backupped = TRUE;
				}
				error = SetErrorMode(SEM_FAILCRITICALERRORS);
				SetErrorMode(error | SEM_FAILCRITICALERRORS);
				hmod = LoadLibrary(destpath);
				SetErrorMode(error);
				if(hmod)
				{
					if(GetProcAddress(hmod,"IsDXGLDDraw") || force) old_dxgl = TRUE;
					else old_dxgl = FALSE;
					FreeLibrary(hmod);
				}
				else
				{
					if (force) old_dxgl = TRUE;
					else old_dxgl = FALSE;
				}
				if(old_dxgl)
				{
					error = CopyFile(srcpath,destpath,FALSE);
					goto error_loop;
				}
				else
				{
					// Prompt to overwrite
					if (MessageBox(hwnd, _T("A custom DirectDraw library has been detected in \
your game folder.  Would you like to replace it with DXGL?\r\n\r\n\
Warning:  Installing DXGL will remove any customizations that the existing custom DirectDraw \
library may have."), dxglcfgname, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
					{
						error = CopyFile(srcpath, destpath, FALSE);
						goto error_loop;
					}
					else
					{
						if (backupped) DeleteFile(backuppath);
					}
				}
			}
			if((error == ERROR_ACCESS_DENIED) && !admin)
			{
				if(old_dxgl) _tcscpy(command,_T(" install "));
				else _tcscpy(command, _T(" forceinstall "));
				_tcscat(command,path);
				ZeroMemory(&shex,sizeof(SHELLEXECUTEINFO));
				shex.cbSize = sizeof(SHELLEXECUTEINFO);
				shex.lpVerb = _T("runas");
				shex.fMask = SEE_MASK_NOCLOSEPROCESS;
				_tcscat(installpath,_T("\\dxglcfg.exe"));
				shex.lpFile = installpath;
				shex.lpParameters = command;
				ShellExecuteEx(&shex);
				WaitForSingleObject(shex.hProcess,INFINITE);
				GetExitCodeProcess(shex.hProcess,&exitcode);
				return exitcode;
			}
			return error;
		}
	}
	return 0;
}

DWORD DelApp(LPCTSTR path, BOOL admin, HWND hwnd)
{
	BOOL installed = FALSE;
	TCHAR command[MAX_PATH + 32];
	BOOL old_dxgl = TRUE;
	DWORD sizeout = (MAX_PATH+1)*sizeof(TCHAR);
	TCHAR installpath[MAX_PATH+1];
	TCHAR inipath[MAX_PATH + 1];
	TCHAR backuppath[MAX_PATH + 1];
	HKEY hKeyInstall;
	HMODULE hmod;
	SHELLEXECUTEINFO shex;
	DWORD exitcode;
	HANDLE exists;
	app_ini_options inioptions;
	LONG error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
	if(error == ERROR_SUCCESS)
	{
		error = RegQueryValueEx(hKeyInstall,installdir,NULL,NULL,(LPBYTE)installpath,&sizeout);
		if(error == ERROR_SUCCESS) installed = TRUE;
	}
	if(hKeyInstall) RegCloseKey(hKeyInstall);
	if(!installed)
	{
		GetModuleFileName(NULL,installpath,MAX_PATH+1);
	}
	_tcsncpy(inipath, path, MAX_PATH + 1);
	(_tcsrchr(inipath, _T('\\')))[1] = 0;
	_tcsncpy(backuppath, path, MAX_PATH + 1);
	(_tcsrchr(backuppath, _T('\\')))[1] = 0;
	_tcscat(backuppath, _T("ddraw.dll.dxgl-backup"));
	// Check for DXGL ini file and existing ddraw.dll
	ReadAppINIOptions(inipath, &inioptions);
	if (inioptions.NoOverwrite || inioptions.NoUninstall)
	{
		MessageBox(hwnd,_T("DXGL has not been removed from your game folder.  \
An INI file has been found in your game folder prohibiting the DirectDraw \
library in your game folder from being deleted.\r\n\r\n\
If this is in error, you will have to manually delete ddraw.dll from your \
game folder.  If your game was distributed by Steam or a similar service \
please verify your game files after removing the file, in case the game \
shipped with a custom DirectDraw library."), _T("Warning"), MB_OK | MB_ICONWARNING);
		return 0;  // Continue to delete registry profile.
	}
	error = SetErrorMode(SEM_FAILCRITICALERRORS);
	SetErrorMode(error | SEM_FAILCRITICALERRORS);
	hmod = LoadLibrary(path);
	SetErrorMode(error);
	if(hmod)
	{
		if(!GetProcAddress(hmod,"IsDXGLDDraw")) old_dxgl = FALSE;
		FreeLibrary(hmod);
	}
	else old_dxgl = FALSE;
	if(!old_dxgl) return 0;
	if(!DeleteFile(path))
	{
		error = GetLastError();
		if((error == ERROR_ACCESS_DENIED) && !admin)
		{
			_tcscpy(command,_T(" remove "));
			_tcscat(command,path);
			ZeroMemory(&shex,sizeof(SHELLEXECUTEINFO));
			shex.cbSize = sizeof(SHELLEXECUTEINFO);
			shex.lpVerb = _T("runas");
			shex.fMask = SEE_MASK_NOCLOSEPROCESS;
			_tcscat(installpath,_T("\\dxglcfg.exe"));
			shex.lpFile = installpath;
			shex.lpParameters = command;
			ShellExecuteEx(&shex);
			WaitForSingleObject(shex.hProcess,INFINITE);
			GetExitCodeProcess(shex.hProcess,&exitcode);
			return exitcode;
		}
		else if (error != ERROR_FILE_NOT_FOUND) return error;
	}
	exists = CreateFile(backuppath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (exists == INVALID_HANDLE_VALUE) return 0;
	else
	{
		CloseHandle(exists);
		error = MoveFile(backuppath, path);
		if (!error)
		{
			error = GetLastError();
			if ((error == ERROR_ACCESS_DENIED) && !admin)
			{
				_tcscpy(command, _T(" remove "));
				_tcscat(command, path);
				ZeroMemory(&shex, sizeof(SHELLEXECUTEINFO));
				shex.cbSize = sizeof(SHELLEXECUTEINFO);
				shex.lpVerb = _T("runas");
				shex.fMask = SEE_MASK_NOCLOSEPROCESS;
				_tcscat(installpath, _T("\\dxglcfg.exe"));
				shex.lpFile = installpath;
				shex.lpParameters = command;
				ShellExecuteEx(&shex);
				WaitForSingleObject(shex.hProcess, INFINITE);
				GetExitCodeProcess(shex.hProcess, &exitcode);
				return exitcode;
			}
			else return error;
		}
	}
	return 0;
}

void SaveChanges(HWND hWnd)
{
	int i;
	if(apps[0].dirty) SetGlobalConfig(&apps[0].cfg, &apps[0].mask);
	for(i = 1; i < appcount; i++)
	{
		if(apps[i].dirty) SetConfig(&apps[i].cfg,&apps[i].mask,apps[i].regkey);
	}
	EnableWindow(GetDlgItem(hWnd,IDC_APPLY),FALSE);
}

void FloatToAspect(float f, LPTSTR aspect)
{
	float integer;
	float dummy;
	float fract;
	TCHAR denominator[5];
	int i;
	if (_isnan(f)) f = 0.0f; //Handle NAN condition
	if (f >= 1000.0f)  // Clamp ridiculously wide aspects
	{
		_tcscpy(aspect, _T("1000:1"));
		return;
	}
	if (f < 0.001f)   // Exclude ridiculously tall aspects, zero, and negative
	{
		_tcscpy(aspect, _T("Default"));
		return;
	}
	// Handle common aspects
	if (fabsf(f - 1.25f) < 0.0001f)
	{
		_tcscpy(aspect, _T("5:4"));
		return;
	}
	if (fabsf(f - 1.3333333f) < 0.0001f)
	{
		_tcscpy(aspect, _T("4:3"));
		return;
	}
	if (fabsf(f - 1.6f) < 0.0001f)
	{
		_tcscpy(aspect, _T("16:10"));
		return;
	}
	if (fabsf(f - 1.7777777f) < 0.0001f)
	{
		_tcscpy(aspect, _T("16:9"));
		return;
	}
	if (fabsf(f - 1.9333333f) < 0.0001f)
	{
		_tcscpy(aspect, _T("256:135"));
		return;
	}
	fract = modff(f, &integer);
	if (fract < 0.0001f)  //Handle integer aspects
	{
		_itot((int)integer, aspect, 10);
		_tcscat(aspect, _T(":1"));
		return;
	}
	// Finally try from 2 to 1000
	for (i = 2; i < 1000; i++)
	{
		if (fabsf(modff(fract*i, &dummy)) < 0.0001f)
		{
			_itot((int)((f*(float)i) + .5f), aspect, 10);
			_itot(i, denominator, 10);
			_tcscat(aspect, _T(":"));
			_tcscat(aspect, denominator);
			return;
		}
	}
	// Cannot find a reasonable fractional aspect, so display as decimal.
#ifdef _UNICODE
	swprintf(aspect, 31, L"%.6g", f);
#else
	sprintf(aspect,"%.6g", f);
#endif
}

void FloatToScale(float x, float y, LPTSTR scale, float default)
{
	TCHAR numberx[8];
	TCHAR numbery[8];
	if (_isnan(x)) x = default; //Handle NAN condition
	if (_isnan(y)) y = default;
	// Too low number, round to "Auto"
	if (x < 0.25f) x = default;
	if (y < 0.25f) y = default;
	// Too high number, round to 16
	if (x > 16.0f) x = 16.0f;
	if (y > 16.0f) y = 16.0f;
	// Test if either scale is zero
	if ((x == 0) || (y == 0))
	{
		_tcscpy(scale, _T("Auto"));
		return;
	}
	// Write numbers
#ifdef _UNICODE
	swprintf(numberx, 7, L"%.4g", x);
	swprintf(numbery, 7, L"%.4g", y);
#else
	sprintf(numberx, ".4g", x);
	sprintf(numbery, ".4g", y);
#endif
	// Fill out string
	_tcscpy(scale, numberx);
	_tcscat(scale, _T("x"));
	if (x != y) _tcscat(scale, numbery);
}

void SetCheck(HWND hWnd, int DlgItem, BOOL value, BOOL mask, BOOL tristate)
{
	if(tristate && !mask)
		SendDlgItemMessage(hWnd,DlgItem,BM_SETCHECK,BST_INDETERMINATE,0);
	else
	{
		if(value) SendDlgItemMessage(hWnd,DlgItem,BM_SETCHECK,BST_CHECKED,0);
		else SendDlgItemMessage(hWnd,DlgItem,BM_SETCHECK,BST_UNCHECKED,0);
	}
}

void SetCombo(HWND hWnd, int DlgItem, DWORD value, DWORD mask, BOOL tristate)
{
	if(tristate && !mask)
		SendDlgItemMessage(hWnd,DlgItem,CB_SETCURSEL,
		SendDlgItemMessage(hWnd,DlgItem,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
	else
		SendDlgItemMessage(hWnd,DlgItem,CB_SETCURSEL,value,0);
}

void SetGLCombo(HWND hWnd, int DlgItem, DWORD *major, DWORD *minor, DWORD *majormask, DWORD *minormask, DWORD tristate, HWND hDialog)
{
	BOOL badversion = FALSE;
	int position;
	if (tristate && (!majormask || !minormask))
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL,
		SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
	else
	{
		switch (*major)
		{
		case 0:
			position = 0;
			break;
		case 2:
			switch (*minor)
			{
			case 0:
				position = 1;
				break;
			case 1:
				position = 2;
				break;
			default:
				badversion = TRUE;
				break;
			}
			break;
		case 3:
			switch (*minor)
			{
			case 0:
				position = 3;
				break;
			case 1:
				position = 4;
				break;
			case 2:
				position = 5;
				break;
			case 3:
				position = 6;
				break;
			default:
				badversion = TRUE;
				break;
			}
			break;
		case 4:
			switch (*minor)
			{
			case 0:
				position = 7;
				break;
			case 1:
				position = 8;
				break;
			case 2:
				position = 9;
				break;
			case 3:
				position = 10;
				break;
			case 4:
				position = 11;
				break;
			case 5:
				position = 12;
				break;
			case 6:
				position = 13;
				break;
			default:
				badversion = TRUE;
				break;
			}
			break;
		default:
			badversion = TRUE;
			break;
		}
		if (badversion)
		{
			*major = 0;
			*minor = 0;
			if (tristate)
			{
				position = (int)SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)strdefault);
				*majormask = 0;
				*minormask = 0;
			}
			else position = 0;
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
		}
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL, position, 0);
	}
}

void SetRGBHex(HWND hWnd, int DlgItem, DWORD value, DWORD mask)
{
	TCHAR number[32];
	if (mask) _sntprintf(number, 31, _T("%06X"), value);
	else _tcscpy(number, strdefault);
	EditInterlock = TRUE;
	SendDlgItemMessage(hWnd, DlgItem, WM_SETTEXT, 0, (LPARAM)number);
	EditInterlock = FALSE;
}

void SetFloat3place(HWND hWnd, int DlgItem, float value, float mask)
{
	TCHAR number[32];
	if(mask) _sntprintf(number, 31, _T("%.4g"), value);
	else _tcscpy(number, strdefaultshort);
	EditInterlock = TRUE;
	SendDlgItemMessage(hWnd, DlgItem, WM_SETTEXT, 0, (LPARAM)number);
	EditInterlock = FALSE;
}

void SetInteger(HWND hWnd, int DlgItem, int value, int mask)
{
	TCHAR number[32];
	if (mask) _itot(value, number, 10);
	else _tcscpy(number, strdefaultshort);
	EditInterlock = TRUE;
	SendDlgItemMessage(hWnd, DlgItem, WM_SETTEXT, 0, (LPARAM)number);
	EditInterlock = FALSE;
}

void SetResolution(HWND hWnd, int DlgItem, const DXGLCFG *cfg, const DXGLCFG *cfgmask)
{
	TCHAR output[104];
	TCHAR *ptr;
	ptr = output;
	if (!cfgmask->CustomResolutionX) _tcscpy(output, strdefault);
	else
	{
		_itot(cfg->CustomResolutionX, ptr, 10);
		_tcscat(ptr, _T("x"));
		ptr = _tcschr(ptr, 0);
		_itot(cfg->CustomResolutionY, ptr, 10);
		if (cfgmask->CustomRefresh)
		{
			_tcscat(ptr, _T(", "));
			ptr = _tcschr(ptr, 0);
			_itot(cfg->CustomRefresh, ptr, 10);
			_tcscat(ptr, _T("Hz"));
		}
	}
	EditInterlock = TRUE;
	SendDlgItemMessage(hWnd, DlgItem, WM_SETTEXT, 0, (LPARAM)output);
	EditInterlock = FALSE;
}

__inline DWORD EncodePrimaryScale(DWORD scale)
{
	switch (scale)
	{
	case 0:
		return 2;
	case 1:
		return 0;
	case 2:
		return 1;
	default:
		return scale;
	}
}

void SetPrimaryScaleCombo(HWND hWnd, int DlgItem, DWORD value, DWORD mask, BOOL tristate)
{
	if (tristate && !mask)
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL,
		SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
	else
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL, EncodePrimaryScale(value), 0);
}

void SetAspectCombo(HWND hWnd, int DlgItem, float value, DWORD mask, BOOL tristate)
{
	TCHAR buffer[32];
	if (tristate && !mask)
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL,
			SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
	else
	{
		FloatToAspect(value, buffer);
		SendDlgItemMessage(hWnd,DlgItem,CB_SETCURSEL,
			SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)buffer), 0);
		SetDlgItemText(hWnd, DlgItem, buffer);
	}
}

void SetWindowScaleCombo(HWND hWnd, int DlgItem, float x, float y, DWORD maskx, DWORD masky, BOOL tristate)
{
	TCHAR buffer[32];
	if (tristate && !maskx && !masky)
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL,
			SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
	else
	{
		FloatToScale(x, y, buffer, 1.0f);
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL,
			SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)buffer), 0);
		SetDlgItemText(hWnd, DlgItem, buffer);
	}
}

void SetPostScaleCombo(HWND hWnd, int DlgItem, float x, float y, DWORD maskx, DWORD masky, BOOL tristate)
{
	TCHAR buffer[32];
	if (tristate && !maskx && !masky)
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL,
			SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
	else
	{
		FloatToScale(x, y, buffer, 0.0f);
		SendDlgItemMessage(hWnd, DlgItem, CB_SETCURSEL,
			SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)buffer), 0);
		SetDlgItemText(hWnd, DlgItem, buffer);
	}
}

void SetText(HWND hWnd, int DlgItem, TCHAR *value, TCHAR *mask, BOOL tristate)
{
	if(tristate && (mask[0] == 0))
		SetWindowText(GetDlgItem(hWnd,DlgItem),_T(""));
	else SetWindowText(GetDlgItem(hWnd,DlgItem),value);
}

BOOL GetCheck(HWND hWnd, int DlgItem, BOOL *mask)
{
	int check = (int)SendDlgItemMessage(hWnd,DlgItem,BM_GETCHECK,0,0);
	switch(check)
	{
	case BST_CHECKED:
		*mask = TRUE;
		return TRUE;
	case BST_UNCHECKED:
		*mask = TRUE;
		return FALSE;
	case BST_INDETERMINATE:
	default:
		*mask = FALSE;
		return FALSE;
	}
}

DWORD GetCombo(HWND hWnd, int DlgItem, DWORD *mask)
{
	int value = (int)SendDlgItemMessage(hWnd,DlgItem,CB_GETCURSEL,0,0);
	if(value == SendDlgItemMessage(hWnd,DlgItem,CB_FINDSTRING,-1,(LPARAM)strdefault))
	{
		*mask = 0;
		return 0;
	}
	else
	{
		*mask = 1;
		return value;
	}
}

void GetGLCombo(HWND hWnd, int DlgItem, DWORD *major, DWORD *minor, DWORD *majormask, DWORD *minormask)
{
	int value = (int)SendDlgItemMessage(hWnd, DlgItem, CB_GETCURSEL, 0, 0);
	if (value == SendDlgItemMessage(hWnd, DlgItem, CB_FINDSTRING, -1, (LPARAM)strdefault))
	{
		*majormask = 0;
		*minormask = 0;
		*major = 0;
		*minor = 0;
		return;
	}
	else
	{
		*majormask = 1;
		*minormask = 1;
		switch (value)
		{
		case 0:
			*major = 0;
			*minor = 0;
			break;
		case 1:
			*major = 2;
			*minor = 0;
			break;
		case 2:
			*major = 2;
			*minor = 1;
			break;
		case 3:
			*major = 3;
			*minor = 0;
			break;
		case 4:
			*major = 3;
			*minor = 1;
			break;
		case 5:
			*major = 3;
			*minor = 2;
			break;
		case 6:
			*major = 3;
			*minor = 3;
			break;
		case 7:
			*major = 4;
			*minor = 0;
			break;
		case 8:
			*major = 4;
			*minor = 1;
			break;
		case 9:
			*major = 4;
			*minor = 2;
			break;
		case 10:
			*major = 4;
			*minor = 3;
			break;
		case 11:
			*major = 4;
			*minor = 4;
			break;
		case 12:
			*major = 4;
			*minor = 5;
			break;
		case 13:
			*major = 4;
			*minor = 6;
			break;
		}
		return;
	}
}

DWORD GetRGBHex(HWND hWnd, int dlgitem, DWORD *mask)
{
	TCHAR buffer[32];
	SendDlgItemMessage(hWnd, dlgitem, WM_GETTEXT, 32, (LPARAM)buffer);
	if ((buffer[0] == 0) || (!_tcsicmp(buffer, strdefaultshort)) || (!_tcsicmp(buffer, strdefault)))
	{
		if (!current_app)
		{
			*mask = 1;
			return 1;
		}
		else
		{
			*mask = 0;
			return 0;
		}
	}
	else
	{
		*mask = 1;
		return _tcstol(buffer, NULL, 16) & 0xFFFFFF;
	}
}

float GetFloat(HWND hWnd, int dlgitem, float *mask)
{
	TCHAR buffer[32];
	SendDlgItemMessage(hWnd, dlgitem, WM_GETTEXT, 32, (LPARAM)buffer);
	if ((buffer[0] == 0) || (!_tcsicmp(buffer, strdefaultshort)) || (!_tcsicmp(buffer, strdefault)))
	{
		if (!current_app)
		{
			*mask = 1.0f;
			return 1.0f;
		}
		else
		{
			*mask = 0.0f;
			return 0.0f;
		}
	}
	else
	{
		*mask = 1.0f;
		return (float)_ttof(buffer);
	}
}

int GetInteger(HWND hWnd, int dlgitem, int *mask, int defaultnum, BOOL usemask)
{
	TCHAR buffer[32];
	SendDlgItemMessage(hWnd, dlgitem, WM_GETTEXT, 32, (LPARAM)buffer);
	if ((buffer[0] == 0) || (!_tcsicmp(buffer, strdefaultshort)) || (!_tcsicmp(buffer, strdefault)))
	{
		if (!usemask)
		{
			*mask = 1;
			return defaultnum;
		}
		else
		{
			*mask = 0;
			return defaultnum;
		}
	}
	else
	{
		*mask = 1;
		return _ttoi(buffer);
	}
}

void ProcessResolutionString(LPTSTR input)
{
	TCHAR buffer[32];
	size_t ptr;
	int number[3];
	size_t length;
	size_t i;
	BOOL found = FALSE;
	BOOL skip = FALSE;
	length = _tcslen(input);
	for (i = 0; i < length; i++)
	{
		if (_istdigit(input[i]))
		{
			found = TRUE;
			ptr = i;
			break;
		}
	}
	if (!found) // Totally invalid, no numbers
	{
		if (current_app)
		{
			cfgmask->CustomResolutionX = 0;
			cfgmask->CustomResolutionY = 0;
			cfgmask->CustomRefresh = 0;
		}
		return;
	}
	found = FALSE;
	for (i = ptr; i < length; i++)
	{
		if (!(_istdigit(input[i])))
		{
			found = TRUE;
			memset(buffer, 0, 32 * sizeof(TCHAR));
			_tcsncpy(buffer, &input[ptr], i - ptr);
			number[0] = _ttoi(buffer);
			ptr = i;
			break;
		}
	}
	if (!found) // No separating character found
	{
		if (current_app)
		{
			cfgmask->CustomResolutionX = 0;
			cfgmask->CustomResolutionY = 0;
			cfgmask->CustomRefresh = 0;
		}
		return;
	}
	found = FALSE;
	for (i = ptr; i < length; i++)
	{
		if (_istdigit(input[i]))
		{
			found = TRUE;
			ptr = i;
			break;
		}
	}
	if (!found) // Needs two numbers
	{
		if (current_app)
		{
			cfgmask->CustomResolutionX = 0;
			cfgmask->CustomResolutionY = 0;
			cfgmask->CustomRefresh = 0;
		}
		return;
	}
	found = FALSE;
	for (i = ptr; i < length; i++)
	{
		if (!(_istdigit(input[i])))
		{
			found = TRUE;
			memset(buffer, 0, 32 * sizeof(TCHAR));
			_tcsncpy(buffer, &input[ptr], i - ptr);
			number[1] = _ttoi(buffer);
			ptr = i;
			break;
		}
	}
	if (!found)
	{
		number[1] = _ttoi(&input[ptr]);
		skip = TRUE;
	}
	found = FALSE;
	if (!skip)
	{
		for (i = ptr; i < length; i++)
		{
			if (_istdigit(input[i]))
			{
				found = TRUE;
				ptr = i;
				break;
			}
		}
	}
	if (!found)  // Found two numbers
	{
		cfg->CustomResolutionX = number[0];
		cfg->CustomResolutionY = number[1];
		cfgmask->CustomResolutionX = 1;
		cfgmask->CustomResolutionY = 1;
		if (current_app) cfgmask->CustomRefresh = 0;
		return;
	}
	found = FALSE;
	for (i = ptr; i < length; i++)
	{
		if (!(_istdigit(input[i])))
		{
			found = TRUE;
			memset(buffer, 0, 32 * sizeof(TCHAR));
			_tcsncpy(buffer, &input[ptr], i - ptr);
			number[2] = _ttoi(buffer);
			ptr = i;
			break;
		}
	}
	if (!found)	number[2] = _ttoi(&input[ptr]);
	// Found the refresh rate too.
	cfg->CustomResolutionX = number[0];
	cfg->CustomResolutionY = number[1];
	cfg->CustomRefresh = number[2];
	cfgmask->CustomResolutionX = 1;
	cfgmask->CustomResolutionY = 1;
	cfgmask->CustomRefresh = 1;
}

void GetResolution(HWND hWnd, int dlgitem, DXGLCFG *cfg, DXGLCFG *cfgmask)
{
	TCHAR input[104];
	SendDlgItemMessage(hWnd, dlgitem, WM_GETTEXT, 104, (LPARAM)input);
	ProcessResolutionString(input);
}

void GetWindowScaleCombo(HWND hWnd, int DlgItem, float *x, float *y, float *maskx, float *masky)
{
	TCHAR buffer[32];
	TCHAR *ptr;
	GetDlgItemText(hWnd, DlgItem, buffer, 31);
	buffer[31] = 0;
	if (!_tcscmp(buffer, strdefault))
	{
		*maskx = 0.0f;
		*masky = 0.0f;
		*x = 0.0f;
		*y = 0.0f;
		return;
	}
	else
	{
		*maskx = 1.0f;
		*masky = 1.0f;
		// Check for certain characters
		ptr = _tcsstr(buffer, _T("x"));
		if (!ptr) ptr = _tcsstr(buffer, _T("X"));
		if (!ptr) ptr = _tcsstr(buffer, _T(","));
		if (!ptr) ptr = _tcsstr(buffer, _T("-"));
		if (!ptr) ptr = _tcsstr(buffer, _T(":"));
		if (ptr)
		{
			*ptr = 0;
			*x = (float)_ttof(buffer);
			*y = (float)_ttof(ptr + 1);
			if ((*x >= 0.25f) && (*y < 0.25f)) *y = *x;
			return;
		}
		else
		{
			*x = (float)_ttof(buffer);
			*y = (float)_ttof(buffer);
			return;
		}
	}
}

void GetPostScaleCombo(HWND hWnd, int DlgItem, float *x, float *y, float *maskx, float *masky)
{
	TCHAR buffer[32];
	TCHAR *ptr;
	GetDlgItemText(hWnd, DlgItem, buffer, 31);
	buffer[31] = 0;
	if (!_tcscmp(buffer, strdefault))
	{
		*maskx = 0.0f;
		*masky = 0.0f;
		*x = 0.0f;
		*y = 0.0f;
		return;
	}
	else
	{
		*maskx = 1.0f;
		*masky = 1.0f;
		// Check for Auto
		if (!_tcsicmp(buffer, _T("Auto)")))
		{
			*x = 0.0f;
			*y = 0.0f;
			return;
		}
		else
		{
			// Check for certain characters
			ptr = _tcsstr(buffer, _T("x"));
			if (!ptr) ptr = _tcsstr(buffer, _T("X"));
			if (!ptr) ptr = _tcsstr(buffer, _T(","));
			if (!ptr) ptr = _tcsstr(buffer, _T("-"));
			if (!ptr) ptr = _tcsstr(buffer, _T(":"));
			if (ptr)
			{
				*ptr = 0;
				*x = (float)_ttof(buffer);
				*y = (float)_ttof(ptr + 1);
				if ((*x >= 0.25f) && (*y < 0.25f)) *y = *x;
				return;
			}
			else
			{
				*x = (float)_ttof(buffer);
				*y = (float)_ttof(buffer);
				return;
			}
		}
	}
}

float GetAspectCombo(HWND hWnd, int DlgItem, float *mask)
{
	TCHAR buffer[32];
	TCHAR *ptr;
	float numerator, denominator;
	GetDlgItemText(hWnd, DlgItem, buffer, 31);
	buffer[31] = 0;
	if (!_tcscmp(buffer, strdefault))
	{
		*mask = 0.0f;
		return 0.0f;
	}
	else
	{
		*mask = 1.0f;
		if (!_tcsicmp(buffer, _T("Default"))) return 0.0f;
		else
		{
			// Check for colon
			ptr = _tcsstr(buffer, _T(":"));
			if (ptr)
			{
				*ptr = 0;
				numerator = (float)_ttof(buffer);
				denominator = (float)_ttof(ptr + 1);
				return numerator / denominator;
			}
			else return (float)_ttof(buffer);
		}
	}
}

void GetText(HWND hWnd, int DlgItem, TCHAR *str, TCHAR *mask)
{
	GetDlgItemText(hWnd,DlgItem,str,MAX_PATH+1);
	if(str[0] == 0) mask[0] = 0;
	else mask[0] = (TCHAR)0xff;
}

void DrawCheck(HDC hdc, BOOL selected, BOOL checked, BOOL grayed, BOOL tristate, RECT *r)
{
	if (grayed)
	{
		if (checked)
		{
			if (hThemeDisplay)
			{
				if (selected)
					_DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_CHECKEDHOT, r, NULL);
				else _DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_CHECKEDDISABLED, r, NULL);
			}
			else
			{
				if (selected)
					DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_INACTIVE | DFCS_HOT);
				else DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_INACTIVE);
			}
		}
		else if (tristate)
		{
			if (hThemeDisplay)
			{
				if (selected)
					_DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_MIXEDHOT, r, NULL);
				else _DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_MIXEDDISABLED, r, NULL);
			}
			else
			{
				if (selected)
					DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTON3STATE | DFCS_CHECKED | DFCS_INACTIVE | DFCS_HOT);
				else DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTON3STATE | DFCS_CHECKED | DFCS_INACTIVE);
			}
		}
		else
		{
			if (hThemeDisplay)
			{
				if (selected)
					_DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_UNCHECKEDHOT, r, NULL);
				else _DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_UNCHECKEDDISABLED, r, NULL);
			}
			else
			{
				if (selected)
					DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_INACTIVE | DFCS_HOT);
				else DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_INACTIVE);
			}
		}
	}
	else
	{
		if (checked)
		{
			if (hThemeDisplay)
			{
				if (selected)
					_DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_CHECKEDHOT, r, NULL);
				else _DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_CHECKEDNORMAL, r, NULL);
			}
			else
			{
				if (selected)
					DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_HOT);
				else DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED);
			}
		}
		else if (tristate)
		{
			if (hThemeDisplay)
			{
				if (selected)
					_DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_MIXEDHOT, r, NULL);
				else _DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_MIXEDNORMAL, r, NULL);
			}
			else
			{
				if (selected)
					DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTON3STATE | DFCS_CHECKED | DFCS_HOT);
				else DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTON3STATE | DFCS_CHECKED);
			}
		}
		else
		{
			if (hThemeDisplay)
			{
				if (selected)
					_DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_UNCHECKEDHOT, r, NULL);
				else _DrawThemeBackground(hThemeDisplay, hdc, BS_AUTOCHECKBOX, CBS_UNCHECKEDNORMAL, r, NULL);
			}
			else
			{
				if (selected)
					DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_HOT);
				else DrawFrameControl(hdc, r, DFC_BUTTON, DFCS_BUTTONCHECK);
			}
		}
	}
}

static int CompareMode(const void *a, const void *b)
{
	DEVMODE *modea, *modeb;
	modea = (DEVMODE*)a;
	modeb = (DEVMODE*)b;
	if (modea->dmPelsWidth < modeb->dmPelsWidth) return -1;
	else if (modea->dmPelsWidth > modeb->dmPelsWidth) return 1;
	if (modea->dmPelsHeight < modeb->dmPelsHeight) return -1;
	else if (modea->dmPelsHeight > modeb->dmPelsHeight) return 1;
	if (modea->dmDisplayFrequency < modeb->dmDisplayFrequency) return -1;
	else if (modea->dmDisplayFrequency > modeb->dmDisplayFrequency) return 1;
	return 0;
}

void DiscardDuplicateModes(DEVMODE **array, DWORD *count)
{
	DEVMODE *newarray = (DEVMODE *)malloc(sizeof(DEVMODE)*(*count));
	if (!newarray) return;
	DWORD newcount = 0;
	bool match;
	for (DWORD x = 0; x < (*count); x++)
	{
		match = false;
		memcpy(&newarray[newcount], &(*array)[x], sizeof(DEVMODE));
		for (int y = newcount; y > 0; y--)
		{
			if ((*array)[x].dmDisplayFrequency == newarray[y - 1].dmDisplayFrequency &&
				(*array)[x].dmPelsWidth == newarray[y - 1].dmPelsWidth &&
				(*array)[x].dmPelsHeight == newarray[y - 1].dmPelsHeight)
			{
				match = true;
				break;
			}
		}
		if (!match) newcount++;
	}
	DEVMODE *newarray2 = (DEVMODE*)realloc(newarray, sizeof(DEVMODE)*newcount);
	if (newarray2) newarray = newarray2;
	free(*array);
	*array = newarray;
	*count = newcount;
}

LRESULT CALLBACK ModeListCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DEVMODE mode;
	DEVMODE *modes;
	DEVMODE *tmpmodes;
	DWORD modenum;
	DWORD modemax;
	TCHAR str[64];
	TCHAR *ptr;
	DWORD i;
	switch (Msg)
	{
	case WM_INITDIALOG:
		modenum = 0;
		modemax = 128;
		modes = (DEVMODE*)malloc(128 * sizeof(DEVMODE));
		while (EnumDisplaySettings(NULL, modenum++, &mode))
		{
			modes[modenum - 1] = mode;
			if (modenum >= modemax)
			{
				modemax += 128;
				tmpmodes = (DEVMODE*)realloc(modes, modemax * sizeof(DEVMODE));
				if (tmpmodes == NULL)
				{
					free(modes);
					MessageBox(hWnd, _T("Out of memory!"), _T("Fatal error"), MB_OK | MB_ICONSTOP);
					ExitProcess(ERROR_NOT_ENOUGH_MEMORY);
				}
				modes = tmpmodes;
			}
		}
		DiscardDuplicateModes(&modes, &modenum);
		qsort(modes, modenum-1, sizeof(DEVMODE), CompareMode);
		for (i = 0; i < modenum-1; i++)
		{
			ptr = str;
			_itot(modes[i].dmPelsWidth, ptr, 10);
			_tcscat(ptr, _T("x"));
			ptr = _tcschr(ptr, 0);
			_itot(modes[i].dmPelsHeight, ptr, 10);
			_tcscat(ptr, _T(", "));
			ptr = _tcschr(ptr, 0);
			_itot(modes[i].dmDisplayFrequency, ptr, 10);
			_tcscat(ptr, _T("Hz"));
			SendDlgItemMessage(hWnd, IDC_MODELIST, LB_ADDSTRING, 0, (LPARAM)str);
		}
		free(modes);
		return TRUE;
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_MODELIST:
			if ((HIWORD(wParam) == LBN_SELCHANGE))
				EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
			break;
		case IDOK:
			SendDlgItemMessage(hWnd, IDC_MODELIST, LB_GETTEXT,
				SendDlgItemMessage(hWnd, IDC_MODELIST, LB_GETCURSEL, 0, 0), (LPARAM)str);
			ProcessResolutionString(str);
			EndDialog(hWnd, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		default:
			break;
		}
		return TRUE;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK DisplayTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DRAWITEMSTRUCT* drawitem;
	COLORREF OldTextColor, OldBackColor;
	RECT r;
	TCHAR combotext[64];
	DWORD cursel;
	HDC hdc;
	HFONT font1, font2;
	SIZE size;
	int i;
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (uxtheme) hThemeDisplay = _OpenThemeData(hWnd, L"Button");
		else hThemeDisplay = NULL;
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		return TRUE;
	case WM_MEASUREITEM:
		switch (wParam)
		{
		case IDC_COLORDEPTH:
		case IDC_EXTRAMODES:
			if (((LPMEASUREITEMSTRUCT)lParam)->itemID == -1)
			{
				hdc = GetDC(hWnd);
				font1 = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
				font2 = (HFONT)SelectObject(hdc, font1);
				GetTextExtentPoint(hdc, _T(" "), 1, &size);
				SelectObject(hdc, font2);
				ReleaseDC(hWnd, hdc);
				((LPMEASUREITEMSTRUCT)lParam)->itemHeight = size.cy + 2;
			}
			else
			{
				((LPMEASUREITEMSTRUCT)lParam)->itemHeight = GetSystemMetrics(SM_CYMENUCHECK);
				((LPMEASUREITEMSTRUCT)lParam)->itemWidth = GetSystemMetrics(SM_CXMENUCHECK);
			}
			break;
		default:
			break;
		}
	case WM_DRAWITEM:
		if(createdialoglock) break;
		drawitem = (DRAWITEMSTRUCT*)lParam;
		switch (wParam)
		{
		case IDC_COLORDEPTH:
			OldTextColor = GetTextColor(drawitem->hDC);
			OldBackColor = GetBkColor(drawitem->hDC);
			if ((drawitem->itemState & ODS_SELECTED) && !(drawitem->itemState & ODS_COMBOBOXEDIT))
			{
				SetTextColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHT));
				FillRect(drawitem->hDC, &drawitem->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
			}
			else ExtTextOut(drawitem->hDC, 0, 0, ETO_OPAQUE, &drawitem->rcItem, NULL, 0, NULL);
			memcpy(&r, &drawitem->rcItem, sizeof(RECT));
			if (drawitem->itemID != -1 && !(drawitem->itemState & ODS_COMBOBOXEDIT))
			{
				r.left = r.left + 2;
				r.right = r.left + GetSystemMetrics(SM_CXMENUCHECK);
				if (drawitem->itemID == 5)
				{
					if(!cfgmask->AddColorDepths)
						DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, TRUE, FALSE, FALSE, &r);
					else DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, FALSE, FALSE, FALSE, &r);
				}
				else
				{
					if ((cfg->AddColorDepths >> drawitem->itemID) & 1)
						DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, TRUE, !cfgmask->AddColorDepths, FALSE, &r);
					else DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, FALSE, !cfgmask->AddColorDepths, FALSE, &r);
				}
				drawitem->rcItem.left += GetSystemMetrics(SM_CXMENUCHECK) + 5;
			}
			combotext[0] = 0;
			if (drawitem->itemID != -1 && !(drawitem->itemState & ODS_COMBOBOXEDIT))
				SendDlgItemMessage(hWnd, IDC_COLORDEPTH, CB_GETLBTEXT, drawitem->itemID, (LPARAM)combotext);
			else
			{
				if(!cfgmask->AddColorDepths) _tcscpy(combotext, strdefault);
				else _tcscpy(combotext, colormodes[cfg->AddColorDepths & 31]);
			}
			DrawText(drawitem->hDC, combotext, (int)_tcslen(combotext), &drawitem->rcItem,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			SetTextColor(drawitem->hDC, OldTextColor);
			SetBkColor(drawitem->hDC, OldBackColor);
			if (drawitem->itemID != -1 && !(drawitem->itemState & ODS_COMBOBOXEDIT))
				drawitem->rcItem.left -= GetSystemMetrics(SM_CXMENUCHECK) + 5;
			if (drawitem->itemState & ODS_FOCUS) DrawFocusRect(drawitem->hDC, &drawitem->rcItem);
			DefWindowProc(hWnd, Msg, wParam, lParam);
			break;
		case IDC_EXTRAMODES:
			OldTextColor = GetTextColor(drawitem->hDC);
			OldBackColor = GetBkColor(drawitem->hDC);
			if ((drawitem->itemState & ODS_SELECTED) && !(drawitem->itemState & ODS_COMBOBOXEDIT))
			{
				SetTextColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHT));
				FillRect(drawitem->hDC, &drawitem->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
			}
			else ExtTextOut(drawitem->hDC, 0, 0, ETO_OPAQUE, &drawitem->rcItem, NULL, 0, NULL);
			memcpy(&r, &drawitem->rcItem, sizeof(RECT));
			if (drawitem->itemID != -1 && !(drawitem->itemState & ODS_COMBOBOXEDIT))
			{
				r.left = r.left + 2;
				r.right = r.left + GetSystemMetrics(SM_CXMENUCHECK);
				if (drawitem->itemID == 8)
				{
					if (!cfgmask->AddModes)
						DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, TRUE, FALSE, FALSE, &r);
					else DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, FALSE, FALSE, FALSE, &r);
				}
				else
				{
					if ((cfg->AddModes >> drawitem->itemID) & 1)
						DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, TRUE, !cfgmask->AddModes, FALSE, &r);
					else DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, FALSE, !cfgmask->AddModes, FALSE, &r);
				}
				drawitem->rcItem.left += GetSystemMetrics(SM_CXMENUCHECK) + 5;
			}
			combotext[0] = 0;
			if (drawitem->itemID != -1 && !(drawitem->itemState & ODS_COMBOBOXEDIT))
				SendDlgItemMessage(hWnd, IDC_EXTRAMODES, CB_GETLBTEXT, drawitem->itemID, (LPARAM)combotext);
			else
			{
				if (!cfgmask->AddModes) _tcscpy(combotext, strdefault);
				else
				{
					switch (cfg->AddModes)
					{
					case 0:
						_tcscpy(combotext, _T("None"));
						break;
					case 1:
						_tcscpy(combotext, extramodes[0]);
						break;
					case 2:
						_tcscpy(combotext, extramodes[1]);
						break;
					case 4:
						_tcscpy(combotext, extramodes[2]);
						break;
					case 8:
						_tcscpy(combotext, extramodes[3]);
						break;
					case 16:
						_tcscpy(combotext, extramodes[4]);
						break;
					case 32:
						_tcscpy(combotext, extramodes[5]);
						break;
					case 64:
						_tcscpy(combotext, extramodes[6]);
						break;
					case 128:
						_tcscpy(combotext, extramodes[7]);
					break;
					default:
						_tcscpy(combotext, _T("Multiple selections"));
					}
				}
			}
			DrawText(drawitem->hDC, combotext, (int)_tcslen(combotext), &drawitem->rcItem,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			SetTextColor(drawitem->hDC, OldTextColor);
			SetBkColor(drawitem->hDC, OldBackColor);
			if (drawitem->itemID != -1 && !(drawitem->itemState & ODS_COMBOBOXEDIT))
				drawitem->rcItem.left -= GetSystemMetrics(SM_CXMENUCHECK) + 5;
			if (drawitem->itemState & ODS_FOCUS) DrawFocusRect(drawitem->hDC, &drawitem->rcItem);
			DefWindowProc(hWnd, Msg, wParam, lParam);
			break;
		default:
			break;
		}
	case WM_THEMECHANGED:
		if (uxtheme)
		{
			if (hThemeDisplay) _CloseThemeData(hThemeDisplay);
			hThemeDisplay = _OpenThemeData(hWnd, L"Button");
		}
		break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_COLORDEPTH:
			if (HIWORD(wParam) == CBN_SELENDOK)
			{
				if (ColorDepth_Dropdown)
				{
					cursel = (DWORD)SendDlgItemMessage(hWnd, IDC_COLORDEPTH, CB_GETCURSEL, 0, 0);
					if (cursel == 5)
					{
						if (cfgmask->AddColorDepths) cfgmask->AddColorDepths = 0;
						else cfgmask->AddColorDepths = 1;
					}
					else
					{
						if (!cfgmask->AddColorDepths) cfgmask->AddColorDepths = 1;
						i = ((cfg->AddColorDepths >> cursel) & 1);
						if (i) cfg->AddColorDepths &= ~(1 << cursel);
						else cfg->AddColorDepths |= 1 << cursel;
					}
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
					modelistdirty = TRUE;
				}
			}
			if (HIWORD(wParam) == CBN_DROPDOWN)
			{
				ColorDepth_Dropdown = TRUE;
			}
			if (HIWORD(wParam) == CBN_CLOSEUP)
			{
				ColorDepth_Dropdown = FALSE;
			}
			break;
		case IDC_EXTRAMODES:
			if (HIWORD(wParam) == CBN_SELENDOK)
			{
				if (ExtraModes_Dropdown)
				{
					cursel = (DWORD)SendDlgItemMessage(hWnd, IDC_EXTRAMODES, CB_GETCURSEL, 0, 0);
					if (cursel == 8)
					{
						if (cfgmask->AddModes) cfgmask->AddModes = 0;
						else cfgmask->AddModes = 1;
					}
					else
					{
						if (!cfgmask->AddModes) cfgmask->AddModes = 1;
						i = ((cfg->AddModes >> cursel) & 1);
						if (i) cfg->AddModes &= ~(1 << cursel);
						else cfg->AddModes |= 1 << cursel;
					}
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
					modelistdirty = TRUE;
				}
			}
			if (HIWORD(wParam) == CBN_DROPDOWN)
			{
				ExtraModes_Dropdown = TRUE;
			}
			if (HIWORD(wParam) == CBN_CLOSEUP)
			{
				ExtraModes_Dropdown = FALSE;
			}
			break;
		case IDC_VIDMODE:
			cfg->scaler = GetCombo(hWnd, IDC_VIDMODE, &cfgmask->scaler);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			modelistdirty = TRUE;
			if (cfg->scaler == 8)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALELABEL), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALELABELX), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALELABELY), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALEX), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALEY), TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALELABEL), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALELABELX), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALELABELY), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALEX), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_FIXEDSCALEY), FALSE);
			}
			if ((cfg->scaler == 9) || (cfg->scaler == 10))
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMMODELABEL), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMMODE), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_SETMODE), TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMMODELABEL), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMMODE), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_SETMODE), FALSE);
			}
			break;
		case IDC_FIXEDSCALEX:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->DisplayMultiplierX = GetFloat(hWnd, IDC_FIXEDSCALEX, &cfgmask->DisplayMultiplierX);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetFloat3place(hWnd, IDC_FIXEDSCALEX, cfg->DisplayMultiplierX, cfgmask->DisplayMultiplierX);
			}
			break;
		case IDC_FIXEDSCALEY:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->DisplayMultiplierY = GetFloat(hWnd, IDC_FIXEDSCALEY, &cfgmask->DisplayMultiplierY);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetFloat3place(hWnd, IDC_FIXEDSCALEY, cfg->DisplayMultiplierY, cfgmask->DisplayMultiplierY);
			}
			break;
		case IDC_CUSTOMMODE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					GetResolution(hWnd, IDC_CUSTOMMODE, cfg, cfgmask);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetResolution(hWnd, IDC_CUSTOMMODE, cfg, cfgmask);
			}
			break;
		case IDC_SETMODE:
		{
			if (DialogBox(hinstance, MAKEINTRESOURCE(IDD_MODELIST), hWnd, (DLGPROC)ModeListCallback))
				SetResolution(hWnd, IDC_CUSTOMMODE, cfg, cfgmask);
		}
		case IDC_SCALE:
			cfg->scalingfilter = GetCombo(hWnd, IDC_SCALE, &cfgmask->scalingfilter);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_ASPECT:
			if (HIWORD(wParam) == CBN_KILLFOCUS)
			{
				cfg->aspect = GetAspectCombo(hWnd, IDC_ASPECT, &cfgmask->aspect);
				SetAspectCombo(hWnd, IDC_ASPECT, cfg->aspect, (DWORD)cfgmask->aspect, tristate);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				cfg->aspect = GetAspectCombo(hWnd, IDC_ASPECT, &cfgmask->aspect);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
			}
			break;
		case IDC_SORTMODES:
			cfg->SortModes = GetCombo(hWnd, IDC_SORTMODES, &cfgmask->SortModes);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			modelistdirty = TRUE;
			break;
		case IDC_DPISCALE:
			cfg->DPIScale = GetCombo(hWnd, IDC_DPISCALE, &cfgmask->DPIScale);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_VSYNC:
			cfg->vsync = GetCombo(hWnd, IDC_VSYNC, &cfgmask->vsync);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_FULLMODE:
			cfg->fullmode = GetCombo(hWnd, IDC_FULLMODE, &cfgmask->fullmode);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_WINDOWSCALE:
			if (HIWORD(wParam) == CBN_KILLFOCUS)
			{
				GetWindowScaleCombo(hWnd, IDC_WINDOWSCALE, &cfg->WindowScaleX, &cfg->WindowScaleY,
					&cfgmask->WindowScaleX, &cfgmask->WindowScaleY);
				SetWindowScaleCombo(hWnd, IDC_WINDOWSCALE, cfg->WindowScaleX, cfg->WindowScaleY,
					(DWORD)cfgmask->WindowScaleX, (DWORD)cfgmask->WindowScaleY, tristate);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				modelistdirty = TRUE;
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				GetWindowScaleCombo(hWnd, IDC_WINDOWSCALE, &cfg->WindowScaleX, &cfg->WindowScaleY,
					&cfgmask->WindowScaleX, &cfgmask->WindowScaleY);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				modelistdirty = TRUE;
			}
			break;
		case IDC_COLOR:
			cfg->colormode = GetCheck(hWnd, IDC_COLOR, &cfgmask->colormode);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_SINGLEBUFFER:
			cfg->SingleBufferDevice = GetCheck(hWnd, IDC_SINGLEBUFFER, &cfgmask->SingleBufferDevice);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_SETDISPLAYCONFIG:
			cfg->UseSetDisplayConfig = GetCheck(hWnd, IDC_SETDISPLAYCONFIG, &cfgmask->UseSetDisplayConfig);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		default:
			break;
		}
	}
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK EffectsTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_POSTSCALE:
			cfg->postfilter = GetCombo(hWnd, IDC_POSTSCALE, &cfgmask->postfilter);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_POSTSCALESIZE:
			if (HIWORD(wParam) == CBN_KILLFOCUS)
			{
				GetPostScaleCombo(hWnd, IDC_POSTSCALESIZE, &cfg->postsizex, &cfg->postsizey,
					&cfgmask->postsizex, &cfgmask->postsizey);
				SetPostScaleCombo(hWnd, IDC_POSTSCALESIZE, cfg->postsizex, cfg->postsizey,
					(DWORD)cfgmask->postsizex, (DWORD)cfgmask->postsizey, tristate);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				modelistdirty = TRUE;
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				GetPostScaleCombo(hWnd, IDC_POSTSCALESIZE, &cfg->postsizex, &cfg->postsizey,
					&cfgmask->postsizex, &cfgmask->postsizey);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				modelistdirty = TRUE;
			}
			break;
		case IDC_PRIMARYSCALE:
			cfg->primaryscale = GetCombo(hWnd, IDC_PRIMARYSCALE, &cfgmask->primaryscale);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			if (cfg->primaryscale == 12)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALELABEL), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALELABELX), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALEX), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALELABELY), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALEY), TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALELABEL), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALELABELX), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALEX), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALELABELY), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOMSCALEY), FALSE);
			}
			break;
		case IDC_CUSTOMSCALEX:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->primaryscalex = GetFloat(hWnd, IDC_CUSTOMSCALEX, &cfgmask->primaryscalex);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetFloat3place(hWnd, IDC_CUSTOMSCALEX, cfg->primaryscalex, cfgmask->primaryscalex);
			}
			break;
		case IDC_CUSTOMSCALEY:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->primaryscaley = GetFloat(hWnd, IDC_CUSTOMSCALEY, &cfgmask->primaryscaley);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetFloat3place(hWnd, IDC_CUSTOMSCALEY, cfg->primaryscaley, cfgmask->primaryscaley);
			}
			break;
		// Removed for DXGL 0.5.13 release
		/*case IDC_BLTTHRESHOLD:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->BltThreshold = GetInteger(hWnd, IDC_BLTTHRESHOLD, (int*)&cfgmask->BltThreshold, 127, current_app);
					if (cfg->BltThreshold > 255) cfg->BltThreshold = 255;
					SendDlgItemMessage(hWnd, IDC_BLTTHRESHOLDSLIDER, TBM_SETPOS, TRUE, cfg->BltThreshold);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetInteger(hWnd, IDC_BLTTHRESHOLD, cfg->BltThreshold, cfgmask->BltThreshold);
			}
			break;*/
		case IDC_SHADER:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				GetText(hWnd, IDC_SHADER, cfg->shaderfile, cfgmask->shaderfile);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
			}
			break;
		case IDC_BLTFILTER:
			cfg->BltScale = GetCombo(hWnd, IDC_BLTFILTER, &cfgmask->BltScale);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		default:
			break;
		}
	// Removed for DXGL 0.5.13 release
	/* case WM_HSCROLL:
		switch (LOWORD(wParam))
		{
		case TB_ENDTRACK:
		case TB_THUMBTRACK:
		case TB_THUMBPOSITION:
		case TB_BOTTOM:
		case TB_TOP:
		case TB_LINEDOWN:
		case TB_LINEUP:
		case TB_PAGEDOWN:
		case TB_PAGEUP:
			cfgmask->BltThreshold = 1;
			cfg->BltThreshold = SendDlgItemMessage(hWnd, IDC_BLTTHRESHOLDSLIDER, TBM_GETPOS, 0, 0);
			SetInteger(hWnd, IDC_BLTTHRESHOLD, cfg->BltThreshold, cfgmask->BltThreshold);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		default:
			break;
		}
		break;*/
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK Tab3DCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_TEXFILTER:
			cfg->texfilter = GetCombo(hWnd, IDC_TEXFILTER, &cfgmask->texfilter);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_ANISO:
			cfg->anisotropic = GetCombo(hWnd, IDC_ANISO, &cfgmask->anisotropic);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_MSAA:
			cfg->msaa = GetCombo(hWnd, IDC_MSAA, &cfgmask->msaa);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_ASPECT3D:
			cfg->aspect3d = GetCombo(hWnd, IDC_ASPECT3D, &cfgmask->aspect3d);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_LOWCOLORRENDER:
			cfg->LowColorRendering = GetCombo(hWnd, IDC_LOWCOLORRENDER, &cfgmask->LowColorRendering);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_DITHERING:
			cfg->EnableDithering = GetCombo(hWnd, IDC_DITHERING, &cfgmask->EnableDithering);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_LIMITTEXFORMATS:
			cfg->LimitTextureFormats = GetCombo(hWnd, IDC_LIMITTEXFORMATS, &cfgmask->LimitTextureFormats);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;

		default:
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK SaveINICallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL unused;
	DWORD error;
	TCHAR errormsg[2048+MAX_PATH];
	switch(Msg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hWnd, IDC_NOWRITEREGISTRY, BM_SETCHECK, BST_CHECKED, 0);
		SendDlgItemMessage(hWnd, IDC_OVERRIDEREGISTRY, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(hWnd, IDC_NOOVERWRITE, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(hWnd, IDC_SAVESHA256, BM_SETCHECK, BST_CHECKED, 0);
		SendDlgItemMessage(hWnd, IDC_NOUNINSTALL, BM_SETCHECK, BST_UNCHECKED, 0);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			cfg->NoWriteRegistry = GetCheck(hWnd, IDC_NOWRITEREGISTRY, &unused);
			cfg->OverrideDefaults = GetCheck(hWnd, IDC_OVERRIDEREGISTRY, &unused);
			cfg->NoOverwrite = GetCheck(hWnd, IDC_NOOVERWRITE, &unused);
			cfg->SaveSHA256 = GetCheck(hWnd, IDC_SAVESHA256, &unused);
			cfg->NoUninstall = GetCheck(hWnd, IDC_NOUNINSTALL, &unused);
			error = WriteINI(cfg, cfgmask, apps[current_app].path, hWnd);
			if (error == ERROR_ACCESS_DENIED)
			{
				MessageBox(hWnd, _T("Access denied error writing .ini file.  Please re-launch DXGL Config as Administrator and try again."),
					_T("Error"), MB_OK | MB_ICONWARNING);
			}
			else if (error != ERROR_SUCCESS)
			{
				_tcscpy(errormsg, _T("Error writing .ini file:\r\n"));
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, errormsg + _tcslen(errormsg),
					(DWORD)(2048 - _tcslen(errormsg)), NULL);
				MessageBox(hWnd, errormsg, _T("Error"), MB_OK | MB_ICONERROR);
			}
			else
			{
				_tcscpy(errormsg, _T("Saved dxgl.ini to "));
				_tcscat(errormsg, apps[current_app].path);
				_tcscat(errormsg, _T("\\dxgl.ini"));
				MessageBox(hWnd, errormsg, _T("Notice"), MB_OK | MB_ICONINFORMATION);
			}
			EndDialog(hWnd, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		default:
			break;
		}
		return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK AdvancedTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_TEXTUREFORMAT:
			cfg->TextureFormat = GetCombo(hWnd, IDC_TEXTUREFORMAT, &cfgmask->TextureFormat);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_TEXUPLOAD:
			cfg->TexUpload = GetCombo(hWnd, IDC_TEXUPLOAD, &cfgmask->TexUpload);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_WINDOWPOS:
			cfg->WindowPosition = GetCombo(hWnd, IDC_WINDOWPOS, &cfgmask->WindowPosition);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_REMEMBERWINDOWPOS:
			cfg->RememberWindowPosition = GetCheck(hWnd, IDC_REMEMBERWINDOWPOS, &cfgmask->RememberWindowPosition);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_REMEMBERWINDOWSIZE:
			cfg->RememberWindowSize = GetCheck(hWnd, IDC_REMEMBERWINDOWSIZE, &cfgmask->RememberWindowSize);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_WINDOWX:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->WindowX = GetInteger(hWnd, IDC_WINDOWX, (int*)&cfgmask->WindowX, 0, TRUE);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetInteger(hWnd, IDC_WINDOWX, cfg->WindowX, cfgmask->WindowX);
			}
			break;
		case IDC_WINDOWY:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->WindowY = GetInteger(hWnd, IDC_WINDOWY, (int*)&cfgmask->WindowY, 0, TRUE);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetInteger(hWnd, IDC_WINDOWY, cfg->WindowY, cfgmask->WindowY);
			}
			break;
		case IDC_WINDOWWIDTH:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->WindowWidth = GetInteger(hWnd, IDC_WINDOWWIDTH, (int*)&cfgmask->WindowWidth, 0, TRUE);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetInteger(hWnd, IDC_WINDOWWIDTH, cfg->WindowWidth, cfgmask->WindowWidth);
			}
			break;
		case IDC_WINDOWHEIGHT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (!EditInterlock)
				{
					cfg->WindowHeight = GetInteger(hWnd, IDC_WINDOWHEIGHT, (int*)&cfgmask->WindowHeight, 0, TRUE);
					EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
					*dirty = TRUE;
				}
			}
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				SetInteger(hWnd, IDC_WINDOWHEIGHT, cfg->WindowHeight, cfgmask->WindowHeight);
			}
			break;
		case IDC_WINDOWMAXIMIZED:
			cfg->WindowMaximized = GetCheck(hWnd, IDC_WINDOWMAXIMIZED, &cfgmask->WindowMaximized);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_NOAUTOSIZE:
			cfg->NoResizeWindow = GetCheck(hWnd, IDC_NOAUTOSIZE, &cfgmask->NoResizeWindow);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_CAPTUREMOUSE:
			cfg->CaptureMouse = GetCheck(hWnd, IDC_CAPTUREMOUSE, &cfgmask->CaptureMouse);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_WRITEINI:
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SAVEINI), hWnd, (DLGPROC)SaveINICallback);
			break;
		default:
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

void ReadDebugItem(int item, BOOL *value, BOOL *mask)
{
	switch (item)
	{
	case 0:
		*value = cfg->DebugNoExtFramebuffer;
		*mask = cfgmask->DebugNoExtFramebuffer;
		break;
	case 1:
		*value = cfg->DebugNoArbFramebuffer;
		*mask = cfgmask->DebugNoArbFramebuffer;
		break;
	case 2:
		*value = cfg->DebugNoES2Compatibility;
		*mask = cfgmask->DebugNoES2Compatibility;
		break;
	case 3:
		*value = cfg->DebugNoExtDirectStateAccess;
		*mask = cfgmask->DebugNoExtDirectStateAccess;
		break;
	case 4:
		*value = cfg->DebugNoArbDirectStateAccess;
		*mask = cfgmask->DebugNoArbDirectStateAccess;
		break;
	case 5:
		*value = cfg->DebugNoSamplerObjects;
		*mask = cfgmask->DebugNoSamplerObjects;
		break;
	case 6:
		*value = cfg->DebugNoGpuShader4;
		*mask = cfgmask->DebugNoGpuShader4;
		break;
	case 7:
		*value = cfg->DebugNoGLSL130;
		*mask = cfgmask->DebugNoGLSL130;
		break;
	case 8:
		*value = cfg->DebugUploadAfterUnlock;
		*mask = cfgmask->DebugUploadAfterUnlock;
		break;
	case 9:
		*value = cfg->DebugNoMouseHooks;
		*mask = cfgmask->DebugNoMouseHooks;
		break;
	case 10:
		*value = cfg->DebugNoPaletteRedraw;
		*mask = cfgmask->DebugNoPaletteRedraw;
		break;
	case 11:
		*value = cfg->DebugBlendDestColorKey;
		*mask = cfgmask->DebugBlendDestColorKey;
		break;
	/*case 12:
		*value = cfg->DebugDisableErrors;
		*mask = cfgmask->DebugDisableErrors;
		break;*/
	default:
		*value = FALSE;
		*mask = FALSE;
		break;
	}
}

void WriteDebugItem(int item, BOOL value, BOOL mask)
{
	switch (item)
	{
	case 0:
		cfg->DebugNoExtFramebuffer = value;
		cfgmask->DebugNoExtFramebuffer = mask;
		break;
	case 1:
		cfg->DebugNoArbFramebuffer = value;
		cfgmask->DebugNoArbFramebuffer = mask;
		break;
	case 2:
		cfg->DebugNoES2Compatibility = value;
		cfgmask->DebugNoES2Compatibility = mask;
		break;
	case 3:
		cfg->DebugNoExtDirectStateAccess = value;
		cfgmask->DebugNoExtDirectStateAccess = mask;
		break;
	case 4:
		cfg->DebugNoArbDirectStateAccess = value;
		cfgmask->DebugNoArbDirectStateAccess = mask;
		break;
	case 5:
		cfg->DebugNoSamplerObjects = value;
		cfgmask->DebugNoSamplerObjects = mask;
		break;
	case 6:
		cfg->DebugNoGpuShader4 = value;
		cfgmask->DebugNoGpuShader4 = mask;
		break;
	case 7:
		cfg->DebugNoGLSL130 = value;
		cfgmask->DebugNoGLSL130 = mask;
		break;
	case 8:
		cfg->DebugUploadAfterUnlock = value;
		cfgmask->DebugUploadAfterUnlock = mask;
		break;
	case 9:
		cfg->DebugNoMouseHooks = value;
		cfgmask->DebugNoMouseHooks = mask;
		break;
	case 10:
		cfg->DebugNoPaletteRedraw = value;
		cfgmask->DebugNoPaletteRedraw = mask;
		break;
	case 11:
		cfg->DebugBlendDestColorKey = value;
		cfgmask->DebugBlendDestColorKey = mask;
		break;
	/*case 12:
		cfg->DebugDisableErrors = value;
		cfgmask->DebugDisableErrors = mask;
		break;*/
	default:
		break;
	}
}

LRESULT CALLBACK DebugTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR str[64];
	RECT r;
	DRAWITEMSTRUCT* drawitem;
	COLORREF OldTextColor, OldBackColor;
	BOOL debugvalue, debugmask;
	DWORD item;
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		return TRUE;
	case WM_MEASUREITEM:
		switch (wParam)
		{
		case IDC_DEBUGLIST:
			((LPMEASUREITEMSTRUCT)lParam)->itemHeight = GetSystemMetrics(SM_CYMENUCHECK);
			((LPMEASUREITEMSTRUCT)lParam)->itemWidth = GetSystemMetrics(SM_CXMENUCHECK);
			break;
		default:
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_DEBUGLIST:
			if ((HIWORD(wParam) == LBN_SELCHANGE) || (HIWORD(wParam) == LBN_DBLCLK))
			{
				item = (DWORD)SendDlgItemMessage(hWnd, IDC_DEBUGLIST, LB_GETCURSEL, 0, 0);
				ReadDebugItem(item, &debugvalue, &debugmask);
				if (tristate)
				{
					if (debugvalue && debugmask)
					{
						debugvalue = FALSE;
						debugmask = FALSE;
					}
					else if (!debugmask)
					{
						debugvalue = FALSE;
						debugmask = TRUE;
					}
					else
					{
						debugvalue = TRUE;
						debugmask = TRUE;
					}
				}
				else
				{
					if (debugvalue)
						debugvalue = FALSE;
					else debugvalue = TRUE;
				}
				WriteDebugItem(item, debugvalue, debugmask);
				RedrawWindow(GetDlgItem(hWnd, IDC_DEBUGLIST), NULL, NULL, RDW_INVALIDATE);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
			}
			break;
		case IDC_GLVERSION:
			GetGLCombo(hWnd, IDC_GLVERSION, &cfg->DebugMaxGLVersionMajor, &cfg->DebugMaxGLVersionMinor,
				&cfgmask->DebugMaxGLVersionMajor, &cfgmask->DebugMaxGLVersionMinor);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		default:
			break;
		}
	case WM_DRAWITEM:
		drawitem = (DRAWITEMSTRUCT*)lParam;
		switch (wParam)
		{
		case IDC_DEBUGLIST:
			OldTextColor = GetTextColor(drawitem->hDC);
			OldBackColor = GetBkColor(drawitem->hDC);
			if((drawitem->itemState & ODS_SELECTED))
			{
				SetTextColor(drawitem->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(drawitem->hDC,GetSysColor(COLOR_HIGHLIGHT));
				FillRect(drawitem->hDC,&drawitem->rcItem,(HBRUSH)(COLOR_HIGHLIGHT+1));
			}
			else
			{
				SetTextColor(drawitem->hDC, GetSysColor(COLOR_WINDOWTEXT));
				SetBkColor(drawitem->hDC, GetSysColor(COLOR_WINDOW));
				FillRect(drawitem->hDC, &drawitem->rcItem, (HBRUSH)(COLOR_WINDOW + 1));
			}
			memcpy(&r, &drawitem->rcItem, sizeof(RECT));
			r.left = r.left + 2;
			r.right = r.left + GetSystemMetrics(SM_CXMENUCHECK);
			ReadDebugItem(drawitem->itemID, &debugvalue, &debugmask);
			DrawCheck(drawitem->hDC, drawitem->itemState & ODS_SELECTED, debugvalue, FALSE, !debugmask, &r);
			drawitem->rcItem.left += GetSystemMetrics(SM_CXSMICON)+5;
			SendDlgItemMessage(hWnd, IDC_DEBUGLIST, LB_GETTEXT, drawitem->itemID, (LPARAM)str);
			DrawText(drawitem->hDC, str, (int)_tcslen(str), &drawitem->rcItem,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			drawitem->rcItem.left -= GetSystemMetrics(SM_CXSMICON)+5;
			if (drawitem->itemState & ODS_FOCUS) DrawFocusRect(drawitem->hDC, &drawitem->rcItem);
			SetTextColor(drawitem->hDC,OldTextColor);
			SetBkColor(drawitem->hDC,OldBackColor);
			DefWindowProc(hWnd,Msg,wParam,lParam);
			break;
		default:
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LPCTSTR strDisabled = _T("Disabled");
LPCTSTR strEnabled = _T("Enabled");
LPCTSTR strExpandViewport[] =
{
	_T("512x448 to 640x480"),
	_T("512x480 to 640x480")
};
LPCTSTR strViewportCompare[] =
{
	_T("Match color"),
	_T("Color less than or equal"),
	_T("Color greater than or equal"),
	_T("Match palette entry"),
	_T("Palette less than or equal"),
	_T("Palette greater than or equal"),
	_T("Match 3 palette entries (8-bit only)")
};
LPCTSTR strUnknown = _T("Unknown");

void UpdateHacksControl(HWND hWnd, int DlgItem, int item)
{
	switch (item)
	{
	case 0:
		SendDlgItemMessage(hWnd, DlgItem, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strDisabled);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strEnabled);
		if (tristate) SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SetCombo(hWnd, DlgItem, cfg->HackCrop640480to640400, cfgmask->HackCrop640480to640400, tristate);
		break;
	case 1:
		SendDlgItemMessage(hWnd, DlgItem, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strDisabled);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strExpandViewport[0]);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strExpandViewport[1]);
		if (tristate) SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SetCombo(hWnd, DlgItem, cfg->HackAutoExpandViewport, cfgmask->HackAutoExpandViewport, tristate);
		break;
	case 2:
		SendDlgItemMessage(hWnd, DlgItem, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strViewportCompare[0]);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strViewportCompare[1]);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strViewportCompare[2]);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strViewportCompare[3]);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strViewportCompare[4]);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strViewportCompare[5]);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strViewportCompare[6]);
		if (tristate) SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SetCombo(hWnd, DlgItem, cfg->HackAutoExpandViewportCompare, cfgmask->HackAutoExpandViewportCompare, tristate);
		break;
	case 3:
		SetRGBHex(hWnd, DlgItem, cfg->HackAutoExpandViewportValue, cfgmask->HackAutoExpandViewportValue);
		break;
	case 4:
		SendDlgItemMessage(hWnd, DlgItem, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strDisabled);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strEnabled);
		if (tristate) SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SetCombo(hWnd, DlgItem, cfg->HackNoTVRefresh, cfgmask->HackNoTVRefresh, tristate);
		break;
	case 5:
		SendDlgItemMessage(hWnd, DlgItem, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strDisabled);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strEnabled);
		if (tristate) SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SetCombo(hWnd, DlgItem, cfg->HackSetCursor, cfgmask->HackSetCursor, tristate);
		break;
	case 6:
		SetInteger(hWnd, DlgItem, cfg->HackPaletteDelay, cfgmask->HackPaletteDelay);
		break;
	case 7:
		SendDlgItemMessage(hWnd, DlgItem, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strDisabled);
		SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strEnabled);
		if (tristate) SendDlgItemMessage(hWnd, DlgItem, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SetCombo(hWnd, DlgItem, cfg->HackPaletteVsync, cfgmask->HackPaletteVsync, tristate);
		break;
	default:
		break;
	}
}

void DrawHacksItemText(HDC hdc, RECT *r, int item)
{
	LPCTSTR str = strUnknown;
	TCHAR buffer[33];
	switch (item)
	{
	case 0:
		if (!cfgmask->HackCrop640480to640400) str = strdefault;
		else
		{
			if (cfg->HackCrop640480to640400) str = strEnabled;
			else str = strDisabled;
		}
		break;
	case 1:
		if (!cfgmask->HackAutoExpandViewport) str = strdefault;
		else
		{
			if (cfg->HackAutoExpandViewport > 2) str = strUnknown;
			else if (cfg->HackAutoExpandViewport == 0) str = strDisabled;
			else str = strExpandViewport[cfg->HackAutoExpandViewport-1];
		}
		break;
	case 2:
		if (!cfgmask->HackAutoExpandViewportCompare) str = strdefault;
		else
		{
			if (cfg->HackAutoExpandViewportCompare > 6) str = strUnknown;
			else str = strViewportCompare[cfg->HackAutoExpandViewportCompare];
		}
		break;
	case 3:
		if (!cfgmask->HackAutoExpandViewportValue) str = strdefault;
		else
		{
			_sntprintf(buffer, 32, _T("%06X"), cfg->HackAutoExpandViewportValue);
			str = buffer;
		}
		break;
	case 4:
		if (!cfgmask->HackNoTVRefresh) str = strdefault;
		else
		{
			if (cfg->HackNoTVRefresh) str = strEnabled;
			else str = strDisabled;
		}
		break;
	case 5:
		if (!cfgmask->HackSetCursor) str = strdefault;
		else
		{
			if (cfg->HackSetCursor) str = strEnabled;
			else str = strDisabled;
		}
		break;
	case 6:
		if (!cfgmask->HackPaletteDelay) str = strdefault;
		else
		{
			_itot(cfg->HackPaletteDelay, buffer, 10);
			str = buffer;
		}
		break;
	case 7:
		if (!cfgmask->HackPaletteVsync) str = strdefault;
		else
		{
			if (cfg->HackPaletteVsync) str = strEnabled;
			else str = strDisabled;
		}
		break;
	default:
		str = strUnknown;
	}
	DrawText(hdc, str, (int)_tcslen(str), r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
}

LRESULT CALLBACK HacksTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR str[64];
	RECT r, r2;
	DWORD x;
	DRAWITEMSTRUCT* drawitem;
	COLORREF OldTextColor, OldBackColor;
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		SetParent(GetDlgItem(hWnd,IDC_HACKSDROPDOWN), GetDlgItem(hWnd,IDC_HACKSLIST));
		SetParent(GetDlgItem(hWnd, IDC_HACKSEDIT), GetDlgItem(hWnd, IDC_HACKSLIST));
		ShowWindow(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN), SW_HIDE);
		ShowWindow(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT), SW_HIDE);
		return TRUE;
	case WM_MEASUREITEM:
		switch (wParam)
		{
		case IDC_HACKSLIST:
			if(((LPMEASUREITEMSTRUCT)lParam)->itemID == 3)
				GetWindowRect(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT), &r);
			else GetWindowRect(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN), &r);
			((LPMEASUREITEMSTRUCT)lParam)->itemHeight = r.bottom - r.top;
			((LPMEASUREITEMSTRUCT)lParam)->itemWidth = r.right - r.left;
			break;
		default:
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_HACKSEDIT:
			switch (hackstabitem)
			{
			case 3:
				if (HIWORD(wParam) == EN_CHANGE)
				{
					if (!EditInterlock)
					{
						cfg->HackAutoExpandViewportValue = GetRGBHex(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT, &cfgmask->HackAutoExpandViewportValue);
						EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
						*dirty = TRUE;
					}
				}
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					SetRGBHex(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT, cfg->HackAutoExpandViewportValue, cfgmask->HackAutoExpandViewportValue);
				}
				break;
			case 6:
				if (HIWORD(wParam) == EN_CHANGE)
				{
					if (!EditInterlock)
					{
						cfg->HackPaletteDelay = GetInteger(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT, (int*)&cfgmask->HackPaletteDelay, 20, TRUE);
						EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
						*dirty = TRUE;
					}
				}
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					SetInteger(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT, cfg->HackPaletteDelay, cfgmask->HackPaletteDelay);
				}
				break;
			}
			break;
		case IDC_HACKSDROPDOWN:
			switch (hackstabitem)
			{
			case 0:
				cfg->HackCrop640480to640400 = GetCombo(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN,
					(DWORD*)&cfgmask->HackCrop640480to640400);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				break;
			case 1:
				cfg->HackAutoExpandViewport = GetCombo(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN,
					&cfgmask->HackAutoExpandViewport);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				break;
			case 2:
				cfg->HackAutoExpandViewportCompare = GetCombo(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN,
					&cfgmask->HackAutoExpandViewportCompare);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				break;
			case 4:
				cfg->HackNoTVRefresh = GetCombo(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN,
					(DWORD*)&cfgmask->HackNoTVRefresh);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				modelistdirty = TRUE;
				break;
			case 5:
				cfg->HackSetCursor = GetCombo(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN,
					(DWORD*)&cfgmask->HackSetCursor);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				break;
			case 7:
				cfg->HackPaletteVsync = GetCombo(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN,
					(DWORD*)&cfgmask->HackPaletteVsync);
				EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
				*dirty = TRUE;
				break;
			default:
				break;
			}
			break;
		case IDC_HACKSLIST:
			if ((HIWORD(wParam) == LBN_SELCHANGE) || (HIWORD(wParam) == LBN_DBLCLK))
			{
				hackstabitem = (DWORD)SendDlgItemMessage(hWnd, IDC_HACKSLIST, LB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hWnd, IDC_HACKSLIST, LB_GETITEMRECT, hackstabitem, (LPARAM)&r2);
				if ((hackstabitem == 3) || (hackstabitem == 6))
				{
					GetWindowRect(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT), &r);
					x = r.right - r.left;
					r2.left = r2.right - x;
					SetWindowPos(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT),
						HWND_TOP, r2.left, r2.top, x, r2.bottom - r2.top, SWP_SHOWWINDOW);
					ShowWindow(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN), SW_HIDE);
					UpdateHacksControl(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT, hackstabitem);
				}
				else
				{
					GetWindowRect(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN), &r);
					x = r.right - r.left;
					r2.left = r2.right - x;
					SetWindowPos(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN),
						HWND_TOP, r2.left, r2.top, x, r2.bottom - r2.top, SWP_SHOWWINDOW);
					ShowWindow(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT), SW_HIDE);
					UpdateHacksControl(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN, hackstabitem);
				}
				RedrawWindow(GetDlgItem(hWnd, IDC_HACKSLIST), NULL, NULL, RDW_INVALIDATE);
			}
			break;
		default:
			break;
		}
	case WM_DRAWITEM:
		drawitem = (DRAWITEMSTRUCT*)lParam;
		switch (wParam)
		{
		case IDC_HACKSLIST:
			OldTextColor = GetTextColor(drawitem->hDC);
			OldBackColor = GetBkColor(drawitem->hDC);
			if ((drawitem->itemState & ODS_SELECTED))
			{
				SetTextColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHT));
				FillRect(drawitem->hDC, &drawitem->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
			}
			else
			{
				SetTextColor(drawitem->hDC, GetSysColor(COLOR_WINDOWTEXT));
				SetBkColor(drawitem->hDC, GetSysColor(COLOR_WINDOW));
				FillRect(drawitem->hDC, &drawitem->rcItem, (HBRUSH)(COLOR_WINDOW + 1));
			}
			memcpy(&r, &drawitem->rcItem, sizeof(RECT));
			r.left = r.left + 2;
			r.right = r.left + GetSystemMetrics(SM_CXMENUCHECK);
			drawitem->rcItem.left += 1;
			SendDlgItemMessage(hWnd, IDC_HACKSLIST, LB_GETTEXT, drawitem->itemID, (LPARAM)str);
			DrawText(drawitem->hDC, str, (int)_tcslen(str), &drawitem->rcItem,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			drawitem->rcItem.left -= 1;
			if ((hackstabitem == 3) || (hackstabitem == 6))
				GetWindowRect(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSEDIT), &r);
			else GetWindowRect(GetDlgItem(GetDlgItem(hWnd, IDC_HACKSLIST), IDC_HACKSDROPDOWN), &r);
			memcpy(&r2, &drawitem->rcItem, sizeof(RECT));
			r2.left = r2.right - (r.right - r.left) + 4;
			DrawHacksItemText(drawitem->hDC, &r2, drawitem->itemID);
			if (drawitem->itemState & ODS_FOCUS) DrawFocusRect(drawitem->hDC, &drawitem->rcItem);
			SetTextColor(drawitem->hDC, OldTextColor);
			SetBkColor(drawitem->hDC, OldBackColor);
			DefWindowProc(hWnd, Msg, wParam, lParam);
			break;
		default:
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK TracingTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_TRACING:
			cfg->DebugTraceLevel = GetCombo(hWnd, IDC_TRACING, &cfgmask->DebugTraceLevel);
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK PathsTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		return TRUE;
	default:
		return FALSE;
	}
	return TRUE;
}

DWORD GetDPISupportLevel()
{
	HMODULE hSHCore = NULL;
	HMODULE hUser32 = NULL;
	HRESULT(WINAPI *_SetProcessDpiAwareness)(DWORD value) = NULL;
	BOOL(WINAPI *_SetProcessDpiAwarenessContext)(HANDLE value) = NULL;
	DWORD level = 0;
	// Vista or higher - supports DWM and DPI scaling
	if (osver.dwMajorVersion >= 6) level = 1;
	else return level;
	// 8.1 or higher - Supports Per-Monitor scaling
	hSHCore = LoadLibrary(_T("SHCore.dll"));
	if (hSHCore)
	{
		_SetProcessDpiAwareness =
			(HRESULT(WINAPI*)(DWORD))GetProcAddress(hSHCore, "SetProcessDpiAwareness");
		if (_SetProcessDpiAwareness) level = 2;
		else
		{
			FreeLibrary(hSHCore);
			return level;
		}
		FreeLibrary(hSHCore);
	}
	else return level;
	// v1703 or higher - Support Per-Monitor scaling v2
	hUser32 = LoadLibrary(_T("User32.dll"));
	if (hUser32)
	{
		_SetProcessDpiAwarenessContext =
			(BOOL(WINAPI*)(HANDLE))GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
		if (_SetProcessDpiAwarenessContext) level = 3;
		else
		{
			FreeLibrary(hUser32);
			return level;
		}
		FreeLibrary(hUser32);
	}
	return level;
}

LRESULT CALLBACK LoadingCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND *hWndReturn = (HWND*)lParam;
	switch (Msg)
	{
	case WM_INITDIALOG:
		*hWndReturn = hWnd;
		return TRUE;
	case WM_USER+1:
		DestroyWindow(hWnd);
		return TRUE;
	default:
		return FALSE;
	}
}

DWORD WINAPI ProgressThread(LPVOID hWndOut)
{
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LOADING), NULL, (DLGPROC)LoadingCallback, (LPARAM)hWndOut);
	ExitThread(0);
	return 0;
}

void RefreshControls(HWND hWnd)
{
	if (current_app)
	{
		EnableWindow(GetDlgItem(hTabs[3], IDC_PATHLABEL), TRUE);
		EnableWindow(GetDlgItem(hTabs[3], IDC_PROFILEPATH), TRUE);
		EnableWindow(GetDlgItem(hTabs[3], IDC_WRITEINI), TRUE);
		SetDlgItemText(hTabs[3], IDC_PROFILEPATH, apps[current_app].path);
		if (apps[current_app].builtin) EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), FALSE);
		else EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hTabs[3], IDC_PATHLABEL), FALSE);
		EnableWindow(GetDlgItem(hTabs[3], IDC_PROFILEPATH), FALSE);
		EnableWindow(GetDlgItem(hTabs[3], IDC_WRITEINI), FALSE);
		SetDlgItemText(hTabs[3], IDC_PROFILEPATH, _T(""));
		EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), FALSE);
	}
	// Set 3-state status
	if (current_app && !tristate)
	{
		tristate = TRUE;
		// Display tab
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_COLORDEPTH, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_SCALE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_EXTRAMODES, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_SORTMODES, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_DPISCALE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_VSYNC, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_WINDOWSCALE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[0], IDC_COLOR, BM_SETSTYLE, BS_AUTO3STATE, (LPARAM)TRUE);
		SendDlgItemMessage(hTabs[0], IDC_SINGLEBUFFER, BM_SETSTYLE, BS_AUTO3STATE, (LPARAM)TRUE);
		SendDlgItemMessage(hTabs[0], IDC_SETDISPLAYCONFIG, BM_SETSTYLE, BS_AUTO3STATE, (LPARAM)TRUE);
		// Effects tab
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_ADDSTRING, 0, (LPARAM)strdefault);
		// 3D tab
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[2], IDC_ASPECT3D, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[2], IDC_LOWCOLORRENDER, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[2], IDC_LIMITTEXFORMATS, CB_ADDSTRING, 0, (LPARAM)strdefault);
		// Advanced tab
		SendDlgItemMessage(hTabs[3], IDC_TEXTUREFORMAT, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[3], IDC_TEXUPLOAD, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_ADDSTRING, 0, (LPARAM)strdefault);
		SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWPOS, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWSIZE, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		SendDlgItemMessage(hTabs[3], IDC_WINDOWMAXIMIZED, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		SendDlgItemMessage(hTabs[3], IDC_NOAUTOSIZE, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		SendDlgItemMessage(hTabs[3], IDC_CAPTUREMOUSE, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		// Debug tab
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)strdefault);
		// Tracing tab
		SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_ADDSTRING, 0, (LPARAM)strdefault);
	}
	else if (!current_app && tristate)
	{
		tristate = FALSE;
		// Display tab
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_COLORDEPTH, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_COLORDEPTH, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_SCALE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_SCALE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_EXTRAMODES, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_EXTRAMODES, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_SORTMODES, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_SORTMODES, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_DPISCALE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_DPISCALE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_VSYNC, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_VSYNC, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_WINDOWSCALE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[0], IDC_WINDOWSCALE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[0], IDC_COLOR, BM_SETSTYLE, BS_AUTOCHECKBOX, (LPARAM)TRUE);
		SendDlgItemMessage(hTabs[0], IDC_SINGLEBUFFER, BM_SETSTYLE, BS_AUTOCHECKBOX, (LPARAM)TRUE);
		SendDlgItemMessage(hTabs[0], IDC_SETDISPLAYCONFIG, BM_SETSTYLE, BS_AUTOCHECKBOX, (LPARAM)TRUE);
		// Effects tab
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[1], IDC_POSTSCALE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		// 3D tab
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[2], IDC_ASPECT3D, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[2], IDC_ASPECT3D, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[2], IDC_LOWCOLORRENDER, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[2], IDC_LOWCOLORRENDER, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[2], IDC_LIMITTEXFORMATS, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[2], IDC_LIMITTEXFORMATS, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		// Advanced tab
		SendDlgItemMessage(hTabs[3], IDC_TEXTUREFORMAT, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[3], IDC_TEXTUREFORMAT, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[3], IDC_TEXUPLOAD, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[3], IDC_TEXUPLOAD, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWPOS, BM_SETSTYLE, BS_AUTOCHECKBOX, TRUE);
		SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWSIZE, BM_SETSTYLE, BS_AUTOCHECKBOX, TRUE);
		SendDlgItemMessage(hTabs[3], IDC_WINDOWMAXIMIZED, BM_SETSTYLE, BS_AUTOCHECKBOX, TRUE);
		SendDlgItemMessage(hTabs[3], IDC_NOAUTOSIZE, BM_SETSTYLE, BS_AUTOCHECKBOX, TRUE);
		// Debug tab
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
		// Tracing tab
		SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_DELETESTRING,
			SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
	}
	// Read settings into controls
	// Display tab
	SetCombo(hTabs[0], IDC_VIDMODE, cfg->scaler, cfgmask->scaler, tristate);
	SetCombo(hTabs[0], IDC_COLORDEPTH, 0, 0, tristate);
	SetCombo(hTabs[0], IDC_SCALE, cfg->scalingfilter, cfgmask->scalingfilter, tristate);
	SetCombo(hTabs[0], IDC_EXTRAMODES, 0, 0, tristate);
	SetAspectCombo(hTabs[0], IDC_ASPECT, cfg->aspect, (DWORD)cfgmask->aspect, tristate);
	SetCombo(hTabs[0], IDC_SORTMODES, cfg->SortModes, cfgmask->SortModes, tristate);
	SetCombo(hTabs[0], IDC_DPISCALE, cfg->DPIScale, cfgmask->DPIScale, tristate);
	SetCombo(hTabs[0], IDC_VSYNC, cfg->vsync, cfgmask->vsync, tristate);
	SetCombo(hTabs[0], IDC_FULLMODE, cfg->fullmode, cfgmask->fullmode, tristate);
	SetCheck(hTabs[0], IDC_COLOR, cfg->colormode, cfgmask->colormode, tristate);
	SetCheck(hTabs[0], IDC_SINGLEBUFFER, cfg->SingleBufferDevice, cfgmask->SingleBufferDevice, tristate);
	SetCheck(hTabs[0], IDC_SETDISPLAYCONFIG, cfg->UseSetDisplayConfig, cfgmask->UseSetDisplayConfig, tristate);
	if (cfg->scaler == 8)
	{
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABEL), TRUE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELX), TRUE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELY), TRUE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEX), TRUE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEY), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABEL), FALSE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELX), FALSE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELY), FALSE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEX), FALSE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEY), FALSE);
	}
	if ((cfg->scaler == 9) || (cfg->scaler == 10))
	{
		EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODELABEL), TRUE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODE), TRUE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_SETMODE), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODELABEL), FALSE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODE), FALSE);
		EnableWindow(GetDlgItem(hTabs[0], IDC_SETMODE), FALSE);
	}
	SetFloat3place(hTabs[0], IDC_FIXEDSCALEX, cfg->DisplayMultiplierX, cfgmask->DisplayMultiplierX);
	SetFloat3place(hTabs[0], IDC_FIXEDSCALEY, cfg->DisplayMultiplierY, cfgmask->DisplayMultiplierY);
	SetResolution(hTabs[0], IDC_CUSTOMMODE, cfg, cfgmask);
	SetWindowScaleCombo(hTabs[0], IDC_WINDOWSCALE, cfg->WindowScaleX, cfg->WindowScaleY,
		(DWORD)cfgmask->WindowScaleX, (DWORD)cfgmask->WindowScaleY, tristate);
	// Effects tab
	SetCombo(hTabs[1], IDC_POSTSCALE, cfg->postfilter, cfgmask->postfilter, tristate);
	SetPostScaleCombo(hTabs[1], IDC_POSTSCALESIZE, cfg->postsizex, cfg->postsizey,
		(DWORD)cfgmask->postsizex, (DWORD)cfgmask->postsizey, tristate);
	SetCombo(hTabs[1], IDC_PRIMARYSCALE, cfg->primaryscale, cfgmask->primaryscale, tristate);
	if (cfg->primaryscale == 12)
	{
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABEL), TRUE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABELX), TRUE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALEX), TRUE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABELY), TRUE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALEY), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABEL), FALSE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABELX), FALSE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALEX), FALSE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABELY), FALSE);
		EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALEY), FALSE);
	}
	SetFloat3place(hTabs[1], IDC_CUSTOMSCALEX, cfg->primaryscalex, cfgmask->primaryscalex);
	SetFloat3place(hTabs[1], IDC_CUSTOMSCALEY, cfg->primaryscaley, cfgmask->primaryscaley);
	SetText(hTabs[1], IDC_SHADER, cfg->shaderfile, cfgmask->shaderfile, tristate);
	SetCombo(hTabs[1], IDC_BLTFILTER, cfg->BltScale, cfgmask->BltScale, tristate);
	// Removed for DXGL 0.5.13 release
	/* SetInteger(hTabs[1], IDC_BLTTHRESHOLD, cfg->BltThreshold, cfgmask->BltThreshold);
	if (cfgmask->BltThreshold)
	{
		SendDlgItemMessage(hTabs[1], IDC_BLTTHRESHOLDSLIDER, TBM_SETPOS, TRUE, cfg->BltThreshold);
	}
	else SendDlgItemMessage(hTabs[1], IDC_BLTTHRESHOLDSLIDER, TBM_SETPOS, TRUE, 127);*/
	// 3D tab
	SetCombo(hTabs[2], IDC_TEXFILTER, cfg->texfilter, cfgmask->texfilter, tristate);
	SetCombo(hTabs[2], IDC_ANISO, cfg->anisotropic, cfgmask->anisotropic, tristate);
	SetCombo(hTabs[2], IDC_MSAA, cfg->msaa, cfgmask->msaa, tristate);
	SetCombo(hTabs[2], IDC_ASPECT3D, cfg->aspect3d, cfgmask->aspect3d, tristate);
	SetCombo(hTabs[2], IDC_LOWCOLORRENDER, cfg->LowColorRendering, cfgmask->LowColorRendering, tristate);
	SetCombo(hTabs[2], IDC_DITHERING, cfg->EnableDithering, cfgmask->EnableDithering, tristate);
	SetCombo(hTabs[2], IDC_LIMITTEXFORMATS, cfg->LimitTextureFormats, cfgmask->LimitTextureFormats, tristate);
	// Advanced tab
	SetCombo(hTabs[3], IDC_TEXTUREFORMAT, cfg->TextureFormat, cfgmask->TextureFormat, tristate);
	SetCombo(hTabs[3], IDC_TEXUPLOAD, cfg->TexUpload, cfgmask->TexUpload, tristate);
	SetCombo(hTabs[3], IDC_WINDOWPOS, cfg->WindowPosition, cfgmask->WindowPosition, tristate);
	SetCheck(hTabs[3], IDC_REMEMBERWINDOWPOS, cfg->RememberWindowPosition, cfgmask->RememberWindowPosition, tristate);
	SetCheck(hTabs[3], IDC_REMEMBERWINDOWSIZE, cfg->RememberWindowSize, cfgmask->RememberWindowSize, tristate);
	SetInteger(hTabs[3], IDC_WINDOWX, cfg->WindowX, cfgmask->WindowX);
	SetInteger(hTabs[3], IDC_WINDOWY, cfg->WindowY, cfgmask->WindowY);
	SetInteger(hTabs[3], IDC_WINDOWWIDTH, cfg->WindowWidth, cfgmask->WindowWidth);
	SetInteger(hTabs[3], IDC_WINDOWHEIGHT, cfg->WindowHeight, cfgmask->WindowHeight);
	SetCheck(hTabs[3], IDC_WINDOWMAXIMIZED, cfg->WindowMaximized, cfgmask->WindowMaximized, tristate);
	SetCheck(hTabs[3], IDC_NOAUTOSIZE, cfg->NoResizeWindow, cfgmask->NoResizeWindow, tristate);
	SetCheck(hTabs[3], IDC_CAPTUREMOUSE, cfg->CaptureMouse, cfgmask->CaptureMouse, tristate);
	// Debug tab
	RedrawWindow(GetDlgItem(hTabs[4], IDC_DEBUGLIST), NULL, NULL, RDW_INVALIDATE);
	SetGLCombo(hTabs[4], IDC_GLVERSION, &cfg->DebugMaxGLVersionMajor, &cfg->DebugMaxGLVersionMinor,
		&cfgmask->DebugMaxGLVersionMajor, &cfgmask->DebugMaxGLVersionMinor, tristate, hWnd);
	// Hacks tab
	if((hackstabitem == 3) || (hackstabitem == 6))
		UpdateHacksControl(GetDlgItem(hTabs[5], IDC_HACKSLIST), IDC_HACKSEDIT, hackstabitem);
	else UpdateHacksControl(GetDlgItem(hTabs[5], IDC_HACKSLIST), IDC_HACKSDROPDOWN, hackstabitem);
	RedrawWindow(GetDlgItem(hTabs[5], IDC_HACKSLIST), NULL, NULL, RDW_INVALIDATE);
	// Tracing tab
	SetCombo(hTabs[6], IDC_TRACING, cfg->DebugTraceLevel, cfgmask->DebugTraceLevel, tristate);
}

LRESULT CALLBACK DXGLCfgCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PIXELFORMATDESCRIPTOR pfd =
	    {
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
			0,                        //Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                        //Number of bits for the depthbuffer
			8,                        //Number of bits for the stencilbuffer
			0,                        //Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
	        0, 0, 0
	    };
	GLfloat anisotropic;
	HDC dc;
	HGLRC rc;
	GLint maxsamples;
	GLint maxcoverage = 0;
	GLint coveragemodes[64];
	int msaamodes[32];
	int pf;
	int i;
	HKEY hKeyBase;
	HKEY hKey;
	DWORD keysize,keysize2;
	DEVMODE mode;
	LPTSTR keyname;
	LPTSTR regbuffer;
	DWORD regbuffersize;
	DWORD buffersize;
	LONG error;
	TCHAR buffer[64];
	TCHAR subkey[MAX_PATH];
	LPTSTR path;
	SHFILEINFO fileinfo;
	DWORD verinfosize;
	LPTSTR outbuffer;
	UINT outlen;
	TCHAR verpath[64];
	WORD translation[2];
	DWORD cursel;
	DRAWITEMSTRUCT* drawitem;
	BOOL hasname;
	void *verinfo;
	COLORREF OldTextColor,OldBackColor;
	HANDLE token = NULL;
	DWORD elevation;
	HWND hGLWnd;
	OPENFILENAME filename;
	TCHAR selectedfile[MAX_PATH + 1];
	LPTSTR regpath;
	LPTSTR regkey;
	BOOL failed;
	LPTSTR installpath;
	DWORD err;
	RECT r;
	NMHDR *nm;
	TCHAR abouttext[1024];
	int newtab;
	DWORD dpisupport;
	TCITEM tab;
	HWND hProgressWnd;
	WNDCLASSEX wndclass;
	HWND hTempWnd;
	//DWORD threadid;
	switch (Msg)
	{
	case WM_INITDIALOG:
		hProgressWnd = NULL;
		/*CreateThread(NULL, 0, ProgressThread, &hProgressWnd, 0, &threadid);
		while (hProgressWnd == NULL) Sleep(10);*/
		hDialog = hWnd;
		ZeroMemory(&wndclass, sizeof(WNDCLASSEX));
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.lpfnWndProc = DefWindowProc;
		wndclass.hInstance = GetModuleHandle(NULL);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wndclass.lpszClassName = _T("DXGLConfigDlgPosition");
		RegisterClassEx(&wndclass);
		GetWindowRect(hWnd, &r);
		hTempWnd = CreateWindow(_T("DXGLConfigDlgPosition"), dxglcfgname,
			DS_3DLOOK | DS_CONTEXTHELP | DS_SHELLFONT | WS_CAPTION | WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT, r.right-r.left,r.bottom-r.top,NULL,NULL,
			GetModuleHandle(NULL), NULL);
		GetWindowRect(hTempWnd, &r);
		SetWindowPos(hWnd, HWND_TOP,r.left,r.top,0,0, SWP_NOSIZE);
		DestroyWindow(hTempWnd);
		UnregisterClass(_T("DXGLConfigDlgPosition"), GetModuleHandle(NULL));
		tristate = FALSE;
		maxapps = 128;
		apps = (app_setting *)malloc(maxapps*sizeof(app_setting));
		apps[0].name = (TCHAR*)malloc(7 * sizeof(TCHAR));
		_tcscpy(apps[0].name,_T("Global"));
		apps[0].regkey = (TCHAR*)malloc(7 * sizeof(TCHAR));
		_tcscpy(apps[0].regkey,regglobal);
		GetGlobalConfigWithMask(&apps[0].cfg, &apps[0].mask, FALSE);
		cfg = &apps[0].cfg;
		cfgmask = &apps[0].mask;
		dirty = &apps[0].dirty;
		apps[0].dirty = FALSE;
		apps[0].icon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_STAR));
		apps[0].icon_shared = TRUE;
		apps[0].path[0] = 0;
		SetClassLongPtr(hWnd,GCLP_HICON,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGL));
		SetClassLongPtr(hWnd,GCLP_HICONSM,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGLSM));
		// create temporary gl context to get AA and AF settings.
		mode.dmSize = sizeof(DEVMODE);
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&mode);
		pfd.cColorBits = (BYTE)mode.dmBitsPerPel;
		hGLWnd = CreateWindow(_T("STATIC"),NULL,WS_CHILD,0,0,16,16,hWnd,NULL,NULL,NULL);
		dc = GetDC(hGLWnd);
		pf = ChoosePixelFormat(dc,&pfd);
		SetPixelFormat(dc,pf,&pfd);
		rc = wglCreateContext(dc);
		wglMakeCurrent(dc,rc);
		extensions_string = (char*)glGetString(GL_EXTENSIONS);
		if(strstr(extensions_string,"GL_EXT_texture_filter_anisotropic"))
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&anisotropic);
		else anisotropic = 0;
		if(strstr(extensions_string,"GL_EXT_framebuffer_multisample"))
		{
			glGetIntegerv(GL_MAX_SAMPLES_EXT,&maxsamples);
			if(strstr(extensions_string,"GL_NV_framebuffer_multisample_coverage")) // Supports NVIDIA CSAA
			{
				glGetIntegerv(GL_MAX_MULTISAMPLE_COVERAGE_MODES_NV,&maxcoverage);
				glGetIntegerv(GL_MULTISAMPLE_COVERAGE_MODES_NV,coveragemodes);
				if(maxcoverage) for(i = 0; i < maxcoverage; i++)
				{
					msaamodes[i] = coveragemodes[2*i]+(4096*coveragemodes[(2*i)+1]);
					msaa_available = TRUE;
				}
			}
		}
		wglMakeCurrent(dc,NULL);
		wglDeleteContext(rc);
		ReleaseDC(hGLWnd,dc);
		DestroyWindow(hGLWnd);
		uxtheme = LoadLibrary(_T("uxtheme.dll"));
		if (uxtheme)
		{

			_OpenThemeData = (HTHEME(WINAPI*)(HWND,LPCWSTR))GetProcAddress(uxtheme, "OpenThemeData");
			_CloseThemeData = (HRESULT(WINAPI*)(HTHEME))GetProcAddress(uxtheme, "CloseThemeData");
			_DrawThemeBackground = 
				(HRESULT(WINAPI*)(HTHEME, HDC, int, int, const RECT*, const RECT*))
				GetProcAddress(uxtheme, "DrawThemeBackground");
			_EnableThemeDialogTexture = (HRESULT(WINAPI*)(HWND, DWORD))
				GetProcAddress(uxtheme, "EnableThemeDialogTexture");
			if (!(_OpenThemeData && _CloseThemeData && _DrawThemeBackground && _EnableThemeDialogTexture))
			{
				FreeLibrary(uxtheme);
				uxtheme = NULL;
			}
		}
		// Add tabs
		ZeroMemory(&tab, sizeof(TCITEM));
		tab.mask = TCIF_TEXT;
		tab.pszText = _T("Display");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 0, (LPARAM)&tab);
		tab.pszText = _T("Effects");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 1, (LPARAM)&tab);
		tab.pszText = _T("3D Graphics");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 2, (LPARAM)&tab);
		tab.pszText = _T("Advanced");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 3, (LPARAM)&tab);
		tab.pszText = _T("Debug");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 4, (LPARAM)&tab);
		tab.pszText = _T("Hacks");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 5, (LPARAM)&tab);
		tab.pszText = _T("Tracing");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 6, (LPARAM)&tab);
		tab.pszText = _T("Graphics Tests");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 7, (LPARAM)&tab);
		tab.pszText = _T("About");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 8, (LPARAM)&tab);
		hTab = GetDlgItem(hWnd, IDC_TABS);
		createdialoglock = TRUE;
		hTabs[0] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_DISPLAY), hTab, (DLGPROC)DisplayTabCallback);
		hTabs[1] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_EFFECTS), hTab, (DLGPROC)EffectsTabCallback);
		hTabs[2] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_3DGRAPHICS), hTab, (DLGPROC)Tab3DCallback);
		hTabs[3] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_ADVANCED), hTab, (DLGPROC)AdvancedTabCallback);
		hTabs[4] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_DEBUG), hTab, (DLGPROC)DebugTabCallback);
		hTabs[5] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_HACKS), hTab, (DLGPROC)HacksTabCallback);
		hTabs[6] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_TRACING), hTab, (DLGPROC)TracingTabCallback);
		hTabs[7] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_TESTGFX), hTab, (DLGPROC)TestTabCallback);
		hTabs[8] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_ABOUT), hTab, (DLGPROC)AboutTabCallback);
		createdialoglock = FALSE;
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETITEMRECT, 0, (LPARAM)&r);
		SetWindowPos(hTabs[0], NULL, r.left, r.bottom + 3, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		ShowWindow(hTabs[1], SW_HIDE);
		ShowWindow(hTabs[2], SW_HIDE);
		ShowWindow(hTabs[3], SW_HIDE);
		ShowWindow(hTabs[4], SW_HIDE);
		ShowWindow(hTabs[5], SW_HIDE);
		ShowWindow(hTabs[6], SW_HIDE);
		ShowWindow(hTabs[7], SW_HIDE);
		ShowWindow(hTabs[8], SW_HIDE);
		tabopen = 0;

		// Load global settings.
		// video mode
		_tcscpy(buffer,_T("Change desktop resolution"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Stretch to screen"));
		SendDlgItemMessage(hTabs[0],IDC_VIDMODE,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Aspect corrected stretch"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 2, (LPARAM)buffer);
		_tcscpy(buffer,_T("Center image on screen"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 3, (LPARAM)buffer);
		_tcscpy(buffer,_T("Stretch if mode not found"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 4, (LPARAM)buffer);
		_tcscpy(buffer,_T("Scale if mode not found"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 5, (LPARAM)buffer);
		_tcscpy(buffer,_T("Center if mode not found"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 6, (LPARAM)buffer);
		_tcscpy(buffer,_T("Crop to screen"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 7, (LPARAM)buffer);
		_tcscpy(buffer, _T("Custom size multiplier"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 8, (LPARAM)buffer);
		_tcscpy(buffer, _T("Custom display mode"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 9, (LPARAM)buffer);
		_tcscpy(buffer, _T("Custom size, centered"));
		SendDlgItemMessage(hTabs[0], IDC_VIDMODE, CB_ADDSTRING, 10, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[0],IDC_VIDMODE,CB_SETCURSEL,cfg->scaler,0);
		if (cfg->scaler == 8)
		{
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABEL), TRUE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELX), TRUE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELY), TRUE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEX), TRUE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEY), TRUE);
		}
		else
		{
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABEL), FALSE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELX), FALSE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALELABELY), FALSE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEX), FALSE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_FIXEDSCALEY), FALSE);
		}
		if ((cfg->scaler == 9) || (cfg->scaler == 10))
		{
			EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODELABEL), TRUE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODE), TRUE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_SETMODE), TRUE);
		}
		else
		{
			EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODELABEL), FALSE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_CUSTOMMODE), FALSE);
			EnableWindow(GetDlgItem(hTabs[0], IDC_SETMODE), FALSE);
		}
		// custom scale
		SetFloat3place(hTabs[0], IDC_FIXEDSCALEX, cfg->DisplayMultiplierX, cfgmask->DisplayMultiplierX);
		SetFloat3place(hTabs[0], IDC_FIXEDSCALEY, cfg->DisplayMultiplierY, cfgmask->DisplayMultiplierY);
		// custom resolution
		SetResolution(hTabs[0], IDC_CUSTOMMODE, cfg, cfgmask);
		// window scale
		_tcscpy(buffer, _T("1x"));
		SendDlgItemMessage(hTabs[0], IDC_WINDOWSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("2x"));
		SendDlgItemMessage(hTabs[0], IDC_WINDOWSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("3x"));
		SendDlgItemMessage(hTabs[0], IDC_WINDOWSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("4x"));
		SendDlgItemMessage(hTabs[0], IDC_WINDOWSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		SetWindowScaleCombo(hTabs[0], IDC_WINDOWSCALE, cfg->WindowScaleX, cfg->WindowScaleY,
			(DWORD)cfgmask->WindowScaleX, (DWORD)cfgmask->WindowScaleY, tristate);
		// fullscreen window mode
		_tcscpy(buffer, _T("Exclusive fullscreen"));
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Non-exclusive fullscreen"));
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer, _T("Non-resizable window"));
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_ADDSTRING, 2, (LPARAM)buffer);
		_tcscpy(buffer, _T("Resizable window"));
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_ADDSTRING, 3, (LPARAM)buffer);
		_tcscpy(buffer, _T("Borderless window"));
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_ADDSTRING, 4, (LPARAM)buffer);
		_tcscpy(buffer, _T("Borderless window (scaled)"));
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_ADDSTRING, 5, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[0], IDC_FULLMODE, CB_SETCURSEL, cfg->fullmode, 0);
		// vsync mode
		_tcscpy(buffer, _T("Application default"));
		SendDlgItemMessage(hTabs[0], IDC_VSYNC, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disabled"));
		SendDlgItemMessage(hTabs[0], IDC_VSYNC, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer, _T("Enabled every flip or primary Blt"));
		SendDlgItemMessage(hTabs[0], IDC_VSYNC, CB_ADDSTRING, 2, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[0], IDC_VSYNC, CB_SETCURSEL, cfg->vsync, 0);
		// colormode
		if (cfg->colormode) SendDlgItemMessage(hTabs[0], IDC_COLOR, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[0], IDC_COLOR, BM_SETCHECK, BST_UNCHECKED, 0);
		// single buffer
		if(cfg->SingleBufferDevice) SendDlgItemMessage(hTabs[0], IDC_SINGLEBUFFER, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[0], IDC_SINGLEBUFFER, BM_SETCHECK, BST_UNCHECKED, 0);
		// use SetDisplayConfig
		if (cfg->UseSetDisplayConfig) SendDlgItemMessage(hTabs[0], IDC_SETDISPLAYCONFIG, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[0], IDC_SETDISPLAYCONFIG, BM_SETCHECK, BST_UNCHECKED, 0);
		if ((osver.dwMajorVersion > 6) || ((osver.dwMajorVersion) == 6 && (osver.dwMinorVersion >= 1)))
			EnableWindow(GetDlgItem(hTabs[0], IDC_SETDISPLAYCONFIG), TRUE);
		// postprocess scaling filter
		_tcscpy(buffer, _T("Nearest"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Bilinear"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALE, CB_ADDSTRING, 1, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALE, CB_SETCURSEL, cfg->postfilter, 0);
		// postprocess scaling sizes
		_tcscpy(buffer, _T("Auto"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("1x"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("2x1"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("2x"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("3x"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("4x"));
		SendDlgItemMessage(hTabs[1], IDC_POSTSCALESIZE, CB_ADDSTRING, 0, (LPARAM)buffer);
		SetPostScaleCombo(hTabs[1], IDC_POSTSCALESIZE, cfg->postsizex, cfg->postsizey,
			(DWORD)cfgmask->postsizex, (DWORD)cfgmask->postsizey, tristate);
		// primary scaling
		_tcscpy(buffer, _T("1x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Scale to screen"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Maximum integer scaling"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("1.5x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("2x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("2.5x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("3x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("4x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("5x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("6x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("7x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("8x scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Custom scale"));
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[1], IDC_PRIMARYSCALE, CB_SETCURSEL, cfg->primaryscale, 0);
		// Primary scaling size
		if (cfg->primaryscale == 12)
		{
			EnableWindow(GetDlgItem(hTabs[1],IDC_CUSTOMSCALELABEL), TRUE);
			EnableWindow(GetDlgItem(hTabs[1],IDC_CUSTOMSCALELABELX), TRUE);
			EnableWindow(GetDlgItem(hTabs[1],IDC_CUSTOMSCALEX), TRUE);
			EnableWindow(GetDlgItem(hTabs[1],IDC_CUSTOMSCALELABELY), TRUE);
			EnableWindow(GetDlgItem(hTabs[1],IDC_CUSTOMSCALEY), TRUE);
		}
		else
		{
			EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABEL), FALSE);
			EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABELX), FALSE);
			EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALEX), FALSE);
			EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALELABELY), FALSE);
			EnableWindow(GetDlgItem(hTabs[1], IDC_CUSTOMSCALEY), FALSE);
		}
		SetFloat3place(hTabs[1], IDC_CUSTOMSCALEX, cfg->primaryscalex, cfgmask->primaryscalex);
		SetFloat3place(hTabs[1], IDC_CUSTOMSCALEY, cfg->primaryscaley, cfgmask->primaryscaley);
		// scaling filter
		_tcscpy(buffer,_T("Nearest"));
		SendDlgItemMessage(hTabs[0], IDC_SCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear"));
		SendDlgItemMessage(hTabs[0],IDC_SCALE,CB_ADDSTRING,1,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[0],IDC_SCALE,CB_SETCURSEL,cfg->scalingfilter,0);
		// Blt scaling filter
		_tcscpy(buffer, _T("Nearest"));
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Bilinear"));
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_ADDSTRING, 1, (LPARAM)buffer);
		/* Temporarily removed until implemented
		_tcscpy(buffer, _T("Bilinear, nearest colorkey"));
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_ADDSTRING, 2, (LPARAM)buffer);
		_tcscpy(buffer, _T("Bilinear, sharp colorkey"));
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_ADDSTRING, 3, (LPARAM)buffer);
		_tcscpy(buffer, _T("Bilinear, soft colorkey"));
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_ADDSTRING, 4, (LPARAM)buffer);
		*/
		SendDlgItemMessage(hTabs[1], IDC_BLTFILTER, CB_SETCURSEL, cfg->BltScale, 0);
		// Removed for DXGL 0.5.13 release
		// Blt scaling threshold
		// SendDlgItemMessage(hTabs[1], IDC_BLTTHRESHOLDSLIDER, TBM_SETRANGE, 0, 0xFE0000);
		// SendDlgItemMessage(hTabs[1], IDC_BLTTHRESHOLDSLIDER, TBM_SETPOS, TRUE, cfg->BltThreshold);
		// SendDlgItemMessage(hTabs[1], IDC_BLTTHRESHOLD, EM_SETLIMITTEXT, 3, 0);
		// SetInteger(hTabs[1], IDC_BLTTHRESHOLD, cfg->BltThreshold, cfgmask->BltThreshold);
		// aspect
		_tcscpy(buffer,_T("Default"));
		SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("4:3"));
		SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("16:10"));
		SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("16:9"));
		SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("5:4"));
		SendDlgItemMessage(hTabs[0], IDC_ASPECT, CB_ADDSTRING, 0, (LPARAM)buffer);
		SetAspectCombo(hTabs[0], IDC_ASPECT, cfg->aspect, (DWORD)cfgmask->aspect, tristate);
		// texfilter
		_tcscpy(buffer,_T("Application default"));
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest"));
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear"));
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_ADDSTRING, 2, (LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest, nearest mipmap"));
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_ADDSTRING, 3, (LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest, linear mipmap"));
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_ADDSTRING, 4, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear, nearest mipmap"));
		SendDlgItemMessage(hTabs[2], IDC_TEXFILTER, CB_ADDSTRING, 5, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear, linear mipmap"));
		SendDlgItemMessage(hTabs[2],IDC_TEXFILTER,CB_ADDSTRING,6,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[2],IDC_TEXFILTER,CB_SETCURSEL,cfg->texfilter,0);
		// anisotropic
		if (anisotropic < 2)
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 0, (LPARAM)buffer);
			SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_SETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(hTabs[2], IDC_ANISO), FALSE);
			cfg->anisotropic = 0;
		}
		else
		{
			_tcscpy(buffer,_T("Application default"));
			SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 0, (LPARAM)buffer);
			_tcscpy(buffer,_T("Disabled"));
			SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 1, (LPARAM)buffer);
			if(anisotropic >= 2)
			{
				_tcscpy(buffer,_T("2x"));
				SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 2, (LPARAM)buffer);
			}
			if(anisotropic >= 4)
			{
				_tcscpy(buffer,_T("4x"));
				SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 4, (LPARAM)buffer);
			}
			if(anisotropic >= 8)
			{
				_tcscpy(buffer,_T("8x"));
				SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 8, (LPARAM)buffer);
			}
			if(anisotropic >= 16)
			{
				_tcscpy(buffer,_T("16x"));
				SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 16, (LPARAM)buffer);
			}
			if(anisotropic >= 32)
			{
				_tcscpy(buffer,_T("32x"));
				SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_ADDSTRING, 4, (LPARAM)buffer);
			}
			SendDlgItemMessage(hTabs[2], IDC_ANISO, CB_SETCURSEL, cfg->anisotropic, 0);
		}
		// msaa
#ifdef _DEBUG
		if(msaa_available)
		{
			_tcscpy(buffer,_T("Application default"));
			SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 0, (LPARAM)buffer);
			_tcscpy(buffer,_T("Disabled"));
			SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 1, (LPARAM)buffer);
			if(maxcoverage)
			{
				for(i = 0; i < maxcoverage; i++)
				{
					if((msaamodes[i] & 0xfff) <= 4)
						_sntprintf(buffer,64,_T("%dx"),msaamodes[i] & 0xfff);
					else _sntprintf(buffer,64,_T("%dx coverage, %dx color"),(msaamodes[i] & 0xfff), (msaamodes[i] >> 12));
					SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, msaamodes[i], (LPARAM)buffer);
				}
			}
			else
			{
				if(maxsamples >= 2)
				{
					_tcscpy(buffer,_T("2x"));
					SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 2, (LPARAM)buffer);
				}
				if(maxsamples >= 4)
				{
					_tcscpy(buffer,_T("4x"));
					SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 4, (LPARAM)buffer);
				}
				if(maxsamples >= 8)
				{
					_tcscpy(buffer,_T("8x"));
					SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 8, (LPARAM)buffer);
				}
				if(maxsamples >= 16)
				{
					_tcscpy(buffer,_T("16x"));
					SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 16, (LPARAM)buffer);
				}
				if(maxsamples >= 32)
				{
					_tcscpy(buffer,_T("32x"));
					SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 32, (LPARAM)buffer);
				}
			}
			SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_SETCURSEL, cfg->msaa, 0);
		}
		else
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 0, (LPARAM)buffer);
			SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_SETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(hTabs[2], IDC_MSAA), FALSE);
			cfg->msaa = 0;
		}
#else
		_tcscpy(buffer, _T("Not yet implemented"));
		SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[2], IDC_MSAA, CB_SETCURSEL, 0, 0);
		EnableWindow(GetDlgItem(hTabs[2], IDC_MSAA), FALSE);
		cfg->msaa = 0;
#endif
		// aspect3d
		_tcscpy(buffer,_T("Stretch to display"));
		SendDlgItemMessage(hTabs[2], IDC_ASPECT3D, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Expand viewable area"));
		SendDlgItemMessage(hTabs[2], IDC_ASPECT3D, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer,_T("Crop to display"));
		SendDlgItemMessage(hTabs[2],IDC_ASPECT3D,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[2],IDC_ASPECT3D,CB_SETCURSEL,cfg->aspect3d,0);
		// low color render
		_tcscpy(buffer, _T("Use surface color depth"));
		SendDlgItemMessage(hTabs[2], IDC_LOWCOLORRENDER, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Use 32-bpp color depth"));
		SendDlgItemMessage(hTabs[2], IDC_LOWCOLORRENDER, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[2], IDC_LOWCOLORRENDER, CB_SETCURSEL, cfg->LowColorRendering, 0);
		// Enable dithering
		_tcscpy(buffer, _T("Application defined, low color"));
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disabled"));
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Always enabled, low color"));
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Application defined, all modes"));
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Always enabled, low color"));
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[2], IDC_DITHERING, CB_SETCURSEL, cfg->EnableDithering, 0);
		// Limit texture formats
		_tcscpy(buffer, _T("Disabled"));
		SendDlgItemMessage(hTabs[2], IDC_LIMITTEXFORMATS, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Enabled for DX5 and below"));
		SendDlgItemMessage(hTabs[2], IDC_LIMITTEXFORMATS, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Enabled always"));
		SendDlgItemMessage(hTabs[2], IDC_LIMITTEXFORMATS, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[2], IDC_LIMITTEXFORMATS, CB_SETCURSEL, cfg->LimitTextureFormats, 0);
		// sort modes
		_tcscpy(buffer,_T("Use system order"));
		SendDlgItemMessage(hTabs[0], IDC_SORTMODES, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Group by color depth"));
		SendDlgItemMessage(hTabs[0], IDC_SORTMODES, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer,_T("Group by resolution"));
		SendDlgItemMessage(hTabs[0],IDC_SORTMODES,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[0],IDC_SORTMODES,CB_SETCURSEL,cfg->SortModes,0);
		// color depths
		for (i = 0; i < 5; i++)
		{
			_tcscpy(buffer, colormodedropdown[i]);
			SendDlgItemMessage(hTabs[0], IDC_COLORDEPTH, CB_ADDSTRING, i, (LPARAM)buffer);
		}
		SendDlgItemMessage(hTabs[0], IDC_COLORDEPTH, CB_SETCURSEL, cfg->AddColorDepths, 0);
		for (i = 0; i < 8; i++)
		{
			_tcscpy(buffer, extramodes[i]);
			SendDlgItemMessage(hTabs[0], IDC_EXTRAMODES, CB_ADDSTRING, i, (LPARAM)buffer);
		}
		// Enable shader
		if (cfg->colormode) SendDlgItemMessage(hTabs[1], IDC_USESHADER, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[1], IDC_USESHADER, BM_SETCHECK, BST_UNCHECKED, 0);
		// shader path
		SetText(hTabs[1],IDC_SHADER,cfg->shaderfile,cfgmask->shaderfile,FALSE);
		// texture format
		_tcscpy(buffer,_T("Automatic"));
		SendDlgItemMessage(hTabs[3],IDC_TEXTUREFORMAT,CB_ADDSTRING,0,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[3],IDC_TEXTUREFORMAT,CB_SETCURSEL,cfg->TextureFormat,0);
		// Texture upload
		_tcscpy(buffer,_T("Automatic"));
		SendDlgItemMessage(hTabs[3],IDC_TEXUPLOAD,CB_ADDSTRING,0,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[3],IDC_TEXUPLOAD,CB_SETCURSEL,cfg->TexUpload,0);
		// Default window position
		_tcscpy(buffer, _T("Centered"));
		SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Saved position"));
		SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Top left"));
		SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Windows default"));
		SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[3], IDC_WINDOWPOS, CB_SETCURSEL, cfg->WindowPosition, 0);
		// Remember window position
		if (cfg->RememberWindowPosition) SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWPOS, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWPOS, BM_SETCHECK, BST_UNCHECKED, 0);
		// Remember window size
		if (cfg->RememberWindowSize) SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWSIZE, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[3], IDC_REMEMBERWINDOWSIZE, BM_SETCHECK, BST_UNCHECKED, 0);
		// Remembered window position
		SetInteger(hTabs[3], IDC_WINDOWX, cfg->WindowX, cfgmask->WindowX);
		SetInteger(hTabs[3], IDC_WINDOWY, cfg->WindowY, cfgmask->WindowY);
		// Remembered window size
		SetInteger(hTabs[3], IDC_WINDOWWIDTH, cfg->WindowWidth, cfgmask->WindowWidth);
		SetInteger(hTabs[3], IDC_WINDOWHEIGHT, cfg->WindowHeight, cfgmask->WindowHeight);
		// Remembered maximize state
		if (cfg->WindowMaximized) SendDlgItemMessage(hTabs[3], IDC_WINDOWMAXIMIZED, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[3], IDC_WINDOWMAXIMIZED, BM_SETCHECK, BST_UNCHECKED, 0);
		// No autosize
		if (cfg->NoResizeWindow) SendDlgItemMessage(hTabs[3], IDC_NOAUTOSIZE, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[3], IDC_NOAUTOSIZE, BM_SETCHECK, BST_UNCHECKED, 0);
		// Capture mouse
		if (cfg->CaptureMouse) SendDlgItemMessage(hTabs[3], IDC_CAPTUREMOUSE, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[3], IDC_CAPTUREMOUSE, BM_SETCHECK, BST_UNCHECKED, 0);
		// DPI
		dpisupport = GetDPISupportLevel();
		_tcscpy(buffer, _T("Disabled"));
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_ADDSTRING,0,(LPARAM)buffer);
		if (dpisupport >= 2) _tcscpy(buffer, _T("Per-monitor"));
		else _tcscpy(buffer, _T("Enabled"));
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer, _T("Windows AppCompat"));
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_ADDSTRING,2,(LPARAM)buffer);
		if (dpisupport >= 2)
		{
			_tcscpy(buffer, _T("System"));
			SendDlgItemMessage(hTabs[0], IDC_DPISCALE, CB_ADDSTRING, 2, (LPARAM)buffer);
		}
		if (dpisupport >= 3)
		{
			_tcscpy(buffer, _T("Per-monitor V2"));
			SendDlgItemMessage(hTabs[0], IDC_DPISCALE, CB_ADDSTRING, 2, (LPARAM)buffer);
		}
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_SETCURSEL,cfg->DPIScale,0);
		// Paths
		EnableWindow(GetDlgItem(hTabs[3], IDC_PATHLABEL), FALSE);
		EnableWindow(GetDlgItem(hTabs[3], IDC_PROFILEPATH), FALSE);
		EnableWindow(GetDlgItem(hTabs[3], IDC_WRITEINI), FALSE);
		// Debug
		_tcscpy(buffer, _T("Disable EXT framebuffers"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable ARB framebuffers"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable GLES2 compatibility extension"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable EXT direct state access"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable ARB direct state access"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable sampler objects"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable EXT_gpu_shader4 extension"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable GLSL 1.30 support (DDraw ROP only)"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Upload surface contents on unlock"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable mouse cursor hooks"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Disable draw on palette change"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("DEBUG: Blend destination color key texture with source"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		/*_tcscpy(buffer, _T("Disable OpenGL errors (OpenGL 4.6+)"));
		SendDlgItemMessage(hTabs[4], IDC_DEBUGLIST, LB_ADDSTRING, 0, (LPARAM)buffer);*/
		// Max OpenGL
		_tcscpy(buffer, _T("Maximum Available"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 2.0"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 2.1"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 3.0"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 3.1"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 3.2"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 3.3"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 4.0"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 4.1"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 4.2"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 4.3"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 4.4"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 4.5"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("OpenGL 4.6"));
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[4], IDC_GLVERSION, WM_SETTEXT, 0, (LPARAM)buffer);
		SetGLCombo(hTabs[4], IDC_GLVERSION, &cfg->DebugMaxGLVersionMajor, &cfg->DebugMaxGLVersionMinor,
			&cfgmask->DebugMaxGLVersionMajor, &cfgmask->DebugMaxGLVersionMinor, FALSE, hWnd);
		// Hacks
		_tcscpy(buffer, _T("Crop 640x480 to 640x400"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Automatically expand viewport"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Automatic expand comparison method"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Automatic expand comparison value"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Remove TV-compatible refresh rates"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("SetCursor hide visibility"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Palette draw delay (ms)"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Sync palette update to Vsync"));
		SendDlgItemMessage(hTabs[5], IDC_HACKSLIST, LB_ADDSTRING, 0, (LPARAM)buffer);
		// Auto expand viewport hack value
		SetRGBHex(GetDlgItem(hTabs[5], IDC_HACKSLIST), IDC_HACKSEDIT, cfg->HackAutoExpandViewportValue, cfgmask->HackAutoExpandViewportValue);
		// Tracing
		_tcscpy(buffer, _T("Disabled"));
		SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Errors only"));
		SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Information"));
		SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Full API trace"));
		SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_ADDSTRING, 0, (LPARAM)buffer);
		SendDlgItemMessage(hTabs[6], IDC_TRACING, CB_SETCURSEL, cfg->DebugTraceLevel, 0);
#ifndef _M_X64
		DestroyWindow(GetDlgItem(hTabs[6], IDC_TRACEX64NOTICE));
#endif
		// About text
		_tcscpy(abouttext, _T("DXGL\r\nVersion "));
		_tcscat(abouttext, _T(DXGLVERSTRING));
		_tcscat(abouttext, _T("\r\nCopyright ©2011-"));
		_tcscat(abouttext, _T(COPYYEARSTRING));
		_tcscat(abouttext, _T(" William Feely\r\n\r\n\
This library is free software; you can redistribute it and/or\r\n\
modify it under the terms of the GNU Lesser General Public\r\n\
License as published by the Free Software Foundation; either\r\n\
version 2.1 of the License, or (at your option) any later version.\r\n\
\r\n\
This library is distributed in the hope that it will be useful,\r\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\r\n\
Lesser General Public License for more details.\r\n\
\r\n\
You should have received a copy of the GNU Lesser General Public\r\n\
License along with this library; if not, write to the Free Software\r\n\
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA"));
		SendDlgItemMessage(hTabs[8], IDC_ABOUTTEXT, WM_SETTEXT, 0, (LPARAM)abouttext);
		// Check install path
		installpath = NULL;
		error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKey);
		if (error == ERROR_SUCCESS)
		{
			if (RegQueryValueEx(hKey, installdir, NULL, NULL, NULL, &keysize) == ERROR_SUCCESS)
			{
				installpath = (LPTSTR)malloc(keysize);
				error = RegQueryValueEx(hKey, installdir, NULL, NULL, (LPBYTE)installpath, &keysize);
				if (error != ERROR_SUCCESS)
				{
					free(installpath);
					installpath = NULL;
				}
			}
			RegCloseKey(hKey);
		}
		hKey = NULL;
		// Add installed programs
		current_app = 1;
		appcount = 1;
		regbuffersize = 1024;
		regbuffer = (LPTSTR)malloc(regbuffersize * sizeof(TCHAR));
		RegCreateKeyEx(HKEY_CURRENT_USER, profilespath, 0, NULL, 0, KEY_READ, NULL, &hKeyBase, NULL);
		RegQueryInfoKey(hKeyBase, NULL, NULL, NULL, NULL, &keysize, NULL, NULL, NULL, NULL, NULL, NULL);
		keysize++;
		keyname = (LPTSTR)malloc(keysize * sizeof(TCHAR));
		keysize2 = keysize;
		i = 0;
		while (RegEnumKeyEx(hKeyBase, i, keyname, &keysize2, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			keysize2 = keysize;
			i++;
			appcount++;
			if (appcount > maxapps)
			{
				maxapps += 128;
				apps = (app_setting *)realloc(apps, maxapps * sizeof(app_setting));
			}
			_tcscpy(subkey, keyname);
			if (_tcsrchr(subkey, _T('-'))) *(_tcsrchr(subkey, _T('-'))) = 0;
			error = RegOpenKeyEx(hKeyBase, keyname, 0, KEY_READ, &hKey);
			buffersize = regbuffersize;
			RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, NULL, &buffersize);
			if (buffersize > regbuffersize)
			{
				regbuffersize = buffersize;
				regbuffer = (LPTSTR)realloc(regbuffer, regbuffersize);
			}
			buffersize = regbuffersize;
			regbuffer[0] = 0;
			error = RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (LPBYTE)regbuffer, &buffersize);
			apps[appcount - 1].regkey = (LPTSTR)malloc((_tcslen(keyname) + 1) * sizeof(TCHAR));
			_tcscpy(apps[appcount - 1].regkey, keyname);
			GetConfig(&apps[appcount - 1].cfg, &apps[appcount - 1].mask, keyname);
			apps[appcount - 1].dirty = FALSE;
			if ((regbuffer[0] == 0) || error != ERROR_SUCCESS)
			{
				// Default icon
				apps[appcount - 1].icon = LoadIcon(NULL, IDI_APPLICATION);
				apps[appcount - 1].icon_shared = TRUE;
				apps[appcount - 1].name = (TCHAR*)malloc((_tcslen(subkey) + 1) * sizeof(TCHAR));
				_tcscpy(apps[appcount - 1].name, subkey);
				break;
			}
			path = (LPTSTR)malloc(((_tcslen(regbuffer) + _tcslen(subkey) + 2)) * sizeof(TCHAR));
			_tcscpy(path, regbuffer);
			_tcscpy(apps[appcount - 1].path, path);
			if (installpath)
			{
				if (!_tcsicmp(installpath, path)) apps[appcount - 1].builtin = TRUE;
				else apps[appcount - 1].builtin = FALSE;
			}
			_tcscat(path, _T("\\"));
			_tcscat(path, subkey);
			if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
			{
				// Default icon
				apps[appcount - 1].icon = LoadIcon(NULL, IDI_APPLICATION);
				apps[appcount - 1].icon_shared = TRUE;
				apps[appcount - 1].name = (TCHAR*)malloc((_tcslen(subkey) + 1) * sizeof(TCHAR));
				_tcscpy(apps[appcount - 1].name, subkey);
				break;
			}
			// Get exe attributes
			error = (LONG)SHGetFileInfo(path, 0, &fileinfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_ADDOVERLAYS);
			apps[appcount - 1].icon = fileinfo.hIcon;
			apps[appcount - 1].icon_shared = FALSE;
			verinfosize = GetFileVersionInfoSize(path, NULL);
			verinfo = malloc(verinfosize);
			hasname = FALSE;
			if (GetFileVersionInfo(path, 0, verinfosize, verinfo))
			{
				if (VerQueryValue(verinfo, _T("\\VarFileInfo\\Translation"), (LPVOID*)&outbuffer, &outlen))
				{
					memcpy(translation, outbuffer, 4);
					_sntprintf(verpath, 64, _T("\\StringFileInfo\\%04x%04x\\FileDescription"), translation[0], translation[1]);
					if (VerQueryValue(verinfo, verpath, (LPVOID*)&outbuffer, &outlen))
					{
						hasname = TRUE;
						apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(outbuffer) + 1) * sizeof(TCHAR));
						_tcscpy(apps[appcount - 1].name, outbuffer);
					}
					else
					{
						_sntprintf(verpath, 64, _T("\\StringFileInfo\\%04x%04x\\ProductName"), ((WORD*)outbuffer)[0], ((WORD*)outbuffer)[1]);
						if (VerQueryValue(verinfo, verpath, (LPVOID*)&outbuffer, &outlen))
						{
							hasname = TRUE;
							apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(outbuffer) + 1) * sizeof(TCHAR));
							_tcscpy(apps[appcount - 1].name, outbuffer);
						}
						else
						{
							_sntprintf(verpath, 64, _T("\\StringFileInfo\\%04x%04x\\InternalName"), ((WORD*)outbuffer)[0], ((WORD*)outbuffer)[1]);
							if (VerQueryValue(verinfo, verpath, (LPVOID*)&outbuffer, &outlen))
							{
								hasname = TRUE;
								apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(outbuffer) + 1) * sizeof(TCHAR));
								_tcscpy(apps[appcount - 1].name, outbuffer);
							}
						}
					}
				}
			}
			free(path);
			if (!hasname)
			{
				apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(subkey) + 1) * sizeof(TCHAR));
				_tcscpy(apps[appcount - 1].name, subkey);
			}
			free(verinfo);
			RegCloseKey(hKey);
		}
		RegCloseKey(hKeyBase);
		free(keyname);
		for(i = 0; i < appcount; i++)
		{
			SendDlgItemMessage(hWnd,IDC_APPS,CB_ADDSTRING,0,(LPARAM)apps[i].name);
		}
		current_app = 0;
		SendDlgItemMessage(hWnd,IDC_APPS,CB_SETCURSEL,0,0);
		GetWindowRect(GetDlgItem(hWnd, IDC_APPS), &r);
		SetWindowPos(GetDlgItem(hWnd, IDC_APPS), HWND_TOP, r.left, r.top, r.right - r.left,
			(r.bottom - r.top) + (16 * (GetSystemMetrics(SM_CYSMICON) + 1)+(2*GetSystemMetrics(SM_CYBORDER))),
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
		if(osver.dwMajorVersion >= 6)
		{
			if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))
			{
				if(GetTokenInformation(token,(TOKEN_INFORMATION_CLASS)20,&elevation,sizeof(DWORD),(PDWORD)&outlen))
				{
					if(!elevation)
					{
						SendDlgItemMessage(hWnd,IDC_ADD,BCM_SETSHIELD,0,TRUE);
						SendDlgItemMessage(hWnd,IDC_REMOVE,BCM_SETSHIELD,0,TRUE);
					}
				}
			}
		}
		else
		{
			EnableWindow(GetDlgItem(hTabs[0], IDC_DPISCALE), FALSE);
		}
		if(token) CloseHandle(token);
		/*SendMessage(hProgressWnd, WM_USER+1, 0, 0);
		SetForegroundWindow(hWnd);*/
		return TRUE;
	case WM_MEASUREITEM:
		switch(wParam)
		{
		case IDC_APPS:
			((LPMEASUREITEMSTRUCT)lParam)->itemHeight = GetSystemMetrics(SM_CYSMICON) + 1;
			((LPMEASUREITEMSTRUCT)lParam)->itemWidth = GetSystemMetrics(SM_CXSMICON)+1;
			break;
		default:
			break;
		}
		break;
	case WM_NOTIFY:
		nm = (LPNMHDR)lParam;
		if (nm->code == TCN_SELCHANGE)
		{
			newtab = (int)SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETCURSEL, 0, 0);
			if (newtab != tabopen)
			{
				ShowWindow(hTabs[tabopen], SW_HIDE);
				tabopen = newtab;
				SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETITEMRECT, 0, (LPARAM)&r);
				SetWindowPos(hTabs[tabopen], NULL, r.left, r.bottom + 3, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
			}
		}
		break;
	case WM_DRAWITEM:
		drawitem = (DRAWITEMSTRUCT*)lParam;
		switch (wParam)
		{
		case IDC_APPS:
			OldTextColor = GetTextColor(drawitem->hDC);
			OldBackColor = GetBkColor(drawitem->hDC);
			if((drawitem->itemState & ODS_SELECTED))
			{
				SetTextColor(drawitem->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(drawitem->hDC,GetSysColor(COLOR_HIGHLIGHT));
				FillRect(drawitem->hDC,&drawitem->rcItem,(HBRUSH)(COLOR_HIGHLIGHT+1));
			}
			else
			{
				SetTextColor(drawitem->hDC, GetSysColor(COLOR_WINDOWTEXT));
				SetBkColor(drawitem->hDC, GetSysColor(COLOR_WINDOW));
				FillRect(drawitem->hDC, &drawitem->rcItem, (HBRUSH)(COLOR_WINDOW + 1));
			}
			DrawIconEx(drawitem->hDC,drawitem->rcItem.left+2,drawitem->rcItem.top,
				apps[drawitem->itemID].icon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
			drawitem->rcItem.left += GetSystemMetrics(SM_CXSMICON)+5;
			DrawText(drawitem->hDC,apps[drawitem->itemID].name,
				(int)_tcslen(apps[drawitem->itemID].name),&drawitem->rcItem,
				DT_LEFT|DT_SINGLELINE|DT_VCENTER);
			drawitem->rcItem.left -= GetSystemMetrics(SM_CXSMICON)+5;
			if (drawitem->itemState & ODS_FOCUS) DrawFocusRect(drawitem->hDC, &drawitem->rcItem);
			SetTextColor(drawitem->hDC,OldTextColor);
			SetBkColor(drawitem->hDC,OldBackColor);
			DefWindowProc(hWnd,Msg,wParam,lParam);
			break;
		default:
			break;
		}
		break;
	case WM_HELP:
		HtmlHelp(hWnd,hlppath,HH_DISPLAY_TOPIC,(DWORD_PTR)_T("html/configuration.html"));
		return TRUE;
		break;
	case WM_SYSCOMMAND:
		if(LOWORD(wParam) == SC_CONTEXTHELP)
		{
			HtmlHelp(hWnd,hlppath,HH_DISPLAY_TOPIC,(DWORD_PTR)_T("html/configuration.html"));
			return TRUE;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			SaveChanges(hWnd);
			EndDialog(hWnd,IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd,IDCANCEL);
			return TRUE;
		case IDC_APPLY:
			SaveChanges(hWnd);
			if(modelistdirty) ResetModeList(hTabs[7]);
			return TRUE;
		case IDC_APPS:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				cursel = (DWORD)SendDlgItemMessage(hWnd,IDC_APPS,CB_GETCURSEL,0,0);
				if(cursel == current_app) break;
				current_app = cursel;
				cfg = &apps[current_app].cfg;
				cfgmask = &apps[current_app].mask;
				dirty = &apps[current_app].dirty;
				RefreshControls(hWnd);
			}
			break;
		case IDC_ADD:
			selectedfile[0] = 0;
			ZeroMemory(&filename, OPENFILENAME_SIZE_VERSION_400);
			filename.lStructSize = OPENFILENAME_SIZE_VERSION_400;
			filename.hwndOwner = hWnd;
			filename.lpstrFilter = exe_filter;
			filename.lpstrFile = selectedfile;
			filename.nMaxFile = MAX_PATH + 1;
			filename.lpstrInitialDir = _T("%ProgramFiles%");
			filename.lpstrTitle = _T("Select program");
			filename.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if (GetOpenFileName(&filename))
			{
				if (CheckProfileExists(filename.lpstrFile))
				{
					MessageBox(hWnd, _T("A profile already exists for this program."),
						_T("Profile already exists"), MB_OK | MB_ICONWARNING);
					break;
				}
				err = AddApp(filename.lpstrFile, TRUE, FALSE, FALSE, hWnd);
				if (!err)
				{
					LPTSTR newkey = MakeNewConfig(filename.lpstrFile);
					LPTSTR newkey2 = (LPTSTR)malloc((_tcslen(newkey) + 24) * sizeof(TCHAR));
					_tcscpy(newkey2, profilespath2);
					_tcscat(newkey2, newkey);
					appcount++;
					if (appcount > maxapps)
					{
						maxapps += 128;
						apps = (app_setting *)realloc(apps, maxapps * sizeof(app_setting));
					}
					RegOpenKeyEx(HKEY_CURRENT_USER, newkey2, 0, KEY_READ, &hKey);
					RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, NULL, &buffersize);
					regbuffer = (LPTSTR)malloc(buffersize);
					regbuffer[0] = 0;
					error = RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (LPBYTE)regbuffer, &buffersize);
					apps[appcount - 1].regkey = (LPTSTR)malloc((_tcslen(newkey) + 1) * sizeof(TCHAR));
					_tcscpy(apps[appcount - 1].regkey, newkey);
					GetConfig(&apps[appcount - 1].cfg, &apps[appcount - 1].mask, newkey);
					apps[appcount - 1].dirty = FALSE;
					free(newkey2);
					if ((regbuffer[0] == 0) || error != ERROR_SUCCESS)
					{
						// Default icon
						apps[appcount - 1].icon = LoadIcon(NULL, IDI_APPLICATION);
						apps[appcount - 1].icon_shared = TRUE;
						apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(newkey) + 1) * sizeof(TCHAR));
						_tcscpy(apps[appcount - 1].name, newkey);
						break;
					}
					if (_tcsrchr(newkey, _T('-'))) *(_tcsrchr(newkey, _T('-'))) = 0;
					path = (LPTSTR)malloc(((_tcslen(regbuffer) + _tcslen(newkey) + 2)) * sizeof(TCHAR));
					_tcscpy(path, regbuffer);
					_tcscpy(apps[appcount - 1].path, path);
					_tcscat(path, _T("\\"));
					_tcscat(path, newkey);
					if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
					{
						// Default icon
						apps[appcount - 1].icon = LoadIcon(NULL, IDI_APPLICATION);
						apps[appcount - 1].icon_shared = TRUE;
						apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(newkey) + 1) * sizeof(TCHAR));
						_tcscpy(apps[appcount - 1].name, newkey);
						break;
					}
					else
					{
						// Get exe attributes
						error = (LONG)SHGetFileInfo(path, 0, &fileinfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_ADDOVERLAYS);
						apps[appcount - 1].icon = fileinfo.hIcon;
						apps[appcount - 1].icon_shared = FALSE;
						verinfosize = GetFileVersionInfoSize(path, NULL);
						verinfo = malloc(verinfosize);
						hasname = FALSE;
						if (GetFileVersionInfo(path, 0, verinfosize, verinfo))
						{
							if (VerQueryValue(verinfo, _T("\\VarFileInfo\\Translation"), (LPVOID*)&outbuffer, &outlen))
							{
								memcpy(translation, outbuffer, 4);
								_sntprintf(verpath, 64, _T("\\StringFileInfo\\%04x%04x\\FileDescription"), translation[0], translation[1]);
								if (VerQueryValue(verinfo, verpath, (LPVOID*)&outbuffer, &outlen))
								{
									hasname = TRUE;
									apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(outbuffer) + 1) * sizeof(TCHAR));
									_tcscpy(apps[appcount - 1].name, outbuffer);
								}
								else
								{
									_sntprintf(verpath, 64, _T("\\StringFileInfo\\%04x%04x\\ProductName"), ((WORD*)outbuffer)[0], ((WORD*)outbuffer)[1]);
									if (VerQueryValue(verinfo, verpath, (LPVOID*)&outbuffer, &outlen))
									{
										hasname = TRUE;
										apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(outbuffer) + 1) * sizeof(TCHAR));
										_tcscpy(apps[appcount - 1].name, outbuffer);
									}
									else
									{
										_sntprintf(verpath, 64, _T("\\StringFileInfo\\%04x%04x\\InternalName"), ((WORD*)outbuffer)[0], ((WORD*)outbuffer)[1]);
										if (VerQueryValue(verinfo, verpath, (LPVOID*)&outbuffer, &outlen))
										{
											hasname = TRUE;
											apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(outbuffer) + 1) * sizeof(TCHAR));
											_tcscpy(apps[appcount - 1].name, outbuffer);
										}
									}
								}
							}
						}
						if (!hasname)
						{
							apps[appcount - 1].name = (LPTSTR)malloc((_tcslen(newkey) + 1) * sizeof(TCHAR));
							_tcscpy(apps[appcount - 1].name, newkey);
						}
						free(verinfo);
						free(path);
					}
					SendDlgItemMessage(hWnd, IDC_APPS, CB_SETCURSEL,
						SendDlgItemMessage(hWnd, IDC_APPS, CB_ADDSTRING, 0, (LPARAM)apps[appcount - 1].name), 0);
					SendMessage(hWnd, WM_COMMAND, IDC_APPS + 0x10000, 0);
					RegCloseKey(hKey);
					free(regbuffer);
				}
			}
			break;
		case IDC_REMOVE:
			if(MessageBox(hWnd,_T("Do you want to delete the selected application profile and remove DXGL from its installation folder(s)?"),
				_T("Confirmation"),MB_YESNO|MB_ICONQUESTION) != IDYES) return FALSE;
			regpath = (LPTSTR)malloc((_tcslen(apps[current_app].regkey) + 24)*sizeof(TCHAR));
			_tcscpy(regpath, profilespath2);
			_tcscat(regpath, apps[current_app].regkey);
			regkey = (LPTSTR)malloc((_tcslen(apps[current_app].regkey) + 1) * sizeof(TCHAR));
			_tcscpy(regkey, apps[current_app].regkey);
			RegOpenKeyEx(HKEY_CURRENT_USER,regpath,0,KEY_READ,&hKey);
			RegQueryValueEx(hKey,_T("InstallPath"),NULL,NULL,NULL,&buffersize);
			regbuffer = (LPTSTR)malloc(buffersize);
			regbuffer[0] = 0;
			failed = FALSE;
			error = RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (LPBYTE)regbuffer, &buffersize);
			path = (LPTSTR)malloc(((_tcslen(regbuffer) + 12)) * sizeof(TCHAR));
			_tcscpy(path, regbuffer);
			_tcscat(path, _T("\\ddraw.dll"));
			if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
			{
				if (DelApp(path, FALSE, hWnd)) failed = TRUE;
			}
			free(path);
			free(regbuffer);
			RegCloseKey(hKey);
			if(!failed)
			{
				RegDeleteKey(HKEY_CURRENT_USER,regpath);
				if(!apps[current_app].icon_shared) DeleteObject(apps[current_app].icon);
				if(apps[current_app].name) free(apps[current_app].name);
				if(apps[current_app].regkey) free(apps[current_app].regkey);
				for(i = current_app; i < appcount; i++)
				{
					apps[i] = apps[i+1];
				}
				appcount--;
			}
			SendDlgItemMessage(hWnd, IDC_APPS, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hWnd,IDC_APPS,CB_DELETESTRING,current_app,0);
			SendMessage(hWnd, WM_COMMAND, IDC_APPS + 0x10000, 0);
			break;
		case IDC_RESTOREDEFAULTS:
			GetDefaultConfig(&apps[current_app].cfg);
			if(current_app) ZeroMemory(&apps[current_app].mask, sizeof(DXGLCFG));
			EnableWindow(GetDlgItem(hDialog, IDC_APPLY), TRUE);
			*dirty = TRUE;
			RefreshControls(hWnd);
			break;
		}
		break;
	}
	return FALSE;
}

void UpgradeDXGL()
{
	HKEY hKeyBase;
	HKEY hKey;
	DWORD keysize, keysize2;
	int i = 0;
	LONG error;
	LPTSTR keyname;
	DWORD sizeout;
	DWORD buffersize;
	DWORD regbuffersize;
	LPTSTR regbuffer;
	BOOL installed = FALSE;
	BOOL dxgl_installdir = FALSE;
	BOOL old_dxgl = FALSE;
	HKEY hKeyInstall;
	TCHAR installpath[MAX_PATH + 1];
	TCHAR srcpath[MAX_PATH + 1];
	TCHAR destpath[MAX_PATH + 1];
	TCHAR inipath[MAX_PATH + 1];
	TCHAR backuppath[MAX_PATH + 1];
	app_ini_options inioptions;
	HMODULE hmod;
	UpgradeConfig();
	regbuffersize = 1024;
	regbuffer = (LPTSTR)malloc(regbuffersize * sizeof(TCHAR));
	RegCreateKeyEx(HKEY_CURRENT_USER, profilespath, 0, NULL, 0, KEY_READ, NULL, &hKeyBase, NULL);
	RegQueryInfoKey(hKeyBase, NULL, NULL, NULL, NULL, &keysize, NULL, NULL, NULL, NULL, NULL, NULL);
	keysize++;
	keyname = (LPTSTR)malloc(keysize * sizeof(TCHAR));
	keysize2 = keysize;
	error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
	if (error == ERROR_SUCCESS)
	{
		dxgl_installdir = TRUE;
		sizeout = (MAX_PATH + 1) * sizeof(TCHAR);
		error = RegQueryValueEx(hKeyInstall, installdir, NULL, NULL, (LPBYTE)installpath, &sizeout);
		if (error == ERROR_SUCCESS) installed = TRUE;
	}
	if (hKeyInstall) RegCloseKey(hKeyInstall);
	if (!installed)
	{
		GetModuleFileName(NULL, installpath, MAX_PATH + 1);
	}
	if (dxgl_installdir) _tcscat(installpath, _T("\\"));
	else (_tcsrchr(installpath, _T('\\')))[1] = 0;
	_tcsncpy(srcpath, installpath, MAX_PATH + 1);
	_tcscat(srcpath, _T("ddraw.dll"));
	while (RegEnumKeyEx(hKeyBase, i, keyname, &keysize2, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		keysize2 = keysize;
		i++;
		error = RegOpenKeyEx(hKeyBase, keyname, 0, KEY_READ, &hKey);
		buffersize = regbuffersize;
		RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, NULL, &buffersize);
		if (buffersize > regbuffersize)
		{
			regbuffersize = buffersize;
			regbuffer = (LPTSTR)realloc(regbuffer, regbuffersize);
		}
		buffersize = regbuffersize;
		regbuffer[0] = 0;
		error = RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (LPBYTE)regbuffer, &buffersize);
		if (regbuffer[0] != 0)
		{
			_tcsncpy(destpath, regbuffer, MAX_PATH + 1);
			_tcscat(destpath, _T("\\ddraw.dll"));
			_tcsncpy(inipath, regbuffer, MAX_PATH + 1);
			_tcscat(inipath, _T("\\"));
			_tcsncpy(backuppath, regbuffer, MAX_PATH + 1);
			_tcscat(backuppath, _T("\\ddraw.dll.dxgl-backup"));
			ReadAppINIOptions(inipath, &inioptions);
			error = CopyFile(srcpath, destpath, TRUE);
			if (!error)
			{
				error = GetLastError();
				if (error == ERROR_FILE_EXISTS)
				{
					if (inioptions.NoOverwrite) continue;
					if ((inioptions.sha256[0] != 0) && !memcmp(inioptions.sha256, inioptions.sha256comp, 64))
						// Detected original ddraw matches INI hash
						CopyFile(srcpath, backuppath, FALSE);
					old_dxgl = FALSE;
					error = SetErrorMode(SEM_FAILCRITICALERRORS);
					SetErrorMode(error | SEM_FAILCRITICALERRORS);
					hmod = LoadLibrary(destpath);
					SetErrorMode(error);
					if (hmod)
					{
						if (GetProcAddress(hmod, "IsDXGLDDraw")) old_dxgl = TRUE;
						FreeLibrary(hmod);
					}
					if (old_dxgl) CopyFile(srcpath, destpath, FALSE);
				}
			}
		}
		RegCloseKey(hKey);
	}
	free(regbuffer);
	free(keyname);
	RegCloseKey(hKeyBase);
}

// '0' for keep, '1' for remove, personal settings
void UninstallDXGL(TCHAR uninstall)
{
	HKEY hKeyBase;
	HKEY hKey;
	DWORD keysize, keysize2;
	LONG error;
	LPTSTR keyname;
	DWORD sizeout;
	DWORD buffersize;
	DWORD regbuffersize;
	LPTSTR regbuffer;
	BOOL installed = FALSE;
	BOOL dxgl_installdir = FALSE;
	BOOL old_dxgl = FALSE;
	HKEY hKeyInstall;
	TCHAR installpath[MAX_PATH + 1];
	TCHAR srcpath[MAX_PATH + 1];
	TCHAR destpath[MAX_PATH + 1];
	TCHAR inipath[MAX_PATH + 1];
	TCHAR backuppath[MAX_PATH + 1];
	HANDLE exists;
	app_ini_options inioptions;
	HMODULE hmod;
	int i = 0;
	UpgradeConfig();  // Just to make sure the registry format is correct
	regbuffersize = 1024;
	regbuffer = (LPTSTR)malloc(regbuffersize * sizeof(TCHAR));
	error = RegOpenKeyEx(HKEY_CURRENT_USER, profilespath, 0, KEY_ALL_ACCESS, &hKeyBase);
	if (error != ERROR_SUCCESS) return;
	RegQueryInfoKey(hKeyBase, NULL, NULL, NULL, NULL, &keysize, NULL, NULL, NULL, NULL, NULL, NULL);
	keysize++;
	keyname = (LPTSTR)malloc(keysize * sizeof(TCHAR));
	keysize2 = keysize;
	error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
	if (error == ERROR_SUCCESS)
	{
		dxgl_installdir = TRUE;
		sizeout = (MAX_PATH + 1) * sizeof(TCHAR);
		error = RegQueryValueEx(hKeyInstall, installdir, NULL, NULL, (LPBYTE)installpath, &sizeout);
		if (error == ERROR_SUCCESS) installed = TRUE;
	}
	if (hKeyInstall) RegCloseKey(hKeyInstall);
	if (!installed)
	{
		GetModuleFileName(NULL, installpath, MAX_PATH + 1);
	}
	if (dxgl_installdir) _tcscat(installpath, _T("\\"));
	else (_tcsrchr(installpath, _T('\\')))[1] = 0;
	_tcsncpy(srcpath, installpath, MAX_PATH + 1);
	_tcscat(srcpath, _T("ddraw.dll"));
	while (RegEnumKeyEx(hKeyBase, i, keyname, &keysize2, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		keysize2 = keysize;
		i++;
		error = RegOpenKeyEx(hKeyBase, keyname, 0, KEY_READ, &hKey);
		buffersize = regbuffersize;
		if (buffersize > regbuffersize)
		{
			regbuffersize = buffersize;
			regbuffer = (LPTSTR)realloc(regbuffer, regbuffersize);
		}
		buffersize = regbuffersize;
		regbuffer[0] = 0;
		error = RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (LPBYTE)regbuffer, &buffersize);
		if (regbuffer[0] != 0)
		{
			_tcsncpy(destpath, regbuffer, MAX_PATH + 1);
			_tcscat(destpath, _T("\\ddraw.dll"));
			_tcsncpy(inipath, regbuffer, MAX_PATH + 1);
			_tcscat(inipath, _T("\\"));
			_tcsncpy(backuppath, regbuffer, MAX_PATH + 1);
			_tcscat(backuppath, _T("\\ddraw.dll.dxgl-backup"));
			ReadAppINIOptions(inipath, &inioptions);
			if (inioptions.NoOverwrite || inioptions.NoUninstall) continue;
			if (GetFileAttributes(destpath) != INVALID_FILE_ATTRIBUTES)
			{
				old_dxgl = FALSE;
				error = SetErrorMode(SEM_FAILCRITICALERRORS);
				SetErrorMode(error | SEM_FAILCRITICALERRORS);
				hmod = LoadLibrary(destpath);
				SetErrorMode(error);
				if (hmod)
				{
					if (GetProcAddress(hmod, "IsDXGLDDraw")) old_dxgl = TRUE;
					FreeLibrary(hmod);
				}
				if (_tcscmp(srcpath, destpath))
				{
					if (old_dxgl)
					{
						DeleteFile(destpath);
						exists = CreateFile(backuppath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (exists == INVALID_HANDLE_VALUE) continue;
						else
						{
							CloseHandle(exists);
							MoveFile(backuppath, destpath);
						}
					}
				}
			}
		}
		RegCloseKey(hKey);
	}
	free(regbuffer);
	RegQueryInfoKey(hKeyBase, NULL, NULL, NULL, NULL, &keysize, NULL, NULL, NULL, NULL, NULL, NULL);
	keysize++;
	if (uninstall == '1')  // Delete user settings
	{
		while (RegEnumKeyEx(hKeyBase, 0, keyname, &keysize2, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			keysize2 = keysize;
			i++;
			RegDeleteKey(hKeyBase, keyname);
		}
		RegCloseKey(hKeyBase);
		#ifdef _M_X64
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL\\Profiles_x64"));
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL\\Global_x64"));
		#else
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL\\Profiles"));
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL\\Global"));
		#endif
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL"));
	}
	else RegCloseKey(hKeyBase);
}

#ifdef __GNUC__
#ifndef INITCOMMONCONTROLSEX
typedef struct tagINITCOMMONCONTROLSEX {
	DWORD dwSize;
	DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;
#endif
#endif

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR    lpCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX icc;
	HMODULE comctl32;
	HMODULE msimg32 = NULL;
	BOOL(WINAPI *iccex)(LPINITCOMMONCONTROLSEX lpInitCtrls);
	HANDLE hMutex;
	HWND hWnd;
	BOOL(WINAPI *InitPropertyGrid)() = NULL;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	dxglcfgdll = LoadLibrary(_T("dxglcfg.dll"));
	SetErrorMode(0);
	if (dxglcfgdll)
	{
		InitPropertyGrid = (BOOL(WINAPI*)())GetProcAddress(dxglcfgdll, "InitPropertyGrid");
		if (InitPropertyGrid)
		{
			if (!InitPropertyGrid())
			{
				FreeLibrary(dxglcfgdll);
				dxglcfgdll = NULL;
			}
		}
		else
		{
			FreeLibrary(dxglcfgdll);
			dxglcfgdll = NULL;
		}
	}
	else
	{

	}
	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osver);
	if (osver.dwMajorVersion > 4) gradientavailable = true;
	else if (osver.dwMajorVersion >= 4 && osver.dwMinorVersion >= 1) gradientavailable = true;
	else gradientavailable = false;
	if (gradientavailable)
	{
		msimg32 = LoadLibrary(_T("msimg32.dll"));
		if (!msimg32) gradientavailable = false;
		if (gradientavailable) _GradientFill =
			(BOOL(_stdcall*)(HDC, TRIVERTEX*, ULONG, void*, ULONG, DWORD))
			GetProcAddress(msimg32, "GradientFill");
		if (!_GradientFill)
		{
			if (msimg32)FreeLibrary(msimg32);
			msimg32 = NULL;
			gradientavailable = false;
		}
	}
	CoInitialize(NULL);
	if (!_tcsnicmp(lpCmdLine, _T("upgrade"), 7))
	{
		UpgradeDXGL();
		return 0;
	}
	if (!_tcsnicmp(lpCmdLine, _T("uninstall"), 9))
	{
		UninstallDXGL(lpCmdLine[10]);
		return 0;
	}
	if(!_tcsnicmp(lpCmdLine,_T("install "),8))
	{
		return AddApp(lpCmdLine+8,TRUE,TRUE,FALSE,NULL);
	}
	if(!_tcsnicmp(lpCmdLine,_T("forceinstall "),13))
	{
		return AddApp(lpCmdLine+8,TRUE,TRUE,TRUE,NULL);
	}
	if(!_tcsnicmp(lpCmdLine,_T("remove "),7))
	{
		return DelApp(lpCmdLine+7,TRUE,NULL);
	}
	if (!_tcsnicmp(lpCmdLine, _T("profile_install"), 15))
	{
		// FIXME:  Remove DXGL Config profile
		UpgradeDXGLTestToDXGLCfg();
		LPDIRECTDRAW lpdd;
		DirectDrawCreate(NULL, &lpdd, NULL);
		IDirectDraw_Release(lpdd);
		return 0;
	}
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	comctl32 = LoadLibrary(_T("comctl32.dll"));
	if (comctl32) iccex = (BOOL (WINAPI *)(LPINITCOMMONCONTROLSEX))GetProcAddress(comctl32,"InitCommonControlsEx");
	if(iccex) iccex(&icc);
	else InitCommonControls();
	hinstance = hInstance;
	GetModuleFileName(NULL,hlppath,MAX_PATH);
	GetDirFromPath(hlppath);
	_tcscat(hlppath,_T("\\dxgl.chm"));
	#ifdef _M_X64
	hMutex = CreateMutex(NULL, TRUE, _T("DXGLConfigMutex_x64"));
	#else
	hMutex = CreateMutex(NULL, TRUE, _T("DXGLConfigMutex"));
	#endif
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// Find DXGL Config window
		hWnd = FindWindow(NULL, dxglcfgname);
		// Focus DXGL Config window
		if (hWnd) SetForegroundWindow(hWnd);
		return 0;
	}
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_DXGLCFG),0,(DLGPROC)DXGLCfgCallback);
	if (comctl32) FreeLibrary(comctl32);
	if (msimg32) FreeLibrary(msimg32);
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
