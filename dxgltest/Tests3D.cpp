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
#include "tests.h"
#include "surfacegen.h"
#include "MultiDD.h"
#include "timer.h"
#include "misc.h"
#define D3D_OVERLOADS
#include "../ddraw/include/d3d.h"

void InitTest3D(int test);
void RunTestTimed3D(int test);
void RunTestLooped3D(int test);
bool RunTestMouse3D(int test, UINT Msg, WPARAM wParam, LPARAM lParam);


static MultiDirectDraw *ddinterface;
static MultiDirectDrawSurface *ddsurface;
static MultiDirectDrawSurface *ddsrender;
static MultiDirectDrawSurface *zbuffer;
static MultiDirectDrawSurface *textures[8];
static IDirect3D7 *d3d7;
static IDirect3DDevice7 *d3d7dev;
static LPDIRECTDRAWCLIPPER ddclipper;
static int width,height,bpp,refresh,backbuffers;
static double fps;
static bool fullscreen,resizable;
static HWND hWnd;
static int testnum;
static unsigned int randnum;
static int testtypes[] = {0,0,0};

static D3DVECTOR points[256];
static D3DVECTOR normals[256];
static D3DVERTEX vertices[256];
static WORD mesh[256];
static WORD cube_mesh[] = {0,1,2, 2,1,3, 4,5,6, 6,5,7, 8,9,10, 10,9,11, 12,13,14, 14,13,15, 16,17,18,
		18,17,19, 20,21,22, 22,21,23 };
static D3DLIGHT7 lights[8];

LRESULT CALLBACK D3DWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	bool paintwnd = true;
	POINT p;
	RECT srcrect,destrect;
	HRESULT error;
	PAINTSTRUCT paintstruct;
	switch(Msg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		StopTimer();
		Sleep(100);
		for(int i = 0; i < 8; i++)
		{
			if(textures[i])
			{
				textures[i]->Release();
				textures[i] = NULL;
			}
		}
		if(d3d7dev)
		{
			d3d7dev->Release();
			d3d7dev = NULL;
		}
		if(d3d7)
		{
			d3d7->Release();
			d3d7dev = NULL;
		}
		if(ddsrender)
		{
			ddsrender->Release();
			ddsrender = NULL;
		}
		if(ddsurface)
		{
			ddsurface->Release();
			ddsurface = NULL;
		}
		if(zbuffer)
		{
			zbuffer->Release();
			zbuffer = NULL;
		}
		if(ddclipper)
		{
			ddclipper->Release();
			ddclipper = NULL;
		}
		if(ddinterface)
		{
			ddinterface->Release();
			ddinterface = NULL;
		}
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE) DestroyWindow(hWnd);
		break;
	case WM_APP:
		RunTestTimed3D(testnum);
		break;
	case WM_SIZE:
		paintwnd = false;
	case WM_PAINT:
		if(paintwnd) BeginPaint(hWnd,&paintstruct);
		if(!fullscreen)
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd,&p);
			GetClientRect(hWnd,&destrect);
			OffsetRect(&destrect,p.x,p.y);
			SetRect(&srcrect,0,0,width,height);
			if(ddsurface && ddsrender)error = ddsurface->Blt(&destrect,ddsrender,&srcrect,DDBLT_WAIT,NULL);
		}
		if(paintwnd) EndPaint(hWnd,&paintstruct);
		return 0;
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
	case WM_MOUSEHWHEEL:
		if(RunTestMouse3D(testnum,Msg,wParam,lParam))
		{
			if(!fullscreen)
			{
				p.x = 0;
				p.y = 0;
				ClientToScreen(hWnd,&p);
				GetClientRect(hWnd,&destrect);
				OffsetRect(&destrect,p.x,p.y);
				SetRect(&srcrect,0,0,width,height);
				if(ddsurface && ddsrender)error = ddsurface->Blt(&destrect,ddsrender,&srcrect,DDBLT_WAIT,NULL);
			}
		}
		break;
	default:
		return DefWindowProc(hWnd,Msg,wParam,lParam);
	}
	return FALSE;
}

static int d3dtestnum;
static int d3dver;
static int ddver;

bool RunTestMouse3D(int test, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return false;
}

const TCHAR wndclassname3d[] = _T("D3DTestWndClass");

static HRESULT WINAPI zcallback(DDPIXELFORMAT *ddpf, VOID *context)
{
	if(ddpf->dwFlags == DDPF_ZBUFFER)
	{
		memcpy(context,ddpf,sizeof(DDPIXELFORMAT));
		return D3DENUMRET_CANCEL;
	}
	return D3DENUMRET_OK;
}

INT_PTR CALLBACK TexShader7Proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

void RunTest3D(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	int filter,	int msaa, double fps, bool fullscreen, bool resizable)
{	
	if(testnum == 2)
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_TEXSHADER),NULL,TexShader7Proc);
		return;
	}
	DDSCAPS2 caps;
	DDSURFACEDESC2 ddsd;
	DDPIXELFORMAT ddpfz;
	BOOL done = false;
	::testnum = testnum;
	randnum = (unsigned int)time(NULL);
	ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
	ZeroMemory(textures,8*sizeof(MultiDirectDrawSurface*));
	if(apiver >= 3) ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	::fullscreen = fullscreen;
	::resizable = resizable;
	::width = width;
	::height = height;
	::bpp = bpp;
	::refresh = refresh;
	if(fullscreen)::backbuffers = backbuffers;
	else ::backbuffers = backbuffers = 0;
	::fps = fps;
	d3dtestnum = testnum;
	d3dver = apiver;
	if(apiver == 3) ddver = 4;
	else ddver = apiver;
	HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(NULL);
	WNDCLASSEX wc;
	MSG Msg;
	ZeroMemory(&wc,sizeof(WNDCLASS));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = D3DWndProc;
	wc.hInstance = hinstance;
	wc.hIcon = LoadIcon(hinstance,MAKEINTRESOURCE(IDI_DXGL));
	wc.hIconSm = LoadIcon(hinstance,MAKEINTRESOURCE(IDI_DXGLSM));
	if(testnum == 6) wc.hCursor = LoadCursor(NULL,IDC_CROSS);
	else wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = wndclassname3d;
	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL,_T("Can not register window class"),_T("Error"),MB_ICONEXCLAMATION|MB_OK);
		return;
	}
	if(resizable)
		hWnd = CreateWindowEx(WS_EX_APPWINDOW,wndclassname3d,_T("D3D Test Window"),WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,CW_USEDEFAULT,width,height,NULL,NULL,hinstance,NULL);
	else if(!fullscreen)
		hWnd = CreateWindowEx(WS_EX_APPWINDOW,wndclassname3d,_T("D3D Test Window"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
			CW_USEDEFAULT,CW_USEDEFAULT,width,height,NULL,NULL,hinstance,NULL);
#ifdef _DEBUG
	else hWnd = CreateWindowEx(0,wndclassname3d,_T("DDraw Test Window"),WS_POPUP,0,0,
		GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),NULL,NULL,hinstance,NULL);
#else
	else hWnd = CreateWindowEx(WS_EX_TOPMOST,wndclassname3d,_T("DDraw Test Window"),WS_POPUP,0,0,
		GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),NULL,NULL,hinstance,NULL);
#endif
	DWORD err = GetLastError();
	ShowWindow(hWnd,SW_SHOWNORMAL);
	UpdateWindow(hWnd);
	RECT r1,r2;
	POINT p1;
	HRESULT error;
	ddinterface = new MultiDirectDraw(ddver,&error,NULL);
	if(fullscreen) error = ddinterface->SetCooperativeLevel(hWnd,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
	else error = ddinterface->SetCooperativeLevel(hWnd,DDSCL_NORMAL);
	if(fullscreen) error = ddinterface->SetDisplayMode(width,height,bpp,refresh,0);
	else
	{
		GetClientRect(hWnd,&r1);
		GetWindowRect(hWnd,&r2);
		p1.x = (r2.right - r2.left) - r1.right;
		p1.y = (r2.bottom - r2.top) - r1.bottom;
		MoveWindow(hWnd,r2.left,r2.top,width+p1.x,height+p1.y,TRUE);
	}
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if(fullscreen)
	{
		if(backbuffers)ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount = backbuffers;
		if(backbuffers) ddsd.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
	}
	error = ddinterface->CreateSurface(&ddsd,&ddsurface,NULL);
	if(!fullscreen)
	{
		error = ddinterface->CreateClipper(0,&ddclipper,NULL);
		error = ddclipper->SetHWnd(0,hWnd);
		error = ddsurface->SetClipper(ddclipper);
		ZeroMemory(&ddsd,sizeof(ddsd));
		if(apiver > 3) ddsd.dwSize = sizeof(DDSURFACEDESC2);
		else ddsd.dwSize = sizeof(DDSURFACEDESC);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;
		error = ddinterface->CreateSurface(&ddsd,&ddsrender,NULL);
	}
	else
	{
		if(backbuffers)
		{
			ZeroMemory(&caps,sizeof(DDSCAPS2));
			caps.dwCaps = DDSCAPS_BACKBUFFER;
			error = ddsurface->GetAttachedSurface(&caps,&ddsrender);
		}
		else
		{
			ddsrender = ddsurface;
			ddsrender->AddRef();
		}
	}
	error = ddinterface->QueryInterface(IID_IDirect3D7,(VOID**)&d3d7);
	error = d3d7->EnumZBufferFormats(IID_IDirect3DRGBDevice,zcallback,&ddpfz);
	error = ddsrender->GetSurfaceDesc(&ddsd);
	ddsd.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER|DDSCAPS_VIDEOMEMORY;
	memcpy(&ddsd.ddpfPixelFormat,&ddpfz,sizeof(DDPIXELFORMAT));
	error = ddinterface->CreateSurface(&ddsd,&zbuffer,NULL);
	error = ddsrender->AddAttachedSurface(zbuffer);
	error = d3d7->CreateDevice(IID_IDirect3DHALDevice,(LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(),&d3d7dev);
	if(error != D3D_OK)
		error = d3d7->CreateDevice(IID_IDirect3DRGBDevice,(LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(),&d3d7dev);
	ddsrender->GetSurfaceDesc(&ddsd);
	D3DVIEWPORT7 vp = {0,0,ddsd.dwWidth,ddsd.dwHeight,0.0f,1.0f};
	error = d3d7dev->SetViewport(&vp);
	error = d3d7dev->SetRenderState(D3DRENDERSTATE_ZENABLE,TRUE);
	InitTest3D(testnum);
	if(!fullscreen) SendMessage(hWnd,WM_PAINT,0,0);
	if(testtypes[testnum] == 1)
	{
		while(!done)
		{
			if(PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			{
				if(Msg.message == WM_PAINT) RunTestLooped3D(testnum);
				else if(Msg.message  == WM_QUIT) done = TRUE;
				else
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}
			else
			{
				RunTestLooped3D(testnum);
			}
		}
	}
	else if(testtypes[testnum] == 0)
	{
		StartTimer(hWnd,WM_APP,fps);
		while(GetMessage(&Msg, NULL, 0, 0) > 0)
		{
	        TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	else
	{
		while(GetMessage(&Msg, NULL, 0, 0) > 0)
		{
	        TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	UnregisterClass(wndclassname3d,hinstance);
	StopTimer();
}

void MakeCube3D(D3DVECTOR *points, D3DVECTOR *normals, D3DVERTEX *vertices)
{
	points[0] = D3DVECTOR(-2.5f,-2.5f,-2.5f);
	points[1] = D3DVECTOR(-2.5f,2.5f,-2.5f);
	points[2] = D3DVECTOR(2.5f,-2.5f,-2.5f);
	points[3] = D3DVECTOR(2.5f,2.5f,-2.5f);
	points[4] = D3DVECTOR(2.5f,-2.5f,2.5f);
	points[5] = D3DVECTOR(2.5f,2.5f,2.5f);
	points[6] = D3DVECTOR(-2.5f,-2.5f,2.5f);
	points[7] = D3DVECTOR(-2.5f,2.5f,2.5f);
	normals[0] = D3DVECTOR(0.0f,0.0f,-1.0f);
	normals[1] = D3DVECTOR(1.0f,0.0f,0.0f);
	normals[2] = D3DVECTOR(0.0f,0.0f,1.0f);
	normals[3] = D3DVECTOR(-1.0f,0.0f,0.0f);
	normals[4] = D3DVECTOR(0.0f,1.0f,0.0f);
	normals[5] = D3DVECTOR(0.0f,-10.0f,0.0f);
	vertices[0] = D3DVERTEX(points[0],normals[0],0,0);
	vertices[1] = D3DVERTEX(points[1],normals[0],1,0);
	vertices[2] = D3DVERTEX(points[2],normals[0],0,1);
	vertices[3] = D3DVERTEX(points[3],normals[0],1,1);
	vertices[4] = D3DVERTEX(points[2],normals[1],0,0);
	vertices[5] = D3DVERTEX(points[3],normals[1],1,0);
	vertices[6] = D3DVERTEX(points[4],normals[1],0,1);
	vertices[7] = D3DVERTEX(points[5],normals[1],1,1);
	vertices[8] = D3DVERTEX(points[4],normals[2],0,0);
	vertices[9] = D3DVERTEX(points[5],normals[2],1,0);
	vertices[10] = D3DVERTEX(points[6],normals[2],0,1);
	vertices[11] = D3DVERTEX(points[7],normals[2],1,1);
	vertices[12] = D3DVERTEX(points[6],normals[3],0,0);
	vertices[13] = D3DVERTEX(points[7],normals[3],1,0);
	vertices[14] = D3DVERTEX(points[0],normals[3],0,1);
	vertices[15] = D3DVERTEX(points[1],normals[3],1,1);
	vertices[16] = D3DVERTEX(points[1],normals[4],0,0);
	vertices[17] = D3DVERTEX(points[7],normals[4],1,0);
	vertices[18] = D3DVERTEX(points[3],normals[4],0,1);
	vertices[19] = D3DVERTEX(points[5],normals[4],1,1);
	vertices[20] = D3DVERTEX(points[6],normals[5],0,0);
	vertices[21] = D3DVERTEX(points[0],normals[5],1,0);
	vertices[22] = D3DVERTEX(points[4],normals[5],0,1);
	vertices[23] = D3DVERTEX(points[2],normals[5],1,1);
}

DDPIXELFORMAT texformats[256];
int texformatindex = 0;

void cleartexformats()
{
	ZeroMemory(texformats,256*sizeof(DDPIXELFORMAT));
	texformatindex = 0;
}

HRESULT CALLBACK gettexformat(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
	memcpy(&texformats[texformatindex],lpDDPixFmt,sizeof(DDPIXELFORMAT));
	texformatindex++;
	if(texformatindex >= 256) return D3DENUMRET_CANCEL;
	return D3DENUMRET_OK;
}

static bool gentexture(const DDPIXELFORMAT format, MultiDirectDrawSurface **surface, int width, int height, int pattern)
{
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_TEXTURESTAGE;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
	ddsd.ddpfPixelFormat = format;
	HRESULT error = ddinterface->CreateSurface(&ddsd,surface,NULL);
	if(error != D3D_OK) return false;
	switch(pattern)
	{
	case 0:
	default:
		(*surface)->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
		DrawPalette(ddsd,(unsigned char *)ddsd.lpSurface);
		(*surface)->Unlock(NULL);
	}
	return true;
}

static const DDPIXELFORMAT fmt_rgba4444 = {sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,16,0xF00,0xF0,0xF,0xF000};
static const DDPIXELFORMAT fmt_rgba1555 = {sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,16,0x7C00,0x3E0,0x1F,0x8000};
static const DDPIXELFORMAT fmt_rgb565 = {sizeof(DDPIXELFORMAT),DDPF_RGB,0,16,0xF800,0x7E0,0x1F,0};
static const DDPIXELFORMAT fmt_rgba8888 = {sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,32,0xFF0000,0xFF00,0xFF,0xFF000000};

void InitTest3D(int test)
{
	HRESULT error;
	D3DMATRIX matWorld;
	D3DMATRIX matView;
	D3DMATRIX matProj;
	D3DMATRIX mat;
	D3DMATERIAL7 material;
	switch(test)
	{
	case 0:
		MakeCube3D(points,normals,vertices);
		ZeroMemory(&material,sizeof(D3DMATERIAL7));
		material.ambient.r = 0.5f;
		material.ambient.g = 0.5f;
		material.ambient.b = 0.0f;
		material.diffuse.r = 1.0f;
		material.diffuse.g = 1.0f;
		material.diffuse.b = 1.0f;
		error = d3d7dev->SetMaterial(&material);
		error = d3d7dev->LightEnable(0,TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENT, 0x7f7f7f7f);
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
		matWorld = mat;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD,&matWorld);
		matView = mat;
		matView._43 = 10.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_VIEW,&matView);
		matProj = mat;
	    matProj._11 =  2.0f;
	    matProj._22 =  2.0f;
	    matProj._34 =  1.0f;
	    matProj._43 = -1.0f;
	    matProj._44 =  0.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION,&matProj);
		ZeroMemory(&lights[0],sizeof(D3DLIGHT7));
		lights[0].dltType = D3DLIGHT_DIRECTIONAL;
		lights[0].dcvDiffuse.r = 1;
		lights[0].dcvDiffuse.g = 0;
		lights[0].dcvDiffuse.b = 1;
		lights[0].dvDirection = D3DVECTOR(5,5,5);
		lights[0].dvRange = D3DLIGHT_RANGE_MAX;
		lights[0].dvAttenuation1 = 0.4f;
		error = d3d7dev->SetLight(0,&lights[0]);
		break;
	case 1:
		MakeCube3D(points,normals,vertices);
		cleartexformats();		
		d3d7dev->EnumTextureFormats(gettexformat,NULL);
		gentexture(fmt_rgba4444,&textures[0],256,256,0);
		gentexture(fmt_rgba1555,&textures[1],256,256,0);
		gentexture(fmt_rgb565,&textures[2],256,256,0);
		gentexture(fmt_rgba8888,&textures[3],256,256,0);
		ZeroMemory(&material,sizeof(D3DMATERIAL7));
		material.ambient.r = 1.0f;
		material.ambient.g = 1.0f;
		material.ambient.b = 1.0f;
		material.diffuse.r = 1.0f;
		material.diffuse.g = 1.0f;
		material.diffuse.b = 1.0f;
		error = d3d7dev->SetMaterial(&material);
		error = d3d7dev->LightEnable(0,TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xFFFFFFFF);
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
		matWorld = mat;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD,&matWorld);
		matView = mat;
		matView._43 = 10.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_VIEW,&matView);
		matProj = mat;
	    matProj._11 =  2.0f;
	    matProj._22 =  2.0f;
	    matProj._34 =  1.0f;
	    matProj._43 = -1.0f;
	    matProj._44 =  0.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION,&matProj);
		ZeroMemory(&lights[0],sizeof(D3DLIGHT7));
		lights[0].dltType = D3DLIGHT_DIRECTIONAL;
		lights[0].dcvDiffuse.r = 1;
		lights[0].dcvDiffuse.g = 0;
		lights[0].dcvDiffuse.b = 1;
		lights[0].dvDirection = D3DVECTOR(5,5,5);
		lights[0].dvRange = D3DLIGHT_RANGE_MAX;
		lights[0].dvAttenuation1 = 0.4f;
		error = d3d7dev->SetLight(0,&lights[0]);
		break;
	case 2:
		MakeCube3D(points,normals,vertices);
		ZeroMemory(&material,sizeof(D3DMATERIAL7));
		material.ambient.r = 1.0f;
		material.ambient.g = 1.0f;
		material.ambient.b = 1.0f;
		material.diffuse.r = 1.0f;
		material.diffuse.g = 1.0f;
		material.diffuse.b = 1.0f;
		error = d3d7dev->SetMaterial(&material);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xffffffff);
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
		matWorld = mat;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD,&matWorld);
		matView = mat;
		matView._43 = 10.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_VIEW,&matView);
		matProj = mat;
	    matProj._11 =  2.0f;
	    matProj._22 =  2.0f;
	    matProj._34 =  1.0f;
	    matProj._43 = -1.0f;
	    matProj._44 =  0.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION,&matProj);
		break;
	default:
		break;
	}
}

void RunTestTimed3D(int test)
{
	if(!d3d7dev) return;
	POINT p;
	RECT srcrect,destrect;
	HRESULT error;
	D3DMATRIX mat;
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
	if(d3dver >= 3) ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	error = d3d7dev->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1.0,0);
	float time = (float)clock() / (float)CLOCKS_PER_SEC;
	switch(test)
	{
	case 0:
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
	    mat._11 = (FLOAT)cos( (float)time );
	    mat._33 = (FLOAT)cos( (float)time );
	    mat._13 = -(FLOAT)sin( (float)time );
	    mat._31 = (FLOAT)sin( (float)time );
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD, &mat);
		error = d3d7dev->BeginScene();
		error = d3d7dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_VERTEX,vertices,24,cube_mesh,36,0);
		error = d3d7dev->EndScene();
		break;
	case 1:
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
	    mat._11 = (FLOAT)cos( (float)time );
	    mat._33 = (FLOAT)cos( (float)time );
	    mat._13 = -(FLOAT)sin( (float)time );
	    mat._31 = (FLOAT)sin( (float)time );
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD, &mat);
		error = d3d7dev->BeginScene();
		error = d3d7dev->SetTexture(0,(LPDIRECTDRAWSURFACE7)textures[0]->GetSurface());
		error = d3d7dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_VERTEX,vertices,4,0);
		error = d3d7dev->SetTexture(0,(LPDIRECTDRAWSURFACE7)textures[1]->GetSurface());
		error = d3d7dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_VERTEX,vertices+4,4,0);
		error = d3d7dev->SetTexture(0,(LPDIRECTDRAWSURFACE7)textures[2]->GetSurface());
		error = d3d7dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_VERTEX,vertices+8,4,0);
		error = d3d7dev->SetTexture(0,(LPDIRECTDRAWSURFACE7)textures[3]->GetSurface());
		error = d3d7dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_VERTEX,vertices+12,4,0);
		error = d3d7dev->EndScene();
		break;
	case 2:
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
	    mat._11 = (FLOAT)cos( (float)time );
	    mat._33 = (FLOAT)cos( (float)time );
	    mat._13 = -(FLOAT)sin( (float)time );
	    mat._31 = (FLOAT)sin( (float)time );
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD, &mat);
		error = d3d7dev->BeginScene();
		error = d3d7dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_VERTEX,vertices,24,cube_mesh,36,0);
		error = d3d7dev->EndScene();
		break;
	default:
		break;
	}
	if(fullscreen)
	{
		if(backbuffers) ddsurface->Flip(NULL,DDFLIP_WAIT);
	}
	else
	{
		p.x = 0;
		p.y = 0;
		ClientToScreen(hWnd,&p);
		GetClientRect(hWnd,&destrect);
		OffsetRect(&destrect,p.x,p.y);
		SetRect(&srcrect,0,0,width,height);
		if(ddsurface && ddsrender)error = ddsurface->Blt(&destrect,ddsrender,&srcrect,DDBLT_WAIT,NULL);
	}
}

void RunTestLooped3D(int test)
{
}


INT_PTR CALLBACK TexShader7Proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HRESULT error;
	D3DVIEWPORT7 vp;
	HWND hDisplay;
	switch(Msg)
	{
	case WM_INITDIALOG:
		RECT r;
		DDSCAPS2 caps;
		DDSURFACEDESC2 ddsd;
		DDPIXELFORMAT ddpfz;
		testnum = 2;
		ddinterface = new MultiDirectDraw(7,&error,NULL);
		hDisplay = GetDlgItem(hWnd,IDC_DISPLAY);
		::hWnd = hDisplay;
		error = ddinterface->SetCooperativeLevel(hDisplay,DDSCL_NORMAL);
		ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		error = ddinterface->CreateSurface(&ddsd,&ddsurface,NULL);
		error = ddinterface->CreateClipper(0,&ddclipper,NULL);
		error = ddclipper->SetHWnd(0,hDisplay);
		error = ddsurface->SetClipper(ddclipper);
		ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE;
		GetClientRect(hDisplay,&r);
		ddsd.dwWidth = r.right;
		ddsd.dwHeight = r.bottom;
		error = ddinterface->CreateSurface(&ddsd,&ddsrender,NULL);
		error = ddinterface->QueryInterface(IID_IDirect3D7,(VOID**)&d3d7);
		error = d3d7->EnumZBufferFormats(IID_IDirect3DRGBDevice,zcallback,&ddpfz);
		error = ddsrender->GetSurfaceDesc(&ddsd);
		ddsd.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER|DDSCAPS_VIDEOMEMORY;
		memcpy(&ddsd.ddpfPixelFormat,&ddpfz,sizeof(DDPIXELFORMAT));
		error = ddinterface->CreateSurface(&ddsd,&zbuffer,NULL);
		error = ddsrender->AddAttachedSurface(zbuffer);
		error = d3d7->CreateDevice(IID_IDirect3DHALDevice,(LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(),&d3d7dev);
		if(error != D3D_OK)
			error = d3d7->CreateDevice(IID_IDirect3DRGBDevice,(LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(),&d3d7dev);
		ddsrender->GetSurfaceDesc(&ddsd);
		vp.dvMaxZ = 1.0f;
		vp.dvMinZ = 0.0f;
		vp.dwX = vp.dwY = 0;
		vp.dwWidth = ddsd.dwWidth;
		vp.dwHeight = ddsd.dwHeight;
		error = d3d7dev->SetViewport(&vp);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_ZENABLE,TRUE);
		InitTest3D(2);
		::width = ddsd.dwWidth;
		::height = ddsd.dwHeight;
		StartTimer(hWnd,WM_APP,60);
		break;
    case WM_CLOSE:
		ddinterface->Release();
        EndDialog(hWnd,IDCANCEL);
        break;
	case WM_APP:
		RunTestTimed3D(testnum);
		break;
	default:
		return FALSE;
	}
	return TRUE;


}
