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

#define _WIN32_WINNT 0x0600
#define _WIN32_IE 0x0300
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <HtmlHelp.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include "resource.h"
#include "../cfgmgr/cfgmgr.h"
#include <gl/GL.h>
#include <string>

using namespace std;
#ifdef _UNICODE
typedef wstring tstring;
#else
typedef string tstring;
#endif

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
bool *dirty;
HINSTANCE hinstance;
bool msaa = false;
const char *extensions_string = NULL;
OSVERSIONINFO osver;
TCHAR hlppath[MAX_PATH+16];

typedef struct
{
	tstring *regkey;
	tstring *name;
	HICON icon;
	bool icon_shared;
	bool dirty;
	DXGLCFG cfg;
	DXGLCFG mask;
} app_setting;

TCHAR exe_filter[] = _T("Program Files\0*.exe\0All Files\0*.*\0\0");

app_setting *apps;
int appcount;
int maxapps;
DWORD current_app;
bool tristate;
TCHAR strdefault[] = _T("(global default)");

DWORD AddApp(LPCTSTR path, bool copyfile, bool admin)
{
	bool installed = false;
	bool dxgl_installdir = false;
	bool old_dxgl = false;
	tstring command;
	if(copyfile)
	{
		DWORD sizeout = (MAX_PATH+1)*sizeof(TCHAR);
		TCHAR installpath[MAX_PATH+1];
		TCHAR srcpath[MAX_PATH+1];
		TCHAR destpath[MAX_PATH+1];
		HKEY hKeyInstall;
		LONG error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\DXGL"),0,KEY_READ,&hKeyInstall);
		if(error == ERROR_SUCCESS)
		{
			dxgl_installdir = true;
			error = RegQueryValueEx(hKeyInstall,_T("InstallDir"),NULL,NULL,(LPBYTE)installpath,&sizeout);
			if(error == ERROR_SUCCESS) installed = true;
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
					if(GetProcAddress(hmod,"IsDXGLDDraw")) old_dxgl = true;
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
				command.assign(_T(" install "));
				command.append(path);
				SHELLEXECUTEINFO shex;
				ZeroMemory(&shex,sizeof(SHELLEXECUTEINFO));
				shex.cbSize = sizeof(SHELLEXECUTEINFO);
				shex.lpVerb = _T("runas");
				shex.fMask = SEE_MASK_NOCLOSEPROCESS;
				_tcscat(installpath,_T("\\dxglcfg.exe"));
				shex.lpFile = installpath;
				shex.lpParameters = command.c_str();
				ShellExecuteEx(&shex);
				WaitForSingleObject(shex.hProcess,INFINITE);
				DWORD exitcode;
				GetExitCodeProcess(shex.hProcess,&exitcode);
				return exitcode;
			}
			return error;
		}
	}
	return 0;
}

DWORD DelApp(LPCTSTR path, bool admin)
{
	bool installed = false;
	tstring command;
	bool old_dxgl = true;
	DWORD sizeout = (MAX_PATH+1)*sizeof(TCHAR);
	TCHAR installpath[MAX_PATH+1];
	HKEY hKeyInstall;
	LONG error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\DXGL"),0,KEY_READ,&hKeyInstall);
	if(error == ERROR_SUCCESS)
	{
		error = RegQueryValueEx(hKeyInstall,_T("InstallDir"),NULL,NULL,(LPBYTE)installpath,&sizeout);
		if(error == ERROR_SUCCESS) installed = true;
	}
	if(hKeyInstall) RegCloseKey(hKeyInstall);
	if(!installed)
	{
		GetModuleFileName(NULL,installpath,MAX_PATH+1);
	}
	HMODULE hmod = LoadLibrary(path);
	if(hmod)
	{
		if(!GetProcAddress(hmod,"IsDXGLDDraw")) old_dxgl = false;
		FreeLibrary(hmod);
	}
	if(!old_dxgl) return 0;
	if(!DeleteFile(path))
	{
		error = GetLastError();
		if(error == ERROR_FILE_NOT_FOUND) return 0;
		if((error == ERROR_ACCESS_DENIED) && !admin)
		{
			command.assign(_T(" remove "));
			command.append(path);
			SHELLEXECUTEINFO shex;
			ZeroMemory(&shex,sizeof(SHELLEXECUTEINFO));
			shex.cbSize = sizeof(SHELLEXECUTEINFO);
			shex.lpVerb = _T("runas");
			shex.fMask = SEE_MASK_NOCLOSEPROCESS;
			_tcscat(installpath,_T("\\dxglcfg.exe"));
			shex.lpFile = installpath;
			shex.lpParameters = command.c_str();
			ShellExecuteEx(&shex);
			WaitForSingleObject(shex.hProcess,INFINITE);
			DWORD exitcode;
			GetExitCodeProcess(shex.hProcess,&exitcode);
			return exitcode;
		}
		return error;
	}
	return 0;
}

void SaveChanges(HWND hWnd)
{
	if(apps[0].dirty) SetGlobalConfig(&apps[0].cfg);
	for(int i = 1; i < appcount; i++)
	{
		if(apps[i].dirty) SetConfig(&apps[i].cfg,&apps[i].mask,apps[i].regkey->c_str());
	}
	EnableWindow(GetDlgItem(hWnd,IDC_APPLY),FALSE);
}

void FloatToAspect(float f, LPTSTR aspect)
{
	float integer;
	float dummy;
	TCHAR denominator[5];
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
	if (abs(f - 1.25f) < 0.0001f)
	{
		_tcscpy(aspect, _T("5:4"));
		return;
	}
	if (abs(f - 1.3333333f) < 0.0001f)
	{
		_tcscpy(aspect, _T("4:3"));
		return;
	}
	if (abs(f - 1.6f) < 0.0001f)
	{
		_tcscpy(aspect, _T("16:10"));
		return;
	}
	if (abs(f - 1.7777777) < 0.0001f)
	{
		_tcscpy(aspect, _T("16:9"));
		return;
	}
	if (abs(f - 1.9333333) < 0.0001f)
	{
		_tcscpy(aspect, _T("256:135"));
		return;
	}
	float fract = modf(f, &integer);
	if (fract < 0.0001f)  //Handle integer aspects
	{
		_itot((int)integer, aspect, 10);
		_tcscat(aspect, _T(":1"));
		return;
	}
	// Finally try from 2 to 1000
	for (int i = 2; i < 1000; i++)
	{
		if (abs(modf(fract*i, &dummy)) < 0.0001f)
		{
			_itot((f*i) + .5f, aspect, 10);
			_itot(i, denominator, 10);
			_tcscat(aspect, _T(":"));
			_tcscat(aspect, denominator);
			return;
		}
	}
	// Cannot find a reasonable fractional aspect, so display as decimal.
	_stprintf(aspect, _T("%.6f"), f);
}

void SetCheck(HWND hWnd, int DlgItem, bool value, bool mask, bool tristate)
{
	if(tristate && !mask)
		SendDlgItemMessage(hWnd,DlgItem,BM_SETCHECK,BST_INDETERMINATE,0);
	else
	{
		if(value) SendDlgItemMessage(hWnd,DlgItem,BM_SETCHECK,BST_CHECKED,0);
		else SendDlgItemMessage(hWnd,DlgItem,BM_SETCHECK,BST_UNCHECKED,0);
	}
}

void SetCombo(HWND hWnd, int DlgItem, DWORD value, DWORD mask, bool tristate)
{
	if(tristate && !mask)
		SendDlgItemMessage(hWnd,DlgItem,CB_SETCURSEL,
		SendDlgItemMessage(hWnd,DlgItem,CB_FINDSTRING,-1,(LPARAM)strdefault),0);
	else
		SendDlgItemMessage(hWnd,DlgItem,CB_SETCURSEL,value,0);
}

void SetAspectCombo(HWND hWnd, int DlgItem, float value, DWORD mask, bool tristate)
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

void SetText(HWND hWnd, int DlgItem, TCHAR *value, TCHAR *mask, bool tristate)
{
	if(tristate && (mask[0] == 0))
		SetWindowText(GetDlgItem(hWnd,DlgItem),_T(""));
	else SetWindowText(GetDlgItem(hWnd,DlgItem),value);
}

bool GetCheck(HWND hWnd, int DlgItem, bool &mask)
{
	int check = SendDlgItemMessage(hWnd,DlgItem,BM_GETCHECK,0,0);
	switch(check)
	{
	case BST_CHECKED:
		mask = true;
		return true;
	case BST_UNCHECKED:
		mask = true;
		return false;
	case BST_INDETERMINATE:
	default:
		mask = false;
		return false;
	}
}

DWORD GetCombo(HWND hWnd, int DlgItem, DWORD &mask)
{
	int value = SendDlgItemMessage(hWnd,DlgItem,CB_GETCURSEL,0,0);
	if(value == SendDlgItemMessage(hWnd,DlgItem,CB_FINDSTRING,-1,(LPARAM)strdefault))
	{
		mask = 0;
		return 0;
	}
	else
	{
		mask = 1;
		return value;
	}
}

float GetAspectCombo(HWND hWnd, int DlgItem, float &mask)
{
	TCHAR buffer[32];
	TCHAR *ptr;
	float numerator, denominator;
	GetDlgItemText(hWnd, DlgItem, buffer, 31);
	if (!_tcscmp(buffer, strdefault))
	{
		mask = 0.0f;
		return 0;
	}
	else
	{
		mask = 1.0f;
		if (!_tcscmp(buffer, _T("Default"))) return 0.0f;
		else
		{
			// Check for colon
			ptr = _tcsstr(buffer, _T(":"));
			if (ptr)
			{
				*ptr = 0;
				numerator = _ttof(buffer);
				denominator = _ttof(ptr + 1);
				return numerator / denominator;
			}
			else return _ttof(buffer);
		}
	}
}

void GetText(HWND hWnd, int DlgItem, TCHAR *str, TCHAR *mask)
{
	GetDlgItemText(hWnd,DlgItem,str,MAX_PATH+1);
	if(str[0] == 0) mask[0] = 0;
	else mask[0] = 0xff;
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
	DWORD regbufferpos;
	DWORD buffersize;
	LONG error;
	TCHAR buffer[64];
	tstring subkey;
	tstring path;
	SHFILEINFO fileinfo;
	DWORD verinfosize;
	LPTSTR outbuffer;
	UINT outlen;
	TCHAR verpath[64];
	WORD translation[2];
	DWORD cursel;
	DRAWITEMSTRUCT* drawitem = (DRAWITEMSTRUCT*)lParam;
	bool hasname;
	void *verinfo;
	COLORREF OldTextColor,OldBackColor;
	HANDLE token = NULL;
	TOKEN_ELEVATION elevation;
	HWND hGLWnd;
	switch(Msg)
	{
	case WM_INITDIALOG:
		tristate = false;
		maxapps = 128;
		apps = (app_setting *)malloc(maxapps*sizeof(app_setting));
		apps[0].name = new tstring(_T("Global"));
		apps[0].regkey = new tstring(_T("Global"));
		GetGlobalConfig(&apps[0].cfg);
		cfg = &apps[0].cfg;
		cfgmask = &apps[0].mask;
		dirty = &apps[0].dirty;
		memset(&apps[0].mask,0xff,sizeof(DXGLCFG));
		apps[0].dirty = false;
		apps[0].icon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_STAR));
		apps[0].icon_shared = true;
		SetClassLong(hWnd,GCL_HICON,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGL));
		SetClassLong(hWnd,GCL_HICONSM,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGLSM));
		// create temporary gl context to get AA and AF settings.
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
					msaa = true;
				}
			}
		}
		wglMakeCurrent(dc,NULL);
		wglDeleteContext(rc);
		ReleaseDC(hGLWnd,dc);
		DestroyWindow(hGLWnd);
		// Load global settings.
		// scaler
		_tcscpy(buffer,_T("Change desktop resolution"));
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Stretch to screen"));
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Aspect corrected stretch"));
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,2,(LPARAM)buffer);
		_tcscpy(buffer,_T("Center image on screen"));
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,3,(LPARAM)buffer);
		_tcscpy(buffer,_T("Stretch if mode not found"));
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,4,(LPARAM)buffer);
		_tcscpy(buffer,_T("Scale if mode not found"));
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,5,(LPARAM)buffer);
		_tcscpy(buffer,_T("Center if mode not found"));
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_ADDSTRING,6,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_SETCURSEL,cfg->scaler,0);
		// colormode
		if(cfg->colormode) SendDlgItemMessage(hWnd,IDC_COLOR,BM_SETCHECK,BST_CHECKED,0);
		else SendDlgItemMessage(hWnd,IDC_COLOR,BM_SETCHECK,BST_UNCHECKED,0);
		// scalingfilter
		_tcscpy(buffer,_T("Nearest"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Custom shader"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,2,(LPARAM)buffer);
		_tcscpy(buffer,_T("Shader (primary only)"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,3,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_SETCURSEL,cfg->scalingfilter,0);
		// aspect
		_tcscpy(buffer,_T("Default"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("4:3"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("16:10"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("16:9"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("5:4"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,0,(LPARAM)buffer);
		SetAspectCombo(hWnd, IDC_ASPECT, cfg->aspect, cfgmask->aspect, tristate);
		
		// highres
		if(cfg->highres) SendDlgItemMessage(hWnd,IDC_HIGHRES,BM_SETCHECK,BST_CHECKED,0);
		else SendDlgItemMessage(hWnd,IDC_HIGHRES,BM_SETCHECK,BST_UNCHECKED,0);
		// texfilter
		_tcscpy(buffer,_T("Application default"));
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest"));
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear"));
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,2,(LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest, nearest mipmap"));
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,3,(LPARAM)buffer);
		_tcscpy(buffer,_T("Nearest, linear mipmap"));
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,4,(LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear, nearest mipmap"));
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,5,(LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear, linear mipmap"));
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_ADDSTRING,6,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_SETCURSEL,cfg->texfilter,0);
		// anisotropic
		if (anisotropic < 2)
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,0,(LPARAM)buffer);
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_SETCURSEL,0,0);
			EnableWindow(GetDlgItem(hWnd,IDC_ANISO),FALSE);
			cfg->anisotropic = 0;
		}
		else
		{
			_tcscpy(buffer,_T("Application default"));
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,0,(LPARAM)buffer);
			_tcscpy(buffer,_T("Disabled"));
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,1,(LPARAM)buffer);
			if(anisotropic >= 2)
			{
				_tcscpy(buffer,_T("2x"));
				SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,2,(LPARAM)buffer);
			}
			if(anisotropic >= 4)
			{
				_tcscpy(buffer,_T("4x"));
				SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,4,(LPARAM)buffer);
			}
			if(anisotropic >= 8)
			{
				_tcscpy(buffer,_T("8x"));
				SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,8,(LPARAM)buffer);
			}
			if(anisotropic >= 16)
			{
				_tcscpy(buffer,_T("16x"));
				SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,16,(LPARAM)buffer);
			}
			if(anisotropic >= 32)
			{
				_tcscpy(buffer,_T("32x"));
				SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,4,(LPARAM)buffer);
			}
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_SETCURSEL,cfg->anisotropic,0);
		}
		// msaa
		if(msaa)
		{
			_tcscpy(buffer,_T("Application default"));
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,0,(LPARAM)buffer);
			_tcscpy(buffer,_T("Disabled"));
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,1,(LPARAM)buffer);
			if(maxcoverage)
			{
				for(i = 0; i < maxcoverage; i++)
				{
					if((msaamodes[i] & 0xfff) <= 4)
						_sntprintf(buffer,64,_T("%dx"),msaamodes[i] & 0xfff);
					else _sntprintf(buffer,64,_T("%dx coverage, %dx color"),(msaamodes[i] & 0xfff), (msaamodes[i] >> 12));
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,msaamodes[i],(LPARAM)buffer);
				}
			}
			else
			{
				if(maxsamples >= 2)
				{
					_tcscpy(buffer,_T("2x"));
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,2,(LPARAM)buffer);
				}
				if(maxsamples >= 4)
				{
					_tcscpy(buffer,_T("4x"));
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,4,(LPARAM)buffer);
				}
				if(maxsamples >= 8)
				{
					_tcscpy(buffer,_T("8x"));
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,8,(LPARAM)buffer);
				}
				if(maxsamples >= 16)
				{
					_tcscpy(buffer,_T("16x"));
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,16,(LPARAM)buffer);
				}
				if(maxsamples >= 32)
				{
					_tcscpy(buffer,_T("32x"));
					SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,32,(LPARAM)buffer);
				}
			}
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_SETCURSEL,cfg->msaa,0);
		}
		else
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,0,(LPARAM)buffer);
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_SETCURSEL,0,0);
			EnableWindow(GetDlgItem(hWnd,IDC_MSAA),FALSE);
			cfg->msaa = 0;
		}
		// aspect3d
		_tcscpy(buffer,_T("Stretch to display"));
		SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Expand viewable area"));
		SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Crop to display"));
		SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_ASPECT3D,CB_SETCURSEL,cfg->aspect3d,0);
		// sort modes
		_tcscpy(buffer,_T("Use system order"));
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Group by color depth"));
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Group by resolution"));
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_SETCURSEL,cfg->SortModes,0);
		// color depths
		if(cfg->AllColorDepths) SendDlgItemMessage(hWnd,IDC_UNCOMMONCOLOR,BM_SETCHECK,BST_CHECKED,0);
		else SendDlgItemMessage(hWnd,IDC_UNCOMMONCOLOR,BM_SETCHECK,BST_UNCHECKED,0);
		// extra modes
		if(cfg->ExtraModes) SendDlgItemMessage(hWnd,IDC_EXTRAMODES,BM_SETCHECK,BST_CHECKED,0);
		else SendDlgItemMessage(hWnd,IDC_EXTRAMODES,BM_SETCHECK,BST_UNCHECKED,0);
		// shader path
		SetText(hWnd,IDC_SHADER,cfg->shaderfile,cfgmask->shaderfile,false);
		// texture format
		_tcscpy(buffer,_T("Automatic"));
		SendDlgItemMessage(hWnd,IDC_TEXTUREFORMAT,CB_ADDSTRING,0,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_TEXTUREFORMAT,CB_SETCURSEL,cfg->TextureFormat,0);
		// Texture upload
		_tcscpy(buffer,_T("Automatic"));
		SendDlgItemMessage(hWnd,IDC_TEXUPLOAD,CB_ADDSTRING,0,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_TEXUPLOAD,CB_SETCURSEL,cfg->TexUpload,0);
		// DPI
		_tcscpy(buffer, _T("Disabled"));
		SendDlgItemMessage(hWnd,IDC_DPISCALE,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer, _T("Enabled"));
		SendDlgItemMessage(hWnd,IDC_DPISCALE,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer, _T("Windows AppCompat"));
		SendDlgItemMessage(hWnd,IDC_DPISCALE,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_DPISCALE,CB_SETCURSEL,cfg->DPIScale,0);
		// Add installed programs
		current_app = 1;
		appcount = 1;
		regbuffersize = 1024;
		regbuffer = (LPTSTR)malloc(regbuffersize*sizeof(TCHAR));
		RegCreateKeyEx(HKEY_CURRENT_USER,_T("Software\\DXGL"),0,NULL,0,KEY_READ,NULL,&hKeyBase,NULL);
		RegQueryInfoKey(hKeyBase,NULL,NULL,NULL,NULL,&keysize,NULL,NULL,NULL,NULL,NULL,NULL);
		keysize++;
		keyname = (LPTSTR)malloc(keysize*sizeof(TCHAR));
		keysize2 = keysize;
		i = 0;
		while(RegEnumKeyEx(hKeyBase,i,keyname,&keysize2,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
		{
			keysize2 = keysize;
			i++;
			if(!_tcscmp(keyname,_T("Global"))) continue;
			appcount++;
			if(appcount > maxapps)
			{
				maxapps += 128;
				apps = (app_setting *)realloc(apps,maxapps*sizeof(app_setting));
			}
			if(!_tcscmp(keyname,_T("DXGLTestApp"))) subkey = _T("dxgltest.exe-0");
			else subkey = keyname;
			if(subkey.rfind(_T("-")) != string::npos) subkey.resize(subkey.rfind(_T("-")));
			error = RegOpenKeyEx(hKeyBase,keyname,0,KEY_READ,&hKey);
			buffersize = regbuffersize;
			RegQueryValueEx(hKey,_T("InstallPaths"),NULL,NULL,NULL,&buffersize);
			if(buffersize > regbuffersize)
			{
				regbuffersize = buffersize;
				regbuffer = (LPTSTR)realloc(regbuffer,regbuffersize);
			}
			buffersize = regbuffersize;
			regbuffer[0] = regbuffer[1] = 0;
			error = RegQueryValueEx(hKey,_T("InstallPaths"),NULL,NULL,(LPBYTE)regbuffer,&buffersize);
			regbufferpos = 0;
			apps[appcount-1].regkey = new tstring(keyname);
			GetConfig(&apps[appcount-1].cfg,&apps[appcount-1].mask,keyname);
			apps[appcount-1].dirty = false;
			while(1)
			{
				if((regbuffer[regbufferpos] == 0) || error != ERROR_SUCCESS)
				{
					// Default icon
					apps[appcount-1].icon = LoadIcon(NULL,IDI_APPLICATION);
					apps[appcount-1].icon_shared = true;
					apps[appcount-1].name = new tstring(subkey);
					break;
				}
				path = tstring(((LPTSTR)regbuffer+regbufferpos))+tstring(_T("\\"))+subkey;
				if(GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
				{
					regbufferpos += (_tcslen(regbuffer+regbufferpos)+1);
					continue;
				}
				// Get exe attributes
				error = SHGetFileInfo(path.c_str(),0,&fileinfo,sizeof(SHFILEINFO),SHGFI_ICON|SHGFI_SMALLICON|SHGFI_ADDOVERLAYS);
				apps[appcount-1].icon = fileinfo.hIcon;
				apps[appcount-1].icon_shared = false;
				verinfosize = GetFileVersionInfoSize(path.c_str(),NULL);
				verinfo = malloc(verinfosize);
				hasname = false;
				if(GetFileVersionInfo(path.c_str(),0,verinfosize,verinfo))
				{
					if(VerQueryValue(verinfo,_T("\\VarFileInfo\\Translation"),(LPVOID*)&outbuffer,&outlen))
					{
						memcpy(translation,outbuffer,4);
						_sntprintf(verpath,64,_T("\\StringFileInfo\\%04x%04x\\FileDescription"),translation[0],translation[1]);
						if(VerQueryValue(verinfo,verpath,(LPVOID*)&outbuffer,&outlen))
						{
							hasname = true;
							apps[appcount-1].name = new tstring(outbuffer);
						}
						else
						{
							_sntprintf(verpath,64,_T("\\StringFileInfo\\%04x%04x\\ProductName"),((WORD*)outbuffer)[0],((WORD*)outbuffer)[1]);
							if(VerQueryValue(verinfo,verpath,(LPVOID*)&outbuffer,&outlen))
							{
								hasname = true;
								apps[appcount-1].name = new tstring(outbuffer);
							}
							else
							{
								_sntprintf(verpath,64,_T("\\StringFileInfo\\%04x%04x\\InternalName"),((WORD*)outbuffer)[0],((WORD*)outbuffer)[1]);
								if(VerQueryValue(verinfo,verpath,(LPVOID*)&outbuffer,&outlen))
								{
									hasname = true;
									apps[appcount-1].name = new tstring(outbuffer);
								}
							}
						}
					}
				}
				if(!hasname) apps[appcount-1].name = new tstring(subkey);
				free(verinfo);
				break;
			}
			RegCloseKey(hKey);
		}
		RegCloseKey(hKeyBase);
		free(keyname);
		for(i = 0; i < appcount; i++)
		{
			SendDlgItemMessage(hWnd,IDC_APPS,LB_ADDSTRING,0,(LPARAM)apps[i].name->c_str());
		}
		current_app = 0;
		SendDlgItemMessage(hWnd,IDC_APPS,LB_SETCURSEL,0,0);
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
			EnableWindow(GetDlgItem(hWnd, IDC_DPISCALE), FALSE);
		}
		if(token) CloseHandle(token);
		return true;
	case WM_MEASUREITEM:
		switch(wParam)
		{
		case IDC_APPS:
			((LPMEASUREITEMSTRUCT)lParam)->itemHeight = GetSystemMetrics(SM_CYSMICON)+1;
			((LPMEASUREITEMSTRUCT)lParam)->itemWidth = GetSystemMetrics(SM_CXSMICON)+1;
		default:
			break;
		}
		break;
	case WM_DRAWITEM:
		switch(wParam)
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
			DrawText(drawitem->hDC,apps[drawitem->itemID].name->c_str(),
				apps[drawitem->itemID].name->length(),&drawitem->rcItem,
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
		return true;
		break;
	case WM_SYSCOMMAND:
		if(LOWORD(wParam) == SC_CONTEXTHELP)
		{
			HtmlHelp(hWnd,hlppath,HH_DISPLAY_TOPIC,(DWORD_PTR)_T("configuration.htm"));
			return true;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			SaveChanges(hWnd);
			EndDialog(hWnd,IDOK);
			return true;
		case IDCANCEL:
			EndDialog(hWnd,IDCANCEL);
			return true;
		case IDC_APPLY:
			SaveChanges(hWnd);
			return true;
		case IDC_APPS:
			if(HIWORD(wParam) == LBN_SELCHANGE)
			{
				cursel = SendDlgItemMessage(hWnd,IDC_APPS,LB_GETCURSEL,0,0);
				if(cursel == current_app) break;
				current_app = cursel;
				cfg = &apps[current_app].cfg;
				cfgmask = &apps[current_app].mask;
				dirty = &apps[current_app].dirty;
				if(current_app)
				{
					if(!_tcscmp(apps[current_app].regkey->c_str(),_T("DXGLTestApp"))) EnableWindow(GetDlgItem(hWnd,IDC_REMOVE),false);
					else EnableWindow(GetDlgItem(hWnd,IDC_REMOVE),true);
				}
				else EnableWindow(GetDlgItem(hWnd,IDC_REMOVE),false);
				// Set 3-state status
				if(current_app && !tristate)
				{
					tristate = true;
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
					tristate = false;
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
				SetAspectCombo(hWnd, IDC_ASPECT, cfg->aspect, cfgmask->aspect, tristate);
			}
		case IDC_VIDMODE:
			cfg->scaler = GetCombo(hWnd,IDC_VIDMODE,cfgmask->scaler);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_SORTMODES:
			cfg->SortModes = GetCombo(hWnd,IDC_SORTMODES,cfgmask->SortModes);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_SCALE:
			cfg->scalingfilter = GetCombo(hWnd,IDC_SCALE,cfgmask->scalingfilter);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_VSYNC:
			cfg->vsync = GetCombo(hWnd,IDC_VSYNC,cfgmask->vsync);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_MSAA:
			cfg->msaa = GetCombo(hWnd,IDC_MSAA,cfgmask->msaa);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_ANISO:
			cfg->anisotropic = GetCombo(hWnd,IDC_ANISO,cfgmask->anisotropic);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_TEXFILTER:
			cfg->texfilter = GetCombo(hWnd,IDC_TEXFILTER,cfgmask->texfilter);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_ASPECT3D:
			cfg->aspect3d = GetCombo(hWnd,IDC_ASPECT3D,cfgmask->aspect3d);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_COLOR:
			cfg->colormode = GetCheck(hWnd,IDC_COLOR,cfgmask->colormode);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_HIGHRES:
			cfg->highres = GetCheck(hWnd,IDC_HIGHRES,cfgmask->highres);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_UNCOMMONCOLOR:
			cfg->AllColorDepths = GetCheck(hWnd,IDC_UNCOMMONCOLOR,cfgmask->AllColorDepths);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_EXTRAMODES:
			cfg->ExtraModes = GetCheck(hWnd,IDC_EXTRAMODES,cfgmask->ExtraModes);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_TEXTUREFORMAT:
			cfg->TextureFormat = GetCombo(hWnd,IDC_TEXTUREFORMAT,cfgmask->TextureFormat);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_TEXUPLOAD:
			cfg->TexUpload = GetCombo(hWnd,IDC_TEXUPLOAD,cfgmask->TexUpload);
			EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
			*dirty = true;
			break;
		case IDC_DPISCALE:
			cfg->DPIScale = GetCombo(hWnd,IDC_DPISCALE,cfgmask->DPIScale);
			EnableWindow(GetDlgItem(hWnd, IDC_APPLY), true);
			*dirty = true;
			break;
		case IDC_SHADER:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				GetText(hWnd,IDC_SHADER,cfg->shaderfile,cfgmask->shaderfile);
				EnableWindow(GetDlgItem(hWnd,IDC_APPLY),true);
				*dirty = true;
			}
			break;
		case IDC_ASPECT:
			if (HIWORD(wParam) == CBN_KILLFOCUS)
			{
				cfg->aspect = GetAspectCombo(hWnd, IDC_ASPECT, cfgmask->aspect);
				SetAspectCombo(hWnd, IDC_ASPECT, cfg->aspect, cfgmask->aspect, tristate);
				EnableWindow(GetDlgItem(hWnd, IDC_APPLY), true);
				*dirty = true;
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				cfg->aspect = GetAspectCombo(hWnd, IDC_ASPECT, cfgmask->aspect);
				EnableWindow(GetDlgItem(hWnd, IDC_APPLY), true);
				*dirty = true;
			}
			break;
		case IDC_ADD:
			OPENFILENAME filename;
			TCHAR selectedfile[MAX_PATH+1];
			selectedfile[0] = 0;
			ZeroMemory(&filename,OPENFILENAME_SIZE_VERSION_400);
			filename.lStructSize = OPENFILENAME_SIZE_VERSION_400;
			filename.hwndOwner = hWnd;
			filename.lpstrFilter = exe_filter;
			filename.lpstrFile = selectedfile;
			filename.nMaxFile = MAX_PATH+1;
			filename.lpstrInitialDir = _T("%ProgramFiles%");
			filename.lpstrTitle = _T("Select program");
			filename.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if(GetOpenFileName(&filename))
			{
				DWORD err = AddApp(filename.lpstrFile,true,false);
				if(!err)
				{
					tstring newkey = MakeNewConfig(filename.lpstrFile);
					tstring newkey2 = _T("Software\\DXGL\\") + newkey;
					appcount++;
					if(appcount > maxapps)
					{
						maxapps += 128;
						apps = (app_setting *)realloc(apps,maxapps*sizeof(app_setting));
					}
					RegOpenKeyEx(HKEY_CURRENT_USER,newkey2.c_str(),0,KEY_READ,&hKey);
					RegQueryValueEx(hKey,_T("InstallPaths"),NULL,NULL,NULL,&buffersize);
					regbuffer = (LPTSTR)malloc(buffersize);
					regbuffer[0] = regbuffer[1] = 0;
					error = RegQueryValueEx(hKey,_T("InstallPaths"),NULL,NULL,(LPBYTE)regbuffer,&buffersize);
					regbufferpos = 0;
					apps[appcount-1].regkey = new tstring(newkey);
					GetConfig(&apps[appcount-1].cfg,&apps[appcount-1].mask,newkey.c_str());
					apps[appcount-1].dirty = false;
					while(1)
					{
						if((regbuffer[regbufferpos] == 0) || error != ERROR_SUCCESS)
						{
							// Default icon
							apps[appcount-1].icon = LoadIcon(NULL,IDI_APPLICATION);
							apps[appcount-1].icon_shared = true;
							apps[appcount-1].name = new tstring(newkey);
							break;
						}
						if(newkey.rfind(_T("-")) != string::npos) newkey.resize(newkey.rfind(_T("-")));
						path = tstring(((LPTSTR)regbuffer+regbufferpos))+tstring(_T("\\"))+newkey;
						if(GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
						{
							regbufferpos += (_tcslen(regbuffer+regbufferpos)+1);
							continue;
						}
						// Get exe attributes
						error = SHGetFileInfo(path.c_str(),0,&fileinfo,sizeof(SHFILEINFO),SHGFI_ICON|SHGFI_SMALLICON|SHGFI_ADDOVERLAYS);
						apps[appcount-1].icon = fileinfo.hIcon;
						apps[appcount-1].icon_shared = false;
						verinfosize = GetFileVersionInfoSize(path.c_str(),NULL);
						verinfo = malloc(verinfosize);
						hasname = false;
						if(GetFileVersionInfo(path.c_str(),0,verinfosize,verinfo))
						{
							if(VerQueryValue(verinfo,_T("\\VarFileInfo\\Translation"),(LPVOID*)&outbuffer,&outlen))
							{
								memcpy(translation,outbuffer,4);
								_sntprintf(verpath,64,_T("\\StringFileInfo\\%04x%04x\\FileDescription"),translation[0],translation[1]);
								if(VerQueryValue(verinfo,verpath,(LPVOID*)&outbuffer,&outlen))
								{
									hasname = true;
									apps[appcount-1].name = new tstring(outbuffer);
								}
								else
								{
									_sntprintf(verpath,64,_T("\\StringFileInfo\\%04x%04x\\ProductName"),((WORD*)outbuffer)[0],((WORD*)outbuffer)[1]);
									if(VerQueryValue(verinfo,verpath,(LPVOID*)&outbuffer,&outlen))
									{
										hasname = true;
										apps[appcount-1].name = new tstring(outbuffer);
									}
									else
									{
										_sntprintf(verpath,64,_T("\\StringFileInfo\\%04x%04x\\InternalName"),((WORD*)outbuffer)[0],((WORD*)outbuffer)[1]);
										if(VerQueryValue(verinfo,verpath,(LPVOID*)&outbuffer,&outlen))
										{
											hasname = true;
											apps[appcount-1].name = new tstring(outbuffer);
										}
									}
								}
							}
						}
						if(!hasname) apps[appcount-1].name = new tstring(newkey);
						free(verinfo);
						break;
					}
					SendDlgItemMessage(hWnd,IDC_APPS,LB_ADDSTRING,0,(LPARAM)apps[appcount-1].name->c_str());
					RegCloseKey(hKey);
					free(regbuffer);
				}
			}

			break;
		case IDC_REMOVE:
			if(MessageBox(hWnd,_T("Do you want to delete the selected application profile and remove DXGL from its installation folder(s)?"),
				_T("Confirmation"),MB_YESNO|MB_ICONQUESTION) != IDYES) return false;
			tstring regpath = _T("Software\\DXGL\\");
			tstring regkey = *apps[current_app].regkey;
			regpath.append(*apps[current_app].regkey);
			RegOpenKeyEx(HKEY_CURRENT_USER,regpath.c_str(),0,KEY_READ,&hKey);
			RegQueryValueEx(hKey,_T("InstallPaths"),NULL,NULL,NULL,&buffersize);
			regbuffer = (LPTSTR)malloc(buffersize);
			regbuffer[0] = regbuffer[1] = 0;
			error = RegQueryValueEx(hKey,_T("InstallPaths"),NULL,NULL,(LPBYTE)regbuffer,&buffersize);
			regbufferpos = 0;
			bool failed = false;
			while(1)
			{
				if((regbuffer[regbufferpos] == 0) || error != ERROR_SUCCESS) break;
				if(regkey.rfind(_T("-")) != string::npos) regkey.resize(regkey.rfind(_T("-")));
				path = tstring(((LPTSTR)regbuffer+regbufferpos))+tstring(_T("\\ddraw.dll"));
				if(GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
				{
					regbufferpos += (_tcslen(regbuffer+regbufferpos)+1);
					continue;
				}
				if(DelApp(path.c_str(),false)) failed = true;
				regbufferpos += (_tcslen(regbuffer+regbufferpos)+1);
			}
			RegCloseKey(hKey);
			if(!failed)
			{
				RegDeleteKey(HKEY_CURRENT_USER,regpath.c_str());
				if(!apps[current_app].icon_shared) DeleteObject(apps[current_app].icon);
				if(apps[current_app].name) delete apps[current_app].name;
				if(apps[current_app].regkey) delete apps[current_app].regkey;
				for(int i = current_app; i < appcount; i++)
				{
					apps[i] = apps[i+1];
				}
				appcount--;
			}
			SendDlgItemMessage(hWnd,IDC_APPS,LB_DELETESTRING,current_app,0);
			break;
		}
		break;
	}
	return false;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR    lpCmdLine, int nCmdShow)
{
	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osver);
	CoInitialize(NULL);
	INITCOMMONCONTROLSEX icc;
	if(!_tcsnicmp(lpCmdLine,_T("install "),8))
	{
		return AddApp(lpCmdLine+8,true,true);
	}
	if(!_tcsnicmp(lpCmdLine,_T("remove "),7))
	{
		return DelApp(lpCmdLine+7,true);
	}
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	HMODULE comctl32 = LoadLibrary(_T("comctl32.dll"));
	BOOL (WINAPI *iccex)(LPINITCOMMONCONTROLSEX lpInitCtrls) =
		(BOOL (WINAPI *)(LPINITCOMMONCONTROLSEX))GetProcAddress(comctl32,"InitCommonControlsEx");
	if(iccex) iccex(&icc);
	else InitCommonControls();
	hinstance = hInstance;
	GetModuleFileName(NULL,hlppath,MAX_PATH);
	GetDirFromPath(hlppath);
	_tcscat(hlppath,_T("\\dxgl.chm"));
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_DXGLCFG),0,reinterpret_cast<DLGPROC>(DXGLCfgCallback));
	return 0;
}
