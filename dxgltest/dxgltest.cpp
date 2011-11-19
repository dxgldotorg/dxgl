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

#include "common.h"
#include "dxgltest.h"
#include "tests.h"
#ifdef _UNICODE
#define _ttof _wtof
#else
#define _ttof atof
#endif

HINSTANCE hinstance;
bool gradientavailable;
BOOL (WINAPI *_GradientFill)(HDC hdc, TRIVERTEX* pVertices, ULONG nVertices, void* pMesh, ULONG nMeshElements, DWORD dwMode) = NULL;

void GetFileVersion(tstring &version, LPCTSTR filename)
{
	UINT outlen;
	DWORD verinfosize = GetFileVersionInfoSize(filename,NULL);
	void *verinfo = malloc(verinfosize);
	VS_FIXEDFILEINFO *rootblock;
	if(GetFileVersionInfo(filename,0,verinfosize,verinfo))
	{
		VerQueryValue(verinfo,_T("\\"),(VOID **)&rootblock,&outlen);
		TCHAR number[16];
		_itot(HIWORD(rootblock->dwFileVersionMS),number,10);
		version.assign(number);
		version.append(_T("."));
		_itot(LOWORD(rootblock->dwFileVersionMS),number,10);
		version.append(number);
		version.append(_T("."));
		_itot(HIWORD(rootblock->dwFileVersionLS),number,10);
		version.append(number);
		version.append(_T("."));
		_itot(LOWORD(rootblock->dwFileVersionLS),number,10);
		version.append(number);
		free(verinfo);
	}
	else
	{
		version = _T("NOT FOUND");
		free(verinfo);
	}
}
int modenum = 0;
HRESULT WINAPI EnumModesCallback(LPDDSURFACEDESC ddsd, void *list)
{
	HWND hWnd = (HWND)list;
	tstring resolution;
	int bpp;
	if(ddsd->ddpfPixelFormat.dwRGBBitCount == 16)
	{
		if((ddsd->ddpfPixelFormat.dwRBitMask | ddsd->ddpfPixelFormat.dwGBitMask |
			ddsd->ddpfPixelFormat.dwBBitMask) == 0x7FFF) bpp = 15;
		else bpp = 16;
	}
	else bpp = ddsd->ddpfPixelFormat.dwRGBBitCount;
	TCHAR number[16];
	_itot(ddsd->dwWidth,number,10);
	resolution.append(number);
	resolution.append(_T("x"));
	_itot(ddsd->dwHeight,number,10);
	resolution.append(number);
	resolution.append(_T("x"));
	_itot(bpp,number,10);
	resolution.append(number);
	resolution.append(_T(","));
	_itot(ddsd->dwRefreshRate,number,10);
	resolution.append(number);
	resolution.append(_T("Hz"));
	int listnum = SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM)resolution.c_str());
	if(ddsd->dwWidth == 640 && ddsd->dwHeight == 480 && ddsd->dwRefreshRate == 60 && ddsd->ddpfPixelFormat.dwRGBBitCount == 8)
		modenum = listnum;
	return DDENUMRET_OK;
}

void TranslateResolutionString(LPCTSTR str, int &width, int &height, int &bpp, int &refresh)
{
	tstring tmp = str;
	tstring tmp2 = tmp.substr(0,tmp.find(_T("x")));
	width = _ttoi(tmp2.c_str());
	tmp = tmp.substr(tmp2.length()+1);
	tmp2 = tmp.substr(0,tmp.find(_T("x")));
	height = _ttoi(tmp2.c_str());
	tmp = tmp.substr(tmp2.length()+1);
	tmp2 = tmp.substr(0,tmp.find(_T(",")));
	bpp = _ttoi(tmp2.c_str());
	tmp = tmp.substr(tmp2.length()+1);
	tmp2 = tmp.substr(0,tmp.find(_T("H")));
	refresh = _ttoi(tmp2.c_str());
}


const TCHAR *dllnames[] = {
	_T("ddraw.dll"),
	_T("ddrawex.dll"),
	_T("d3dim.dll"),
	_T("d3dim700.dll"),
	_T("d3dref.dll"),
	_T("d3dramp.dll"),
	_T("d3drm.dll"),
	_T("d3dxof.dll"),
	_T("d3dpmesh.dll")
};

int dllboxes[] = {
	IDC_DDVER,
	IDC_DDEXVER,
	IDC_D3DVER,
	IDC_D3D7VER,
	IDC_D3DREFVER,
	IDC_D3DRAMPVER,
	IDC_D3DRMVER,
	IDC_D3DXOFVER,
	IDC_D3DPMESHVER
};

INT_PTR CALLBACK SysTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HMODULE mod_ddraw;
	BOOL (WINAPI *IsDXGLDDraw)();
	int i;
	tstring ver;
	switch(Msg)
	{
	case WM_INITDIALOG:
		mod_ddraw = LoadLibrary(_T("ddraw.dll"));
		IsDXGLDDraw = GetProcAddress(mod_ddraw,"IsDXGLDDraw");
		if(IsDXGLDDraw)	SetWindowText(GetDlgItem(hWnd,IDC_DDTYPE),_T("DXGL"));
		else SetWindowText(GetDlgItem(hWnd,IDC_DDTYPE),_T("System"));
		FreeLibrary(mod_ddraw);
		GetFileVersion(ver,dllnames[0]);
		SetWindowText(GetDlgItem(hWnd,dllboxes[0]),ver.c_str());
		if(!IsDXGLDDraw)
		{
			for(i = 1; i < 9; i++)
			{
				GetFileVersion(ver,dllnames[i]);
				SetWindowText(GetDlgItem(hWnd,dllboxes[i]),ver.c_str());
			}
		}
		else
		{
			for(i = 1; i < 9; i++)
				SetWindowText(GetDlgItem(hWnd,dllboxes[i]),_T("N/A"));
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_DXDIAG:
			_spawnlp(_P_NOWAIT,"dxdiag.exe","dxdiag.exe",NULL);
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

typedef struct
{
	int minver;
	int maxver;
	int buffermin;
	int buffermax;
	bool usesfps;
	float defaultfps;
	bool usestexture;
	bool usesfsaa;
	TCHAR *name;
} TEST_ITEM;

// Use EXACTLY one line per entry.  Don't change layout of the list.
const int START_2D = __LINE__;
const TEST_ITEM Tests2D[] =
{ // minver maxver  buffermin max   usesfps		defaultfps		usestexture	usesfsaa	name
	{1,		7,		0,		4,		true,		1.0,			false,		false,		_T("Color palette and gradient screens (direct surface access)")},
	{1,		7,		0,		1,		false,		0.0,			false,		false,		_T("Random noise (direct surface access speed test)")},
	{1,		7,		0,		7,		true,		1.0,			false,		false,		_T("GDI Test patterns (GetDC() test)")},
	{1,		7,		0,		0,		false,		0.0,			false,		false,		_T("Random GDI patterns (does not clear screen between paints)")},
	{1,		7,		0,		1,		true,		60.0,			false,		false,		_T("BltFast background and sprites")},
	{1,		7,		0,		0,		false,		0.0,			false,		false,		_T("Random color fill Blt() patterns")}
};
const int END_2D = __LINE__ - 4;
const int numtests2d = END_2D - START_2D;
int currenttest2d = 0;
int minapi2d = 1;
int maxapi2d = 7;
int minbuffer2d = 0;
int maxbuffer2d = 4;
int fps_enabled2d = false;
int api2d = 1;
int buffer2d = 1;
bool fullscreen2d = true;
bool resizable2d = false;
double framerate2d = 1.00;
TCHAR framerate2dstring[33];
TCHAR tmpstring[33];
INT_PTR CALLBACK Test2DCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	int pos;
	double f;
	LPDIRECTDRAW lpdd;
	HRESULT error;
	int width,height,bpp,refresh;
	switch(Msg)
	{
	case WM_INITDIALOG:
		DestroyWindow(GetDlgItem(hWnd,IDC_FILTERLABEL));
		DestroyWindow(GetDlgItem(hWnd,IDC_FILTER));
		DestroyWindow(GetDlgItem(hWnd,IDC_FSAALABEL));
		DestroyWindow(GetDlgItem(hWnd,IDC_FSAA));
		SendDlgItemMessage(hWnd,IDC_FULLSCREEN,BM_SETCHECK,1,0);
		SetDlgItemText(hWnd,IDC_TESTHEADER,_T("Test 2D graphics functionality in DXGL or DirectDraw.  Press ESC to quit any test."));
		for(i = 0; i < numtests2d; i++)
			SendDlgItemMessage(hWnd,IDC_TESTLIST,LB_ADDSTRING,0,(LPARAM)Tests2D[i].name);
		error = DirectDrawCreate(NULL,&lpdd,NULL);
		if(error == DD_OK)
		{
			error = lpdd->EnumDisplayModes(DDEDM_REFRESHRATES,NULL,GetDlgItem(hWnd,IDC_VIDMODES),EnumModesCallback);
			lpdd->Release();
		}
		SendDlgItemMessage(hWnd,IDC_VIDMODES,LB_SETCURSEL,modenum,0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_TESTLIST:
			if(HIWORD(wParam) == LBN_SELCHANGE)
			{
				i = SendDlgItemMessage(hWnd,IDC_TESTLIST,LB_GETCURSEL,0,0);
				if(i != -1)
				{
					currenttest2d = i;
					minapi2d = Tests2D[i].minver;
					maxapi2d = Tests2D[i].maxver;
					minbuffer2d = Tests2D[i].buffermin;
					maxbuffer2d = Tests2D[i].buffermax;
					fps_enabled2d = Tests2D[i].usesfps;
					if(Tests2D[i].usesfps) framerate2d = Tests2D[i].defaultfps;
					EnableWindow(GetDlgItem(hWnd,IDC_BUFFERS),TRUE);
					EnableWindow(GetDlgItem(hWnd,IDC_APIVER),TRUE);
					EnableWindow(GetDlgItem(hWnd,IDC_FRAMERATE),fps_enabled2d);
					EnableWindow(GetDlgItem(hWnd,IDC_TEST),TRUE);
					SendDlgItemMessage(hWnd,IDC_BUFFERS,EM_SETLIMITTEXT,2,0);
					SendDlgItemMessage(hWnd,IDC_APIVER,EM_SETLIMITTEXT,1,0);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,EM_SETLIMITTEXT,5,0);
					SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_SETTEXT,0,(LPARAM)_itot(buffer2d,tmpstring,10));
					SendDlgItemMessage(hWnd,IDC_APIVER,WM_SETTEXT,0,(LPARAM)_itot(api2d,tmpstring,10));
					_stprintf(framerate2dstring,_T("%.2f"),framerate2d);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_SETTEXT,0,(LPARAM)framerate2dstring);
				}
			}
			break;
		case IDC_FRAMERATE:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_GETTEXT,6,(LPARAM)tmpstring);
				if(_tcscmp(framerate2dstring,tmpstring))
				{
					pos = LOWORD(SendDlgItemMessage(hWnd,IDC_FRAMERATE,EM_GETSEL,NULL,NULL));
					i = _tcslen(tmpstring);
					f = _ttof(tmpstring);
					if(errno != ERANGE) framerate2d = f;
					if(framerate2d < 0.5) framerate2d = 0.5;
					if(framerate2d > 99.99) framerate2d = 99.99;
					_stprintf(framerate2dstring,_T("%.2f"),framerate2d);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_SETTEXT,0,(LPARAM)framerate2dstring);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,EM_SETSEL,pos,pos);
				}
			}
			break;
		case IDC_APIVER:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd,IDC_APIVER,WM_GETTEXT,6,(LPARAM)tmpstring);
				i = _ttoi(tmpstring);
				if(i != api2d)
				{
					api2d = i;
					if(api2d == 5) api2d = 4;
					if(api2d == 6) api2d = 7;
					if(api2d < minapi2d) api2d = minapi2d;
					if(api2d > maxapi2d) api2d = maxapi2d;
					_itot(api2d,tmpstring,10);
					SendDlgItemMessage(hWnd,IDC_APIVER,WM_SETTEXT,0,(LPARAM)tmpstring);
				}
			}
			break;
		case IDC_BUFFERS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_GETTEXT,6,(LPARAM)tmpstring);
				i = _ttoi(tmpstring);
				if(i != buffer2d)
				{
					buffer2d = i;
					if(buffer2d < minbuffer2d) buffer2d = minbuffer2d;
					if(buffer2d > maxbuffer2d) buffer2d = maxbuffer2d;
					_itot(buffer2d,tmpstring,10);
					SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_SETTEXT,0,(LPARAM)tmpstring);
				}
			}
			break;
		case IDC_TEST:
			if(SendDlgItemMessage(hWnd,IDC_RESIZABLE,BM_GETCHECK,0,0)) resizable2d = true;
			else resizable2d = false;
			i = SendDlgItemMessage(hWnd,IDC_VIDMODES,LB_GETCURSEL,0,0);
			SendDlgItemMessage(hWnd,IDC_VIDMODES,LB_GETTEXT,i,(LPARAM)tmpstring);
			TranslateResolutionString(tmpstring,width,height,bpp,refresh);
			RunTest2D(currenttest2d,width,height,bpp,refresh,buffer2d,api2d,framerate2d,fullscreen2d,resizable2d);
			break;
		case IDC_WINDOWED:
			SendDlgItemMessage(hWnd,IDC_FULLSCREEN,BM_SETCHECK,0,0);
			SendDlgItemMessage(hWnd,IDC_WINDOWED,BM_SETCHECK,1,0);
			fullscreen2d = false;
			break;
		case IDC_FULLSCREEN:
			SendDlgItemMessage(hWnd,IDC_FULLSCREEN,BM_SETCHECK,1,0);
			SendDlgItemMessage(hWnd,IDC_WINDOWED,BM_SETCHECK,0,0);
			fullscreen2d = true;
			break;
		}
		break;
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
		{
		case UDN_DELTAPOS:
			switch(((LPNMHDR)lParam)->idFrom)
			{
			case IDC_SPINFRAME:
				framerate2d -= ((LPNMUPDOWN)lParam)->iDelta;
				if(framerate2d < 0.01) framerate2d = 0.01;
				if(framerate2d > 99.99) framerate2d = 99.99;
				_stprintf(framerate2dstring,_T("%.2f"),framerate2d);
				SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_SETTEXT,0,(LPARAM)framerate2dstring);
				break;
			case IDC_SPINAPI:
				if(api2d < 1) api2d = 1;
				if(api2d > 7) api2d = 7;
				if(((LPNMUPDOWN)lParam)->iDelta > 0)
				{
					switch(api2d)
					{
					case 1:
					case 2:
					default:
						api2d = 1;
						break;
					case 3:
						api2d = 2;
						break;
					case 4:
						api2d = 3;
						break;
					case 7:
					case 6:
					case 5:
						api2d = 4;
					}
				}
				else
				{
					switch(api2d)
					{
					case 1:
					default:
						api2d = 2;
						break;
					case 2:
						api2d = 3;
						break;
					case 3:
						api2d = 4;
						break;
					case 4:
					case 5:
					case 6:
					case 7:
						api2d = 7;
					}
				}
				if(api2d < minapi2d) api2d = minapi2d;
				if(api2d > maxapi2d) api2d = maxapi2d;
				SendDlgItemMessage(hWnd,IDC_APIVER,WM_SETTEXT,0,(LPARAM)_itot(api2d,tmpstring,10));
				break;
			case IDC_SPINBACK:
				buffer2d -= ((LPNMUPDOWN)lParam)->iDelta;
				if(buffer2d < minbuffer2d) buffer2d = minbuffer2d;
				if(buffer2d > maxbuffer2d) buffer2d = maxbuffer2d;
				SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_SETTEXT,0,(LPARAM)_itot(buffer2d,tmpstring,10));
				break;
			default:
				break;
			}
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

HWND tabwnd[3];
int tabopen;
INT_PTR CALLBACK DXGLTestCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    TCITEM tab;
	HWND hTab;
	RECT tabrect;
	NMHDR *nm;
	HICON icon;
	int newtab;
    switch(Msg)
    {
    case WM_INITDIALOG:
		icon = (HICON)LoadImage(hinstance,MAKEINTRESOURCE(IDI_DXGLSM),IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
		SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)icon);
		icon = (HICON)LoadImage(hinstance,MAKEINTRESOURCE(IDI_DXGL),IMAGE_ICON,
			GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),0);
		SendMessage(hWnd,WM_SETICON,ICON_BIG,(LPARAM)icon);
        tab.mask = TCIF_TEXT;
        tab.pszText = _T("System");
        SendDlgItemMessage(hWnd,IDC_TABS,TCM_INSERTITEM,0,(LPARAM)&tab);
        tab.pszText = _T("2D Graphics");
		hTab = GetDlgItem(hWnd,IDC_TABS);
        SendDlgItemMessage(hWnd,IDC_TABS,TCM_INSERTITEM,1,(LPARAM)&tab);
        tabwnd[0] = CreateDialog(hinstance,MAKEINTRESOURCE(IDD_SYSINFO),hTab,SysTabCallback);
        tabwnd[1] = CreateDialog(hinstance,MAKEINTRESOURCE(IDD_TESTGFX),hTab,Test2DCallback);
		SendDlgItemMessage(hWnd,IDC_TABS,TCM_GETITEMRECT,0,(LPARAM)&tabrect);
		SetWindowPos(tabwnd[0],NULL,tabrect.left,tabrect.bottom+3,0,0,SWP_SHOWWINDOW|SWP_NOSIZE);
		ShowWindow(tabwnd[1],SWP_HIDEWINDOW);
		tabopen = 0;
		ShowWindow(hWnd,SW_SHOWNORMAL);
        return TRUE;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
            case IDOK:
                EndDialog(hWnd,IDOK);
                break;
        }
		break;
    case WM_CLOSE:
        EndDialog(hWnd,IDCANCEL);
        break;
	case WM_NOTIFY:
		nm = (LPNMHDR) lParam;
		if(nm->code == TCN_SELCHANGE)
		{
			newtab = SendDlgItemMessage(hWnd,IDC_TABS,TCM_GETCURSEL,0,0);
			if(newtab != tabopen)
			{
				ShowWindow(tabwnd[tabopen],SW_HIDE);
				tabopen = newtab;
				SendDlgItemMessage(hWnd,IDC_TABS,TCM_GETITEMRECT,0,(LPARAM)&tabrect);
				SetWindowPos(tabwnd[tabopen],NULL,tabrect.left,tabrect.bottom+3,0,0,SWP_SHOWWINDOW|SWP_NOSIZE);
			}
		}
		break;
    break;
    default:
        return FALSE;
    }
    return TRUE;
}

#ifdef __GNUC__
#ifndef INITCOMMONCONTROLSEX
typedef struct tagINITCOMMONCONTROLSEX {
  DWORD dwSize;
  DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;
#endif
#endif

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	OSVERSIONINFO verinfo;
	verinfo.dwOSVersionInfoSize = sizeof(verinfo);
	GetVersionEx(&verinfo);
	if(verinfo.dwMajorVersion > 4) gradientavailable = true;
	else if(verinfo.dwMajorVersion >= 4 && verinfo.dwMinorVersion >= 1) gradientavailable = true;
	else gradientavailable = false;
	HMODULE msimg32 = NULL;;
	if(gradientavailable)
	{
		msimg32 = LoadLibrary(_T("msimg32.dll"));
		if(!msimg32) gradientavailable = false;
		if(gradientavailable) _GradientFill =
			(BOOL(_stdcall*)(HDC,TRIVERTEX*,ULONG,void*,ULONG,DWORD))
			GetProcAddress(msimg32,"GradientFill");
		if(!_GradientFill)
		{
			FreeLibrary(msimg32);
			msimg32 = NULL;
			gradientavailable = false;
		}
	}
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	HMODULE comctl32 = LoadLibrary(_T("comctl32.dll"));
	BOOL (WINAPI *iccex)(LPINITCOMMONCONTROLSEX lpInitCtrls) =
		(BOOL (WINAPI *)(LPINITCOMMONCONTROLSEX))GetProcAddress(comctl32,"InitCommonControlsEx");
	if(iccex) iccex(&icc);
	else InitCommonControls();
    hinstance = hInstance;
    DialogBox(hinstance,MAKEINTRESOURCE(IDD_DXGLTEST),NULL,DXGLTestCallback);
	if(msimg32) FreeLibrary(msimg32);
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
    return 0;
}
