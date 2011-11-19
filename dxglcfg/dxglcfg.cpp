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

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include "resource.h"
#include "../cfgmgr/cfgmgr.h"
#include <gl/GL.h>

#define GL_TEXTURE_MAX_ANISOTROPY_EXT          0x84FE
#define GL_MAX_SAMPLES_EXT                     0x8D57
#define GL_MAX_MULTISAMPLE_COVERAGE_MODES_NV        0x8E11
#define GL_MULTISAMPLE_COVERAGE_MODES_NV            0x8E12
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT      0x84FF

DXGLCFG globalcfg;
DXGLCFG currentcfg;
HINSTANCE hinstance;
bool msaa = false;
const char *extensions_string = NULL;

void SaveChanges(HWND hWnd)
{
	globalcfg.scaler = SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_GETCURSEL,0,0);
	globalcfg.colormode = SendDlgItemMessage(hWnd,IDC_COLOR,BM_GETCHECK,0,0);
	globalcfg.scalingfilter = SendDlgItemMessage(hWnd,IDC_SCALE,CB_GETCURSEL,0,0);
	globalcfg.vsync = SendDlgItemMessage(hWnd,IDC_VSYNC,CB_GETCURSEL,0,0);
	SendDlgItemMessageW(hWnd,IDC_SHADER,WM_GETTEXT,MAX_PATH+1,(LPARAM)globalcfg.shaderfile);
	globalcfg.texfilter = SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_GETCURSEL,0,0);
	globalcfg.anisotropic = SendDlgItemMessage(hWnd,IDC_ANISO,CB_GETCURSEL,0,0);
	globalcfg.msaa = SendDlgItemMessage(hWnd,IDC_MSAA,CB_GETCURSEL,0,0);
	globalcfg.aspect = SendDlgItemMessage(hWnd,IDC_ASPECT,CB_GETCURSEL,0,0);
	globalcfg.highres = SendDlgItemMessage(hWnd,IDC_HIGHRES,BM_GETCHECK,0,0);
	globalcfg.AllColorDepths = SendDlgItemMessage(hWnd,IDC_UNCOMMONCOLOR,BM_GETCHECK,0,0);
	globalcfg.ExtraModes = SendDlgItemMessage(hWnd,IDC_EXTRAMODES,BM_GETCHECK,0,0);
	globalcfg.SortModes = SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_GETCURSEL,0,0);
	SetGlobalConfig(&globalcfg);
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
	DEVMODE mode;
	TCHAR buffer[64];
	switch(Msg)
	{
	case WM_INITDIALOG:
		SetClassLong(hWnd,GCL_HICON,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGL));
		SetClassLong(hWnd,GCL_HICONSM,(LONG)LoadIcon(hinstance,(LPCTSTR)IDI_DXGLSM));
		// create temporary gl context to get AA and AF settings.
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&mode);
		pfd.cColorBits = mode.dmBitsPerPel;
		dc = GetDC(hWnd);
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
		ReleaseDC(hWnd,dc);
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
		SendDlgItemMessage(hWnd,IDC_VIDMODE,CB_SETCURSEL,globalcfg.scaler,NULL);
		// colormode
		if(globalcfg.colormode) SendDlgItemMessage(hWnd,IDC_COLOR,BM_SETCHECK,BST_CHECKED,NULL);
		else SendDlgItemMessage(hWnd,IDC_COLOR,BM_SETCHECK,BST_UNCHECKED,NULL);
		// scalingfilter
		_tcscpy(buffer,_T("Nearest"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Bilinear"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Custom shader"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,2,(LPARAM)buffer);
		_tcscpy(buffer,_T("Shader (primary only)"));
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_ADDSTRING,3,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_SCALE,CB_SETCURSEL,globalcfg.scalingfilter,NULL);
		// highres
		if(globalcfg.highres) SendDlgItemMessage(hWnd,IDC_HIGHRES,BM_SETCHECK,BST_CHECKED,NULL);
		else SendDlgItemMessage(hWnd,IDC_HIGHRES,BM_SETCHECK,BST_UNCHECKED,NULL);
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
		SendDlgItemMessage(hWnd,IDC_TEXFILTER,CB_SETCURSEL,globalcfg.texfilter,NULL);
		// anisotropic
		if (anisotropic < 2)
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_ADDSTRING,0,(LPARAM)buffer);
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_SETCURSEL,0,NULL);
			EnableWindow(GetDlgItem(hWnd,IDC_ANISO),FALSE);
			globalcfg.anisotropic = 0;
			currentcfg.anisotropic = 0;
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
			SendDlgItemMessage(hWnd,IDC_ANISO,CB_SETCURSEL,globalcfg.anisotropic,NULL);
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
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_SETCURSEL,globalcfg.msaa,NULL);
		}
		else
		{
			_tcscpy(buffer,_T("Not supported"));
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_ADDSTRING,0,(LPARAM)buffer);
			SendDlgItemMessage(hWnd,IDC_MSAA,CB_SETCURSEL,0,NULL);
			EnableWindow(GetDlgItem(hWnd,IDC_MSAA),FALSE);
			globalcfg.msaa = 0;
			currentcfg.msaa = 0;
		}
		// aspect
		_tcscpy(buffer,_T("Stretch to display"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Expand viewable area"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Crop to display"));
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_ASPECT,CB_SETCURSEL,globalcfg.aspect,NULL);
		// sort modes
		_tcscpy(buffer,_T("Use system order"));
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_ADDSTRING,0,(LPARAM)buffer);
		_tcscpy(buffer,_T("Group by color depth"));
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_ADDSTRING,1,(LPARAM)buffer);
		_tcscpy(buffer,_T("Group by resolution"));
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_ADDSTRING,2,(LPARAM)buffer);
		SendDlgItemMessage(hWnd,IDC_SORTMODES,CB_SETCURSEL,globalcfg.SortModes,NULL);
		// color depths
		if(globalcfg.AllColorDepths) SendDlgItemMessage(hWnd,IDC_UNCOMMONCOLOR,BM_SETCHECK,BST_CHECKED,NULL);
		else SendDlgItemMessage(hWnd,IDC_UNCOMMONCOLOR,BM_SETCHECK,BST_UNCHECKED,NULL);
		// extra modes
		if(globalcfg.ExtraModes) SendDlgItemMessage(hWnd,IDC_EXTRAMODES,BM_SETCHECK,BST_CHECKED,NULL);
		else SendDlgItemMessage(hWnd,IDC_EXTRAMODES,BM_SETCHECK,BST_UNCHECKED,NULL);
		return true;
	case WM_COMMAND:
		switch(wParam)
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
		}
		break;
	}
	return false;
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
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	HMODULE comctl32 = LoadLibrary(_T("comctl32.dll"));
	BOOL (WINAPI *iccex)(LPINITCOMMONCONTROLSEX lpInitCtrls) =
		(BOOL (WINAPI *)(LPINITCOMMONCONTROLSEX))GetProcAddress(comctl32,"InitCommonControlsEx");
	if(iccex) iccex(&icc);
	else InitCommonControls();
	hinstance = hInstance;
	GetGlobalConfig(&globalcfg);
	int result = DialogBox(hInstance,MAKEINTRESOURCE(IDD_DXGLCFG),0,reinterpret_cast<DLGPROC>(DXGLCfgCallback));
	switch(result)
	{
	case IDOK:
		// Save settings
		break;
	case IDCANCEL:
		break;
	}
	return 0;
}
