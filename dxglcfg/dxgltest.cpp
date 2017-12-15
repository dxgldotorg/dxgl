// DXGL
// Copyright (C) 2011-2017 William Feely

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
#include "MultiDD.h"
#include "tests.h"

static HINSTANCE hinstance;
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
HRESULT WINAPI EnumModesCallback8(LPDDSURFACEDESC ddsd, void *list)
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
HRESULT WINAPI EnumModesCallback32(LPDDSURFACEDESC ddsd, void *list)
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
	if(ddsd->dwWidth == 640 && ddsd->dwHeight == 480 && ddsd->dwRefreshRate == 60 && ddsd->ddpfPixelFormat.dwRGBBitCount == 32)
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
};

int dllboxes[] = {
	IDC_DDVER,
};

INT_PTR CALLBACK AboutTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HMODULE mod_ddraw;
	BOOL (WINAPI *IsDXGLDDraw)();
	int i;
	tstring ver;
	switch(Msg)
	{
	case WM_INITDIALOG:
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		mod_ddraw = LoadLibrary(_T("ddraw.dll"));
		IsDXGLDDraw = GetProcAddress(mod_ddraw,"IsDXGLDDraw");
		if(IsDXGLDDraw)	SetWindowText(GetDlgItem(hWnd,IDC_DDTYPE),_T("DXGL"));
		else SetWindowText(GetDlgItem(hWnd,IDC_DDTYPE),_T("System"));
		FreeLibrary(mod_ddraw);
		GetFileVersion(ver,dllnames[0]);
		SetWindowText(GetDlgItem(hWnd,dllboxes[0]),ver.c_str());
		if(!IsDXGLDDraw)
		{
			for(i = 1; i < 1; i++)
			{
				GetFileVersion(ver,dllnames[i]);
				SetWindowText(GetDlgItem(hWnd,dllboxes[i]),ver.c_str());
			}
		}
		else
		{
			for(i = 1; i < 1; i++)
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
	BOOL usesfps;
	float defaultfps;
	BOOL is3d;
	BOOL usestexture;
	BOOL usesfsaa;
	TCHAR *name;
} TEST_ITEM;

// Use EXACTLY one line per entry.  Don't change layout of the list.
const int START_TESTS = __LINE__;
const TEST_ITEM Tests[] =
{ // minver maxver  buffermin max   usesfps		defaultfps	is3d	usestexture	usesfsaa	name
	{1,		7,		0,		4,		TRUE,		1.0,		FALSE,	FALSE,		FALSE,		_T("Color palette and gradient screens (direct surface access)")},
	{1,		7,		0,		1,		FALSE,		0.0,		FALSE,	FALSE,		FALSE,		_T("Random noise (continuous surface upload)")},
	{1,		7,		0,		7,		TRUE,		1.0,		FALSE,	FALSE,		FALSE,		_T("GDI Test patterns (GetDC() test)")},
	{1,		7,		0,		0,		FALSE,		0.0,		FALSE,	FALSE,		FALSE,		_T("Random GDI patterns (does not clear screen between paints)")},
	{1,		7,		0,		1,		TRUE,		60.0,		FALSE,	FALSE,		FALSE,		_T("BltFast background and sprites")},
	{1,		7,		0,		0,		FALSE,		0.0,		FALSE,	FALSE,		FALSE,		_T("Random color fill Blt() paterns")},
	{1,		7,		0,		0,		FALSE,		0.0,		FALSE,	FALSE,		FALSE,		_T("Mouse pointer event test")},
	{1,		7,		1,		1,		TRUE,		1.0,		FALSE,	FALSE,		FALSE,		_T("Raster Operation Blt() test")},
	{1,		7,		0,		0,		FALSE,		0.0,		FALSE,	FALSE,		FALSE,		_T("Mirrored and Rotated Blt() test")},
	{1,		7,		0,		1,		FALSE,		0.0,		FALSE,	FALSE,		FALSE,		_T("Large batch color fill Blt() operations")},
	{1,		7,		0,		1,		TRUE,		60.0,		FALSE,	FALSE,		FALSE,		_T("Source Color Key Override test")},
	{1,		7,		0,		1,		TRUE,		60.0,		FALSE,	FALSE,		FALSE,		_T("Destination Color Key Override test")},
	{7,		7,		0,		2,		TRUE,		60.0,		TRUE,	FALSE,		TRUE,		_T("DrawIndexedPrimitive cube with directional light (DX7)")},
	{7,		7,		0,		2,		TRUE,		60.0,		TRUE,	TRUE,		TRUE,		_T("DrawPrimitive textured cube (DX7)")},
	{7,		7,		0,		0,		TRUE,		60.0,		TRUE,	TRUE,		TRUE,		_T("Texture Stage shaders (Interactive, DX7)")},
	{7,		7,		0,		0,		TRUE,		60.0,		TRUE,	TRUE,		TRUE,		_T("Vertex shaders (Interactive, DX7)")}
};
const int END_TESTS = __LINE__ - 4;
const int numtests = END_TESTS - START_TESTS;

int currenttest = 0;
int minapi = 1;
int maxapi = 7;
int minbuffer = 0;
int maxbuffer = 4;
int fps_enabled = false;
int api = 1;
int buffer = 1;
int filter = 0;
int msaa = 0;
bool fullscreen = true;
bool resizable = false;
double framerate = 1.00;
TCHAR frameratestring[33];

TCHAR tmpstring[33];

INT_PTR CALLBACK TestTabCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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
		if (_EnableThemeDialogTexture) _EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
		SendDlgItemMessage(hWnd,IDC_FULLSCREEN,BM_SETCHECK,1,0);
		for(i = 0; i < numtests; i++)
			SendDlgItemMessage(hWnd,IDC_TESTLIST,LB_ADDSTRING,0,(LPARAM)Tests[i].name);
		error = DirectDrawCreate(NULL,&lpdd,NULL);
		if(error == DD_OK)
		{
			error = lpdd->EnumDisplayModes(DDEDM_REFRESHRATES,NULL,GetDlgItem(hWnd,IDC_VIDMODES),EnumModesCallback8);
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
					currenttest = i;
					minapi = Tests[i].minver;
					maxapi = Tests[i].maxver;
					if (api < minapi)
					{
						SendDlgItemMessage(hWnd, IDC_APIVER, WM_SETTEXT, 0, (LPARAM)_itot(minapi, tmpstring, 10));
						api = minapi;
					}
					if (api > maxapi)
					{
						SendDlgItemMessage(hWnd, IDC_APIVER, WM_SETTEXT, 0, (LPARAM)_itot(maxapi, tmpstring, 10));
						api = maxapi;
					}
					minbuffer = Tests[i].buffermin;
					maxbuffer = Tests[i].buffermax;
					fps_enabled = Tests[i].usesfps;
					if(Tests[i].usesfps) framerate = Tests[i].defaultfps;
					if((i != 14) && (i != 15))
					{
						EnableWindow(GetDlgItem(hWnd,IDC_BUFFERS),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_APIVER),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_FRAMERATE),fps_enabled);
						EnableWindow(GetDlgItem(hWnd,IDC_TEST),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_WINDOWED),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_FULLSCREEN),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_RESIZABLE),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_TESTVSYNC),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_VIDMODES),TRUE);
					}
					else
					{
						EnableWindow(GetDlgItem(hWnd,IDC_BUFFERS),FALSE);
						EnableWindow(GetDlgItem(hWnd,IDC_APIVER),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_FRAMERATE),FALSE);
						EnableWindow(GetDlgItem(hWnd,IDC_TEST),TRUE);
						EnableWindow(GetDlgItem(hWnd,IDC_WINDOWED),FALSE);
						EnableWindow(GetDlgItem(hWnd,IDC_FULLSCREEN),FALSE);
						EnableWindow(GetDlgItem(hWnd,IDC_RESIZABLE),FALSE);
						EnableWindow(GetDlgItem(hWnd,IDC_TESTVSYNC),FALSE);
						EnableWindow(GetDlgItem(hWnd,IDC_VIDMODES),FALSE);
						SendDlgItemMessage(hWnd,IDC_WINDOWED,BM_SETCHECK,BST_CHECKED,0);
						SendDlgItemMessage(hWnd,IDC_FULLSCREEN,BM_SETCHECK,BST_UNCHECKED,0);
						fullscreen = false;
					}
					SendDlgItemMessage(hWnd,IDC_BUFFERS,EM_SETLIMITTEXT,2,0);
					SendDlgItemMessage(hWnd,IDC_APIVER,EM_SETLIMITTEXT,1,0);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,EM_SETLIMITTEXT,5,0);
					SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_SETTEXT,0,(LPARAM)_itot(buffer,tmpstring,10));
					SendDlgItemMessage(hWnd,IDC_APIVER,WM_SETTEXT,0,(LPARAM)_itot(api,tmpstring,10));
					_stprintf(frameratestring,_T("%.2f"),framerate);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_SETTEXT,0,(LPARAM)frameratestring);
				}
			}
			else if (HIWORD(wParam) == LBN_DBLCLK)
			{
				if (SendDlgItemMessage(hWnd, IDC_RESIZABLE, BM_GETCHECK, 0, 0)) resizable = true;
				else resizable = false;
				if (buffer < minbuffer) buffer = minbuffer;
				if (buffer > maxbuffer) buffer = maxbuffer;
				i = SendDlgItemMessage(hWnd, IDC_VIDMODES, LB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hWnd, IDC_VIDMODES, LB_GETTEXT, i, (LPARAM)tmpstring);
				TranslateResolutionString(tmpstring, width, height, bpp, refresh);
				RunDXGLTest(currenttest, width, height, bpp, refresh, buffer, api,
					filter, msaa, framerate, fullscreen, resizable, Tests[currenttest].is3d);
				break;
			}
			break;
		case IDC_FRAMERATE:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_GETTEXT,6,(LPARAM)tmpstring);
				if(_tcscmp(frameratestring,tmpstring))
				{
					pos = LOWORD(SendDlgItemMessage(hWnd,IDC_FRAMERATE,EM_GETSEL,NULL,NULL));
					i = _tcslen(tmpstring);
					f = _ttof(tmpstring);
					if(errno != ERANGE) framerate = f;
					if(framerate < 0.5) framerate = 0.5;
					if(framerate > 99.99) framerate = 99.99;
					_stprintf(frameratestring,_T("%.2f"),framerate);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_SETTEXT,0,(LPARAM)frameratestring);
					SendDlgItemMessage(hWnd,IDC_FRAMERATE,EM_SETSEL,pos,pos);
				}
			}
			break;
		case IDC_APIVER:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd,IDC_APIVER,WM_GETTEXT,6,(LPARAM)tmpstring);
				i = _ttoi(tmpstring);
				if(i != api)
				{
					api = i;
					if(api == 5) api = 4;
					if(api == 6) api = 7;
					if(api < minapi) api = minapi;
					if(api > maxapi) api = maxapi;
					_itot(api,tmpstring,10);
					SendDlgItemMessage(hWnd,IDC_APIVER,WM_SETTEXT,0,(LPARAM)tmpstring);
				}
			}
			break;
		case IDC_BUFFERS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_GETTEXT,6,(LPARAM)tmpstring);
				i = _ttoi(tmpstring);
				if(i != buffer)
				{
					buffer = i;
					if(buffer < minbuffer) buffer = minbuffer;
					if(buffer > maxbuffer) buffer = maxbuffer;
					_itot(buffer,tmpstring,10);
					SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_SETTEXT,0,(LPARAM)tmpstring);
				}
			}
			break;
		case IDC_TEST:
			if (IsWindowEnabled(GetDlgItem(hDialog, IDC_APPLY)))
			{
				if(MessageBox(hDialog, _T("You have unsaved changes to your configuration.\r\n\
Do you want to apply them before running this test?"),
					_T("Notice"), MB_YESNO| MB_ICONQUESTION) == IDYES)
					SaveChanges(hDialog);
			}
			if(SendDlgItemMessage(hWnd,IDC_RESIZABLE,BM_GETCHECK,0,0)) resizable = true;
			else resizable = false;
			if (buffer < minbuffer) buffer = minbuffer;
			if (buffer > maxbuffer) buffer = maxbuffer;
			i = SendDlgItemMessage(hWnd, IDC_VIDMODES, LB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hWnd,IDC_VIDMODES,LB_GETTEXT,i,(LPARAM)tmpstring);
			TranslateResolutionString(tmpstring,width,height,bpp,refresh);
			RunDXGLTest(currenttest, width, height, bpp, refresh, buffer, api,
				filter, msaa, framerate, fullscreen, resizable, Tests[currenttest].is3d);
			break;
		case IDC_WINDOWED:
			SendDlgItemMessage(hWnd,IDC_FULLSCREEN,BM_SETCHECK,0,0);
			SendDlgItemMessage(hWnd,IDC_WINDOWED,BM_SETCHECK,1,0);
			fullscreen = false;
			break;
		case IDC_FULLSCREEN:
			SendDlgItemMessage(hWnd,IDC_FULLSCREEN,BM_SETCHECK,1,0);
			SendDlgItemMessage(hWnd,IDC_WINDOWED,BM_SETCHECK,0,0);
			fullscreen = true;
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
				framerate -= ((LPNMUPDOWN)lParam)->iDelta;
				if(framerate < 0.01) framerate = 0.01;
				if(framerate > 99.99) framerate = 99.99;
				_stprintf(frameratestring,_T("%.2f"),framerate);
				SendDlgItemMessage(hWnd,IDC_FRAMERATE,WM_SETTEXT,0,(LPARAM)frameratestring);
				break;
			case IDC_SPINAPI:
				if(api < 1) api = 1;
				if(api > 7) api = 7;
				if(((LPNMUPDOWN)lParam)->iDelta > 0)
				{
					switch(api)
					{
					case 1:
					case 2:
					default:
						api = 1;
						break;
					case 3:
						api = 2;
						break;
					case 4:
						api = 3;
						break;
					case 7:
					case 6:
					case 5:
						api = 4;
					}
				}
				else
				{
					switch(api)
					{
					case 1:
					default:
						api = 2;
						break;
					case 2:
						api = 3;
						break;
					case 3:
						api = 4;
						break;
					case 4:
					case 5:
					case 6:
					case 7:
						api = 7;
					}
				}
				if(api < minapi) api = minapi;
				if(api > maxapi) api = maxapi;
				SendDlgItemMessage(hWnd,IDC_APIVER,WM_SETTEXT,0,(LPARAM)_itot(api,tmpstring,10));
				break;
			case IDC_SPINBACK:
				buffer -= ((LPNMUPDOWN)lParam)->iDelta;
				if(buffer < minbuffer) buffer = minbuffer;
				if(buffer > maxbuffer) buffer = maxbuffer;
				SendDlgItemMessage(hWnd,IDC_BUFFERS,WM_SETTEXT,0,(LPARAM)_itot(buffer,tmpstring,10));
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
