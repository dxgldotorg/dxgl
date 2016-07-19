// DXGL
// Copyright (C) 2011-2016 William Feely

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
#define _WIN32_IE 0x0300
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
#include <math.h>
#include <io.h>
#include <Uxtheme.h>
#include <Vsstyle.h>
#include "resource.h"
#include "../cfgmgr/LibSha256.h"
#include "../cfgmgr/cfgmgr.h"
#include <gl/GL.h>

#ifndef SHGFI_ADDOVERLAYS
#define SHGFI_ADDOVERLAYS 0x000000020
#endif //SHGFI_ADDOVERLAYS

#ifndef BCM_SETSHIELD
#define BCM_SETSHIELD 0x160C
#endif

#define GL_TEXTURE_MAX_ANISOTROPY_EXT          0x84FE
#define GL_MAX_SAMPLES_EXT                     0x8D57
#define GL_MAX_MULTISAMPLE_COVERAGE_MODES_NV        0x8E11
#define GL_MULTISAMPLE_COVERAGE_MODES_NV            0x8E12
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT      0x84FF

DXGLCFG *cfg;
DXGLCFG *cfgmask;
BOOL *dirty;
HINSTANCE hinstance;
BOOL msaa = FALSE;
const char *extensions_string = NULL;
OSVERSIONINFO osver;
TCHAR hlppath[MAX_PATH+16];
HMODULE uxtheme = NULL;
HTHEME hThemeDisplay = NULL;
HTHEME(WINAPI *_OpenThemeData)(HWND hwnd, LPCWSTR pszClassList) = NULL;
HRESULT(WINAPI *_CloseThemeData)(HTHEME hTheme) = NULL;
HRESULT(WINAPI *_DrawThemeBackground)(HTHEME hTheme, HDC hdc, int iPartID,
	int iStateID, const RECT *pRect, const RECT *pClipRect) = NULL;


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
HWND hTab;
HWND hTabs[6];
int tabopen;

static const TCHAR *colormodes[32] = {
	_T("None"),
	_T("8-bit"),
	_T("15-bit"),
	_T("8/15-bit"),
	_T("16-bit"),
	_T("8/16-bit"),
	_T("8/15-bit"),
	_T("8/15/16-bit"),
	_T("24-bit"),
	_T("8/24-bit"),
	_T("15/24-bit"),
	_T("8/15/24-bit"),
	_T("16/24-bit"),
	_T("8/16/24-bit"),
	_T("8/15/24-bit"),
	_T("8/15/16/24-bit"),
	_T("32-bit"),
	_T("8/32-bit"),
	_T("15/32-bit"),
	_T("8/15/32-bit"),
	_T("16/32-bit"),
	_T("8/16/32-bit"),
	_T("8/15/32-bit"),
	_T("8/15/16/32-bit"),
	_T("24/32-bit"),
	_T("8/24/32-bit"),
	_T("15/24/32-bit"),
	_T("8/15/24/32-bit"),
	_T("16/24/32-bit"),
	_T("8/16/24/32-bit"),
	_T("8/15/24/32-bit"),
	_T("8/15/16/24/32-bit")
};

DWORD AddApp(LPCTSTR path, BOOL copyfile, BOOL admin)
{
	BOOL installed = FALSE;
	BOOL dxgl_installdir = FALSE;
	BOOL old_dxgl = FALSE;
	TCHAR command[MAX_PATH+32];
	SHELLEXECUTEINFO shex;
	DWORD exitcode;
	if (copyfile)
	{
		DWORD sizeout = (MAX_PATH+1)*sizeof(TCHAR);
		TCHAR installpath[MAX_PATH+1];
		TCHAR srcpath[MAX_PATH+1];
		TCHAR destpath[MAX_PATH+1];
		HKEY hKeyInstall;
		LONG error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\DXGL"),0,KEY_READ,&hKeyInstall);
		if(error == ERROR_SUCCESS)
		{
			dxgl_installdir = TRUE;
			error = RegQueryValueEx(hKeyInstall,_T("InstallDir"),NULL,NULL,(LPBYTE)installpath,&sizeout);
			if(error == ERROR_SUCCESS) installed = TRUE;
		}
		if(hKeyInstall) RegCloseKey(hKeyInstall);
		if(!installed)
		{
			GetModuleFileName(NULL,installpath,MAX_PATH+1);
		}
		if(dxgl_installdir) _tcscat(installpath,_T("\\"));
		else (_tcsrchr(installpath,_T('\\')))[1] = 0;
		_tcsncpy(srcpath,installpath,MAX_PATH+1);
		_tcscat(srcpath,_T("ddraw.dll"));
		_tcsncpy(destpath,path,MAX_PATH+1);
		(_tcsrchr(destpath,_T('\\')))[1] = 0;
		_tcscat(destpath,_T("ddraw.dll"));
		error = CopyFile(srcpath,destpath,TRUE);
		error_loop:
		if(!error)
		{
			error = GetLastError();
			if(error == ERROR_FILE_EXISTS)
			{
				HMODULE hmod = LoadLibrary(destpath);
				if(hmod)
				{
					if(GetProcAddress(hmod,"IsDXGLDDraw")) old_dxgl = TRUE;
					FreeLibrary(hmod);
				}
				if(old_dxgl)
				{
					error = CopyFile(srcpath,destpath,FALSE);
					goto error_loop;
				}
			}
			if((error == ERROR_ACCESS_DENIED) && !admin)
			{
				_tcscpy(command,_T(" install "));
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

DWORD DelApp(LPCTSTR path, BOOL admin)
{
	BOOL installed = FALSE;
	TCHAR command[MAX_PATH + 32];
	BOOL old_dxgl = TRUE;
	DWORD sizeout = (MAX_PATH+1)*sizeof(TCHAR);
	TCHAR installpath[MAX_PATH+1];
	HKEY hKeyInstall;
	HMODULE hmod;
	SHELLEXECUTEINFO shex;
	DWORD exitcode;
	LONG error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
	if(error == ERROR_SUCCESS)
	{
		error = RegQueryValueEx(hKeyInstall,_T("InstallDir"),NULL,NULL,(LPBYTE)installpath,&sizeout);
		if(error == ERROR_SUCCESS) installed = TRUE;
	}
	if(hKeyInstall) RegCloseKey(hKeyInstall);
	if(!installed)
	{
		GetModuleFileName(NULL,installpath,MAX_PATH+1);
	}
	hmod = LoadLibrary(path);
	if(hmod)
	{
		if(!GetProcAddress(hmod,"IsDXGLDDraw")) old_dxgl = FALSE;
		FreeLibrary(hmod);
	}
	if(!old_dxgl) return 0;
	if(!DeleteFile(path))
	{
		error = GetLastError();
		if(error == ERROR_FILE_NOT_FOUND) return 0;
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
		return error;
	}
	return 0;
}

void SaveChanges(HWND hWnd)
{
	int i;
	if(apps[0].dirty) SetGlobalConfig(&apps[0].cfg);
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
	if (_isnan(f)) f = 0; //Handle NAN condition
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
	if (fabsf(f - 1.7777777) < 0.0001f)
	{
		_tcscpy(aspect, _T("16:9"));
		return;
	}
	if (fabsf(f - 1.9333333) < 0.0001f)
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
			_itot((f*i) + .5f, aspect, 10);
			_itot(i, denominator, 10);
			_tcscat(aspect, _T(":"));
			_tcscat(aspect, denominator);
			return;
		}
	}
	// Cannot find a reasonable fractional aspect, so display as decimal.
#ifdef _UNICODE
	swprintf(aspect, 31, L"%.6f", f);
#else
	sprintf(aspect,"%.6f", f);
#endif

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

void SetText(HWND hWnd, int DlgItem, TCHAR *value, TCHAR *mask, BOOL tristate)
{
	if(tristate && (mask[0] == 0))
		SetWindowText(GetDlgItem(hWnd,DlgItem),_T(""));
	else SetWindowText(GetDlgItem(hWnd,DlgItem),value);
}

BOOL GetCheck(HWND hWnd, int DlgItem, BOOL *mask)
{
	int check = SendDlgItemMessage(hWnd,DlgItem,BM_GETCHECK,0,0);
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
	int value = SendDlgItemMessage(hWnd,DlgItem,CB_GETCURSEL,0,0);
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

float GetAspectCombo(HWND hWnd, int DlgItem, float *mask)
{
	TCHAR buffer[32];
	TCHAR *ptr;
	float numerator, denominator;
	GetDlgItemText(hWnd, DlgItem, buffer, 31);
	if (!_tcscmp(buffer, strdefault))
	{
		*mask = 0.0f;
		return 0;
	}
	else
	{
		*mask = 1.0f;
		if (!_tcscmp(buffer, _T("Default"))) return 0.0f;
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
	else mask[0] = 0xff;
}
LRESULT CALLBACK DisplayTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DRAWITEMSTRUCT* drawitem;
	COLORREF OldTextColor, OldBackColor;
	RECT r;
	TCHAR combotext[64];
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (uxtheme) hThemeDisplay = _OpenThemeData(hWnd, L"Button");
		else hThemeDisplay = NULL;
		return TRUE;
	case WM_MEASUREITEM:
		switch (wParam)
		{
		case IDC_EXTRAMODES:
			((LPMEASUREITEMSTRUCT)lParam)->itemHeight = GetSystemMetrics(SM_CYMENUCHECK);
			((LPMEASUREITEMSTRUCT)lParam)->itemWidth = GetSystemMetrics(SM_CXMENUCHECK);
			break;
		default:
			break;
		}
	case WM_DRAWITEM:
		drawitem = (DRAWITEMSTRUCT*)lParam;
		switch (wParam)
		{
		case IDC_EXTRAMODES:
			OldTextColor = GetTextColor(drawitem->hDC);
			OldBackColor = GetBkColor(drawitem->hDC);
			if ((drawitem->itemAction | ODA_SELECT) && (drawitem->itemState & ODS_SELECTED) &&
				!(drawitem->itemState & ODS_COMBOBOXEDIT))
			{
				SetTextColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(drawitem->hDC, GetSysColor(COLOR_HIGHLIGHT));
				FillRect(drawitem->hDC, &drawitem->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
			}
			else ExtTextOut(drawitem->hDC, 0, 0, ETO_OPAQUE, &drawitem->rcItem, NULL, 0, NULL);
			memcpy(&r, &drawitem->rcItem, sizeof(RECT));
			r.left = r.left + 2;
			r.right = r.left + GetSystemMetrics(SM_CXMENUCHECK);
			if (hThemeDisplay) _DrawThemeBackground(hThemeDisplay, drawitem->hDC, BS_AUTOCHECKBOX,
				CBS_CHECKEDNORMAL, &r, NULL);
			else DrawFrameControl(drawitem->hDC, &r, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_HOT);
			drawitem->rcItem.left += GetSystemMetrics(SM_CXMENUCHECK) + 5;
			combotext[0] = 0;
			SendDlgItemMessage(hWnd, IDC_EXTRAMODES, CB_GETLBTEXT, drawitem->itemID, combotext);
			DrawText(drawitem->hDC, combotext,	_tcslen(combotext), &drawitem->rcItem,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			SetTextColor(drawitem->hDC, OldTextColor);
			SetBkColor(drawitem->hDC, OldBackColor);
			DefWindowProc(hWnd, Msg, wParam, lParam);
			break;
		default:
			break;
		}
	case WM_THEMECHANGED:
		if (uxtheme)
		{
			if (hThemeDisplay) _CloseThemeData(hThemeDisplay);
			_OpenThemeData(hWnd, L"Button");
		}
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
		return TRUE;
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
		return TRUE;
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
		return TRUE;
	default:
		return FALSE;
	}
	return TRUE;
}
LRESULT CALLBACK DebugTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		return TRUE;
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
		return TRUE;
	default:
		return FALSE;
	}
	return TRUE;
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
	TOKEN_ELEVATION elevation;
	HWND hGLWnd;
	OPENFILENAME filename;
	TCHAR selectedfile[MAX_PATH + 1];
	LPTSTR regpath;
	LPTSTR regkey;
	BOOL failed;
	LPTSTR installpath;
	RECT r;
	NMHDR *nm;
	int newtab;
	TCITEM tab;
	switch (Msg)
	{
	case WM_INITDIALOG:
		tristate = FALSE;
		maxapps = 128;
		apps = (app_setting *)malloc(maxapps*sizeof(app_setting));
		apps[0].name = (TCHAR*)malloc(7 * sizeof(TCHAR));
		_tcscpy(apps[0].name,_T("Global"));
		apps[0].regkey = (TCHAR*)malloc(7 * sizeof(TCHAR));
		_tcscpy(apps[0].regkey,_T("Global"));
		GetGlobalConfig(&apps[0].cfg, FALSE);
		cfg = &apps[0].cfg;
		cfgmask = &apps[0].mask;
		dirty = &apps[0].dirty;
		memset(&apps[0].mask,0xff,sizeof(DXGLCFG));
		apps[0].dirty = FALSE;
		apps[0].icon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_STAR));
		apps[0].icon_shared = TRUE;
		apps[0].path[0] = 0;
		SetClassLong(hWnd,GCL_HICON,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGL));
		SetClassLong(hWnd,GCL_HICONSM,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGLSM));
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
					msaa = TRUE;
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
			if (!(_OpenThemeData && _CloseThemeData && _DrawThemeBackground))
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
		tab.pszText = _T("3D Graphics");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 1, (LPARAM)&tab);
		tab.pszText = _T("Effects");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 2, (LPARAM)&tab);
		tab.pszText = _T("Advanced");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 3, (LPARAM)&tab);
		tab.pszText = _T("Debug");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 4, (LPARAM)&tab);
		tab.pszText = _T("Paths");
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_INSERTITEM, 5, (LPARAM)&tab);
		hTab = GetDlgItem(hWnd, IDC_TABS);
		hTabs[0] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_DISPLAY), hTab, DisplayTabCallback);
		hTabs[1] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_3DGRAPHICS), hTab, Tab3DCallback);
		hTabs[2] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_EFFECTS), hTab, EffectsTabCallback);
		hTabs[3] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_ADVANCED), hTab, AdvancedTabCallback);
		hTabs[4] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_DEBUG), hTab, DebugTabCallback);
		hTabs[5] = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_PATHS), hTab, PathsTabCallback);
		SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETITEMRECT, 0, (LPARAM)&r);
		SetWindowPos(hTabs[0], NULL, r.left, r.bottom + 3, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		ShowWindow(hTabs[1], SW_HIDE);
		ShowWindow(hTabs[2], SW_HIDE);
		ShowWindow(hTabs[3], SW_HIDE);
		ShowWindow(hTabs[4], SW_HIDE);
		ShowWindow(hTabs[5], SW_HIDE);
		tabopen = 0;

		// Load global settings.
		// scaler
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
		_tcscpy(buffer,_T("Crop to screen (experimental)"));
		SendDlgItemMessage(hTabs[0],IDC_VIDMODE,CB_ADDSTRING,7,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[0],IDC_VIDMODE,CB_SETCURSEL,cfg->scaler,0);
		// colormode
		if (cfg->colormode) SendDlgItemMessage(hTabs[0], IDC_COLOR, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[0], IDC_COLOR, BM_SETCHECK, BST_UNCHECKED, 0);
		// scalingfilter
		_tcscpy(buffer,_T("Nearest"));
		SendDlgItemMessage(hTabs[0], IDC_SCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear"));
		SendDlgItemMessage(hTabs[0],IDC_SCALE,CB_ADDSTRING,1,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[0],IDC_SCALE,CB_SETCURSEL,cfg->scalingfilter,0);
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
		SetAspectCombo(hTabs[0], IDC_ASPECT, cfg->aspect, cfgmask->aspect, tristate);
		// primaryscale
		_tcscpy(buffer, _T("Auto (Window Size)"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Auto (Multiple of Native)"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer, _T("1x Native (Recommended)"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 2, (LPARAM)buffer);
		_tcscpy(buffer, _T("1.5x Native"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 3, (LPARAM)buffer);
		_tcscpy(buffer, _T("2x Native"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 4, (LPARAM)buffer);
		_tcscpy(buffer, _T("2.5x Native"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 5, (LPARAM)buffer);
		_tcscpy(buffer, _T("3x Native"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 6, (LPARAM)buffer);
		_tcscpy(buffer, _T("4x Native"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 7, (LPARAM)buffer);
		_tcscpy(buffer, _T("Custom"));
		SendDlgItemMessage(hTabs[2], IDC_PRIMARYSCALE, CB_ADDSTRING, 8, (LPARAM)buffer);
		SetPrimaryScaleCombo(hTabs[2], IDC_PRIMARYSCALE, cfg->primaryscale, cfgmask->primaryscale, tristate);
		// texfilter
		_tcscpy(buffer,_T("Application default"));
		SendDlgItemMessage(hTabs[1], IDC_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest"));
		SendDlgItemMessage(hTabs[1], IDC_TEXFILTER, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear"));
		SendDlgItemMessage(hTabs[1], IDC_TEXFILTER, CB_ADDSTRING, 2, (LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest, nearest mipmap"));
		SendDlgItemMessage(hTabs[1], IDC_TEXFILTER, CB_ADDSTRING, 3, (LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest, linear mipmap"));
		SendDlgItemMessage(hTabs[1], IDC_TEXFILTER, CB_ADDSTRING, 4, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear, nearest mipmap"));
		SendDlgItemMessage(hTabs[1], IDC_TEXFILTER, CB_ADDSTRING, 5, (LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear, linear mipmap"));
		SendDlgItemMessage(hTabs[1],IDC_TEXFILTER,CB_ADDSTRING,6,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[1],IDC_TEXFILTER,CB_SETCURSEL,cfg->texfilter,0);
		// anisotropic
		if (anisotropic < 2)
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 0, (LPARAM)buffer);
			SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_SETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(hTabs[1], IDC_ANISO), FALSE);
			cfg->anisotropic = 0;
		}
		else
		{
			_tcscpy(buffer,_T("Application default"));
			SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 0, (LPARAM)buffer);
			_tcscpy(buffer,_T("Disabled"));
			SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 1, (LPARAM)buffer);
			if(anisotropic >= 2)
			{
				_tcscpy(buffer,_T("2x"));
				SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 2, (LPARAM)buffer);
			}
			if(anisotropic >= 4)
			{
				_tcscpy(buffer,_T("4x"));
				SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 4, (LPARAM)buffer);
			}
			if(anisotropic >= 8)
			{
				_tcscpy(buffer,_T("8x"));
				SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 8, (LPARAM)buffer);
			}
			if(anisotropic >= 16)
			{
				_tcscpy(buffer,_T("16x"));
				SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 16, (LPARAM)buffer);
			}
			if(anisotropic >= 32)
			{
				_tcscpy(buffer,_T("32x"));
				SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_ADDSTRING, 4, (LPARAM)buffer);
			}
			SendDlgItemMessage(hTabs[1], IDC_ANISO, CB_SETCURSEL, cfg->anisotropic, 0);
		}
		// msaa
		if(msaa)
		{
			_tcscpy(buffer,_T("Application default"));
			SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 0, (LPARAM)buffer);
			_tcscpy(buffer,_T("Disabled"));
			SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 1, (LPARAM)buffer);
			if(maxcoverage)
			{
				for(i = 0; i < maxcoverage; i++)
				{
					if((msaamodes[i] & 0xfff) <= 4)
						_sntprintf(buffer,64,_T("%dx"),msaamodes[i] & 0xfff);
					else _sntprintf(buffer,64,_T("%dx coverage, %dx color"),(msaamodes[i] & 0xfff), (msaamodes[i] >> 12));
					SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, msaamodes[i], (LPARAM)buffer);
				}
			}
			else
			{
				if(maxsamples >= 2)
				{
					_tcscpy(buffer,_T("2x"));
					SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 2, (LPARAM)buffer);
				}
				if(maxsamples >= 4)
				{
					_tcscpy(buffer,_T("4x"));
					SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 4, (LPARAM)buffer);
				}
				if(maxsamples >= 8)
				{
					_tcscpy(buffer,_T("8x"));
					SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 8, (LPARAM)buffer);
				}
				if(maxsamples >= 16)
				{
					_tcscpy(buffer,_T("16x"));
					SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 16, (LPARAM)buffer);
				}
				if(maxsamples >= 32)
				{
					_tcscpy(buffer,_T("32x"));
					SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 32, (LPARAM)buffer);
				}
			}
			SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_SETCURSEL, cfg->msaa, 0);
		}
		else
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_ADDSTRING, 0, (LPARAM)buffer);
			SendDlgItemMessage(hTabs[1], IDC_MSAA, CB_SETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(hTabs[1], IDC_MSAA), FALSE);
			cfg->msaa = 0;
		}
		// aspect3d
		_tcscpy(buffer,_T("Stretch to display"));
		SendDlgItemMessage(hTabs[1], IDC_ASPECT3D, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Expand viewable area"));
		SendDlgItemMessage(hTabs[1], IDC_ASPECT3D, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer,_T("Crop to display"));
		SendDlgItemMessage(hTabs[1],IDC_ASPECT3D,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[1],IDC_ASPECT3D,CB_SETCURSEL,cfg->aspect3d,0);
		// sort modes
		_tcscpy(buffer,_T("Use system order"));
		SendDlgItemMessage(hTabs[0], IDC_SORTMODES, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer,_T("Group by color depth"));
		SendDlgItemMessage(hTabs[0], IDC_SORTMODES, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer,_T("Group by resolution"));
		SendDlgItemMessage(hTabs[0],IDC_SORTMODES,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[0],IDC_SORTMODES,CB_SETCURSEL,cfg->SortModes,0);
		// color depths
		for (i = 0; i < 32; i++)
		{
			_tcscpy(buffer, colormodes[i]);
			SendDlgItemMessage(hTabs[0], IDC_COLORDEPTH, CB_ADDSTRING, i, (LPARAM)buffer);
		}
		SendDlgItemMessage(hTabs[0], IDC_COLORDEPTH, CB_SETCURSEL, cfg->AddColorDepths, 0);
		_tcscpy(buffer, _T("Common low resolutions"));
		SendDlgItemMessage(hTabs[0], IDC_EXTRAMODES, CB_ADDSTRING, 0, (LPARAM)buffer);
		_tcscpy(buffer, _T("Uncommon low resolutions"));
		SendDlgItemMessage(hTabs[0], IDC_EXTRAMODES, CB_ADDSTRING, 1, (LPARAM)buffer);
		_tcscpy(buffer, _T("Higher resolutions"));
		SendDlgItemMessage(hTabs[0], IDC_EXTRAMODES, CB_ADDSTRING, 2, (LPARAM)buffer);
		//FIXME:  Populate extra resolution combobox
		// Enable shader
		if (cfg->colormode) SendDlgItemMessage(hTabs[2], IDC_USESHADER, BM_SETCHECK, BST_CHECKED, 0);
		else SendDlgItemMessage(hTabs[2], IDC_USESHADER, BM_SETCHECK, BST_UNCHECKED, 0);
		// shader path
		SetText(hTabs[2],IDC_SHADER,cfg->shaderfile,cfgmask->shaderfile,FALSE);
		// texture format
		_tcscpy(buffer,_T("Automatic"));
		SendDlgItemMessage(hTabs[3],IDC_TEXTUREFORMAT,CB_ADDSTRING,0,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[3],IDC_TEXTUREFORMAT,CB_SETCURSEL,cfg->TextureFormat,0);
		// Texture upload
		_tcscpy(buffer,_T("Automatic"));
		SendDlgItemMessage(hTabs[3],IDC_TEXUPLOAD,CB_ADDSTRING,0,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[3],IDC_TEXUPLOAD,CB_SETCURSEL,cfg->TexUpload,0);
		// DPI
		_tcscpy(buffer, _T("Disabled"));
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer, _T("Enabled"));
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer, _T("Windows AppCompat"));
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hTabs[0],IDC_DPISCALE,CB_SETCURSEL,cfg->DPIScale,0);
		//EnableWindow(GetDlgItem(hWnd, IDC_PATHLABEL), FALSE);
		//EnableWindow(GetDlgItem(hWnd, IDC_PROFILEPATH), FALSE);
		// Check install path
		error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKey);
		if (error == ERROR_SUCCESS)
		{
			if (RegQueryValueEx(hKey, _T("InstallDir"), NULL, NULL, NULL, &keysize) == ERROR_SUCCESS)
			{
				installpath = (LPTSTR)malloc(keysize);
				error = RegQueryValueEx(hKey, _T("InstallDir"), NULL, NULL, installpath, &keysize);
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
		RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DXGL\\Profiles"), 0, NULL, 0, KEY_READ, NULL, &hKeyBase, NULL);
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
			error = SHGetFileInfo(path, 0, &fileinfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_ADDOVERLAYS);
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
				if(GetTokenInformation(token,(TOKEN_INFORMATION_CLASS)TokenElevation,&elevation,sizeof(TOKEN_ELEVATION),(PDWORD)&outlen))
				{
					if(!elevation.TokenIsElevated)
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
			newtab = SendDlgItemMessage(hWnd, IDC_TABS, TCM_GETCURSEL, 0, 0);
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
			if((drawitem->itemAction | ODA_SELECT) && (drawitem->itemState & ODS_SELECTED) &&
				!(drawitem->itemState & ODS_COMBOBOXEDIT))
			{
				SetTextColor(drawitem->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(drawitem->hDC,GetSysColor(COLOR_HIGHLIGHT));
				FillRect(drawitem->hDC,&drawitem->rcItem,(HBRUSH)(COLOR_HIGHLIGHT+1));
			}
			else ExtTextOut(drawitem->hDC,0,0,ETO_OPAQUE,&drawitem->rcItem,NULL,0,NULL);
			DrawIconEx(drawitem->hDC,drawitem->rcItem.left+2,drawitem->rcItem.top,
				apps[drawitem->itemID].icon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
			drawitem->rcItem.left += GetSystemMetrics(SM_CXSMICON)+5;
			DrawText(drawitem->hDC,apps[drawitem->itemID].name,
				_tcslen(apps[drawitem->itemID].name),&drawitem->rcItem,
				DT_LEFT|DT_SINGLELINE|DT_VCENTER);
			SetTextColor(drawitem->hDC,OldTextColor);
			SetBkColor(drawitem->hDC,OldBackColor);
			DefWindowProc(hWnd,Msg,wParam,lParam);
			break;
		default:
			break;
		}
		break;
	case WM_HELP:
		HtmlHelp(hWnd,hlppath,HH_DISPLAY_TOPIC,(DWORD_PTR)_T("configuration.htm"));
		return TRUE;
		break;
	case WM_SYSCOMMAND:
		if(LOWORD(wParam) == SC_CONTEXTHELP)
		{
			HtmlHelp(hWnd,hlppath,HH_DISPLAY_TOPIC,(DWORD_PTR)_T("configuration.htm"));
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
			return TRUE;
		case IDC_APPS:
			if(HIWORD(wParam) == LBN_SELCHANGE)
			{
				cursel = SendDlgItemMessage(hWnd,IDC_APPS,LB_GETCURSEL,0,0);
				if(cursel == current_app) break;
				current_app = cursel;
				cfg = &apps[current_app].cfg;
				cfgmask = &apps[current_app].mask;
				dirty = &apps[current_app].dirty;
				if (current_app)
				{
//					EnableWindow(GetDlgItem(hWnd, IDC_PATHLABEL), TRUE);
//					EnableWindow(GetDlgItem(hWnd, IDC_PROFILEPATH), TRUE);
//					SetDlgItemText(hWnd, IDC_PROFILEPATH, apps[current_app].path);
					EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), TRUE);
				}
				else
				{
//					EnableWindow(GetDlgItem(hWnd, IDC_PATHLABEL), FALSE);
//					EnableWindow(GetDlgItem(hWnd, IDC_PROFILEPATH), FALSE);
//					SetDlgItemText(hWnd, IDC_PROFILEPATH, _T(""));
					EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), FALSE);
				}
				/*				// Set 3-state status
				if(current_app && !tristate)
				{
					tristate = TRUE;
					SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_VSYNC,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_COLOR,BM_SETSTYLE,BS_AUTO3STATE,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_HIGHRES,BM_SETSTYLE,BS_AUTO3STATE,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_UNCOMMONCOLOR,BM_SETSTYLE,BS_AUTO3STATE,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_EXTRAMODES,BM_SETSTYLE,BS_AUTO3STATE,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_TEXTUREFORMAT,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd,IDC_TEXUPLOAD,CB_ADDSTRING,0,(LPARAM)strdefault);
					SendDlgItemMessage(hWnd, IDC_DPISCALE, CB_ADDSTRING, 0, (LPARAM)strdefault);
					SendDlgItemMessage(hWnd, IDC_ASPECT, CB_ADDSTRING, 0, (LPARAM)strdefault);
				}
				else if(!current_app && tristate)
				{
					tristate = FALSE;
					SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_SCALE,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_SCALE,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_VSYNC,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_VSYNC,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_MSAA,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_ANISO,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_ANISO,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_COLOR,BM_SETSTYLE,BS_AUTOCHECKBOX,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_HIGHRES,BM_SETSTYLE,BS_AUTOCHECKBOX,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_UNCOMMONCOLOR,BM_SETSTYLE,BS_AUTOCHECKBOX,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_EXTRAMODES,BM_SETSTYLE,BS_AUTOCHECKBOX,(LPARAM)TRUE);
					SendDlgItemMessage(hWnd,IDC_TEXTUREFORMAT,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd,IDC_TEXUPLOAD,CB_DELETESTRING,
						SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
					SendDlgItemMessage(hWnd, IDC_DPISCALE, CB_DELETESTRING,
						SendDlgItemMessage(hWnd, IDC_DPISCALE, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
					SendDlgItemMessage(hWnd, IDC_ASPECT, CB_DELETESTRING,
						SendDlgItemMessage(hWnd, IDC_ASPECT, CB_FINDSTRING, -1, (LPARAM)strdefault), 0);
				}
				// Read settings into controls
				SetCombo(hWnd,IDC_VIDMODE,cfg->scaler,cfgmask->scaler,tristate);
				SetCombo(hWnd,IDC_SORTMODES,cfg->SortModes,cfgmask->SortModes,tristate);
				SetCombo(hWnd,IDC_SCALE,cfg->scalingfilter,cfgmask->scalingfilter,tristate);
				SetCombo(hWnd,IDC_VSYNC,cfg->vsync,cfgmask->vsync,tristate);
				SetCombo(hWnd,IDC_MSAA,cfg->msaa,cfgmask->msaa,tristate);
				SetCombo(hWnd,IDC_ANISO,cfg->anisotropic,cfgmask->anisotropic,tristate);
				SetCombo(hWnd,IDC_TEXFILTER,cfg->texfilter,cfgmask->texfilter,tristate);
				SetCombo(hWnd,IDC_ASPECT3D,cfg->aspect3d,cfgmask->aspect3d,tristate);
				SetCheck(hWnd,IDC_COLOR,cfg->colormode,cfgmask->colormode,tristate);
				SetCheck(hWnd,IDC_HIGHRES,cfg->highres,cfgmask->highres,tristate);
				SetCheck(hWnd,IDC_UNCOMMONCOLOR,cfg->AllColorDepths,cfgmask->AllColorDepths,tristate);
				SetCombo(hWnd,IDC_TEXTUREFORMAT,cfg->TextureFormat,cfgmask->TextureFormat,tristate);
				SetCombo(hWnd,IDC_TEXUPLOAD,cfg->TexUpload,cfgmask->TexUpload,tristate);
				SetCheck(hWnd,IDC_EXTRAMODES,cfg->ExtraModes,cfgmask->ExtraModes,tristate);
				SetText(hWnd,IDC_SHADER,cfg->shaderfile,cfgmask->shaderfile,tristate);
				SetCombo(hWnd, IDC_DPISCALE, cfg->DPIScale, cfgmask->DPIScale, tristate);
				SetAspectCombo(hWnd, IDC_ASPECT, cfg->aspect, cfgmask->aspect, tristate);*/
			}
			break;
/*		case IDC_VIDMODE:
			cfg->scaler = GetCombo(hWnd,IDC_VIDMODE,&cfgmask->scaler);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_SORTMODES:
			cfg->SortModes = GetCombo(hWnd,IDC_SORTMODES,&cfgmask->SortModes);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_SCALE:
			cfg->scalingfilter = GetCombo(hWnd,IDC_SCALE,&cfgmask->scalingfilter);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_VSYNC:
			cfg->vsync = GetCombo(hWnd,IDC_VSYNC,&cfgmask->vsync);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_MSAA:
			cfg->msaa = GetCombo(hWnd,IDC_MSAA,&cfgmask->msaa);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_ANISO:
			cfg->anisotropic = GetCombo(hWnd,IDC_ANISO,&cfgmask->anisotropic);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_TEXFILTER:
			cfg->texfilter = GetCombo(hWnd,IDC_TEXFILTER,&cfgmask->texfilter);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_ASPECT3D:
			cfg->aspect3d = GetCombo(hWnd,IDC_ASPECT3D,&cfgmask->aspect3d);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_COLOR:
			cfg->colormode = GetCheck(hWnd,IDC_COLOR,&cfgmask->colormode);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_HIGHRES:
			cfg->highres = GetCheck(hWnd,IDC_HIGHRES,&cfgmask->highres);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_UNCOMMONCOLOR:
			cfg->AllColorDepths = GetCheck(hWnd,IDC_UNCOMMONCOLOR,&cfgmask->AllColorDepths);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_EXTRAMODES:
			cfg->ExtraModes = GetCheck(hWnd,IDC_EXTRAMODES,&cfgmask->ExtraModes);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_TEXTUREFORMAT:
			cfg->TextureFormat = GetCombo(hWnd,IDC_TEXTUREFORMAT,&cfgmask->TextureFormat);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_TEXUPLOAD:
			cfg->TexUpload = GetCombo(hWnd,IDC_TEXUPLOAD,&cfgmask->TexUpload);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
			*dirty = TRUE;
			break;
		case IDC_DPISCALE:
			cfg->DPIScale = GetCombo(hWnd,IDC_DPISCALE,&cfgmask->DPIScale);
			EnableWindow(GetDlgItem(hWnd, IDC_APPLY), TRUE);
			*dirty = TRUE;
			break;
		case IDC_SHADER:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				GetText(hWnd,IDC_SHADER,cfg->shaderfile,cfgmask->shaderfile);
				EnableWindow(GetDlgItem(hWnd,IDC_APPLY),TRUE);
				*dirty = TRUE;
			}
			break;
		case IDC_ASPECT:
			if (HIWORD(wParam) == CBN_KILLFOCUS)
			{
				cfg->aspect = GetAspectCombo(hWnd, IDC_ASPECT, &cfgmask->aspect);
				SetAspectCombo(hWnd, IDC_ASPECT, cfg->aspect, cfgmask->aspect, tristate);
				EnableWindow(GetDlgItem(hWnd, IDC_APPLY), TRUE);
				*dirty = TRUE;
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				cfg->aspect = GetAspectCombo(hWnd, IDC_ASPECT, &cfgmask->aspect);
				EnableWindow(GetDlgItem(hWnd, IDC_APPLY), TRUE);
				*dirty = TRUE;
			}
			break;*/
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
				DWORD err = AddApp(filename.lpstrFile, TRUE, FALSE);
				if (!err)
				{
					LPTSTR newkey = MakeNewConfig(filename.lpstrFile);
					LPTSTR newkey2 = (LPTSTR)malloc((_tcslen(newkey) + 24) * sizeof(TCHAR));
					_tcscpy(newkey2, _T("Software\\DXGL\\Profiles\\"));
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
						error = SHGetFileInfo(path, 0, &fileinfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_ADDOVERLAYS);
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
					SendDlgItemMessage(hWnd, IDC_APPS, LB_SETCURSEL,
						SendDlgItemMessage(hWnd, IDC_APPS, LB_ADDSTRING, 0, (LPARAM)apps[appcount - 1].name), 0);
					SendMessage(hWnd, WM_COMMAND, IDC_APPS + 0x10000, 0);
					RegCloseKey(hKey);
					free(regbuffer);
				}
			}
			break;
		case IDC_REMOVE:
			if(MessageBox(hWnd,_T("Do you want to delete the selected application profile and remove DXGL from its installation folder(s)?"),
				_T("Confirmation"),MB_YESNO|MB_ICONQUESTION) != IDYES) return FALSE;
			regpath = (LPTSTR)malloc((_tcslen(apps[current_app].regkey) + 15)*sizeof(TCHAR));
			_tcscpy(regpath, _T("Software\\DXGL\\Profiles\\"));
			_tcscat(regpath, apps[current_app].regkey);
			regkey = (LPTSTR)malloc(_tcslen(apps[current_app].regkey));
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
				if (DelApp(path, FALSE)) failed = TRUE;
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
			SendDlgItemMessage(hWnd,IDC_APPS,LB_DELETESTRING,current_app,0);
			SendDlgItemMessage(hWnd, IDC_APPS, LB_SETCURSEL, 0, 0);
			SendMessage(hWnd, WM_COMMAND, IDC_APPS + 0x10000, 0);
			break;
		}
		break;
	}
	return FALSE;
}

void UpgradeDXGL()
{
	UpgradeConfig();
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
	regbuffersize = 1024;
	regbuffer = (LPTSTR)malloc(regbuffersize * sizeof(TCHAR));
	RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DXGL\\Profiles"), 0, NULL, 0, KEY_READ, NULL, &hKeyBase, NULL);
	RegQueryInfoKey(hKeyBase, NULL, NULL, NULL, NULL, &keysize, NULL, NULL, NULL, NULL, NULL, NULL);
	keysize++;
	keyname = (LPTSTR)malloc(keysize * sizeof(TCHAR));
	keysize2 = keysize;
	error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
	if (error == ERROR_SUCCESS)
	{
		dxgl_installdir = TRUE;
		error = RegQueryValueEx(hKeyInstall, _T("InstallDir"), NULL, NULL, (LPBYTE)installpath, &sizeout);
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
			_tcscat(destpath, _T("\\"));
			_tcscat(destpath, _T("ddraw.dll"));
			error = CopyFile(srcpath, destpath, TRUE);
			if (!error)
			{
				error = GetLastError();
				if (error == ERROR_FILE_EXISTS)
				{
					old_dxgl = FALSE;
					HMODULE hmod = LoadLibrary(destpath);
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
	int i = 0;
	UpgradeConfig();  // Just to make sure the registry format is correct
	regbuffersize = 1024;
	regbuffer = (LPTSTR)malloc(regbuffersize * sizeof(TCHAR));
	error = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\DXGL\\Profiles"), 0, KEY_ALL_ACCESS, &hKeyBase);
	if (error != ERROR_SUCCESS) return;
	RegQueryInfoKey(hKeyBase, NULL, NULL, NULL, NULL, &keysize, NULL, NULL, NULL, NULL, NULL, NULL);
	keysize++;
	keyname = (LPTSTR)malloc(keysize * sizeof(TCHAR));
	keysize2 = keysize;
	error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\DXGL"), 0, KEY_READ, &hKeyInstall);
	if (error == ERROR_SUCCESS)
	{
		dxgl_installdir = TRUE;
		error = RegQueryValueEx(hKeyInstall, _T("InstallDir"), NULL, NULL, (LPBYTE)installpath, &sizeout);
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
			_tcscat(destpath, _T("\\"));
			_tcscat(destpath, _T("ddraw.dll"));
			if (GetFileAttributes(destpath) != INVALID_FILE_ATTRIBUTES)
			{
				old_dxgl = FALSE;
				HMODULE hmod = LoadLibrary(destpath);
				if (hmod)
				{
					if (GetProcAddress(hmod, "IsDXGLDDraw")) old_dxgl = TRUE;
					FreeLibrary(hmod);
				}
				if (_tcscmp(srcpath, destpath))
				{
					if (old_dxgl) DeleteFile(destpath);
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
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL\\Profiles"));
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL\\Global"));
		RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\DXGL"));
	}
	else RegCloseKey(hKeyBase);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR    lpCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX icc;
	HMODULE comctl32;
	BOOL(WINAPI *iccex)(LPINITCOMMONCONTROLSEX lpInitCtrls);
	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osver);
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
		return AddApp(lpCmdLine+8,TRUE,TRUE);
	}
	if(!_tcsnicmp(lpCmdLine,_T("remove "),7))
	{
		return DelApp(lpCmdLine+7,TRUE);
	}
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	comctl32 = LoadLibrary(_T("comctl32.dll"));
	iccex = (BOOL (WINAPI *)(LPINITCOMMONCONTROLSEX))GetProcAddress(comctl32,"InitCommonControlsEx");
	if(iccex) iccex(&icc);
	else InitCommonControls();
	hinstance = hInstance;
	GetModuleFileName(NULL,hlppath,MAX_PATH);
	GetDirFromPath(hlppath);
	_tcscat(hlppath,_T("\\dxgl.chm"));
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_DXGLCFG),0,(DLGPROC)DXGLCfgCallback);
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
