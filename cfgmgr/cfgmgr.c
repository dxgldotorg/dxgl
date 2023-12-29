// DXGL
// Copyright (C) 2011-2023 William Feely
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

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <ShellAPI.h>
#include "crc32.h"
#include "LibSha256.h"
#include "cfgmgr.h"
#include "../ddraw/resource.h"
#include <tchar.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>
#include "inih/ini.h"

#ifndef GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 4
#endif
#ifndef LSTATUS
typedef LONG LSTATUS;
#endif

#ifdef _M_X64
static const TCHAR regkeyglobal[] = _T("Software\\DXGL\\Global_x64");
static const TCHAR regkeyprofiles[] = _T("Software\\DXGL\\Profiles_x64\\");
static const TCHAR globalname[] = _T("Global");
static const TCHAR profilesname[] = _T("Profiles_x64\\");
static const TCHAR configversion[] = _T("Configuration Version x64");
static const TCHAR regkeyprofilesmigrated[] = _T("Software\\DXGL\\ProfilesMigrated_x64\\");
#else
static const TCHAR regkeyglobal[] = _T("Software\\DXGL\\Global");
static const TCHAR regkeyprofiles[] = _T("Software\\DXGL\\Profiles\\");
static const TCHAR globalname[] = _T("Global");
static const TCHAR profilesname[] = _T("Profiles\\");
static const TCHAR configversion[] = _T("Configuration Version");
static const TCHAR regkeyprofilesmigrated[] = _T("Software\\DXGL\\ProfilesMigrated\\");
#endif
static const TCHAR regkeybase[] = _T("Software\\DXGL\\");
static const TCHAR regkeydxgl[] = _T("Software\\DXGL");

DXGLCFG defaultmask;

#define INISECTION_NULL 0
#define INISECTION_SYSTEM 1
#define INISECTION_DISPLAY 2
#define INISECTION_SCALING 3
#define INISECTION_POSTPROCESS 4
#define INISECTION_D3D 5
#define INISECTION_ADVANCED 6
#define INISECTION_DEBUG 7
#define INISECTION_HACKS 8

static int ini_currentsection = 0;
static int ini_depth = 0;

void _tchartowchar(WCHAR *dest, TCHAR *src, int length)
{
#ifdef _UNICODE
	if (length == -1) wcscpy(dest, src);
	else wcsncpy(dest,src,length);
#else
	int length2 = length;
	if(length == -1) length2 = (int)strlen(src) + 1;
	MultiByteToWideChar(CP_ACP,0,src,length,dest,length2);
#endif
}

void _wchartotchar(TCHAR *dest, const WCHAR *src, int length)
{
#ifdef _UNICODE
	if (length == -1) wcscpy(dest, src);
	else wcsncpy(dest, src, length);
#else
	int length2 = length;
	if(length == -1) length2 = (int)wcslen(src) + 1;
	WideCharToMultiByte(CP_ACP,0,src,length,dest,length2,NULL,NULL);
#endif
}

void utf8to16(WCHAR* dest, const CHAR* src)
{
	int sizein;
	int sizeout;
	sizein = strlen(src)+1;
	sizeout = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, src, sizein, NULL, 0);
	MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, src, sizein, dest, sizeout);
}

/**
* Gets the hexadecimal digit for a number; the number must be less than 16
* or 0x10.
* @param c
*  Number from 0 to 15 or 0x0 to 0xF
* @return
*  A character representing a hexidecimal digit, letters are uppercase.
*/
static unsigned char hexdigit(unsigned char c)
{
	if (c < 10) return c + '0';
	else return (c + 'A' - 10);
}

INT_PTR CALLBACK CompatDialogCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		SetTimer(hWnd, 0, 3000, NULL);
		return TRUE;
	case WM_TIMER:
		EndDialog(hWnd, 0);
		break;
	default:
		return FALSE;
	}
	return FALSE;
}

void ShowRestartDialog()
{
	LPTSTR cmdline;
	LPTSTR cmdline2;
	STARTUPINFO startupinfo;
	PROCESS_INFORMATION procinfo;
	BOOL(WINAPI *_GetModuleHandleEx)(DWORD dwFlags, LPCTSTR lpModuleName, HMODULE* phModule) = FALSE;
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	HMODULE hddraw;
	if (hKernel32)
	{
#ifdef _UNICODE
		_GetModuleHandleEx = (BOOL(WINAPI*)(DWORD, LPCTSTR, HMODULE*))GetProcAddress(hKernel32, "GetModuleHandleExW");
#else
		_GetModuleHandleEx = (BOOL(WINAPI*)(DWORD, LPCTSTR, HMODULE*))GetProcAddress(hKernel32, "GetModuleHandleExA");
#endif
	}
	if (_GetModuleHandleEx) _GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)ShowRestartDialog, &hddraw);
	else hddraw = GetModuleHandle(_T("ddraw.dll"));  //For old versions of Windows, may fail but they shouldn't have AppCompat anyways.
	if (hKernel32) FreeLibrary(hKernel32);
	DialogBox(hddraw, MAKEINTRESOURCE(IDD_COMPAT), NULL, CompatDialogCallback);
	cmdline = GetCommandLine();
	cmdline2 = (LPTSTR)malloc((_tcslen(cmdline) + 1)*sizeof(TCHAR));
	if (!cmdline2)
	{
		MessageBox(NULL, _T("Fatal error restarting application"), _T("Out of memory"), MB_OK | MB_ICONSTOP);
		ExitProcess(ERROR_NOT_ENOUGH_MEMORY);
	}
	_tcscpy(cmdline2, cmdline);
	ZeroMemory(&startupinfo, sizeof(STARTUPINFO));
	startupinfo.cb = sizeof(STARTUPINFO);
	CreateProcess(NULL, cmdline2, NULL, NULL, FALSE, 0, NULL, NULL, &startupinfo, &procinfo);
	CloseHandle(procinfo.hProcess);
	CloseHandle(procinfo.hThread);
	free(cmdline2);
	ExitProcess(0);
}

BOOL AddCompatFlag(LPTSTR flag)
{
	BOOL is64 = FALSE;
	HKEY hKey;
	LRESULT error, error2;
	TCHAR filename[MAX_PATH + 1];
	TCHAR buffer[1024];
	DWORD sizeout;
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	BOOL(WINAPI *iswow64)(HANDLE, PBOOL) = NULL;
	if (hKernel32) iswow64 = (BOOL(WINAPI*)(HANDLE, PBOOL))GetProcAddress(hKernel32, "IsWow64Process");
	if (iswow64) iswow64(GetCurrentProcess(), &is64);
	if (hKernel32) FreeLibrary(hKernel32);
	error = RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"),
		0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (error == ERROR_SUCCESS)
	{
		GetModuleFileName(NULL, filename, MAX_PATH);
		ZeroMemory(buffer, 1024 * sizeof(TCHAR));
		sizeout = 1024 * sizeof(TCHAR);
		error2 = RegQueryValueEx(hKey, filename, NULL, NULL, (LPBYTE)buffer, &sizeout);
		if (error2 == ERROR_SUCCESS)
		{
			if (_tcsstr(buffer,flag))
			{ 
				RegCloseKey(hKey);
				return TRUE;
			}
			else
			{
				_tcscat(buffer, _T(" "));
				_tcscat(buffer, flag);
				error2 = RegSetValueEx(hKey, filename, 0, REG_SZ, (BYTE*)buffer, (DWORD)(_tcslen(buffer) + 1)*sizeof(TCHAR));
				if (error2 == ERROR_SUCCESS) ShowRestartDialog();
				else
				{
					RegCloseKey(hKey);
					return FALSE;
				}
			}
		}
		else if (error2 == ERROR_FILE_NOT_FOUND)
		{
			error2 = RegSetValueEx(hKey, filename, 0, REG_SZ, (BYTE*)flag, (DWORD)(_tcslen(flag) + 1)*sizeof(TCHAR));
			if (error2 == ERROR_SUCCESS) ShowRestartDialog();
			else
			{
				RegCloseKey(hKey);
				return FALSE;
			}
		}
		else
		{
			RegCloseKey(hKey);
			return FALSE;
		}

	}
	return FALSE;
}

BOOL DelCompatFlag(LPTSTR flag, BOOL initial)
{
	BOOL is64 = FALSE;
	HKEY hKey;
	HKEY hKeyWrite = NULL;
	LRESULT error;
	TCHAR filename[MAX_PATH + 1];
	TCHAR buffer[1024];
	TCHAR *bufferpos;
	TCHAR writekey[67];
	DWORD sizeout;
	SHELLEXECUTEINFO info;
	BOOL(WINAPI *iswow64)(HANDLE, PBOOL) = NULL;
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32) iswow64 = (BOOL(WINAPI*)(HANDLE, PBOOL))GetProcAddress(hKernel32, "IsWow64Process");
	if (iswow64) iswow64(GetCurrentProcess(), &is64);
	if (hKernel32) FreeLibrary(hKernel32);
	// Check system first.
	_tcscpy(writekey,_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"));
	if (is64) error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	else error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey, 0, KEY_READ, &hKey);
	if (error == ERROR_SUCCESS)
	{
		GetModuleFileName(NULL, filename, MAX_PATH);
		ZeroMemory(buffer, 1024 * sizeof(TCHAR));
		sizeout = 1024 * sizeof(TCHAR);
		if (RegQueryValueEx(hKey, filename, NULL, NULL, (LPBYTE)buffer, &sizeout) == ERROR_SUCCESS)
		{
			bufferpos = _tcsstr(buffer, flag);
			if (bufferpos)
			{
				memmove(bufferpos, bufferpos + _tcslen(flag), (_tcslen(bufferpos + _tcslen(flag))+1)*sizeof(TCHAR));
				if(is64) error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKeyWrite);
				else error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, writekey, 0, KEY_WRITE, &hKeyWrite);
				if (error == ERROR_SUCCESS)
				{
					error = RegSetValueEx(hKeyWrite, filename, 0, REG_SZ, (BYTE*)buffer, (DWORD)(_tcslen(bufferpos + _tcslen(flag)))*sizeof(TCHAR)+sizeof(TCHAR));
					RegCloseKey(hKeyWrite);
					hKeyWrite = NULL;
				}
				if (error == ERROR_ACCESS_DENIED)
				{
					if (!initial)
					{
						if (MessageBox(NULL, _T("DXGL has detected an incompatible AppCompat flag for the program you are currently running and requires administrative rights to remove it.\nWould you like to continue?"),
							_T("AppCompat error"), MB_YESNO | MB_ICONWARNING) == IDYES)
						{
							TCHAR command[(MAX_PATH*2)+32];
							if (is64) _tcscpy(command,_T("ADD \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\" /reg:64 /f /v \""));
							else _tcscpy(command,_T("ADD \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\" /f /v \""));
							_tcscat(command,filename);
							_tcscat(command,_T("\" /t REG_SZ /d \""));
							_tcscat(command,buffer);
							_tcscat(command,_T("\""));
							ZeroMemory(&info, sizeof(SHELLEXECUTEINFO));
							info.cbSize = sizeof(SHELLEXECUTEINFO);
							info.lpVerb = _T("runas");
							info.lpFile = _T("reg.exe");
							info.lpParameters = command;
							info.nShow = SW_SHOWNORMAL;
							info.fMask = SEE_MASK_NOCLOSEPROCESS;
							ShellExecuteEx(&info);
							WaitForSingleObject(info.hProcess, INFINITE);
							GetExitCodeProcess(info.hProcess, (LPDWORD)&error);
						}
						if (!error) ShowRestartDialog();
						else
						{
							MessageBox(NULL, _T("Registry value could not be updated.  Your program may crash as a result."), _T("Error"), MB_OK | MB_ICONWARNING);
							if(hKeyWrite) RegCloseKey(hKeyWrite);
							RegCloseKey(hKey);
						}
						return FALSE;
					}
					else
					{
						if (hKeyWrite) RegCloseKey(hKeyWrite);
						RegCloseKey(hKey);
						return FALSE;
					}
				}
				else if (error == ERROR_SUCCESS) ShowRestartDialog();
			}
		}
		RegCloseKey(hKey);
	}
	// Next check user.
	_tcscpy(writekey, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"));
	error = RegOpenKeyEx(HKEY_CURRENT_USER, writekey, 0, KEY_READ, &hKey);
	if (error == ERROR_SUCCESS)
	{
		GetModuleFileName(NULL, filename, MAX_PATH);
		ZeroMemory(buffer, 1024 * sizeof(TCHAR));
		sizeout = 1024 * sizeof(TCHAR);
		if (RegQueryValueEx(hKey, filename, NULL, NULL, (LPBYTE)buffer, &sizeout) == ERROR_SUCCESS)
		{
			bufferpos = _tcsstr(buffer, flag);
			if (bufferpos)
			{
				memmove(bufferpos, bufferpos + _tcslen(flag), (_tcslen(bufferpos + _tcslen(flag))+1)*sizeof(TCHAR));
				error = RegOpenKeyEx(HKEY_CURRENT_USER, writekey, 0, KEY_WRITE, &hKeyWrite);
				if (error == ERROR_SUCCESS)
				{
					error = RegSetValueEx(hKeyWrite, filename, 0, REG_SZ, (BYTE*)buffer, (DWORD)(_tcslen(bufferpos + _tcslen(flag)))*sizeof(TCHAR)+sizeof(TCHAR));
					RegCloseKey(hKeyWrite);
					hKeyWrite = NULL;
				}
				if (error == ERROR_ACCESS_DENIED)
				{
					if (hKeyWrite)
					{
						RegCloseKey(hKeyWrite);
						hKeyWrite = NULL;
					}
					if (!initial)
					{
						if (MessageBox(NULL, _T("DXGL has detected an incompatible AppCompat flag for the program you are currently running and requires administrative rights to remove it.\nWould you like to continue?"),
							_T("AppCompat error"), MB_YESNO | MB_ICONWARNING) == IDYES)
						{
							TCHAR command[(MAX_PATH * 2) + 32];
							_tcscpy(command,_T("ADD \"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\" /f /v \""));
							_tcscat(command,filename);
							_tcscat(command,_T("\" /t REG_SZ /d \""));
							_tcscat(command,buffer);
							_tcscat(command,_T("\""));
							ZeroMemory(&info, sizeof(SHELLEXECUTEINFO));
							info.cbSize = sizeof(SHELLEXECUTEINFO);
							info.lpVerb = _T("runas");
							info.lpFile = _T("reg.exe");
							info.lpParameters = command;
							info.nShow = SW_SHOWNORMAL;
							info.fMask = SEE_MASK_NOCLOSEPROCESS;
							ShellExecuteEx(&info);
							WaitForSingleObject(info.hProcess, INFINITE);
							GetExitCodeProcess(info.hProcess, (LPDWORD)&error);
						}
						if (!error)
						{
							ShowRestartDialog();
						}
						else MessageBox(NULL, _T("Registry value could not be updated.  Your program may crash as a result."), _T("Error"), MB_OK | MB_ICONWARNING);
						if (hKeyWrite) RegCloseKey(hKeyWrite);
						return FALSE;
					}
					else
					{
						if (hKeyWrite) RegCloseKey(hKeyWrite);
						return FALSE;
					}
				}
				else if (error == ERROR_SUCCESS)
				{
					ShowRestartDialog();
				}
			}
		}
		RegCloseKey(hKey);
	}
	return TRUE;
}

void GetDirFromPath(LPTSTR path)
{
	size_t i;
	size_t len = _tcslen(path);
	for(i = len; i > 0; i--)
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


BOOL ReadBool(HKEY hKey, BOOL original, BOOL *mask, LPCTSTR value)
{
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		*mask = TRUE;
		if(dwOut) return TRUE;
		else return FALSE;
	}
	else
	{
		*mask = FALSE;
		return original;
	}
}

DWORD ReadDeprecatedBool(HKEY hKey, DWORD original, DWORD *mask, LPCTSTR value, DWORD true_value, DWORD false_value)
{
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey, value, NULL, &regdword, (LPBYTE)&dwOut, &sizeout);
	if (error == ERROR_SUCCESS)
	{
		*mask = 1;
		if (dwOut) return true_value;
		else return false_value;
	}
	else
	{
		*mask = 0;
		return original;
	}
}

DWORD ReadDWORDWithObsolete(HKEY hKey, DWORD original, DWORD *mask, LPCTSTR value,
	unsigned int obsolete_count, ...) // obsolete items are LPCTSTRs
{
	LPCTSTR obsolete;
	va_list va;
	unsigned int i;
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey, value, NULL, &regdword, (LPBYTE)&dwOut, &sizeout);
	if (error == ERROR_SUCCESS)
	{
		*mask = 1;
		return dwOut;
	}
	else
	{
		va_start(va, obsolete_count);
		for (i = 0; i < obsolete_count; i++)
		{
			regdword = REG_DWORD;
			obsolete = va_arg(va, LPCTSTR);
			error = RegQueryValueEx(hKey, obsolete, NULL, &regdword, (LPBYTE)&dwOut, &sizeout);
			if (error == ERROR_SUCCESS)
			{
				*mask = 1;
				va_end(va);
				return dwOut;
			}
		}
		*mask = 0;
		va_end(va);
		return original;
	}
}

DWORD ReadDWORD(HKEY hKey, DWORD original, DWORD *mask, LPCTSTR value)
{
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		*mask = 1;
		return dwOut;
	}
	else
	{
		*mask = 0;
		return original;
	}
}

// This should be called after cfg->fullmode, cfg->WindowWidth, and
// cfg->WindowHeight are set.
void ReadWindowPos(HKEY hKey, DXGLCFG *cfg, DXGLCFG *cfgmask)
{
	int screenx, screeny;
	RECT wndrect;
	DWORD dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error;
	error = RegQueryValueEx(hKey, _T("WindowX"), NULL, &regdword, (LPBYTE)&dwOut, &sizeout);
	if (error == ERROR_SUCCESS)
	{
		cfgmask->WindowX = 1;
		cfg->WindowX = dwOut;
	}
	else cfgmask->WindowX = 0;
	error = RegQueryValueEx(hKey, _T("WindowY"), NULL, &regdword, (LPBYTE)&dwOut, &sizeout);
	if (error == ERROR_SUCCESS)
	{
		cfgmask->WindowY = 1;
		cfg->WindowY = dwOut;
	}
	else cfgmask->WindowY = 0;
	if ((!cfgmask->WindowX) || (!cfgmask->WindowY))
	{
		switch (cfg->fullmode)
		{
		case 0:
		case 1:
		case 5:
		default:
			if (!cfgmask->WindowX) cfg->WindowX = 0;
			if (!cfgmask->WindowY) cfg->WindowY = 0;
			break;
		case 2:
			screenx = GetSystemMetrics(SM_CXSCREEN);
			screeny = GetSystemMetrics(SM_CYSCREEN);
			wndrect.right = 640 + (screenx / 2) - (640 / 2);
			wndrect.bottom = 480 + (screeny / 2) - (480 / 2);
			wndrect.left = (screenx / 2) - (640 / 2);
			wndrect.top = (screeny / 2) - (480 / 2);
			AdjustWindowRectEx(&wndrect, WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
				WS_EX_APPWINDOW);
			if (!cfgmask->WindowX) cfg->WindowX = wndrect.left;
			if (!cfgmask->WindowY) cfg->WindowY = wndrect.top;
			break;
		case 3:
			screenx = GetSystemMetrics(SM_CXSCREEN);
			screeny = GetSystemMetrics(SM_CYSCREEN);
			wndrect.right = 640 + (screenx / 2) - (640 / 2);
			wndrect.bottom = 480 + (screeny / 2) - (480 / 2);
			wndrect.left = (screenx / 2) - (640 / 2);
			wndrect.top = (screeny / 2) - (480 / 2);
			AdjustWindowRectEx(&wndrect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
			if (!cfgmask->WindowX) cfg->WindowX = wndrect.left;
			if (!cfgmask->WindowY) cfg->WindowY = wndrect.top;
			break;
		case 4:
			screenx = GetSystemMetrics(SM_CXSCREEN);
			screeny = GetSystemMetrics(SM_CYSCREEN);
			wndrect.right = 640 + (screenx / 2) - (640 / 2);
			wndrect.bottom = 480 + (screeny / 2) - (480 / 2);
			wndrect.left = (screenx / 2) - (640 / 2);
			wndrect.top = (screeny / 2) - (480 / 2);
			if (!cfgmask->WindowX) cfg->WindowX = wndrect.left;
			if (!cfgmask->WindowY) cfg->WindowY = wndrect.top;
			break;
		}
	}
}

float ReadFloatWithObsolete(HKEY hKey, float original, float *mask, LPCTSTR value,
	unsigned int obsolete_count, ...) // obsolete items are LPCTSTRs
{
	LPCTSTR obsolete;
	va_list va;
	unsigned int i;
	float dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey, value, NULL, &regdword, (LPBYTE)&dwOut, &sizeout);
	if (error == ERROR_SUCCESS)
	{
		*mask = 1.0f;
		return dwOut;
	}
	else
	{
		va_start(va, obsolete_count);
		for (i = 0; i < obsolete_count; i++)
		{
			regdword = REG_DWORD;
			obsolete = va_arg(va, LPCTSTR);
			error = RegQueryValueEx(hKey, obsolete, NULL, &regdword, (LPBYTE)&dwOut, &sizeout);
			if (error == ERROR_SUCCESS)
			{
				*mask = 1.0f;
				va_end(va);
				return dwOut;
			}
		}
		*mask = 0.0f;
		va_end(va);
		return original;
	}
}

float ReadFloat(HKEY hKey, float original, float *mask, LPCTSTR value)
{
	float dwOut;
	DWORD sizeout = 4;
	DWORD regdword = REG_DWORD;
	LSTATUS error = RegQueryValueEx(hKey,value,NULL,&regdword,(LPBYTE)&dwOut,&sizeout);
	if(error == ERROR_SUCCESS)
	{
		*mask = 1.0f;
		return dwOut;
	}
	else
	{
		*mask = 0.0f;
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

void ReadSettings(HKEY hKey, DXGLCFG *cfg, DXGLCFG *mask, BOOL global, BOOL dll, LPTSTR dir)
{
	DXGLCFG *cfgmask;
	TCHAR path[MAX_PATH+1];
	LONG error;
	DWORD regsz = REG_SZ;
	DWORD sizeout=4;
	if (mask) cfgmask = mask;
	else cfgmask = &defaultmask;
	cfg->scaler = ReadDWORD(hKey, cfg->scaler, &cfgmask->scaler, _T("ScalingMode"));
	cfg->fullmode = ReadDWORD(hKey, cfg->fullmode, &cfgmask->fullmode, _T("FullscreenWindowMode"));
	cfg->colormode = ReadBool(hKey,cfg->colormode,&cfgmask->colormode,_T("ChangeColorDepth"));
	cfg->postfilter = ReadDWORDWithObsolete(hKey, cfg->postfilter, &cfgmask->postfilter, _T("PostprocessFilter"),
		1, _T("FirstScaleFilter"));
	cfg->postsizex = ReadFloatWithObsolete(hKey, cfg->postsizex, &cfgmask->postsizex, _T("PostprocessScaleX"),
		1, _T("FirstScaleX"));
	cfg->postsizey = ReadFloatWithObsolete(hKey, cfg->postsizey, &cfgmask->postsizey, _T("PostprocessScaleY"),
		1, _T("FirstScaleY"));
	cfg->scalingfilter = ReadDWORD(hKey,cfg->scalingfilter,&cfgmask->scalingfilter,_T("ScalingFilter"));
	cfg->BltScale = ReadDWORD(hKey, cfg->BltScale, &cfgmask->BltScale, _T("BltScale"));
	// Removed for DXGL 0.5.13 release
	// cfg->BltThreshold = ReadDWORD(hKey, cfg->BltThreshold, &cfgmask->BltThreshold, _T("BltThreshold"));
	cfg->texfilter = ReadDWORD(hKey,cfg->texfilter,&cfgmask->texfilter,_T("TextureFilter"));
	cfg->anisotropic = ReadDWORD(hKey,cfg->anisotropic,&cfgmask->anisotropic,_T("AnisotropicFiltering"));
	cfg->msaa = ReadDWORD(hKey,cfg->msaa,&cfgmask->msaa,_T("Antialiasing"));
	cfg->aspect3d = ReadDWORD(hKey,cfg->aspect3d,&cfgmask->aspect3d,_T("AdjustAspectRatio"));
	cfg->LowColorRendering = ReadDWORD(hKey, cfg->LowColorRendering, &cfgmask->LowColorRendering, _T("LowColorRendering"));
	cfg->EnableDithering = ReadDWORD(hKey, cfg->EnableDithering, &cfgmask->EnableDithering, _T("EnableDithering"));
	cfg->LimitTextureFormats = ReadDWORD(hKey, cfg->LimitTextureFormats, &cfgmask->LimitTextureFormats, _T("LimitTextureFormats"));
	cfg->VertexBufferSize = ReadDWORD(hKey, cfg->VertexBufferSize, &cfgmask->VertexBufferSize, _T("VertexBufferSize"));
	cfg->IndexBufferSize = ReadDWORD(hKey, cfg->IndexBufferSize, &cfgmask->IndexBufferSize, _T("IndexBufferSize"));
	cfg->UnpackBufferSize = ReadDWORD(hKey, cfg->UnpackBufferSize, &cfgmask->UnpackBufferSize, _T("UnpackBufferSize"));
	cfg->CmdBufferSize = ReadDWORD(hKey, cfg->CmdBufferSize, &cfgmask->CmdBufferSize, _T("CmdBufferSize"));
	cfg->MaxSpinCount = ReadDWORD(hKey, cfg->MaxSpinCount, &cfgmask->MaxSpinCount, _T("MaxSpinCount"));
	cfg->primaryscale = ReadDWORD(hKey,cfg->primaryscale,&cfgmask->primaryscale,_T("AdjustPrimaryResolution"));
	cfg->primaryscalex = ReadFloat(hKey,cfg->primaryscalex,&cfgmask->primaryscalex,_T("PrimaryScaleX"));
	cfg->primaryscaley = ReadFloat(hKey,cfg->primaryscaley,&cfgmask->primaryscaley,_T("PrimaryScaleY"));
	cfg->WindowScaleX = ReadFloat(hKey, cfg->WindowScaleX, &cfgmask->WindowScaleX, _T("WindowScaleX"));
	cfg->WindowScaleY = ReadFloat(hKey, cfg->WindowScaleY, &cfgmask->WindowScaleY, _T("WindowScaleY"));
	cfg->EnableShader = ReadBool(hKey, cfg->EnableShader, &cfgmask->EnableShader, _T("EnableShader"));
	ReadPath(hKey,cfg->shaderfile,cfgmask->shaderfile,_T("ShaderFile"));
	cfg->SortModes = ReadDWORD(hKey,cfg->SortModes,&cfgmask->SortModes,_T("SortModes"));
	cfg->AddColorDepths = ReadDeprecatedBool(hKey, cfg->AddColorDepths, &cfgmask->AddColorDepths, _T("AllColorDepths"), 1 | 4 | 16, 0);
	cfg->AddColorDepths = ReadDWORD(hKey,cfg->AddColorDepths,&cfgmask->AddColorDepths,_T("AddColorDepths"));
	cfg->AddModes = ReadDeprecatedBool(hKey, cfg->AddModes, &cfgmask->AddModes, _T("ExtraModes"),7,0);
	cfg->AddModes = ReadDWORD(hKey, cfg->AddModes, &cfgmask->AddModes, _T("AddModes"));
	cfg->CustomResolutionX = ReadDWORD(hKey, cfg->CustomResolutionX, &cfgmask->CustomResolutionX, _T("CustomResolutionX"));
	cfg->CustomResolutionY = ReadDWORD(hKey, cfg->CustomResolutionY, &cfgmask->CustomResolutionY, _T("CustomResolutionY"));
	cfg->CustomRefresh = ReadDWORD(hKey, cfg->CustomRefresh, &cfgmask->CustomRefresh, _T("CustomRefresh"));
	cfg->DisplayMultiplierX = ReadFloat(hKey, cfg->DisplayMultiplierX, &cfgmask->DisplayMultiplierX, _T("DisplayMultiplierX"));
	cfg->DisplayMultiplierY = ReadFloat(hKey, cfg->DisplayMultiplierY, &cfgmask->DisplayMultiplierY, _T("DisplayMultiplierY"));
	cfg->UseSetDisplayConfig = ReadBool(hKey, cfg->UseSetDisplayConfig, &cfgmask->UseSetDisplayConfig, _T("UseSetDisplayConfig"));
	cfg->vsync = ReadDWORD(hKey,cfg->vsync,&cfgmask->vsync,_T("VSync"));
	cfg->TextureFormat = ReadDWORD(hKey,cfg->TextureFormat,&cfgmask->TextureFormat,_T("TextureFormat"));
	cfg->TexUpload = ReadDWORD(hKey,cfg->TexUpload,&cfgmask->TexUpload,_T("TexUpload"));
	cfg->SingleBufferDevice = ReadBool(hKey,cfg->SingleBufferDevice,&cfgmask->SingleBufferDevice,_T("SingleBufferDevice"));
	cfg->WindowPosition = ReadDWORD(hKey, cfg->WindowPosition, &cfgmask->WindowPosition, _T("WindowPosition"));
	cfg->RememberWindowSize = ReadBool(hKey, cfg->RememberWindowSize, &cfgmask->RememberWindowSize, _T("RememberWindowSize"));
	cfg->RememberWindowPosition = ReadBool(hKey, cfg->RememberWindowPosition, &cfgmask->RememberWindowPosition, _T("RememberWindowPosition"));
	cfg->NoResizeWindow = ReadBool(hKey, cfg->NoResizeWindow, &cfgmask->NoResizeWindow, _T("NoResizeWindow"));
	cfg->WindowWidth = ReadDWORD(hKey, cfg->WindowWidth, &cfgmask->WindowWidth, _T("WindowWidth"));
	cfg->WindowHeight = ReadDWORD(hKey, cfg->WindowHeight, &cfgmask->WindowHeight, _T("WindowHeight"));
	cfg->WindowMaximized = ReadDWORD(hKey, cfg->WindowMaximized, &cfgmask->WindowMaximized, _T("WindowMaximized"));
	cfg->CaptureMouse = ReadDWORD(hKey, cfg->CaptureMouse, &cfgmask->CaptureMouse, _T("CaptureMouse"));
	ReadWindowPos(hKey, cfg, cfgmask);
	cfg->Windows8Detected = ReadBool(hKey,cfg->Windows8Detected,&cfgmask->Windows8Detected,_T("Windows8Detected"));
	cfg->DPIScale = ReadDWORD(hKey,cfg->DPIScale,&cfgmask->DPIScale,_T("DPIScale"));
	cfg->aspect = ReadFloat(hKey, cfg->aspect, &cfgmask->aspect, _T("ScreenAspect"));
	cfg->DebugNoExtFramebuffer = ReadBool(hKey, cfg->DebugNoExtFramebuffer, &cfgmask->DebugNoExtFramebuffer, _T("DebugNoExtFramebuffer"));
	cfg->DebugNoArbFramebuffer = ReadBool(hKey, cfg->DebugNoArbFramebuffer, &cfgmask->DebugNoArbFramebuffer, _T("DebugNoArbFramebuffer"));
	cfg->DebugNoES2Compatibility = ReadBool(hKey, cfg->DebugNoES2Compatibility, &cfgmask->DebugNoES2Compatibility, _T("DebugNoES2Compatibility"));
	cfg->DebugNoExtDirectStateAccess = ReadBool(hKey, cfg->DebugNoExtDirectStateAccess, &cfgmask->DebugNoExtDirectStateAccess, _T("DebugNoExtDirectStateAccess"));
	cfg->DebugNoArbDirectStateAccess = ReadBool(hKey, cfg->DebugNoArbDirectStateAccess, &cfgmask->DebugNoArbDirectStateAccess, _T("DebugNoArbDirectStateAccess"));
	cfg->DebugNoSamplerObjects = ReadBool(hKey, cfg->DebugNoSamplerObjects, &cfgmask->DebugNoSamplerObjects, _T("DebugNoSamplerObjects"));
	cfg->DebugNoGpuShader4 = ReadBool(hKey, cfg->DebugNoGpuShader4, &cfgmask->DebugNoGpuShader4, _T("DebugNoGpuShader4"));
	cfg->DebugNoGLSL130 = ReadBool(hKey, cfg->DebugNoGLSL130, &cfgmask->DebugNoGLSL130, _T("DebugNoGLSL130"));
	cfg->DebugUploadAfterUnlock = ReadBool(hKey, cfg->DebugUploadAfterUnlock, &cfgmask->DebugUploadAfterUnlock, _T("DebugUploadAfterUnlock"));
	cfg->DebugBlendDestColorKey = ReadBool(hKey, cfg->DebugBlendDestColorKey, &cfgmask->DebugBlendDestColorKey, _T("DebugBlendDestColorKey"));
	cfg->DebugNoMouseHooks = ReadBool(hKey, cfg->DebugNoMouseHooks, &cfgmask->DebugNoMouseHooks, _T("DebugNoMouseHooks"));
	cfg->DebugNoPaletteRedraw = ReadBool(hKey, cfg->DebugNoPaletteRedraw, &cfgmask->DebugNoPaletteRedraw, _T("DebugNoPaletteRedraw"));
	cfg->DebugMaxGLVersionMajor = ReadDWORD(hKey, cfg->DebugMaxGLVersionMajor, &cfgmask->DebugMaxGLVersionMajor, _T("DebugMaxGLVersionMajor"));
	cfg->DebugMaxGLVersionMinor = ReadDWORD(hKey, cfg->DebugMaxGLVersionMinor, &cfgmask->DebugMaxGLVersionMinor, _T("DebugMaxGLVersionMinor"));
	cfg->DebugTraceLevel = ReadDWORD(hKey, cfg->DebugTraceLevel, &cfgmask->DebugTraceLevel, _T("DebugTraceLevel"));
	cfg->HackCrop640480to640400 = ReadBool(hKey, cfg->HackCrop640480to640400, &cfgmask->HackCrop640480to640400, _T("HackCrop640480to640400"));
	cfg->HackAutoExpandViewport = ReadDWORDWithObsolete(hKey, cfg->HackAutoExpandViewport, &cfgmask->HackAutoExpandViewport, _T("HackAutoExpandViewport"),
		1, _T("HackAutoScale512448to640480"));
	cfg->HackAutoExpandViewportCompare = ReadDWORD(hKey, cfg->HackAutoExpandViewportCompare, &cfgmask->HackAutoExpandViewportCompare, _T("HackAutoExpandViewportCompare"));
	cfg->HackAutoExpandViewportValue = ReadDWORD(hKey, cfg->HackAutoExpandViewportValue, &cfgmask->HackAutoExpandViewportValue, _T("HackAutoExpandViewportValue"));
	cfg->HackNoTVRefresh = ReadBool(hKey, cfg->HackNoTVRefresh, &cfgmask->HackNoTVRefresh, _T("HackNoTVRefresh"));
	cfg->HackSetCursor = ReadBool(hKey, cfg->HackSetCursor, &cfgmask->HackSetCursor, _T("HackSetCursor"));
	cfg->HackPaletteDelay = ReadDWORD(hKey, cfg->HackPaletteDelay, &cfgmask->HackPaletteDelay, _T("HackPaletteDelay"));
	cfg->HackPaletteVsync = ReadBool(hKey, cfg->HackPaletteVsync, &cfgmask->HackPaletteVsync, _T("HackPaletteVsync"));
	if(!global && dll)
	{
		sizeout = 0;
		if(!dir) GetModuleFileName(NULL,path,MAX_PATH);
		else _tcsncpy(path,dir,MAX_PATH+1);
		GetDirFromPath(path);
		error = RegQueryValueEx(hKey,_T("InstallPath"),NULL,&regsz,NULL,&sizeout);
		if(error == ERROR_FILE_NOT_FOUND)
			RegSetValueEx(hKey, _T("InstallPath"), 0, REG_SZ,
				(LPBYTE)path, (DWORD)(_tcslen(path) * sizeof(TCHAR)));
	}
	if (global && !cfg->Windows8Detected)
	{
		OSVERSIONINFO osver;
		osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osver);
		if (osver.dwMajorVersion > 6) cfg->Windows8Detected = TRUE;
		if ((osver.dwMajorVersion == 6) && (osver.dwMinorVersion >= 2)) cfg->Windows8Detected = TRUE;
		if (cfg->Windows8Detected) cfg->AddColorDepths |= 1 | 4 | 16;
	}
}

void WriteBool(HKEY hKey, BOOL value, BOOL mask, LPCTSTR name)
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

void WriteDWORDDeleteObsolete(HKEY hKey, DWORD value, DWORD mask, LPCTSTR name,
	unsigned int obsolete_count, ...) // obsolete items are LPCTSTRs
{
	LPCTSTR obsolete;
	va_list va;
	unsigned int i;
	va_start(va, obsolete_count);
	for (i = 0; i < obsolete_count; i++)
	{
		obsolete = va_arg(va, LPCTSTR);
		RegDeleteValue(hKey, obsolete);
	}
	va_end(va);
	if (mask) RegSetValueEx(hKey, name, 0, REG_DWORD, (BYTE*)&value, 4);
	else RegDeleteValue(hKey, name);
}

void WriteDWORD(HKEY hKey, DWORD value, DWORD mask, LPCTSTR name)
{
	if(mask) RegSetValueEx(hKey,name,0,REG_DWORD,(BYTE*)&value,4);
	else RegDeleteValue(hKey,name);
}
void WritePath(HKEY hKey, const TCHAR *path, const TCHAR *mask, LPCTSTR name)
{
	if(mask[0]) RegSetValueEx(hKey,name,0,REG_SZ,(BYTE*)path,(DWORD)(_tcslen(path)+1)*sizeof(TCHAR));
	else RegDeleteValue(hKey,name);
}

void WriteFloatDeleteObsolete(HKEY hKey, float value, float mask, LPCTSTR name,
	unsigned int obsolete_count, ...) // obsolete items are LPCTSTRs
{
	LPCTSTR obsolete;
	va_list va;
	unsigned int i;
	va_start(va, obsolete_count);
	for (i = 0; i < obsolete_count; i++)
	{
		obsolete = va_arg(va, LPCTSTR);
		RegDeleteValue(hKey, obsolete);
	}
	va_end(va);
	if (fabsf(mask) > 0.5f) RegSetValueEx(hKey, name, 0, REG_DWORD, (BYTE*)&value, 4);
	else RegDeleteValue(hKey, name);
}
void WriteFloat(HKEY hKey, float value, float mask, LPCTSTR name)
{
	if (fabsf(mask) > 0.5f) RegSetValueEx(hKey, name, 0, REG_DWORD, (BYTE*)&value, 4);
	else RegDeleteValue(hKey, name);
}

void WriteSettings(HKEY hKey, const DXGLCFG *cfg, const DXGLCFG *mask)
{
	const DXGLCFG *cfgmask;
	if(mask) cfgmask = mask;
	else cfgmask = &defaultmask;
	memset(&defaultmask,1,sizeof(DXGLCFG));
	defaultmask.aspect = 1.0f;
	defaultmask.postsizex = 1.0f;
	defaultmask.postsizey = 1.0f;
	defaultmask.DisplayMultiplierX = 1.0f;
	defaultmask.DisplayMultiplierY = 1.0f;
	defaultmask.primaryscalex = 1.0f;
	defaultmask.primaryscaley = 1.0f;
	defaultmask.WindowScaleX = 1.0f;
	defaultmask.WindowScaleY = 1.0f;
	WriteDWORD(hKey,cfg->scaler,cfgmask->scaler,_T("ScalingMode"));
	WriteDWORD(hKey, cfg->fullmode, cfgmask->fullmode, _T("FullscreenWindowMode"));
	WriteBool(hKey,cfg->colormode,cfgmask->colormode,_T("ChangeColorDepth"));
	WriteDWORDDeleteObsolete(hKey, cfg->postfilter, cfgmask->postfilter, _T("PostprocessFilter"),
		1, _T("FirstScaleFilter"));
	WriteFloatDeleteObsolete(hKey, cfg->postsizex, cfgmask->postsizex, _T("PostprocessScaleX"),
		1, _T("FirstScaleX"));
	WriteFloatDeleteObsolete(hKey, cfg->postsizey, cfgmask->postsizey, _T("PostprocessScaleY"),
		1, _T("FirstScaleY"));
	WriteDWORD(hKey,cfg->scalingfilter,cfgmask->scalingfilter,_T("ScalingFilter"));
	WriteDWORD(hKey, cfg->BltScale, cfgmask->BltScale, _T("BltScale"));
	// Removed for DXGL 0.5.13 release
	// WriteDWORD(hKey, cfg->BltThreshold, cfgmask->BltThreshold, _T("BltThreshold"));
	WriteDWORD(hKey,cfg->texfilter,cfgmask->texfilter,_T("TextureFilter"));
	WriteDWORD(hKey,cfg->anisotropic,cfgmask->anisotropic,_T("AnisotropicFiltering"));
	WriteDWORD(hKey,cfg->msaa,cfgmask->msaa,_T("Antialiasing"));
	WriteDWORD(hKey,cfg->aspect3d,cfgmask->aspect3d,_T("AdjustAspectRatio"));
	WriteDWORD(hKey, cfg->LowColorRendering, cfgmask->LowColorRendering, _T("LowColorRendering"));
	WriteDWORD(hKey, cfg->EnableDithering, cfgmask->EnableDithering, _T("EnableDithering"));
	WriteDWORD(hKey, cfg->LimitTextureFormats, cfgmask->LimitTextureFormats, _T("LimitTextureFormats"));
	WriteDWORD(hKey, cfg->VertexBufferSize, cfgmask->VertexBufferSize, _T("VertexBufferSize"));
	WriteDWORD(hKey, cfg->IndexBufferSize, cfgmask->IndexBufferSize, _T("IndexBufferSize"));
	WriteDWORD(hKey, cfg->UnpackBufferSize, cfgmask->UnpackBufferSize, _T("UnpackBufferSize"));
	WriteDWORD(hKey, cfg->CmdBufferSize, cfgmask->CmdBufferSize, _T("CmdBufferSize"));
	WriteDWORD(hKey, cfg->MaxSpinCount, cfgmask->MaxSpinCount, _T("MaxSpinCount"));
	WriteDWORD(hKey,cfg->primaryscale,cfgmask->primaryscale,_T("AdjustPrimaryResolution"));
	WriteFloat(hKey,cfg->primaryscalex,cfgmask->primaryscalex,_T("PrimaryScaleX"));
	WriteFloat(hKey,cfg->primaryscaley,cfgmask->primaryscaley,_T("PrimaryScaleY"));
	WriteFloat(hKey, cfg->WindowScaleX, cfgmask->WindowScaleX, _T("WindowScaleX"));
	WriteFloat(hKey, cfg->WindowScaleY, cfgmask->WindowScaleY, _T("WindowScaleY"));
	WriteBool(hKey, cfg->EnableShader, cfgmask->EnableShader, _T("EnableShader"));
	WritePath(hKey,cfg->shaderfile,cfgmask->shaderfile,_T("ShaderFile"));
	WriteDWORD(hKey,cfg->SortModes,cfgmask->SortModes,_T("SortModes"));
	WriteDWORD(hKey,cfg->AddColorDepths,cfgmask->AddColorDepths,_T("AddColorDepths"));
	WriteDWORD(hKey,cfg->AddModes,cfgmask->AddModes,_T("AddModes"));
	WriteDWORD(hKey, cfg->CustomResolutionX, cfgmask->CustomResolutionX, _T("CustomResolutionX"));
	WriteDWORD(hKey, cfg->CustomResolutionY, cfgmask->CustomResolutionY, _T("CustomResolutionY"));
	WriteDWORD(hKey, cfg->CustomRefresh, cfgmask->CustomRefresh, _T("CustomRefresh"));
	WriteFloat(hKey, cfg->DisplayMultiplierX, cfgmask->DisplayMultiplierX, _T("DisplayMultiplierX"));
	WriteFloat(hKey, cfg->DisplayMultiplierY, cfgmask->DisplayMultiplierY, _T("DisplayMultiplierY"));
	WriteBool(hKey, cfg->UseSetDisplayConfig, cfgmask->UseSetDisplayConfig, _T("UseSetDisplayConfig"));
	WriteDWORD(hKey,cfg->vsync,cfgmask->vsync,_T("VSync"));
	WriteDWORD(hKey,cfg->TextureFormat,cfgmask->TextureFormat,_T("TextureFormat"));
	WriteDWORD(hKey,cfg->TexUpload,cfgmask->TexUpload,_T("TexUpload"));
	WriteBool(hKey,cfg->SingleBufferDevice,cfgmask->SingleBufferDevice,_T("SingleBufferDevice"));
	WriteDWORD(hKey, cfg->WindowPosition, cfgmask->WindowPosition, _T("WindowPosition"));
	WriteBool(hKey, cfg->RememberWindowSize, cfgmask->RememberWindowSize, _T("RememberWindowSize"));
	WriteBool(hKey, cfg->RememberWindowPosition, cfgmask->RememberWindowPosition, _T("RememberWindowPosition"));
	WriteBool(hKey, cfg->NoResizeWindow, cfgmask->NoResizeWindow, _T("NoResizeWindow"));
	WriteDWORD(hKey, cfg->WindowX, cfgmask->WindowX, _T("WindowX"));
	WriteDWORD(hKey, cfg->WindowY, cfgmask->WindowY, _T("WindowY"));
	WriteDWORD(hKey, cfg->WindowWidth, cfgmask->WindowWidth, _T("WindowWidth"));
	WriteDWORD(hKey, cfg->WindowHeight, cfgmask->WindowHeight, _T("WindowHeight"));
	WriteDWORD(hKey, cfg->WindowMaximized, cfgmask->WindowMaximized, _T("WindowMaximized"));
	WriteDWORD(hKey, cfg->CaptureMouse, cfgmask->CaptureMouse, _T("CaptureMouse"));
	WriteBool(hKey,cfg->Windows8Detected,cfgmask->Windows8Detected,_T("Windows8Detected"));
	WriteDWORD(hKey,cfg->DPIScale,cfgmask->DPIScale,_T("DPIScale"));
	WriteFloat(hKey, cfg->aspect, cfgmask->aspect, _T("ScreenAspect"));
	WriteBool(hKey, cfg->DebugNoExtFramebuffer, cfgmask->DebugNoExtFramebuffer, _T("DebugNoExtFramebuffer"));
	WriteBool(hKey, cfg->DebugNoArbFramebuffer, cfgmask->DebugNoArbFramebuffer, _T("DebugNoArbFramebuffer"));
	WriteBool(hKey, cfg->DebugNoES2Compatibility, cfgmask->DebugNoES2Compatibility, _T("DebugNoES2Compatibility"));
	WriteBool(hKey, cfg->DebugNoExtDirectStateAccess, cfgmask->DebugNoExtDirectStateAccess, _T("DebugNoExtDirectStateAccess"));
	WriteBool(hKey, cfg->DebugNoArbDirectStateAccess, cfgmask->DebugNoArbDirectStateAccess, _T("DebugNoArbDirectStateAccess"));
	WriteBool(hKey, cfg->DebugNoSamplerObjects, cfgmask->DebugNoSamplerObjects, _T("DebugNoSamplerObjects"));
	WriteBool(hKey, cfg->DebugNoGpuShader4, cfgmask->DebugNoGpuShader4, _T("DebugNoGpuShader4"));
	WriteBool(hKey, cfg->DebugNoGLSL130, cfgmask->DebugNoGLSL130, _T("DebugNoGLSL130"));
	WriteBool(hKey, cfg->DebugUploadAfterUnlock, cfgmask->DebugUploadAfterUnlock, _T("DebugUploadAfterUnlock"));
	WriteBool(hKey, cfg->DebugBlendDestColorKey, cfgmask->DebugBlendDestColorKey, _T("DebugBlendDestColorKey"));
	WriteBool(hKey, cfg->DebugNoMouseHooks, cfgmask->DebugNoMouseHooks, _T("DebugNoMouseHooks"));
	WriteBool(hKey, cfg->DebugNoPaletteRedraw, cfgmask->DebugNoPaletteRedraw, _T("DebugNoPaletteRedraw"));
	WriteDWORD(hKey, cfg->DebugMaxGLVersionMajor, cfgmask->DebugMaxGLVersionMajor, _T("DebugMaxGLVersionMajor"));
	WriteDWORD(hKey, cfg->DebugMaxGLVersionMinor, cfgmask->DebugMaxGLVersionMinor, _T("DebugMaxGLVersionMinor"));
	WriteDWORD(hKey, cfg->DebugTraceLevel, cfgmask->DebugTraceLevel, _T("DebugTraceLevel"));
	WriteBool(hKey, cfg->HackCrop640480to640400, cfgmask->HackCrop640480to640400, _T("HackCrop640480to640400"));
	WriteDWORDDeleteObsolete(hKey, cfg->HackAutoExpandViewport, cfgmask->HackAutoExpandViewport, _T("HackAutoExpandViewport"),
		1, _T("HackAutoScale512448to640480"));
	WriteDWORD(hKey, cfg->HackAutoExpandViewportCompare, cfgmask->HackAutoExpandViewportCompare, _T("HackAutoExpandViewportCompare"));
	WriteDWORD(hKey, cfg->HackAutoExpandViewportValue, cfgmask->HackAutoExpandViewportValue, _T("HackAutoExpandViewportValue"));
	WriteBool(hKey, cfg->HackNoTVRefresh, cfgmask->HackNoTVRefresh, _T("HackNoTVRefresh"));
	WriteBool(hKey, cfg->HackSetCursor, cfgmask->HackSetCursor, _T("HackSetCursor"));
	WriteDWORD(hKey, cfg->HackPaletteDelay, cfgmask->HackPaletteDelay, _T("HackPaletteDelay"));
	WriteDWORD(hKey, cfg->HackPaletteVsync, cfgmask->HackPaletteVsync, _T("HackPaletteVsync"));
}

TCHAR newregname[MAX_PATH+65];

BOOL CheckProfileExists(LPTSTR path)
{
	Sha256Context sha_context;
	SHA256_HASH sha256;
	TCHAR sha256string[65];
	TCHAR regkey[MAX_PATH + 80];
	TCHAR filename[MAX_PATH + 1];
	WCHAR filename2[MAX_PATH + 1];
	HKEY hKey;
	LONG error;
	int i;
	_tcscpy(regkey, regkeybase);
	_tcscat(regkey, profilesname);
	_tcscpy(filename, path);
	for (i = (int)_tcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
	i++;
	_tcscat(regkey, &filename[i]);
	_tcscat(regkey, _T("-"));
	i--;
	filename[i] = 0;
	_tcslwr(filename);
	_tchartowchar(filename2,filename,-1);
	Sha256Initialise(&sha_context);
	Sha256Update(&sha_context, filename2, (uint32_t)wcslen(filename2) * sizeof(WCHAR));
	Sha256Finalise(&sha_context, &sha256);
	for (i = 0; i < (256 / 8); i++)
	{
		sha256string[i * 2] = (TCHAR)hexdigit(sha256.bytes[i] >> 4);
		sha256string[(i * 2) + 1] = (TCHAR)hexdigit(sha256.bytes[i] & 0xF);
	}
	sha256string[256 / 4] = 0;
	_tcscat(regkey, sha256string);
	error = RegOpenKeyEx(HKEY_CURRENT_USER, regkey, 0, KEY_READ, &hKey);
	if (error == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return TRUE;
	}
	else return FALSE;
}

LPTSTR MakeNewConfig(LPTSTR path)
{
	Sha256Context sha_context;
	SHA256_HASH sha256;
	TCHAR sha256string[65];
	TCHAR pathlwr[MAX_PATH + 1];
	WCHAR pathlwr2[MAX_PATH + 1];
	HKEY hKey;
	DXGLCFG tmp;
	TCHAR regkey[MAX_PATH + 80];
	int i;
	TCHAR filename[MAX_PATH + 1];
	_tcsncpy(pathlwr, path, MAX_PATH);
	pathlwr[MAX_PATH] = 0;
	for (i = (int)_tcslen(pathlwr); (i > 0) && (pathlwr[i] != 92) && (pathlwr[i] != 47); i--);
	pathlwr[i] = 0;
	_tcslwr(pathlwr);
	_tchartowchar(pathlwr2,pathlwr,-1);
	Sha256Initialise(&sha_context);
	Sha256Update(&sha_context, pathlwr2, (uint32_t)wcslen(pathlwr2) * sizeof(WCHAR));
	Sha256Finalise(&sha_context, &sha256);
	for (i = 0; i < (256 / 8); i++)
	{
		sha256string[i * 2] = (TCHAR)hexdigit(sha256.bytes[i] >> 4);
		sha256string[(i * 2) + 1] = (TCHAR)hexdigit(sha256.bytes[i] & 0xF);
	}
	sha256string[256 / 4] = 0;
	_tcscpy(regkey,regkeybase);
	_tcsncpy(filename,path,MAX_PATH);
	filename[MAX_PATH] = 0;
	for(i = (int)_tcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
	i++;
	_tcscat(regkey, profilesname);
	_tcscat(regkey,&filename[i]);
	_tcscat(regkey,_T("-"));
	_tcscat(regkey,sha256string);
	_tcscpy(newregname,&filename[i]);
	_tcscat(newregname,_T("-"));
	_tcscat(newregname,sha256string);
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,&tmp,NULL,FALSE,TRUE,path);
	RegCloseKey(hKey);
	return newregname;
}

void GetDefaultConfig(DXGLCFG *cfg)
{
	OSVERSIONINFO osver;
	SYSTEM_INFO sysinfo;
	BOOL Windows8Detected = FALSE;
	ZeroMemory(cfg, sizeof(DXGLCFG));
	cfg->DPIScale = 1;
	cfg->AddModes = 1;
	cfg->CustomResolutionX = 640;
	cfg->CustomResolutionY = 480;
	cfg->CustomRefresh = 60;
	cfg->DisplayMultiplierX = 1.0f;
	cfg->DisplayMultiplierY = 1.0f;
	cfg->primaryscalex = 1.0f;
	cfg->primaryscaley = 1.0f;
	cfg->WindowScaleX = 1.0f;
	cfg->WindowScaleY = 1.0f;
	// Removed for DXGL 0.5.13 release
	// cfg->BltThreshold = 127;
	cfg->WindowPosition = 1;
	cfg->RememberWindowSize = TRUE;
	cfg->RememberWindowPosition = TRUE;
	cfg->WindowWidth = 640;
	cfg->WindowHeight = 480;
	cfg->HackPaletteDelay = 30;
	cfg->LimitTextureFormats = 1;
	if (!cfg->Windows8Detected)
	{
		osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osver);
		if (osver.dwMajorVersion > 6) Windows8Detected = TRUE;
		if ((osver.dwMajorVersion == 6) && (osver.dwMinorVersion >= 2)) Windows8Detected = TRUE;
		if (Windows8Detected) cfg->AddColorDepths |= (1 | 4 | 16);
	}
	GetSystemInfo(&sysinfo);
	if (sysinfo.dwNumberOfProcessors > 1) cfg->MaxSpinCount = 100000;
	else cfg->MaxSpinCount = 0; // Don't spin on uniprocessor systems.
}

DWORD INIBoolValue(const char *value)
{
	if (value[0] == 'F') return 0;
	if (value[0] == 'f') return 0;
	if (value[0] == 'T') return 1;
	if (value[0] == 't') return 1;
	if (!atoi(value)) return 0;
	return 1;
}

DWORD INIIntValue(const char *value)
{
	return atoi(value);
}

DWORD INIIntBoolValue(const char *value)
{
	if (value[0] == 'F') return 0;
	if (value[0] == 'f') return 0;
	if (value[0] == 'T') return 1;
	if (value[0] == 't') return 1;
	return atoi(value);
}

DWORD INIHexValue(const char *value)
{
	return (DWORD)strtoul(value, NULL, 0);
}

float INIAspectValue(const char *value)
{
	char *ptr;
	double numerator, denominator;
	if (!strcmp(value, "Default")) return 0.0f;
	else
	{
		// Check for colon
		ptr = strstr(value, ":");
		if (ptr)
		{
			*ptr = 0;
			numerator = atof(value);
			denominator = atof(ptr + 1);
			return (float)(numerator / denominator);
		}
		else return (float)atof(value);
	}
}

float INIFloatValue(const char *value)
{
	return (float)atof(value);
}

int ReadINICallback(DXGLCFG *cfg, const char *section, const char *name,
	const char *value)
{
	FILE *file;
	TCHAR inipath[MAX_PATH + 10];
#ifdef _UNICODE
	TCHAR unicode_path[MAX_PATH + 1];
#endif
	if (!_stricmp(name, "Include"))
	{
		ini_depth++;
		if(ini_depth <= 16)
		{
			_tcscpy(inipath, cfg->inipath);
			_tcscat(inipath, _T("\\"));
			#ifdef _UNICODE
			utf8to16(unicode_path, value);
			_tcsncat(inipath, unicode_path, MAX_PATH);
			#else
			_tcscat(inipath, value);
			#endif
			file = _tfopen(inipath, _T("r"));
			if (file)
			{
				ini_parse_file(file, ReadINICallback, cfg);
				fclose(file);
			}
		}
		ini_depth--;
	}
	else {
		if (!_stricmp(section, "system"))
		{
			if (!_stricmp(name, "NoWriteRegistry")) cfg->NoWriteRegistry = INIBoolValue(value);
			if (!_stricmp(name, "OverrideDefaults")) cfg->OverrideDefaults = INIBoolValue(value);
		}
		if (!_stricmp(section, "display"))
		{
			if (!_stricmp(name, "ScalingMode")) cfg->scaler = INIIntValue(value);
			if (!_stricmp(name, "FullscreenWindowMode")) cfg->fullmode = INIIntValue(value);
			if (!_stricmp(name, "ChangeColorDepth")) cfg->colormode = INIBoolValue(value);
			if (!_stricmp(name, "AllColorDepths"))
			{
				if (!cfg->ParsedAddColorDepths)
				{
					if (INIBoolValue(value)) cfg->AddColorDepths = 1 | 4 | 16;
					else cfg->AddColorDepths = 0;
				}
			}
			if (!_stricmp(name, "AddColorDepths"))
			{
				cfg->ParsedAddColorDepths = TRUE;
				cfg->AddColorDepths = INIIntValue(value);
			}
			if (!_stricmp(name, "ExtraModes"))
			{
				if (!cfg->ParsedAddModes)
				{
					if (INIBoolValue(value)) cfg->AddModes = 7;
					else cfg->AddModes = 0;
				}
			}
			if (!_stricmp(name, "AddModes"))
			{
				cfg->ParsedAddModes = TRUE;
				cfg->AddModes = INIIntValue(value);
			}
			if (!_stricmp(name, "SortModes")) cfg->SortModes = INIIntValue(value);
			if (!_stricmp(name, "VSync")) cfg->vsync = INIIntValue(value);
			if (!_stricmp(name, "CustomResolutionX")) cfg->CustomResolutionX = INIIntValue(value);
			if (!_stricmp(name, "CustomResolutionY")) cfg->CustomResolutionY = INIIntValue(value);
			if (!_stricmp(name, "CustomRefresh")) cfg->CustomRefresh = INIIntValue(value);
			if (!_stricmp(name, "DisplayMultiplierX")) cfg->DisplayMultiplierX = INIFloatValue(value);
			if (!_stricmp(name, "DisplayMultiplierY")) cfg->DisplayMultiplierY = INIFloatValue(value);
			if (!_stricmp(name, "SingleBufferDevice")) cfg->SingleBufferDevice = INIBoolValue(value);
			if (!_stricmp(name, "UseSetDisplayConfig")) cfg->UseSetDisplayConfig = INIBoolValue(value);
		}
		if (!_stricmp(section, "scaling"))
		{
			if (!_stricmp(name, "ScalingFilter")) cfg->scalingfilter = INIIntValue(value);
			if (!_stricmp(name, "BltScale")) cfg->BltScale = INIIntValue(value);
			// Removed for DXGL 0.5.13 release
			// if (!_stricmp(name, "BltThreshold")) cfg->BltThreshold = INIIntValue(value);
			if (!_stricmp(name, "AdjustPrimaryResolution")) cfg->primaryscale = INIIntValue(value);
			if (!_stricmp(name, "PrimaryScaleX")) cfg->primaryscalex = INIFloatValue(value);
			if (!_stricmp(name, "PrimaryScaleY")) cfg->primaryscaley = INIFloatValue(value);
			if (!_stricmp(name, "WindowScaleX")) cfg->WindowScaleX = INIFloatValue(value);
			if (!_stricmp(name, "WindowScaleY")) cfg->WindowScaleY = INIFloatValue(value);
			if (!_stricmp(name, "ScreenAspect")) cfg->aspect = INIAspectValue(value);
			if (!_stricmp(name, "DPIScale")) cfg->DPIScale = INIIntValue(value);
		}
		if (!_stricmp(section, "postprocess"))
		{
			if (!_stricmp(name, "PostprocessFilter")) cfg->postfilter = INIIntValue(value);
			if (!_stricmp(name, "PostprocessScaleX")) cfg->postsizex = INIFloatValue(value);
			if (!_stricmp(name, "PostprocessScaleY")) cfg->postsizex = INIFloatValue(value);
			if (!_stricmp(name, "EnableShader")) cfg->EnableShader = INIBoolValue(value);
			if (!_stricmp(name, "ShaderFile"))
			{
#ifdef _UNICODE
				utf8to16(unicode_path, value);
				_tcsncpy(cfg->shaderfile, unicode_path, MAX_PATH);
#else
				_tcsncpy(cfg->shaderfile, value, MAX_PATH);
#endif
			}
		}
		if (!_stricmp(section, "d3d"))
		{
			if (!_stricmp(name, "TextureFilter")) cfg->texfilter = INIIntValue(value);
			if (!_stricmp(name, "AnisotropicFiltering")) cfg->anisotropic = INIIntValue(value);
			if (!_stricmp(name, "Antialiasing")) cfg->msaa = INIHexValue(value);
			if (!_stricmp(name, "D3DAspect")) cfg->aspect3d = INIIntValue(value);
			if (!_stricmp(name, "LowColorRendering")) cfg->LowColorRendering = INIIntValue(value);
			if (!_stricmp(name, "EnableDithering")) cfg->EnableDithering = INIIntValue(value);
			if (!_stricmp(name, "LimitTextureFormats")) cfg->LimitTextureFormats = INIIntValue(value);
		}
		if (!_stricmp(section, "advanced"))
		{
			if (!_stricmp(name, "TextureFormat")) cfg->TextureFormat = INIIntValue(value);
			if (!_stricmp(name, "TexUpload")) cfg->TexUpload = INIIntValue(value);
			if (!_stricmp(name, "WindowPosition")) cfg->WindowPosition = INIIntValue(value);
			if (!_stricmp(name, "RememberWindowSize")) cfg->RememberWindowSize = INIBoolValue(value);
			if (!_stricmp(name, "RememberWindowPosition")) cfg->RememberWindowPosition = INIBoolValue(value);
			if (!_stricmp(name, "NoResizeWindow")) cfg->NoResizeWindow = INIBoolValue(value);
			if (!_stricmp(name, "WindowX")) cfg->WindowX = INIIntValue(value);
			if (!_stricmp(name, "WindowY")) cfg->WindowY = INIIntValue(value);
			if (!_stricmp(name, "WindowWidth")) cfg->WindowWidth = INIIntValue(value);
			if (!_stricmp(name, "WindowHeight")) cfg->WindowHeight = INIIntValue(value);
			if (!_stricmp(name, "WindowMaximized")) cfg->WindowMaximized = INIBoolValue(value);
			if (!_stricmp(name, "CaptureMouse")) cfg->CaptureMouse = INIBoolValue(value);
		}
		if (!_stricmp(section, "debug"))
		{
			if (!_stricmp(name, "DebugNoExtFramebuffer")) cfg->DebugNoExtFramebuffer = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoArbFramebuffer")) cfg->DebugNoArbFramebuffer = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoES2Compatibility")) cfg->DebugNoES2Compatibility = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoExtDirectStateAccess")) cfg->DebugNoExtDirectStateAccess = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoArbDirectStateAccess")) cfg->DebugNoArbDirectStateAccess = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoSamplerObjects")) cfg->DebugNoSamplerObjects = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoGpuShader4")) cfg->DebugNoGpuShader4 = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoGLSL130")) cfg->DebugNoGLSL130 = INIBoolValue(value);
			if (!_stricmp(name, "DebugUploadAfterUnlock")) cfg->DebugUploadAfterUnlock = INIBoolValue(value);
			if (!_stricmp(name, "DebugBlendDestColorKey")) cfg->DebugBlendDestColorKey = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoMouseHooks")) cfg->DebugNoMouseHooks = INIBoolValue(value);
			if (!_stricmp(name, "DebugNoPaletteRedraw")) cfg->DebugNoPaletteRedraw = INIBoolValue(value);
			if (!_stricmp(name, "DebugMaxGLVersionMajor")) cfg->DebugMaxGLVersionMajor = INIIntValue(value);
			if (!_stricmp(name, "DebugMaxGLVersionMinor")) cfg->DebugMaxGLVersionMinor = INIIntValue(value);
			if (!_stricmp(name, "DebugTraceLevel")) cfg->DebugTraceLevel = INIIntValue(value);
		}
		if (!_stricmp(section, "hacks"))
		{
			if (!_stricmp(name, "HackCrop640480to640400")) cfg->HackCrop640480to640400 = INIBoolValue(value);
			if (!_stricmp(name, "HackAutoExpandViewport")) cfg->HackAutoExpandViewport = INIIntBoolValue(value);
			if (!_stricmp(name, "HackAutoScale512448to640480")) cfg->HackAutoExpandViewport = INIIntBoolValue(value);
			if (!_stricmp(name, "HackAutoExpandViewportCompare")) cfg->HackAutoExpandViewportCompare = INIIntValue(value);
			if (!_stricmp(name, "HackAutoExpandViewportValue")) cfg->HackAutoExpandViewportValue = INIHexValue(value);
			if (!_stricmp(name, "HackNoTVRefresh")) cfg->HackNoTVRefresh = INIBoolValue(value);
			if (!_stricmp(name, "HackSetCursor")) cfg->HackSetCursor = INIBoolValue(value);
			if (!_stricmp(name, "HackPaletteDelay")) cfg->HackPaletteDelay = INIIntValue(value);
			if (!_stricmp(name, "HackPaletteVsync")) cfg->HackPaletteVsync = INIBoolValue(value);
			if (!_stricmp(name, "VertexBufferSize")) cfg->VertexBufferSize = INIIntValue(value);
			if (!_stricmp(name, "IndexBufferSize")) cfg->IndexBufferSize = INIIntValue(value);
			if (!_stricmp(name, "UnpackBufferSize")) cfg->UnpackBufferSize = INIIntValue(value);
			if (!_stricmp(name, "CmdBufferSize")) cfg->CmdBufferSize = INIIntValue(value);
			if (!_stricmp(name, "MaxSpinCount")) cfg->MaxSpinCount = INIIntValue(value);
		}
	}
	return 1;
}

void ReadINI(DXGLCFG *cfg)
{
	FILE *file;
	TCHAR inipath[MAX_PATH + 10];
	ini_depth = 0;
	GetModuleFileName(NULL, inipath, MAX_PATH);
	GetDirFromPath(inipath);
	_tcscpy(cfg->inipath, inipath);
	_tcscat(inipath, _T("\\dxgl.cfg"));
	file = _tfopen(inipath, _T("r"));
	if (file)
	{
		ini_parse_file(file, ReadINICallback, cfg);
		fclose(file);
	}
	else
	{
		GetDirFromPath(inipath);
		_tcscat(inipath, _T("\\dxgl.ini"));
		file = _tfopen(inipath, _T("r"));
		if (file)
		{
			ini_parse_file(file, ReadINICallback, cfg);
			fclose(file);
		}
	}
}

void SetINISection(HANDLE file, int section)
{
	char buffer[32];
	DWORD buffersize;
	DWORD outsize;
	if (section != ini_currentsection)
	{
		ini_currentsection = section;
		switch (section)
		{
		case INISECTION_SYSTEM:
			strcpy(buffer, "\r\n[system]\r\n");
			break;
		case INISECTION_DISPLAY:
			strcpy(buffer, "\r\n[display]\r\n");
			break;
		case INISECTION_SCALING:
			strcpy(buffer, "\r\n[scaling]\r\n");
			break;
		case INISECTION_POSTPROCESS:
			strcpy(buffer, "\r\n[postprocess]\r\n");
			break;
		case INISECTION_D3D:
			strcpy(buffer, "\r\n[d3d]\r\n");
			break;
		case INISECTION_ADVANCED:
			strcpy(buffer, "\r\n[advanced]\r\n");
			break;
		case INISECTION_DEBUG:
			strcpy(buffer, "\r\n[debug]\r\n");
			break;
		case INISECTION_HACKS:
			strcpy(buffer, "\r\n[hacks]\r\n");
			break;
		default:
			return;
		}
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

void INIWriteBool(HANDLE file, const char *name, BOOL value, BOOL mask, int section)
{
	char buffer[256];
	DWORD buffersize;
	DWORD outsize;
	if (mask)
	{
		SetINISection(file, section);
		strcpy(buffer, name);
		strcat(buffer, "=");
		if (value) strcat(buffer, "true");
		else strcat(buffer, "false");
		strcat(buffer, "\r\n");
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

void INIWriteInt(HANDLE file, const char *name, DWORD value, DWORD mask, int section)
{
	char buffer[256];
	char number[32];
	DWORD buffersize;
	DWORD outsize;
	if (mask)
	{
		SetINISection(file, section);
		strcpy(buffer, name);
		strcat(buffer, "=");
		_itoa(value, number, 10);
		strcat(buffer, number);
		strcat(buffer, "\r\n");
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

void INIWriteHex(HANDLE file, const char *name, DWORD value, DWORD mask, int section)
{
	char buffer[256];
	char number[32];
	DWORD buffersize;
	DWORD outsize;
	if (mask)
	{
		SetINISection(file, section);
		strcpy(buffer, name);
		strcat(buffer, "=0x");
		_itoa(value, number, 16);
		strcat(buffer, number);
		strcat(buffer, "\r\n");
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

void INIWriteFloat(HANDLE file, const char *name, float value, float mask, int digits, int section)
{
	char buffer[256];
	char number[32];
	char floatformat[16];
	DWORD buffersize;
	DWORD outsize;
	if (mask)
	{
		SetINISection(file, section);
		strcpy(buffer, name);
		strcat(buffer, "=");
		_itoa(digits, number, 10);
		strcpy(floatformat, "%.");
		strcat(floatformat, number);
		strcat(floatformat, "g");
		_snprintf(number, 31, floatformat, value);
		number[31] = 0;
		strcat(buffer, number);
		strcat(buffer, "\r\n");
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

void FloatToAspectString(float f, char *aspect)
{
	double integer;
	double dummy;
	double fract;
	char denominator[5];
	int i;
	if (_isnan(f)) f = 0.0f; //Handle NAN condition
	if (f >= 1000.0f)  // Clamp ridiculously wide aspects
	{
		strcpy(aspect, "1000:1");
		return;
	}
	if (f < 0.001f)   // Exclude ridiculously tall aspects, zero, and negative
	{
		strcpy(aspect, "Default");
		return;
	}
	// Handle common aspects
	if (fabs(f - 1.25f) < 0.0001f)
	{
		strcpy(aspect, "5:4");
		return;
	}
	if (fabs(f - 1.3333333f) < 0.0001f)
	{
		strcpy(aspect, "4:3");
		return;
	}
	if (fabs(f - 1.6f) < 0.0001f)
	{
		strcpy(aspect, "16:10");
		return;
	}
	if (fabs(f - 1.7777777) < 0.0001f)
	{
		strcpy(aspect, "16:9");
		return;
	}
	if (fabs(f - 1.9333333) < 0.0001f)
	{
		strcpy(aspect, "256:135");
		return;
	}
	fract = modf(f, &integer);
	if (fract < 0.0001f)  //Handle integer aspects
	{
		_itoa((int)integer, aspect, 10);
		strcat(aspect, ":1");
		return;
	}
	// Finally try from 2 to 1000
	for (i = 2; i < 1000; i++)
	{
		if (fabs(modf(fract*i, &dummy)) < 0.0001f)
		{
			_itoa((int)((f*i) + .5f), aspect, 10);
			_itoa(i, denominator, 10);
			strcat(aspect, ":");
			strcat(aspect, denominator);
			return;
		}
	}
	// Cannot find a reasonable fractional aspect, so display as decimal.
	sprintf(aspect, "%.6g", f);
}

void INIWriteAspect(HANDLE file, const char *name, float value, float mask, int section)
{
	char buffer[256];
	char number[32];
	DWORD buffersize;
	DWORD outsize;
	if (mask)
	{
		SetINISection(file, section);
		strcpy(buffer, name);
		strcat(buffer, "=");
		FloatToAspectString(value,number);
		strcat(buffer, number);
		strcat(buffer, "\r\n");
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

void INIWriteString(HANDLE file, const char *name, const char *value, DWORD mask, int section)
{
	char buffer[512];
	DWORD buffersize;
	DWORD outsize;
	if (mask)
	{
		SetINISection(file, section);
		strcpy(buffer, name);
		strcat(buffer, "=");
		strcat(buffer, value);
		strcat(buffer, "\r\n");
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

void INIWriteTCHARString(HANDLE file, const char *name, LPCTSTR value, DWORD mask, int section)
{
	char buffer[512];
#ifdef _UNICODE
	char unicodebuffer[MAX_PATH + 1];
#endif
	DWORD buffersize;
	DWORD outsize;
	if (mask)
	{
		SetINISection(file, section);
		strcpy(buffer, name);
		strcat(buffer, "=");
#ifdef _UNICODE
		WideCharToMultiByte(CP_UTF8, 0, value, _tcslen(value), unicodebuffer, MAX_PATH, NULL, NULL);
		strcat(buffer, unicodebuffer);
#else
		strcat(buffer, value);
#endif
		strcat(buffer, "\r\n");
		buffersize = (DWORD)strlen(buffer);
		WriteFile(file, buffer, buffersize, &outsize, NULL);
	}
}

DWORD WriteINI(DXGLCFG *cfg, DXGLCFG *mask, LPCTSTR path, HWND hWnd)
{
	Sha256Context sha_context;
	SHA256_HASH sha256;
	char sha256string[65];
	char buffer[512];
	DWORD bytesread;
	HANDLE file, file2;
	TCHAR inipath[MAX_PATH + 10];
	DWORD error;
	int i;
	int answer;
	_tcscpy(inipath, path);
	_tcscat(inipath, _T("\\dxgl.ini"));
	file = CreateFile(inipath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		error = GetLastError();
		if (error == ERROR_FILE_EXISTS)
		{
			answer = MessageBox(hWnd, _T("File already exists.  Do you want to overwrite it?"),
				_T("File exists"), MB_YESNO | MB_ICONQUESTION);
			if (answer == IDNO) return error;
		}
		else return error;
		file = CreateFile(inipath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) return GetLastError();
	}
	ini_currentsection = INISECTION_NULL;
	strcpy(buffer, "; DXGL Configuration file\r\n; This file was generated by DXGL Config.\r\n");
	WriteFile(file, buffer, (DWORD)strlen(buffer), &bytesread, NULL);
	// [system]
	if (cfg->NoWriteRegistry) INIWriteBool(file, "NoWriteRegistry", TRUE, TRUE, INISECTION_SYSTEM);
	if (cfg->OverrideDefaults) INIWriteBool(file, "OverrideDefaults", TRUE, TRUE, INISECTION_SYSTEM);
	if (cfg->NoOverwrite) INIWriteBool(file, "NoOverwrite", TRUE, TRUE, INISECTION_SYSTEM);
	if (cfg->SaveSHA256)
	{
		_tcscpy(inipath, path);
		_tcscat(inipath, _T("\\ddraw.dll"));
		Sha256Initialise(&sha_context);
		file2 = CreateFile(inipath, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file2 != INVALID_HANDLE_VALUE)
		{
			while (1)
			{
				if(!ReadFile(file2, buffer, 512, &bytesread, NULL)) break;
				if (!bytesread) break;
				Sha256Update(&sha_context, buffer, bytesread);
				if (bytesread < 512) break;
			}
			Sha256Finalise(&sha_context, &sha256);
			CloseHandle(file2);
			ZeroMemory(sha256string, 65 * sizeof(char));
			for (i = 0; i < (256 / 8); i++)
			{
				sha256string[i * 2] = hexdigit(sha256.bytes[i] >> 4);
				sha256string[(i * 2) + 1] = hexdigit(sha256.bytes[i] & 0xF);
			}
			strcpy(buffer, "; Do not change the following value!\r\n");
			WriteFile(file, buffer, (DWORD)strlen(buffer), &bytesread, NULL);
			INIWriteString(file, "BundledDDrawSHA256", sha256string, 1, INISECTION_SYSTEM);
		}
		else MessageBox(hWnd, _T("Cannot read ddraw.dll, skipping SHA-256 checksum"),
				_T("Warning"), MB_OK | MB_ICONWARNING);
	}
	if (cfg->NoUninstall) INIWriteBool(file, "NoUninstall", TRUE, TRUE, INISECTION_SYSTEM);
	// [display]
	INIWriteInt(file, "ScalingMode", cfg->scaler, mask->scaler, INISECTION_DISPLAY);
	INIWriteInt(file, "FullscreenWindowMode", cfg->fullmode, mask->fullmode, INISECTION_DISPLAY);
	INIWriteBool(file, "ChangeColorDepth", cfg->colormode, mask->colormode, INISECTION_DISPLAY);
	INIWriteInt(file, "AddColorDepths", cfg->AddColorDepths, mask->AddColorDepths, INISECTION_DISPLAY);
	INIWriteInt(file, "AddModes", cfg->AddModes, mask->AddModes, INISECTION_DISPLAY);
	INIWriteInt(file, "SortModes", cfg->SortModes, mask->SortModes, INISECTION_DISPLAY);
	INIWriteInt(file, "VSync", cfg->vsync, mask->vsync, INISECTION_DISPLAY);
	INIWriteInt(file, "CustomResolutionX", cfg->CustomResolutionX, mask->CustomResolutionX, INISECTION_DISPLAY);
	INIWriteInt(file, "CustomResolutionY", cfg->CustomResolutionY, mask->CustomResolutionY, INISECTION_DISPLAY);
	INIWriteInt(file, "CustomRefresh", cfg->CustomRefresh, mask->CustomRefresh, INISECTION_DISPLAY);
	INIWriteFloat(file, "DisplayMultiplierX", cfg->DisplayMultiplierX, mask->DisplayMultiplierX, 4, INISECTION_DISPLAY);
	INIWriteFloat(file, "DisplayMultiplierY", cfg->DisplayMultiplierY, mask->DisplayMultiplierY, 4, INISECTION_DISPLAY);
	INIWriteInt(file, "SingleBufferDevice", cfg->SingleBufferDevice, mask->SingleBufferDevice, INISECTION_DISPLAY);
	INIWriteBool(file, "UseSetDisplayConfig", cfg->UseSetDisplayConfig, mask->UseSetDisplayConfig, INISECTION_DISPLAY);
	// [scaling]
	INIWriteInt(file, "ScalingFilter", cfg->scalingfilter, mask->scalingfilter, INISECTION_SCALING);
	INIWriteInt(file, "BltScale", cfg->BltScale, mask->BltScale, INISECTION_SCALING);
	// Option was temporarily removed for DXGL 0.5.13 release
	//INIWriteInt(file, "BltThreshold", cfg->BltThreshold, mask->BltThreshold, INISECTION_SCALING);
	INIWriteInt(file, "AdjustPrimaryResolution", cfg->primaryscale, mask->primaryscale, INISECTION_SCALING);
	INIWriteFloat(file, "PrimaryScaleX", cfg->primaryscalex, mask->primaryscalex, 4, INISECTION_SCALING);
	INIWriteFloat(file, "PrimaryScaleY", cfg->primaryscaley, mask->primaryscaley, 4, INISECTION_SCALING);
	INIWriteFloat(file, "WindowScaleX", cfg->WindowScaleX, mask->WindowScaleX, 4, INISECTION_SCALING);
	INIWriteFloat(file, "WindowScaleY", cfg->WindowScaleY, mask->WindowScaleY, 4, INISECTION_SCALING);
	INIWriteAspect(file, "ScreenAspect", cfg->aspect, mask->aspect, INISECTION_SCALING);
	INIWriteInt(file, "DPIScale", cfg->DPIScale, mask->DPIScale, INISECTION_SCALING);
	// [postprocess]
	INIWriteInt(file, "PostprocessFilter", cfg->postfilter, mask->postfilter, INISECTION_POSTPROCESS);
	INIWriteFloat(file, "PostprocessScaleX", cfg->postsizex, mask->postsizex, 4, INISECTION_POSTPROCESS);
	INIWriteFloat(file, "PostprocessScaleY", cfg->postsizey, mask->postsizey, 4, INISECTION_POSTPROCESS);
	INIWriteInt(file, "EnableShader", cfg->EnableShader, mask->EnableShader, INISECTION_POSTPROCESS);
	INIWriteTCHARString(file, "ShaderFile", cfg->shaderfile, mask->shaderfile[0], INISECTION_POSTPROCESS);
	// [d3d]
	INIWriteInt(file, "TextureFilter", cfg->texfilter, mask->texfilter, INISECTION_D3D);
	INIWriteInt(file, "AnisotropicFiltering", cfg->anisotropic, mask->anisotropic, INISECTION_D3D);
	INIWriteHex(file, "Antialiasing", cfg->msaa, mask->msaa, INISECTION_D3D);
	INIWriteInt(file, "D3DAspect", cfg->aspect3d, mask->aspect3d, INISECTION_D3D);
	INIWriteInt(file, "LowColorRendering", cfg->LowColorRendering, mask->LowColorRendering, INISECTION_D3D);
	INIWriteInt(file, "EnableDithering", cfg->EnableDithering, mask->EnableDithering, INISECTION_D3D);
	INIWriteInt(file, "LimitTextureFormats", cfg->LimitTextureFormats, mask->LimitTextureFormats, INISECTION_D3D);
	// [advanced]
	INIWriteInt(file, "TextureFormat", cfg->TextureFormat, mask->TextureFormat, INISECTION_ADVANCED);
	INIWriteInt(file, "TexUpload", cfg->TexUpload, mask->TexUpload, INISECTION_ADVANCED);
	INIWriteInt(file, "WindowPosition", cfg->WindowPosition, mask->WindowPosition, INISECTION_ADVANCED);
	INIWriteBool(file, "RememberWindowSize", cfg->RememberWindowSize, mask->RememberWindowSize, INISECTION_ADVANCED);
	INIWriteBool(file, "RememberWindowPosition", cfg->RememberWindowPosition, mask->RememberWindowPosition, INISECTION_ADVANCED);
	INIWriteBool(file, "NoResizeWindow", cfg->NoResizeWindow, mask->NoResizeWindow, INISECTION_ADVANCED);
	INIWriteInt(file, "WindowX", cfg->WindowX, mask->WindowX, INISECTION_ADVANCED);
	INIWriteInt(file, "WindowY", cfg->WindowY, mask->WindowY, INISECTION_ADVANCED);
	INIWriteInt(file, "WindowWidth", cfg->WindowWidth, mask->WindowWidth, INISECTION_ADVANCED);
	INIWriteInt(file, "WindowHeight", cfg->WindowHeight, mask->WindowHeight, INISECTION_ADVANCED);
	INIWriteBool(file, "WindowMaximized", cfg->WindowMaximized, mask->WindowMaximized, INISECTION_ADVANCED);
	INIWriteBool(file, "CaptureMouse", cfg->CaptureMouse, mask->CaptureMouse, INISECTION_ADVANCED);
	// [debug]
	INIWriteBool(file, "DebugNoExtFramebuffer", cfg->DebugNoExtFramebuffer, mask->DebugNoExtFramebuffer, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoArbFramebuffer", cfg->DebugNoArbFramebuffer, mask->DebugNoArbFramebuffer, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoES2Compatibility", cfg->DebugNoES2Compatibility, mask->DebugNoES2Compatibility, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoExtDirectStateAccess", cfg->DebugNoExtDirectStateAccess, mask->DebugNoExtDirectStateAccess, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoArbDirectStateAccess", cfg->DebugNoArbDirectStateAccess, mask->DebugNoArbDirectStateAccess, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoSamplerObjects", cfg->DebugNoSamplerObjects, mask->DebugNoSamplerObjects, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoGpuShader4", cfg->DebugNoGpuShader4, mask->DebugNoGpuShader4, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoGLSL130", cfg->DebugNoGLSL130, mask->DebugNoGLSL130, INISECTION_DEBUG);
	INIWriteBool(file, "DebugUnloadAfterUnlock", cfg->DebugUploadAfterUnlock, mask->DebugUploadAfterUnlock, INISECTION_DEBUG);
	INIWriteBool(file, "DebugBlendDestColorKey", cfg->DebugBlendDestColorKey, mask->DebugBlendDestColorKey, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoMouseHooks", cfg->DebugNoMouseHooks, mask->DebugNoMouseHooks, INISECTION_DEBUG);
	INIWriteBool(file, "DebugNoPaletteRedraw", cfg->DebugNoPaletteRedraw, mask->DebugNoPaletteRedraw, INISECTION_DEBUG);
	INIWriteBool(file, "DebugMaxGLVersionMajor", cfg->DebugMaxGLVersionMajor, mask->DebugMaxGLVersionMajor, INISECTION_DEBUG);
	INIWriteBool(file, "DebugMaxGLVersionMinor", cfg->DebugMaxGLVersionMinor, mask->DebugMaxGLVersionMinor, INISECTION_DEBUG);
	INIWriteBool(file, "DebugTraceLevel", cfg->DebugTraceLevel, mask->DebugTraceLevel, INISECTION_DEBUG);
	// [hacks]
	INIWriteBool(file, "HackCrop640480to640400", cfg->HackCrop640480to640400, mask->HackCrop640480to640400, INISECTION_HACKS);
	INIWriteInt(file, "HackAutoExpandViewport", cfg->HackAutoExpandViewport, mask->HackAutoExpandViewport, INISECTION_HACKS);
	INIWriteInt(file, "HackAutoExpandViewportCompare", cfg->HackAutoExpandViewportCompare, mask->HackAutoExpandViewportCompare, INISECTION_HACKS);
	INIWriteHex(file, "HackAutoExpandViewportValue", cfg->HackAutoExpandViewportValue, mask->HackAutoExpandViewportValue, INISECTION_HACKS);
	INIWriteBool(file, "HackNoTVRefresh", cfg->HackNoTVRefresh, mask->HackNoTVRefresh, INISECTION_HACKS);
	INIWriteBool(file, "HackSetCursor", cfg->HackSetCursor, mask->HackSetCursor, INISECTION_HACKS);
	INIWriteInt(file, "HackPaletteDelay", cfg->HackPaletteDelay, mask->HackPaletteDelay, INISECTION_HACKS);
	INIWriteBool(file, "HackPaletteVsync", cfg->HackPaletteVsync, mask->HackPaletteVsync, INISECTION_HACKS);
	INIWriteInt(file, "VertexBufferSize", cfg->VertexBufferSize, mask->VertexBufferSize, INISECTION_HACKS);
	INIWriteInt(file, "IndexBufferSize", cfg->IndexBufferSize, mask->IndexBufferSize, INISECTION_HACKS);
	INIWriteInt(file, "UnpackBufferSize", cfg->UnpackBufferSize, mask->UnpackBufferSize, INISECTION_HACKS);
	INIWriteInt(file, "CmdBufferSize", cfg->CmdBufferSize, mask->CmdBufferSize, INISECTION_HACKS);
	INIWriteInt(file, "MaxSpinCount", cfg->MaxSpinCount, mask->MaxSpinCount, INISECTION_HACKS);
	CloseHandle(file);
	return ERROR_SUCCESS;
}

void GetCurrentConfig(DXGLCFG *cfg, BOOL initial)
{
	HKEY hKey;
	Sha256Context sha_context;
	SHA256_HASH sha256;
	TCHAR sha256string[65];
	TCHAR filename[MAX_PATH+1];
	WCHAR filename2[MAX_PATH+1];
	TCHAR regkey[MAX_PATH + 80];
	size_t i;
	BOOL DPIAwarePM = FALSE;
	HMODULE hSHCore = NULL;
	HMODULE hUser32 = NULL;
	HRESULT(WINAPI *_SetProcessDpiAwareness)(DWORD value);
	BOOL(WINAPI *_SetProcessDpiAwarenessContext)(HANDLE value) = NULL;
	BOOL(WINAPI *_SetProcessDPIAware)();
	GetModuleFileName(NULL, filename, MAX_PATH);
	_tcscpy(regkey, regkeybase);
	_tcscat(regkey, profilesname);
	for (i = _tcslen(filename); (i > 0) && (filename[i] != 92) && (filename[i] != 47); i--);
	i++;
	_tcscat(regkey, &filename[i]);
	_tcscat(regkey, _T("-"));
	i--;
	filename[i] = 0;
	_tcslwr(filename);
	_tchartowchar(filename2, filename, -1);
	Sha256Initialise(&sha_context);
	Sha256Update(&sha_context, filename2, (uint32_t)wcslen(filename2) * sizeof(WCHAR));
	Sha256Finalise(&sha_context, &sha256);
	for (i = 0; i < (256 / 8); i++)
	{
		sha256string[i * 2] = (TCHAR)hexdigit(sha256.bytes[i] >> 4);
		sha256string[(i * 2) + 1] = (TCHAR)hexdigit(sha256.bytes[i] & 0xF);
	}
	sha256string[256 / 4] = 0;
	_tcscat(regkey, sha256string);
	GetGlobalConfig(cfg, initial);
	_tcscpy(cfg->regkey, regkey);
	ReadINI(cfg);
	if (cfg->OverrideDefaults)
	{
		GetDefaultConfig(cfg);
		ReadINI(cfg);
	}
	if (initial || cfg->NoWriteRegistry) RegOpenKeyEx(HKEY_CURRENT_USER, cfg->regkey, 0, KEY_READ, &hKey);
	else
	{
		RegCreateKeyEx(HKEY_CURRENT_USER, regkeyglobal, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
		if (hKey) RegCloseKey(hKey);
		RegCreateKeyEx(HKEY_CURRENT_USER, cfg->regkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	}
	if (hKey)
	{
		ReadSettings(hKey, cfg, NULL, FALSE, TRUE, NULL);
		RegCloseKey(hKey);
	}
	hKey = NULL;
	if (cfg->DPIScale == 2)	AddCompatFlag(_T("HIGHDPIAWARE"));
	else DelCompatFlag(_T("HIGHDPIAWARE"),initial);
	if (initial)
	{
		switch(cfg->DPIScale)
		{
		case 1:  // Per-monitor DPI Aware V1
			hSHCore = LoadLibrary(_T("SHCore.dll"));
			if (hSHCore)
			{
				_SetProcessDpiAwareness	= 
					(HRESULT(WINAPI*)(DWORD))GetProcAddress(hSHCore, "SetProcessDpiAwareness");
				if (_SetProcessDpiAwareness)
				{
					DPIAwarePM = TRUE;
					_SetProcessDpiAwareness(2);
				}
			}
			if (!DPIAwarePM)
			{
				hUser32 = LoadLibrary(_T("User32.dll"));
				if (hUser32)
				{
					_SetProcessDPIAware	= 
						(BOOL(WINAPI*)())GetProcAddress(hUser32, "SetProcessDPIAware");
					if (_SetProcessDPIAware) _SetProcessDPIAware();
				}
			}
			if (hSHCore) FreeLibrary(hSHCore);
			if (hUser32) FreeLibrary(hUser32);
			break;
		case 3:  // System DPI Aware
			hSHCore = LoadLibrary(_T("SHCore.dll"));
			if (hSHCore)
			{
				_SetProcessDpiAwareness = 
					(HRESULT(WINAPI*)(DWORD))GetProcAddress(hSHCore, "SetProcessDpiAwareness");
				if (_SetProcessDpiAwareness)
				{
					DPIAwarePM = TRUE;
					_SetProcessDpiAwareness(1);
				}
			}
			if (!DPIAwarePM)
			{
				hUser32 = LoadLibrary(_T("User32.dll"));
				if (hUser32)
				{
					_SetProcessDPIAware = 
						(BOOL(WINAPI*)())GetProcAddress(hUser32, "SetProcessDPIAware");
					if (_SetProcessDPIAware) _SetProcessDPIAware();
				}
			}
			if (hSHCore) FreeLibrary(hSHCore);
			if (hUser32) FreeLibrary(hUser32);
			break;
		case 4:  // Per-monitor DPI Aware V2
			hUser32 = LoadLibrary(_T("User32.dll"));
			if (hUser32)
			{
				_SetProcessDpiAwarenessContext = 
					(BOOL(WINAPI*)(HANDLE))GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
				if (_SetProcessDpiAwarenessContext) _SetProcessDpiAwarenessContext((HANDLE)-4);
			}
			if (!_SetProcessDpiAwarenessContext)
			{
				hSHCore = LoadLibrary(_T("SHCore.dll"));
				if (hSHCore)
				{
					_SetProcessDpiAwareness =
						(HRESULT(WINAPI*)(DWORD))GetProcAddress(hSHCore, "SetProcessDpiAwareness");
					if (_SetProcessDpiAwareness)
					{
						DPIAwarePM = TRUE;
						_SetProcessDpiAwareness(2);
					}
				}
				if (!DPIAwarePM)
				{
					if (!hUser32) hUser32 = LoadLibrary(_T("User32.dll"));
					if (hUser32)
					{
						_SetProcessDPIAware =
							(BOOL(WINAPI*)())GetProcAddress(hUser32, "SetProcessDPIAware");
						if (_SetProcessDPIAware) _SetProcessDPIAware();
					}
				}
			}
			if (hSHCore) FreeLibrary(hSHCore);
			if (hUser32) FreeLibrary(hUser32);
			break;
		default:
			break;
		}
	}
	//if(!cfg->colormode) DelCompatFlag(_T("DWM8And16BitMitigation"), initial);  // Windows 10 compatibility issues; not needed?
}
void GetGlobalConfig(DXGLCFG *cfg, BOOL initial)
{
	HKEY hKey = NULL;
	GetDefaultConfig(cfg);
	RegOpenKeyEx(HKEY_CURRENT_USER, regkeyglobal, 0, KEY_READ, &hKey);
	if (hKey)
	{
		ReadSettings(hKey, cfg, NULL, TRUE, FALSE, NULL);
		RegCloseKey(hKey);
	}
}

void GetGlobalConfigWithMask(DXGLCFG *cfg, DXGLCFG *mask, BOOL initial)
{
	DWORD WindowXMask, WindowYMask, WindowWidthMask, WindowHeightMask;
	HKEY hKey = NULL;
	GetDefaultConfig(cfg);
	RegOpenKeyEx(HKEY_CURRENT_USER, regkeyglobal, 0, KEY_READ, &hKey);
	if (hKey)
	{
		ReadSettings(hKey, cfg, mask, TRUE, FALSE, NULL);
		RegCloseKey(hKey);
	}
	// Set mask on optional components
	if (mask)
	{
		WindowXMask = mask->WindowX;
		WindowYMask = mask->WindowY;
		WindowWidthMask = mask->WindowWidth;
		WindowHeightMask = mask->WindowHeight;
		memset(mask, 0xff, sizeof(DXGLCFG));
		mask->WindowX = WindowXMask;
		mask->WindowY = WindowYMask;
		mask->WindowWidth = WindowWidthMask;
		mask->WindowHeight = WindowHeightMask;
	}
}


void GetConfig(DXGLCFG *cfg, DXGLCFG *mask, LPCTSTR name)
{
	HKEY hKey;
	TCHAR regkey[MAX_PATH + 80];
	_tcscpy(regkey,regkeybase);
	_tcscat(regkey, profilesname);
	_tcsncat(regkey, name, MAX_PATH);
	ZeroMemory(cfg,sizeof(DXGLCFG));
	cfg->DPIScale = 1;
	RegCreateKeyEx(HKEY_CURRENT_USER,regkey,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	ReadSettings(hKey,cfg,mask,FALSE,FALSE,NULL);
	RegCloseKey(hKey);
}
void SetGlobalConfig(const DXGLCFG *cfg, const DXGLCFG *mask)
{
	HKEY hKey;
	RegCreateKeyEx(HKEY_CURRENT_USER,regkeyglobal,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
	WriteSettings(hKey,cfg,mask);
	RegCloseKey(hKey);
}

void SetConfig(const DXGLCFG *cfg, const DXGLCFG *mask, LPCTSTR name)
{
	HKEY hKey;
	TCHAR regkey[MAX_PATH + 80];
	_tcscpy(regkey, regkeybase);
	_tcscat(regkey, profilesname);
	_tcsncat(regkey, name, MAX_PATH);
	RegCreateKeyEx(HKEY_CURRENT_USER, regkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	WriteSettings(hKey,cfg,mask);
	RegCloseKey(hKey);
}

void SaveWindowSettings(const DXGLCFG *cfg)
{
	HKEY hKey;
	if ((!cfg->RememberWindowSize) && (!cfg->RememberWindowPosition)) return;
	RegCreateKeyEx(HKEY_CURRENT_USER, cfg->regkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (hKey)
	{
		if (cfg->RememberWindowPosition)
		{
			WriteDWORD(hKey, cfg->WindowX, 1, _T("WindowX"));
			WriteDWORD(hKey, cfg->WindowY, 1, _T("WindowY"));
		}
		if (cfg->RememberWindowSize)
		{
			WriteDWORD(hKey, cfg->WindowWidth, 1, _T("WindowWidth"));
			WriteDWORD(hKey, cfg->WindowHeight, 1, _T("WindowHeight"));
			WriteDWORD(hKey, cfg->WindowMaximized, 1, _T("WindowMaximized"));
		}
		RegCloseKey(hKey);
	}
}

//  Checks for obsolete DXGL Test registry entry and renames it to DXGLCFG.

void UpgradeDXGLTestToDXGLCfg()
{
	Sha256Context sha_context;
	SHA256_HASH sha256;
	TCHAR sha256string[65];
	TCHAR installpath[MAX_PATH + 1];
	WCHAR installpath2[MAX_PATH + 1];
	TCHAR profilepath[MAX_PATH + 80];
	TCHAR destpath[MAX_PATH + 80];
	LONG error;
	DWORD sizeout = (MAX_PATH + 1) * sizeof(TCHAR);
	DWORD sizeout2;
	HKEY hKeyInstall = NULL;
	HKEY hKeyProfile = NULL;
	HKEY hKeyDest = NULL;
	DWORD numvalue;
	DWORD olddirsize = MAX_PATH*4;
	TCHAR *olddir = NULL;
	DWORD oldvaluesize = 1024;
	TCHAR *oldvalue = NULL;
	DWORD regtype;
	int i;
	error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
	if (error == ERROR_SUCCESS)
	{
		#ifdef _M_X64
		error = RegQueryValueEx(hKeyInstall, _T("InstallDir_x64"), NULL, NULL, (LPBYTE)installpath, &sizeout);
		#else
		error = RegQueryValueEx(hKeyInstall, _T("InstallDir"), NULL, NULL, (LPBYTE)installpath, &sizeout);
		#endif
		if (error == ERROR_SUCCESS)
		{
			_tcslwr(installpath);
			_tchartowchar(installpath2, installpath, -1);
			Sha256Initialise(&sha_context);
			Sha256Update(&sha_context, installpath2, (uint32_t)wcslen(installpath2) * sizeof(WCHAR));
			Sha256Finalise(&sha_context, &sha256);
			for (i = 0; i < (256 / 8); i++)
			{
				sha256string[i * 2] = (TCHAR)hexdigit(sha256.bytes[i] >> 4);
				sha256string[(i * 2) + 1] = (TCHAR)hexdigit(sha256.bytes[i] & 0xF);
			}
			sha256string[256 / 4] = 0;
			#ifdef _M_X64
			_tcscpy(profilepath, _T("Software\\DXGL\\Profiles_x64\\dxgltest.exe-"));
			_tcscat(profilepath, sha256string);
			_tcscpy(destpath, _T("Software\\DXGL\\Profiles_x64\\dxglcfg.exe-"));
			#else
			_tcscpy(profilepath, _T("Software\\DXGL\\Profiles\\dxgltest.exe-"));
			_tcscat(profilepath, sha256string);
			_tcscpy(destpath, _T("Software\\DXGL\\Profiles\\dxglcfg.exe-"));
			#endif
			_tcscat(destpath, sha256string);
			error = RegOpenKeyEx(HKEY_CURRENT_USER, profilepath, 0, KEY_READ, &hKeyProfile);
			if (error == ERROR_SUCCESS)
			{
				error = RegOpenKeyEx(HKEY_CURRENT_USER, destpath, 0, KEY_READ, &hKeyDest);
				if (error == ERROR_SUCCESS)  // Clear spurious DXGLCfg entry
				{
					RegCloseKey(hKeyDest);
					RegDeleteKey(HKEY_CURRENT_USER, destpath);
				}
				// Copy over to new key
				error = RegCreateKeyEx(HKEY_CURRENT_USER, destpath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKeyDest, NULL);
				if (error == ERROR_SUCCESS)
				{
					olddir = malloc(olddirsize);
					oldvalue = malloc(oldvaluesize);
					numvalue = 0;
					do
					{
						sizeout = olddirsize;
						sizeout2 = oldvaluesize;
						error = RegEnumValue(hKeyProfile, numvalue, olddir, &sizeout, NULL, &regtype, (LPBYTE)oldvalue, &sizeout2);
						if (error == ERROR_MORE_DATA)
						{
							if (sizeout > olddirsize)
							{
								olddirsize = sizeout;
								olddir = realloc(olddir, olddirsize);
								if (!olddir)
								{
									MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
									ExitProcess(error);
								}
							}
							if (sizeout2 > oldvaluesize)
							{
								oldvaluesize = sizeout2;
								oldvalue = realloc(oldvalue, oldvaluesize);
								if (!oldvalue)
								{
									MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
									ExitProcess(error);
								}
							}
							sizeout = olddirsize;
							sizeout2 = oldvaluesize;
							error = RegEnumValue(hKeyProfile, numvalue, olddir, &sizeout, NULL, &regtype, (LPBYTE)oldvalue, &sizeout2);
						}
						if (error == ERROR_SUCCESS)
						{
							if (_tcsnicmp(olddir, _T("InstallPaths"), sizeout))
								RegSetValueEx(hKeyDest, olddir, 0, regtype, (LPBYTE)oldvalue, sizeout2);
						}
						numvalue++;
					} while (error == ERROR_SUCCESS);
					RegCloseKey(hKeyDest);
					free(olddir);
					free(oldvalue);
				}
				// Delete old key
				RegCloseKey(hKeyProfile);
				RegDeleteKey(HKEY_CURRENT_USER, profilepath);
			}
		}
	}
	if (hKeyInstall) RegCloseKey(hKeyInstall);
}


void FatalMemoryError(int code)
{
	MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
	ExitProcess(code);
}

void FatalRegistryError(int code)
{
	MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
	ExitProcess(code);
}

void UpgradeProfile0to1(HKEY hKey)
{
	// Upgrades from the initial registry format which used EXE checksums
	// to settings format 1 which uses directory hashes and the Profiles subkey.
	// This format was first introduced in DXGL 0.5.9.
	TCHAR olddir[MAX_PATH];
	TCHAR *subkeyin = NULL;
	DWORD subkeyinsize = 0;
	TCHAR *subkeyout = NULL;
	DWORD subkeyoutsize = 0;
	TCHAR *valuename;
	TCHAR *tmpptr = NULL;
	LONG error;
	HKEY hKeyProfiles;
	HKEY hKeyProfileInput;
	HKEY hKeyProfileOutput;
	TCHAR *strbuffer = NULL;
	TCHAR *strptr;
	DWORD strbuffersize = 0;
	DWORD regtype;
	DWORD retsize, retsize2;
	TCHAR exename[(MAX_PATH*2) + 9];
	DWORD size;
	DWORD crc, crccmp;
	BOOL nocrc;
	FILE *file;
	Sha256Context sha_context;
	SHA256_HASH sha_hash;
	TCHAR sha_hash_text[65];
	wchar_t unicodepath[MAX_PATH];
	int keycount = 0;
	int valuecount = 0;
	int profileindex;
	int settingindex;
	int i;
	BYTE *valuebuffer = NULL;
	DWORD valuebuffersize = 0;
#ifdef _M_X64
	return; // Format didn't exist in x64 builds
#endif
	// Allocate value buffer
	valuename = (TCHAR*)malloc(32768);
	if(!valuename) FatalRegistryError(error);
	// Open the output registry key
	error = RegCreateKeyEx(hKey, _T("Profiles"), NULL, NULL, 0,
		KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);
	if (error != ERROR_SUCCESS) FatalRegistryError(error);
	// Get number of subkeys in HKCU\DXGL
	RegQueryInfoKey(hKey, NULL, NULL, NULL, &keycount, &subkeyinsize,
		NULL, NULL, NULL, NULL, NULL, NULL);
	// Allocate buffers for profile subkey names
	if (keycount)
	{
		subkeyinsize++;
		subkeyinsize *= sizeof(TCHAR);
		subkeyin = (TCHAR*)malloc(subkeyinsize);
		if (!subkeyin) FatalMemoryError(ERROR_NOT_ENOUGH_MEMORY);
		subkeyoutsize = subkeyinsize + (65 * sizeof(TCHAR));
		subkeyout = (TCHAR*)malloc(subkeyoutsize);
		if (!subkeyout) FatalMemoryError(ERROR_NOT_ENOUGH_MEMORY);
	}
	for (profileindex = 0; profileindex < keycount; profileindex++)
	{
		retsize = subkeyinsize;
		// Get next subkey
		error = RegEnumKeyEx(hKey,profileindex,subkeyin,&retsize,
			NULL, NULL, NULL, NULL);
		if (error == ERROR_SUCCESS)
		{
			// If subkey is Profiles or Global skip.
			if (_tcsicmp(subkeyin, _T("Profiles")) && _tcsicmp(subkeyin, _T("Global")) &&
				_tcsicmp(subkeyin, _T("Profiles_x64")) && _tcsicmp(subkeyin, _T("Global_x64")))
			{
				// Attempt to open subkey
				error = RegOpenKeyEx(hKey, subkeyin, 0, KEY_READ, &hKeyProfileInput);
				if (error == ERROR_SUCCESS)
				{
					// Get install paths value
					error = RegQueryValueEx(hKeyProfileInput, _T("InstallPaths"), NULL,
						&regtype, NULL, &retsize);
					if (error == ERROR_SUCCESS)
					{
						// Allocate buffer and read paths value
						if (retsize > strbuffersize)
						{
							strbuffersize = retsize;
							if (strbuffer) tmpptr = (TCHAR*)realloc(strbuffer, strbuffersize);
							else tmpptr = (TCHAR*)malloc(strbuffersize);
							if (!tmpptr) FatalMemoryError(ERROR_NOT_ENOUGH_MEMORY);
							strbuffer = tmpptr;
						}
						retsize = strbuffersize;
						error = RegQueryValueEx(hKeyProfileInput, _T("InstallPaths"), NULL,
							&regtype, (LPBYTE)strbuffer,&retsize);
						if (error == ERROR_SUCCESS)
						{
							// Parse path value
							strptr = strbuffer;
							do
							{
								size = _tcslen(strptr);
								if (!size) break; // At end of multi_sz
								// Get executable CRC
								if (!_tcsicmp(subkeyin, _T("DXGLTestApp")))
								{
									crc = 0;
									nocrc = TRUE;
								}
								else
								{
									_tcscpy(exename, strptr);
									_tcscat(exename, _T("\\"));
									_tcscat(exename, subkeyin);
									tmpptr = _tcsrchr(exename, _T('-'));
									if (!tmpptr) break; // Corrupt registry key, skip
									*tmpptr = 0;
									file = _tfopen(exename, _T("rb"));
									if (!file)
									{
										strptr += size + 1;
										continue; // Try next path
									}
									Crc32_ComputeFile(file, &crccmp);
									crc = _tcstoul((TCHAR*)(tmpptr + 1), NULL, 16);
									fclose(file);
									if (crc != crccmp)
									{
										strptr += size + 1;
										continue; // CRC mismatch, skip to next path
									}
									nocrc = FALSE;
								}
								_tchartowchar(unicodepath, strptr, -1);
								wcslwr(unicodepath);
								Sha256Initialise(&sha_context);
								// Use buggy pre-0.5.17 path lengths for this pass, this will be fixed later
								Sha256Update(&sha_context, unicodepath, wcslen(unicodepath));
								Sha256Finalise(&sha_context, &sha_hash);
								for (i = 0; i < (256 / 8); i++)
								{
									sha_hash_text[i * 2] = (TCHAR)hexdigit(sha_hash.bytes[i] >> 4);
									sha_hash_text[(i * 2) + 1] = (TCHAR)hexdigit(sha_hash.bytes[i] & 0xF);
								}
								sha_hash_text[256 / 4] = 0;
								// Create destination key
								if (nocrc) _tcscpy(subkeyout, _T("dxgltest.exe-"));
								else
								{
									_tcscpy(subkeyout, subkeyin);
									tmpptr = _tcsrchr(subkeyout, _T('-'));
									tmpptr[1] = 0;
								}
								_tcscat(subkeyout, sha_hash_text);
								error = RegCreateKeyEx(hKeyProfiles, subkeyout, 0, NULL, 0,
									KEY_ALL_ACCESS, NULL, &hKeyProfileOutput, NULL);
								if (error != ERROR_SUCCESS) FatalRegistryError(error);
								// Copy over settings
								RegQueryInfoKey(hKeyProfileInput, NULL, NULL, NULL,
									NULL, NULL, NULL, &valuecount, NULL, &retsize, NULL, NULL);
								if (retsize > valuebuffersize)
								{
									if (retsize < 16) retsize = 16;
									if (!valuebuffer) tmpptr = (BYTE *)malloc(retsize);
									else tmpptr = (BYTE*)realloc(valuebuffer, retsize);
									if (!tmpptr) FatalMemoryError(ERROR_NOT_ENOUGH_MEMORY);
									valuebuffer = tmpptr;
									valuebuffersize = retsize;
								}
								// Copy over values
								for (settingindex = 0; settingindex < valuecount; settingindex++)
								{
									retsize = 32767;
									retsize2 = valuebuffersize;
									RegEnumValue(hKeyProfileInput, settingindex, valuename,
										&retsize, NULL, &regtype, valuebuffer, &retsize2);
									if (!_tcsicmp(valuename, _T("InstallPaths")))
									{
										// Substitute current REG_MULT_SZ path
										_tcscpy(valuename, _T("InstallPath"));
										_tcscpy(valuebuffer, strptr);
										retsize2 = (_tcslen(valuebuffer) + 1) * sizeof(TCHAR);
									}
									RegSetValueEx(hKeyProfileOutput, valuename, NULL, regtype, valuebuffer, retsize2);
								}
								RegCloseKey(hKeyProfileOutput);
								// Index to next element
								strptr += size+1;
							} while(1);
						}
					}
					RegCloseKey(hKeyProfileInput);
				}
			}
		}
	}
	// Delete old keys
	profileindex = 0;
	do {
		retsize = subkeyinsize;
		// Get next subkey
		error = RegEnumKeyEx(hKey, profileindex, subkeyin, &retsize,
			NULL, NULL, NULL, NULL);
		if (error == ERROR_SUCCESS)
		{
			// If subkey is Profiles or Global skip.
			if (_tcsicmp(subkeyin, _T("Profiles")) && _tcsicmp(subkeyin, _T("Global")) &&
				_tcsicmp(subkeyin, _T("Profiles_x64")) && _tcsicmp(subkeyin, _T("Global_x64")))
				RegDeleteKey(hKey, subkeyin);
			else profileindex++;
		}
		else if (error == ERROR_NO_MORE_ITEMS) break;
		else profileindex++;
	} while (1);
	RegCloseKey(hKeyProfiles);
	retsize = 1;
	RegSetValueEx(hKey, configversion, 0, REG_DWORD, (BYTE*)&retsize, 4);
	if (valuebuffer) free(valuebuffer);
	if (strbuffer) free(strbuffer);
	if (subkeyout) free(subkeyout);
	if (subkeyin) free(subkeyin);
	if (valuename) free(valuename);
}

void UpgradeProfile1to2(HKEY hKey)
{
	// Changes the AddColorDepths value to ensure the default of
	// 8, 16, and 32-bit modes added.
	// This change was originally implmented in DXGL 0.5.13.
	DWORD sizeout;
	DWORD regtype;
	DWORD numvalue;
	DWORD keycount;
	int profileindex;
	LONG error;
	HKEY hKeyProfiles;
	HKEY hKeyProfileInput;
	TCHAR profilespath[MAX_PATH];
	_tcscpy(profilespath, globalname);
	// Start with the global profile
	error = RegCreateKeyEx(hKey, globalname, 0, NULL,0, KEY_ALL_ACCESS,
		NULL, &hKeyProfileInput, NULL);
	if (error == ERROR_SUCCESS)
	{
		sizeout = 4;
		regtype = REG_DWORD;
		error = RegQueryValueEx(hKeyProfileInput, _T("AddColorDepths"), NULL,
			&regtype, (LPBYTE)&numvalue, &sizeout);
		if (error == ERROR_SUCCESS)
		{
			// Ensure 8, 16, and 32-bit modes are added
			numvalue |= (1 | 4 | 16);
			RegSetValueEx(hKeyProfileInput, _T("AddColorDepths"), 0,
				REG_DWORD, (LPBYTE)&numvalue, 4);
		}
		RegCloseKey(hKeyProfileInput);
	}
	_tcscpy(profilespath, profilesname);
	profilespath[_tcslen(profilespath) - 1] = 0;
	// Open Profiles path
	error = RegOpenKeyEx(hKey, profilespath, 0, KEY_ALL_ACCESS, &hKeyProfiles);
	if (error == ERROR_SUCCESS)
	{
		error = RegQueryInfoKey(hKeyProfiles,NULL,NULL,NULL,&keycount,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		for (profileindex = 0; profileindex < keycount; profileindex++)
		{
			sizeout = MAX_PATH * sizeof(TCHAR);
			error = RegEnumKeyEx(hKeyProfiles, profileindex, profilespath, &sizeout,
				NULL, NULL, NULL, NULL);
			if (error == ERROR_SUCCESS)
			{
				// Open profile key
				error = RegOpenKeyEx(hKeyProfiles, profilespath, 0, KEY_ALL_ACCESS, &hKeyProfileInput);
				if (error == ERROR_SUCCESS)
				{
					error = RegQueryValueEx(hKeyProfileInput, _T("AddColorDepths"), NULL,
						&regtype, (LPBYTE)&numvalue, &sizeout);
					if(error == ERROR_SUCCESS)
					{
						// Ensure 8, 16, and 32-bit modes are added
						numvalue |= (1 | 4 | 16);
						RegSetValueEx(hKeyProfileInput, _T("AddColorDepths"), 0,
							REG_DWORD, (LPBYTE)&numvalue, 4);
					}
					RegCloseKey(hKeyProfileInput);
				}
			}
		}
		RegCloseKey(hKeyProfiles);
	}
	sizeout = 2;
	RegSetValueEx(hKey, configversion, 0, REG_DWORD, (BYTE*)&sizeout, 4);
}

/**
  * Checks the registry configuration version and if outdated upgrades to
  * the latest version - currently version 3
  * Pre-versioned configuration is assumed to be version 0.
  */
void UpgradeConfig()
{
	DWORD version = 0;
	DWORD keyindex;
	HKEY hKey;
	HKEY hKeyProfile;
	HKEY hKeyDest;
	HKEY hKeyProfileDest;
	TCHAR regkey[MAX_PATH + 24];
	TCHAR subkey[MAX_PATH];
	TCHAR exepath[(MAX_PATH * 2) + 1];
	FILE *file;
	TCHAR crcstr[10];
	unsigned long crc;
	DWORD numoldconfig;
	DWORD numvalue;
	DWORD oldconfigcount;
	DWORD olddirsize = 1024;
	TCHAR *olddir = NULL;
	DWORD oldvaluesize = 1024;
	WCHAR dir_unicode[MAX_PATH];
	TCHAR *oldvalue = NULL;
	TCHAR *ptr;
	size_t length;
	CFGREG *oldkeys = NULL;
	DWORD regtype;
	DWORD sizeout, sizeout2;
	Sha256Context sha_context;
	SHA256_HASH PathHash;
	TCHAR PathHashString[65];
	LONG error;
	LONG error2;
	DWORD i;
	// Check configuration version first
	_tcscpy(regkey, regkeybase);
	error = RegCreateKeyEx(HKEY_CURRENT_USER, regkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (error != ERROR_SUCCESS)
	{
		MessageBox(NULL, _T("Could not open registry key to upgrade DXGL"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
		ExitProcess(error);
	}
	sizeout = 4;
	regtype = REG_DWORD;
	error = RegQueryValueEx(hKey, configversion, NULL, &regtype, (LPBYTE)&version, &sizeout);
	if (error != ERROR_SUCCESS) version = 0;  // Version is 0 if not set (alpha didn't have version)
	if (regtype != REG_DWORD) version = 0; // Is the key the wrong type?
	if (version < 1) UpgradeProfile0to1(hKey);  // DXGL 0.5.9 and later - upgrade from initial format
	if (version < 2) UpgradeProfile1to2(hKey);  // DXGL 0.5.13 and later - Preset color depths option
ver2to3:
	// Version 2 to 3:  Fix profile path hashes
	if (version >= 3) return;
	// Transfer profiles to pre-migrate path
	error = RegCreateKeyEx(HKEY_CURRENT_USER, regkeyprofiles, 0, NULL, 0, KEY_READ, NULL, &hKey, NULL);
	if (error == ERROR_SUCCESS)
	{
		error = RegCreateKeyEx(HKEY_CURRENT_USER, regkeyprofilesmigrated, 0, NULL, 0, KEY_ALL_ACCESS,
			NULL, &hKeyDest, NULL);
		if (error == ERROR_SUCCESS)
		{
			olddirsize = 1024;
			oldvaluesize = 1024;
			olddir = malloc(olddirsize * 2);
			oldvalue = malloc(oldvaluesize);
			keyindex = 0;
			do
			{
				sizeout = MAX_PATH;
				error = RegEnumKeyEx(hKey, keyindex, subkey, &sizeout,
					NULL, NULL, NULL, NULL);
				keyindex++;
				if (error == ERROR_SUCCESS)
				{
					error2 = RegOpenKeyEx(hKey, subkey, 0, KEY_READ, &hKeyProfile);
					if (error2 == ERROR_SUCCESS)
					{
						// Rename subkey
						sizeout = MAX_PATH;
						regtype = REG_SZ;
						error2 = RegQueryValueEx(hKeyProfile, _T("InstallPath"), NULL, &regtype, (LPBYTE)olddir, &sizeout);
						if (error2 == ERROR_SUCCESS)
						{
							_tcslwr(olddir);
							_tchartowchar(dir_unicode, olddir, MAX_PATH);
							Sha256Initialise(&sha_context);
							Sha256Update(&sha_context, dir_unicode, (uint32_t)wcslen(dir_unicode) * sizeof(WCHAR));
							Sha256Finalise(&sha_context, &PathHash);
							for (i = 0; i < (256 / 8); i++)
							{
								PathHashString[i * 2] = (TCHAR)hexdigit(PathHash.bytes[i] >> 4);
								PathHashString[(i * 2) + 1] = (TCHAR)hexdigit(PathHash.bytes[i] & 0xF);
							}
							PathHashString[256 / 4] = 0;
							ptr = _tcsrchr(subkey, '-');
							_tcscpy(ptr + 1, PathHashString);
						}
						error2 = RegCreateKeyEx(hKeyDest, subkey, 0, NULL, 0, KEY_ALL_ACCESS,
							NULL, &hKeyProfileDest, NULL);
						if (error2 == ERROR_SUCCESS)
						{
							numvalue = 0;
							do
							{
								sizeout = olddirsize;
								sizeout2 = oldvaluesize;
								error2 = RegEnumValue(hKeyProfile, numvalue, olddir, &sizeout, NULL, &regtype, (LPBYTE)oldvalue, &sizeout2);
								if (error2 == ERROR_MORE_DATA)
								{
									if (sizeout > olddirsize)
									{
										olddirsize = sizeout;
										olddir = realloc(olddir, olddirsize * 2);
										if (!olddir)
										{
											MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
											ExitProcess(error2);
										}
									}
									if (sizeout2 > oldvaluesize)
									{
										oldvaluesize = sizeout2;
										oldvalue = realloc(oldvalue, oldvaluesize);
										if (!oldvalue)
										{
											MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
											ExitProcess(error2);
										}
									}
									sizeout = olddirsize;
									sizeout2 = oldvaluesize;
									error2 = RegEnumValue(hKeyProfile, numvalue, olddir, &sizeout, NULL, &regtype, (LPBYTE)oldvalue, &sizeout2);
								}
								if (error2 == ERROR_SUCCESS)
								{
									RegSetValueEx(hKeyProfileDest, olddir, 0, regtype, (BYTE*)oldvalue, sizeout2);
								}
								numvalue++;
							} while (error2 == ERROR_SUCCESS);
							RegCloseKey(hKeyProfileDest);
						}
						RegCloseKey(hKeyProfile);
					}
				}
			} while (error == ERROR_SUCCESS);
			RegCloseKey(hKeyDest);
			free(olddir);
			free(oldvalue);
		}
		// Delete source keys
		do
		{
			sizeout = MAX_PATH;
			error = RegEnumKeyEx(hKey, 0, subkey, &sizeout,
				NULL, NULL, NULL, NULL);
			if (error == ERROR_SUCCESS)
			{
				error2 = RegDeleteKey(hKey, subkey);
				if (error2 != ERROR_SUCCESS) break;
			}
		} while (error == ERROR_SUCCESS);
		RegCloseKey(hKey);
	}
	// Migrate keys to new names
	error = RegCreateKeyEx(HKEY_CURRENT_USER, regkeyprofilesmigrated, 0, NULL, 0, KEY_READ, NULL, &hKey, NULL);
	if (error == ERROR_SUCCESS)
	{
		error = RegCreateKeyEx(HKEY_CURRENT_USER, regkeyprofiles, 0, NULL, 0, KEY_ALL_ACCESS,
			NULL, &hKeyDest, NULL);
		if (error == ERROR_SUCCESS)
		{
			olddirsize = 1024;
			oldvaluesize = 1024;
			olddir = malloc(olddirsize * 2);
			oldvalue = malloc(oldvaluesize);
			keyindex = 0;
			do
			{
				sizeout = MAX_PATH;
				error = RegEnumKeyEx(hKey, keyindex, subkey, &sizeout,
					NULL, NULL, NULL, NULL);
				keyindex++;
				if (error == ERROR_SUCCESS)
				{
					error2 = RegOpenKeyEx(hKey, subkey, 0, KEY_READ, &hKeyProfile);
					if (error2 == ERROR_SUCCESS)
					{
						error2 = RegCreateKeyEx(hKeyDest, subkey, 0, NULL, 0, KEY_ALL_ACCESS,
							NULL, &hKeyProfileDest, NULL);
						if (error2 == ERROR_SUCCESS)
						{
							numvalue = 0;
							do
							{
								sizeout = olddirsize;
								sizeout2 = oldvaluesize;
								error2 = RegEnumValue(hKeyProfile, numvalue, olddir, &sizeout, NULL, &regtype, (LPBYTE)oldvalue, &sizeout2);
								if (error2 == ERROR_MORE_DATA)
								{
									if (sizeout > olddirsize)
									{
										olddirsize = sizeout;
										olddir = realloc(olddir, olddirsize * 2);
										if (!olddir)
										{
											MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
											ExitProcess(error2);
										}
									}
									if (sizeout2 > oldvaluesize)
									{
										oldvaluesize = sizeout2;
										oldvalue = realloc(oldvalue, oldvaluesize);
										if (!oldvalue)
										{
											MessageBox(NULL, _T("Out of memory updating registry"), _T("Fatal error"), MB_ICONSTOP | MB_OK);
											ExitProcess(error2);
										}
									}
									sizeout = olddirsize;
									sizeout2 = oldvaluesize;
									error2 = RegEnumValue(hKeyProfile, numvalue, olddir, &sizeout, NULL, &regtype, (LPBYTE)oldvalue, &sizeout2);
								}
								if (error2 == ERROR_SUCCESS)
								{
									RegSetValueEx(hKeyProfileDest, olddir, 0, regtype, (BYTE*)oldvalue, sizeout2);
								}
								numvalue++;
							} while (error2 == ERROR_SUCCESS);
							RegCloseKey(hKeyProfileDest);
						}
						RegCloseKey(hKeyProfile);
					}
				}
			} while (error == ERROR_SUCCESS);
			RegCloseKey(hKeyDest);
			free(olddir);
			free(oldvalue);
		}
		// Delete temporary keys
		do
		{
			sizeout = MAX_PATH;
			error = RegEnumKeyEx(hKey, 0, subkey, &sizeout,
				NULL, NULL, NULL, NULL);
			if (error == ERROR_SUCCESS)
			{
				error2 = RegDeleteKey(hKey, subkey);
				if (error2 != ERROR_SUCCESS) break;
			}
		} while (error == ERROR_SUCCESS);
		RegCloseKey(hKey);
		RegDeleteKey(HKEY_CURRENT_USER, regkeyprofilesmigrated);
	}
	error = RegCreateKeyEx(HKEY_CURRENT_USER, regkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (error == ERROR_SUCCESS)
	{
		sizeout = 3;
		RegSetValueEx(hKey, configversion, 0, REG_DWORD, (BYTE*)& sizeout, 4);
		RegCloseKey(hKey);
	}
	return;
}

int ReadINIOptionsCallback(app_ini_options *options, const char *section, const char *name,
	const char *value)
{
	if (!_stricmp(section, "system"))
	{
		if (!_stricmp(name, "NoOverwrite")) options->NoOverwrite = INIBoolValue(value);
		if (!_stricmp(name, "BundledDDrawSHA256"))
		{
			strncpy(options->sha256comp, value, 65);
			options->sha256comp[64] = 0;
			if (options->sha256comp[63] == 0) options->sha256comp[0] = 0;
		}
		if (!_stricmp(name, "NoUninstall")) options->NoUninstall = INIBoolValue(value);
	}
	return 1;
}

void ReadAppINIOptions(LPCTSTR path, app_ini_options *options)
{
	int i;
	Sha256Context sha_context;
	SHA256_HASH sha256;
	FILE *file;
	HANDLE file2;
	char buffer[512];
	DWORD bytesread;
	TCHAR path2[MAX_PATH + 1];
	options->NoOverwrite = FALSE;
	options->NoUninstall = FALSE;
	ZeroMemory(options->sha256, 65*sizeof(char));
	ZeroMemory(options->sha256comp, 65*sizeof(char));
	_tcsncpy(path2, path, MAX_PATH + 1);
	_tcscat(path2, _T("dxgl.ini"));
	// Check for INI file and read it.
	file = _tfopen(path2, _T("r"));
	if (file)
	{
		ini_parse_file(file, ReadINIOptionsCallback, options);
		fclose(file);
	}
	// Check for existing ddraw.dll and get SHA256 sum
	if ((options->sha256comp[0] != 0) && !options->NoOverwrite)
	{
		_tcsncpy(path2, path, MAX_PATH + 1);
		_tcscat(path2, _T("ddraw.dll"));
		Sha256Initialise(&sha_context);
		file2 = CreateFile(path2, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(!file2)
		{ 
			options->sha256[0] = 0;
		}
		else
		{
			while (1)
			{
				ReadFile(file2, buffer, 512, &bytesread, NULL);
				if (!bytesread) break;
				Sha256Update(&sha_context, buffer, bytesread);
				if (bytesread < 512) break;
			}
			Sha256Finalise(&sha_context, &sha256);
			CloseHandle(file2);
			for (i = 0; i < (256 / 8); i++)
			{
				options->sha256[i * 2] = hexdigit(sha256.bytes[i] >> 4);
				options->sha256[(i * 2) + 1] = hexdigit(sha256.bytes[i] & 0xF);
			}
		}
	}
}