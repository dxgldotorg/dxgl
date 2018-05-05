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
#include "MultiDD.h"
#include "tests.h"
#include "surfacegen.h"
#include "timer.h"
#include "misc.h"
#define D3D_OVERLOADS
#include "../ddraw/include/d3d.h"
#include <WindowsX.h>

void InitTest(int test);
void RunTestTimed(int test);
void RunTestLooped(int test);


static MultiDirectDraw *ddinterface;
static MultiDirectDrawSurface *ddsurface;
static MultiDirectDrawSurface *ddsrender;
static MultiDirectDrawSurface *zbuffer;
static MultiDirectDrawSurface *textures[8];
static IDirectDrawPalette *pal;
static IDirect3D7 *d3d7;
static IDirect3DDevice7 *d3d7dev;
D3DMATERIAL7 material;
static LPDIRECTDRAWCLIPPER ddclipper;
static int width,height,bpp,refresh,backbuffers;
static double fps;
static bool fullscreen,resizable;
static HWND hWnd;
static int testnum;
static unsigned int randnum;
static int testtypes[] = {0,1,0,1,0,1,0,0,2,1,0,0,0,0,0,0};
static DWORD counter;

#define FVF_COLORVERTEX (D3DFVF_VERTEX | D3DFVF_DIFFUSE | D3DFVF_SPECULAR)
struct COLORVERTEX
{
	D3DVALUE x;
	D3DVALUE y;
	D3DVALUE z;
	D3DVALUE nx;
	D3DVALUE ny;
	D3DVALUE nz;
	D3DCOLOR color;
	D3DCOLOR specular;
	D3DVALUE tu;
	D3DVALUE tv;
public:
	COLORVERTEX() {};
	COLORVERTEX(const D3DVECTOR& v, const D3DVECTOR& n, D3DCOLOR _color,
		D3DCOLOR _specular, D3DVALUE _tu, D3DVALUE _tv)
	{
		x = v.x; y = v.y; z = v.z;
		nx = n.x; ny = n.y; nz = n.z;
		color = _color; specular = _specular;
		tu = _tu; tv = _tv;
	}

};

static int numpoints = 0;
static int numindices = 0;
static D3DVERTEX *vertices = NULL;
static D3DLVERTEX *litvertices = NULL;
static COLORVERTEX *colorvertices = NULL;
static WORD *mesh = NULL;
static D3DLIGHT7 lights[8];
static BOOL lightenable[8];
static DWORD bgcolor = 0;

static UINT lastmousemsg = WM_NULL;
static WPARAM lastmousewparam = 0;
static LPARAM lastmouselparam = NULL;

typedef struct
{
	DWORD ambient;
	DWORD diffuse;
	DWORD specular;
} HEXLIGHTCOLOR;
HEXLIGHTCOLOR hexlightcolor[8];

typedef struct
{
	D3DTEXTUREOP colorop;
	DWORD colorarg1;
	DWORD colorarg2;
	D3DTEXTUREOP alphaop;
	DWORD alphaarg1;
	DWORD alphaarg2;
	DWORD texturetype;
	TCHAR texturefile[MAX_PATH + 1];
	MultiDirectDrawSurface* texture;
	BOOL colorkey;
	DWORD keycolor;
} TEXSTAGE;

typedef struct
{
	int currentstage;
	TEXSTAGE texstages[8];
	DWORD diffuse;
	DWORD specular;
	DWORD factor;
	D3DFOGMODE vertexfog;
	D3DFOGMODE pixelfog;
	FLOAT fogstart;
	FLOAT fogend;
	FLOAT fogdensity;
	BOOL rangefog;
	DWORD fogcolor;
	BOOL alphatest;
	BOOL alphastipple;
	BOOL colorkey;
	BOOL colorkeyblend;
	D3DBLEND srcblend;
	D3DBLEND destblend;
	BYTE refalpha;
	D3DCMPFUNC alphafunc;
	D3DLINEPATTERN linepattern;
	DWORD fillstippletype;
	TCHAR fillstipplefile[MAX_PATH + 1];
	DWORD fillstipple[32];
} TEXSHADERSTATE;

static TEXSHADERSTATE texshaderstate;
const TEXSHADERSTATE defaulttexshaderstate =
{
	0,
	{
		{ D3DTOP_MODULATE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_SELECTARG1,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
		{ D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
		{ D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
		{ D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
		{ D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
		{ D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
		{ D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
		{ D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_TEXTURE,D3DTA_CURRENT,0,_T(""),NULL,FALSE,0 },
	},
	0xFFFFFFFF,
	0,
	0xFFFFFFFF,
	D3DFOG_NONE,
	D3DFOG_NONE,
	0.0,
	1.0,
	1.0,
	FALSE,
	0,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	D3DBLEND_ONE,
	D3DBLEND_ZERO,
	0,
	D3DCMP_ALWAYS,
	{ 0,0 }
};

typedef struct
{
	DWORD texturetype;
	TCHAR texturefile[MAX_PATH + 1];
	MultiDirectDrawSurface* texture;
	int currentlight;
} VERTEXSHADERSTATE;
static VERTEXSHADERSTATE vertexshaderstate;

static DDSPRITE sprites[16];
static HFONT displayfonts[16];
static SIZE textsize;


LRESULT CALLBACK DDWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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
		for(int i = 0; i < 16; i++)
			if(sprites[i].surface) sprites[i].surface->Release();
		for (int i = 0; i < 8; i++)
		{
			if (textures[i])
			{
				textures[i]->Release();
				textures[i] = NULL;
			}
		}
		if (d3d7dev)
		{
			d3d7dev->Release();
			d3d7dev = NULL;
		}
		if (d3d7)
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
		if (zbuffer)
		{
			zbuffer->Release();
			zbuffer = NULL;
		}
		if(pal)
		{
			pal->Release();
			pal = NULL;
		}
		if (ddclipper)
		{
			ddclipper->Release();
			ddclipper = NULL;
		}
		if(ddinterface)
		{
			ddinterface->Release();
			ddinterface = NULL;
		}
		if (mesh)
		{
			free(mesh);
			mesh = NULL;
		}
		if (vertices)
		{
			free(vertices);
			vertices = NULL;
		}
		if (litvertices)
		{
			free(litvertices);
			litvertices = NULL;
		}
		if (colorvertices)
		{
			free(colorvertices);
			colorvertices = NULL;
		}
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE) DestroyWindow(hWnd);
		break;
	case WM_APP:
		RunTestTimed(testnum);
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
		lastmousemsg = Msg;
		lastmousewparam = wParam;
		lastmouselparam = lParam;
		break;
	default:
		return DefWindowProc(hWnd,Msg,wParam,lParam);
	}
	return FALSE;
}

static int ddtestnum;
static int d3dver;
static int ddver;

const TCHAR wndclassname[] = _T("DDTestWndClass");

static HRESULT WINAPI zcallback(DDPIXELFORMAT *ddpf, VOID *context)
{
	if (ddpf->dwFlags == DDPF_ZBUFFER)
	{
		memcpy(context, ddpf, sizeof(DDPIXELFORMAT));
		return D3DENUMRET_CANCEL;
	}
	return D3DENUMRET_OK;
}

INT_PTR CALLBACK TexShader7Proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK VertexShader7Proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

void RunDXGLTest(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	int filter, int msaa, double fps, bool fullscreen, bool resizable, BOOL is3d)
{
	ZeroMemory(sprites,16*sizeof(DDSPRITE));
	if(testnum == 14)
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_TEXSHADER),NULL,TexShader7Proc);
		return;
	}
	if(testnum == 15)
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_VERTEXSHADER),NULL,VertexShader7Proc);
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
	if(apiver > 3) ddsd.dwSize = sizeof(DDSURFACEDESC2);
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
	::testnum = testnum;
	d3dver = apiver;
	if(apiver == 3) ddver = 4;
	else ddver = apiver;
	HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(NULL);
	WNDCLASSEX wc;
	MSG Msg;
	ZeroMemory(&wc,sizeof(WNDCLASS));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DDWndProc;
	wc.hInstance = hinstance;
	wc.hIcon = LoadIcon(hinstance,MAKEINTRESOURCE(IDI_DXGL));
	wc.hIconSm = LoadIcon(hinstance,MAKEINTRESOURCE(IDI_DXGLSM));
	if(testnum == 6) wc.hCursor = LoadCursor(NULL,IDC_CROSS);
	else wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = wndclassname;
	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL,_T("Can not register window class"),_T("Error"),MB_ICONEXCLAMATION|MB_OK);
		return;
	}
	if(resizable)
		hWnd = CreateWindowEx(WS_EX_APPWINDOW,wndclassname,_T("DDraw Test Window"),WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,CW_USEDEFAULT,width,height,NULL,NULL,hinstance,NULL);
	else if(!fullscreen)
		hWnd = CreateWindowEx(WS_EX_APPWINDOW,wndclassname,_T("DDraw Test Window"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
			CW_USEDEFAULT,CW_USEDEFAULT,width,height,NULL,NULL,hinstance,NULL);
	else hWnd = CreateWindowEx(0,wndclassname,_T("DDraw Test Window"),WS_POPUP,0,0,
		GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),NULL,NULL,hinstance,NULL);
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
		if(is3d) ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
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
		if(is3d) ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		else ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;
		error = ddinterface->CreateSurface(&ddsd,&ddsrender,NULL);
		ddsrender->GetSurfaceDesc(&ddsd);
		::bpp = ddsd.ddpfPixelFormat.dwRGBBitCount;
	}
	else
	{
		ddsrender = ddsurface;
		ddsrender->AddRef();
	}
	if(bpp == 8)
	{
		ddinterface->CreatePalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256,(LPPALETTEENTRY)&DefaultPalette,&pal,NULL);
		ddsrender->SetPalette(pal);
	}
	else pal = NULL;
	if (is3d)
	{
		error = ddinterface->QueryInterface(IID_IDirect3D7, (VOID**)&d3d7);
		error = d3d7->EnumZBufferFormats(IID_IDirect3DRGBDevice, zcallback, &ddpfz);
		error = ddsrender->GetSurfaceDesc(&ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
		memcpy(&ddsd.ddpfPixelFormat, &ddpfz, sizeof(DDPIXELFORMAT));
		error = ddinterface->CreateSurface(&ddsd, &zbuffer, NULL);
		error = ddsrender->AddAttachedSurface(zbuffer);
		error = d3d7->CreateDevice(IID_IDirect3DHALDevice, (LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(), &d3d7dev);
		if (error != D3D_OK)
			error = d3d7->CreateDevice(IID_IDirect3DRGBDevice, (LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(), &d3d7dev);
		ddsrender->GetSurfaceDesc(&ddsd);
		D3DVIEWPORT7 vp = { 0,0,ddsd.dwWidth,ddsd.dwHeight,0.0f,1.0f };
		error = d3d7dev->SetViewport(&vp);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
	}
	InitTest(testnum);
	if(!fullscreen) SendMessage(hWnd,WM_PAINT,0,0);
	if(testtypes[testnum] == 1)
	{
		while(!done)
		{
			if(PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			{
				if(Msg.message == WM_PAINT) RunTestLooped(testnum);
				else if(Msg.message  == WM_QUIT) done = TRUE;
				else
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}
			else
			{
				RunTestLooped(testnum);
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
	UnregisterClass(wndclassname,hinstance);
	StopTimer();
}

void MakeCube3D(float size, int detail)
{
	if (detail < 2) return;
	D3DVECTOR normals[6];
	normals[0] = D3DVECTOR(0.0f, 0.0f, -1.0f);
	normals[1] = D3DVECTOR(1.0f, 0.0f, 0.0f);
	normals[2] = D3DVECTOR(0.0f, 0.0f, 1.0f);
	normals[3] = D3DVECTOR(-1.0f, 0.0f, 0.0f);
	normals[4] = D3DVECTOR(0.0f, 1.0f, 0.0f);
	normals[5] = D3DVECTOR(0.0f, -1.0f, 0.0f);
	int numfacevertices = detail*detail;
	D3DVECTOR *face = (D3DVECTOR*)malloc(numfacevertices * sizeof(D3DVECTOR));
	int numfaceindices = ((detail - 1)*(detail - 1)) * 6;
	WORD *faceindex = (WORD*)malloc(numfaceindices * sizeof(WORD));
	int ptr = 0;
	float fx, fy;
	// Generate points
	for (int y = 0; y < detail; y++)
	{
		fy = (((float)(y / (float)(detail - 1))) - .5f)*size;
		for (int x = 0; x < detail; x++)
		{
			fx = (((float)(x / (float)(detail - 1))) - .5f)*size;
			face[ptr] = D3DVECTOR(fx, fy, 0);
			ptr++;
		}
	}
	// Generate triangle indices
	ptr = 0;
	for (int y = 0; y < (detail - 1); y++)
	{
		for (int x = 0; x < (detail - 1); x++)
		{
			faceindex[ptr++] = x + (detail*y);
			faceindex[ptr++] = x + (detail*(y + 1));
			faceindex[ptr++] = (x + 1) + (detail*y);
			faceindex[ptr++] = (x + 1) + (detail*y);
			faceindex[ptr++] = x + (detail*(y + 1));
			faceindex[ptr++] = (x + 1) + (detail*(y + 1));
		}
	}
	numpoints = (detail*detail) * 6;
	numindices = ((detail - 1)*(detail - 1)) * 36;
	if (vertices) free(vertices);
	if (litvertices) free(litvertices);
	if (colorvertices) free(colorvertices);
	if (mesh) free(mesh);
	vertices = (D3DVERTEX*)malloc(numpoints * sizeof(D3DVERTEX));
	litvertices = (D3DLVERTEX*)malloc(numpoints * sizeof(D3DLVERTEX));
	colorvertices = (COLORVERTEX*)malloc(numpoints * sizeof(COLORVERTEX));
	mesh = (WORD*)malloc(numindices * sizeof(WORD));
	// Generate vertex list
	float u, v;
	D3DVECTOR pos;
	D3DVECTOR normal;
	// Front face
	ptr = 0;
	for (int y = 0; y < detail; y++)
	{
		for (int x = 0; x < detail; x++)
		{
			u = (float)x / (float)(detail - 1);
			v = 1.f - ((float)y / (float)(detail - 1));
			ptr = x + (detail*y);
			pos = D3DVECTOR(face[ptr].x, face[ptr].y, -size / 2.f);
			normal = D3DVECTOR(0, 0, -1);
			vertices[ptr] = D3DVERTEX(pos, normal, u, v);
			litvertices[ptr] = D3DLVERTEX(pos, 0xFFFFFFFF, 0, u, v);
			colorvertices[ptr] = COLORVERTEX(pos, normal, 0xFFFFFFFF, 0, u, v);
		}
	}
	// Right face
	ptr = 0;
	for (int y = 0; y < detail; y++)
	{
		for (int x = 0; x < detail; x++)
		{
			u = (float)x / (float)(detail - 1);
			v = 1.f - ((float)y / (float)(detail - 1));
			ptr = x + (detail*y);
			pos = D3DVECTOR(size / 2.f, face[ptr].y, face[ptr].x);
			normal = D3DVECTOR(1, 0, 0);
			vertices[ptr + numfacevertices] = D3DVERTEX(pos, normal, u, v);
			litvertices[ptr + numfacevertices] = D3DLVERTEX(pos, 0xFFFFFFFF, 0, u, v);
			colorvertices[ptr + numfacevertices] = COLORVERTEX(pos, normal, 0xFFFFFFFF, 0, u, v);
		}
	}
	// Back face
	ptr = 0;
	for (int y = 0; y < detail; y++)
	{
		for (int x = 0; x < detail; x++)
		{
			u = (float)x / (float)(detail - 1);
			v = 1.f - ((float)y / (float)(detail - 1));
			ptr = x + (detail*y);
			pos = D3DVECTOR(-face[ptr].x, face[ptr].y, size / 2.f);
			normal = D3DVECTOR(0, 0, 1);
			vertices[ptr + (numfacevertices * 2)] = D3DVERTEX(pos, normal, u, v);
			litvertices[ptr + (numfacevertices * 2)] = D3DLVERTEX(pos, 0xFFFFFFFF, 0, u, v);
			colorvertices[ptr + (numfacevertices * 2)] = COLORVERTEX(pos, normal, 0xFFFFFFFF, 0, u, v);
		}
	}
	// Left face
	ptr = 0;
	for (int y = 0; y < detail; y++)
	{
		for (int x = 0; x < detail; x++)
		{
			u = (float)x / (float)(detail - 1);
			v = 1.f - ((float)y / (float)(detail - 1));
			ptr = x + (detail*y);
			pos = D3DVECTOR(-size / 2.f, face[ptr].y, -face[ptr].x);
			normal = D3DVECTOR(-1, 0, 0);
			vertices[ptr + (numfacevertices * 3)] = D3DVERTEX(pos, normal, u, v);
			litvertices[ptr + (numfacevertices * 3)] = D3DLVERTEX(pos, 0xFFFFFFFF, 0, u, v);
			colorvertices[ptr + (numfacevertices * 3)] = COLORVERTEX(pos, normal, 0xFFFFFFFF, 0, u, v);
		}
	}
	// Top face
	for (int y = 0; y < detail; y++)
	{
		for (int x = 0; x < detail; x++)
		{
			u = (float)x / (float)(detail - 1);
			v = 1.f - ((float)y / (float)(detail - 1));
			ptr = x + (detail*y);
			pos = D3DVECTOR(face[ptr].x, size / 2.f, face[ptr].y);
			normal = D3DVECTOR(0, 1, 0);
			vertices[ptr + (numfacevertices * 4)] = D3DVERTEX(pos, normal, u, v);
			litvertices[ptr + (numfacevertices * 4)] = D3DLVERTEX(pos, 0xFFFFFFFF, 0, u, v);
			colorvertices[ptr + (numfacevertices * 4)] = COLORVERTEX(pos, normal, 0xFFFFFFFF, 0, u, v);
		}
	}
	// Bottom face
	for (int y = 0; y < detail; y++)
	{
		for (int x = 0; x < detail; x++)
		{
			u = (float)x / (float)(detail - 1);
			v = 1.f - ((float)y / (float)(detail - 1));
			ptr = x + (detail*y);
			pos = D3DVECTOR(face[ptr].x, -size / 2.f, -face[ptr].y);
			normal = D3DVECTOR(0, -1, 0);
			vertices[ptr + (numfacevertices * 5)] = D3DVERTEX(pos, normal, u, v);
			litvertices[ptr + (numfacevertices * 5)] = D3DLVERTEX(pos, 0xFFFFFFFF, 0, u, v);
			colorvertices[ptr + (numfacevertices * 5)] = COLORVERTEX(pos, normal, 0xFFFFFFFF, 0, u, v);
		}
	}
	free(face);
	// Generate index table
	ptr = 0;
	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < numfaceindices; j++)
			mesh[ptr++] = faceindex[j] + (i*numfacevertices);
	}
	free(faceindex);
}

void SetVertexColor(D3DLVERTEX *start, COLORVERTEX *start_color, int count, DWORD color)
{
	for (int i = 0; i < count; i++)
	{
		start[i].color = color;
		start_color[i].color = color;
	}
}

void SetVertexSpecular(D3DLVERTEX *start, COLORVERTEX *start_color, int count, DWORD color)
{
	for (int i = 0; i < count; i++)
	{
		start[i].specular = color;
		start_color[i].specular = color;
	}
}


DDPIXELFORMAT texformats[256];
int texformatindex = 0;

void cleartexformats()
{
	ZeroMemory(texformats, 256 * sizeof(DDPIXELFORMAT));
	texformatindex = 0;
}

HRESULT CALLBACK gettexformat(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
	memcpy(&texformats[texformatindex], lpDDPixFmt, sizeof(DDPIXELFORMAT));
	texformatindex++;
	if (texformatindex >= 256) return D3DENUMRET_CANCEL;
	return D3DENUMRET_OK;
}

static bool gentexture(const DDPIXELFORMAT format, MultiDirectDrawSurface **surface, int width, int height, int pattern)
{
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
	ddsd.ddpfPixelFormat = format;
	HRESULT error = ddinterface->CreateSurface(&ddsd, surface, NULL);
	if (error != D3D_OK) return false;
	switch (pattern)
	{
	case 0:
	default:
		(*surface)->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
		DrawPalette(ddsd, (unsigned char *)ddsd.lpSurface);
		(*surface)->Unlock(NULL);
	}
	return true;
}

void MakeLights()
{
	for (int i = 0; i < 8; i++)
	{
		lightenable[i] = FALSE;
		ZeroMemory(&lights[i], sizeof(D3DLIGHT7));
		lights[i].dcvDiffuse.r = lights[i].dcvDiffuse.g = lights[i].dcvDiffuse.b = 1;
		lights[i].dltType = D3DLIGHT_DIRECTIONAL;
		hexlightcolor[i].diffuse = 0xFFFFFF;
	}
	lightenable[0] = TRUE;
	lights[0].dvPosition = D3DVECTOR(-10, -10, -10);
	lights[1].dvPosition = D3DVECTOR(10, -10, -10);
	lights[2].dvPosition = D3DVECTOR(-10, 10, -10);
	lights[3].dvPosition = D3DVECTOR(10, 10, -10);
	lights[4].dvPosition = D3DVECTOR(-10, -10, 10);
	lights[5].dvPosition = D3DVECTOR(10, -10, 10);
	lights[6].dvPosition = D3DVECTOR(-10, 10, 10);
	lights[7].dvPosition = D3DVECTOR(10, 10, 10);
	lights[0].dvDirection = D3DVECTOR(1, 1, 1);
	lights[1].dvDirection = D3DVECTOR(-1, 1, 1);
	lights[2].dvDirection = D3DVECTOR(1, -1, 1);
	lights[3].dvDirection = D3DVECTOR(-1, -1, 1);
	lights[4].dvDirection = D3DVECTOR(1, 1, -1);
	lights[5].dvDirection = D3DVECTOR(-1, 1, -1);
	lights[6].dvDirection = D3DVECTOR(1, -1, -1);
	lights[7].dvDirection = D3DVECTOR(-1, -1, -1);
}

static const DDPIXELFORMAT fmt_rgba4444 = { sizeof(DDPIXELFORMAT),DDPF_RGB | DDPF_ALPHAPIXELS,0,16,0xF00,0xF0,0xF,0xF000 };
static const DDPIXELFORMAT fmt_rgba1555 = { sizeof(DDPIXELFORMAT),DDPF_RGB | DDPF_ALPHAPIXELS,0,16,0x7C00,0x3E0,0x1F,0x8000 };
static const DDPIXELFORMAT fmt_rgb565 = { sizeof(DDPIXELFORMAT),DDPF_RGB,0,16,0xF800,0x7E0,0x1F,0 };
static const DDPIXELFORMAT fmt_rgba8888 = { sizeof(DDPIXELFORMAT),DDPF_RGB | DDPF_ALPHAPIXELS,0,32,0xFF0000,0xFF00,0xFF,0xFF000000 };

void InitTest(int test)
{
	DDSURFACEDESC2 ddsd;
	DDSCAPS2 ddscaps;
	HDC hRenderDC;
	ZeroMemory(&ddscaps,sizeof(DDSCAPS2));
	LPDIRECTDRAWPALETTE palette;
	unsigned char *buffer;
	MultiDirectDrawSurface *temp1 = NULL;
	MultiDirectDrawSurface *temp2 = NULL;
	HRESULT error;
	D3DMATRIX matWorld;
	D3DMATRIX matView;
	D3DMATRIX matProj;
	D3DMATRIX mat;
	bgcolor = 0;
	DDCOLORKEY ckey;
	HCURSOR cursor;
	HDC memorydc;
	BITMAPINFO bitmapinfo;
	HBITMAP bitmap;
	HGDIOBJ temp;
	HBRUSH brush;
	RECT r;
	DDCOLORKEY colorkey;
	void *bmppointer;
	int i;
	const DWORD colormasks[7] = { 0xFF0000, 0xFF00, 0xFFFF00, 0xFF, 0xFF00FF, 0xFFFF, 0xFFFFFF };
	ckey.dwColorSpaceHighValue = ckey.dwColorSpaceLowValue = 0;
	if(ddver > 3)ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	error = ddsrender->GetSurfaceDesc(&ddsd);
	switch (test)
	{
	case 0:
		if (!fullscreen) backbuffers = 0;
		buffer = (unsigned char *)malloc(ddsd.lPitch*ddsd.dwHeight);
		DrawPalette(ddsd, buffer);
		error = ddsrender->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
		memcpy(ddsd.lpSurface, buffer, ddsd.lPitch*ddsd.dwHeight);
		error = ddsrender->Unlock(NULL);
		ddsrender->GetPalette(&palette);
		if (backbuffers > 0)
		{
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
			error = ddsrender->GetAttachedSurface(&ddscaps, &temp1);
			DrawGradients(ddsd, buffer, hWnd, palette, 1, 0);
			error = temp1->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
			memcpy(ddsd.lpSurface, buffer, ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if (backbuffers > 1)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			DrawGradients(ddsd, buffer, hWnd, palette, 0, 0x0000FF);
			error = temp1->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
			memcpy(ddsd.lpSurface, buffer, ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if (backbuffers > 2)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			DrawGradients(ddsd, buffer, hWnd, palette, 0, 0x00FF00);
			error = temp1->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
			memcpy(ddsd.lpSurface, buffer, ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if (backbuffers > 3)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			DrawGradients(ddsd, buffer, hWnd, palette, 0, 0xFF0000);
			error = temp1->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
			memcpy(ddsd.lpSurface, buffer, ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if (temp1) temp1->Release();
		free(buffer);
		if (palette) palette->Release();
		break;
	case 2:
		if (!fullscreen) backbuffers = 0;
		error = ddsrender->GetDC(&hRenderDC);
		DrawGDIPatterns(ddsd, hRenderDC, 0);
		ddsrender->ReleaseDC(hRenderDC);
		if (backbuffers > 0)
		{
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
			error = ddsrender->GetAttachedSurface(&ddscaps, &temp1);
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd, hRenderDC, 1);
			temp1->ReleaseDC(hRenderDC);
		}
		if (backbuffers > 1)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd, hRenderDC, 2);
			temp1->ReleaseDC(hRenderDC);
		}
		if (backbuffers > 2)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd, hRenderDC, 3);
			temp1->ReleaseDC(hRenderDC);
		}
		if (backbuffers > 3)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd, hRenderDC, 4);
			temp1->ReleaseDC(hRenderDC);
		}
		if (backbuffers > 4)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd, hRenderDC, 5);
			temp1->ReleaseDC(hRenderDC);
		}
		if (backbuffers > 5)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd, hRenderDC, 6);
			temp1->ReleaseDC(hRenderDC);
		}
		if (backbuffers > 6)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps, &temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd, hRenderDC, 7);
			temp1->ReleaseDC(hRenderDC);
		}
		if (temp1) temp1->Release();
		break;
	case 4:
		ddsrender->GetSurfaceDesc(&ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddinterface->CreateSurface(&ddsd, &sprites[0].surface, NULL);
		ddsrender->GetPalette(&palette);
		error = sprites[0].surface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
		if(palette) palette->Release();
		DrawGradients(ddsd, (unsigned char *)ddsd.lpSurface, hWnd, palette, 1, 0);
		DrawDitheredColor(&ddsd, (unsigned char *)ddsd.lpSurface, 0, FALSE);
		error = sprites[0].surface->Unlock(NULL);
		sprites[0].width = (float)ddsd.dwWidth;
		sprites[0].height = (float)ddsd.dwHeight;
		sprites[0].rect.left = sprites[0].rect.top = 0;
		sprites[0].rect.right = ddsd.dwWidth;
		sprites[0].rect.bottom = ddsd.dwHeight;
		if (backbuffers) ddsrender->GetAttachedSurface(&ddscaps, &temp1);
		else temp1 = ddsrender;
		temp1->SetColorKey(DDCKEY_DESTBLT, &ckey);
		if (backbuffers) temp1->Release();
		for (i = 1; i < 16; i++)
		{
			switch ((i - 1 & 3))
			{
			case 0:
				sprites[i].width = sprites[i].height = 64.f;
				sprites[i].ddsd.dwWidth = sprites[i].ddsd.dwHeight =
					sprites[i].rect.right = sprites[i].rect.bottom = 64;
				sprites[i].ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
				sprites[i].ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
				if (ddver > 3) sprites[i].ddsd.dwSize = sizeof(DDSURFACEDESC2);
				else sprites[i].ddsd.dwSize = sizeof(DDSURFACEDESC);
				ddinterface->CreateSurface(&sprites[i].ddsd, &sprites[i].surface, NULL);
				error = sprites[i].surface->Lock(NULL, &sprites[i].ddsd, DDLOCK_WAIT, NULL);
				DrawPalette(sprites[i].ddsd, (unsigned char *)sprites[i].ddsd.lpSurface);
				sprites[i].surface->Unlock(NULL);
				break;
			case 1:
				break;
			case 2:
				break;
			case 3:
				break;
			default:
				break;
			}
			if (i < 5) sprites[i].bltflags = 0;
			else if (i < 9)
			{
				sprites[i].bltflags = DDBLTFAST_SRCCOLORKEY;
				if (sprites[i].surface) sprites[i].surface->SetColorKey(DDCKEY_SRCBLT, &ckey);
			}
			else if (i < 13) sprites[i].bltflags = DDBLTFAST_DESTCOLORKEY;
			else
			{
				sprites[i].bltflags = DDBLTFAST_SRCCOLORKEY | DDBLTFAST_DESTCOLORKEY;
				if (sprites[i].surface) sprites[i].surface->SetColorKey(DDCKEY_SRCBLT, &ckey);
			}
			sprites[i].x = randfloat((float)ddsd.dwWidth);
			sprites[i].y = randfloat((float)ddsd.dwHeight);
			sprites[i].xvelocity = randfloat(5);
			sprites[i].yvelocity = randfloat(5);
		}
		break;
	case 6:
		ddsrender->GetSurfaceDesc(&ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = GetSystemMetrics(SM_CXCURSOR);
		ddsd.dwHeight = GetSystemMetrics(SM_CYCURSOR);
		sprites[0].width = (float)ddsd.dwWidth;
		sprites[0].height = (float)ddsd.dwHeight;
		sprites[0].rect.left = sprites[0].rect.top = 0;
		sprites[0].rect.right = ddsd.dwWidth;
		sprites[0].rect.bottom = ddsd.dwHeight;
		memcpy(&sprites[0].ddsd, &ddsd, sizeof(DDSURFACEDESC2));
		for (i = 1; i < 7; i++)
		{
			memcpy(&sprites[i], &sprites[0], sizeof(DDSPRITE));
			memcpy(&sprites[0].ddsd, &ddsd, sizeof(DDSURFACEDESC2));
		}
		colorkey.dwColorSpaceHighValue = colorkey.dwColorSpaceLowValue = 0;
		for (i = 0; i < 7; i++)
		{
			ddinterface->CreateSurface(&sprites[i].ddsd, &sprites[i].surface, NULL);
			sprites[i].surface->SetColorKey(DDCKEY_SRCBLT, &colorkey);
		}
		cursor = LoadCursor(NULL, IDC_ARROW);
		memorydc = CreateCompatibleDC(NULL);
		ZeroMemory(&bitmapinfo, sizeof(BITMAPINFO));
		bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapinfo.bmiHeader.biWidth = 8*GetSystemMetrics(SM_CXCURSOR);
		bitmapinfo.bmiHeader.biHeight = 0 - GetSystemMetrics(SM_CYCURSOR);
		bitmapinfo.bmiHeader.biPlanes = 1;
		bitmapinfo.bmiHeader.biBitCount = 32;
		bitmapinfo.bmiHeader.biCompression = BI_RGB;
		bitmap = CreateDIBSection(memorydc, &bitmapinfo, DIB_RGB_COLORS, &bmppointer, NULL, 0);
		temp = SelectObject(memorydc, bitmap);
		DeleteObject(temp);
		for (i = 0; i < 7; i++)
		{
			DrawIcon(memorydc, i*sprites[0].ddsd.dwWidth, 0, cursor);
		}
		for (i = 0; i < 7; i++)
		{
			brush = CreateSolidBrush(colormasks[i]);
			r.left = 7 * sprites[0].ddsd.dwWidth;
			r.right = 8 * sprites[0].ddsd.dwHeight;
			r.top = 0;
			r.bottom = sprites[0].ddsd.dwHeight;
			FillRect(memorydc, &r, brush);
			DeleteObject(brush);
			BitBlt(memorydc, i*sprites[0].ddsd.dwWidth, 0,
				sprites[0].ddsd.dwWidth, sprites[0].ddsd.dwHeight, memorydc,
				7 * sprites[0].ddsd.dwWidth, 0, SRCAND);
		}
		for (i = 0; i < 7; i++)
		{
			sprites[i].surface->GetDC(&hRenderDC);
			BitBlt(hRenderDC, 0, 0, sprites[0].ddsd.dwWidth, sprites[0].ddsd.dwHeight,
				memorydc, i*sprites[0].ddsd.dwWidth, 0, SRCCOPY);
			sprites[i].surface->ReleaseDC(hRenderDC);
		}
		DeleteDC(memorydc);
		DeleteObject(bitmap);
		break;
	case 7:
		if (!fullscreen) backbuffers = 0;
		ddsrender->GetSurfaceDesc(&ddsd);
		ddsrender->GetPalette(&palette);
		sprites[0].ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		sprites[0].ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		if (ddver > 3) sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC2);
		else sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC);
		memcpy(&sprites[1], &sprites[0], sizeof(DDSPRITE));
		memcpy(&sprites[2], &sprites[0], sizeof(DDSPRITE));
		memcpy(&sprites[3], &sprites[0], sizeof(DDSPRITE));
		memcpy(&sprites[4], &sprites[0], sizeof(DDSPRITE));
		memcpy(&sprites[5], &sprites[0], sizeof(DDSPRITE));
		sprites[0].width = sprites[0].height = 256.0f;
		sprites[1].width = sprites[1].height = 256.0f;
		sprites[2].width = sprites[2].height = 16.0f;
		sprites[3].width = sprites[3].height = 16.0f;
		sprites[4].width = sprites[4].height = 8.0f;
		sprites[5].width = sprites[4].height = 6.0f;
		sprites[0].ddsd.dwWidth = sprites[0].ddsd.dwHeight =
			sprites[0].rect.right = sprites[0].rect.bottom = 256;
		sprites[1].ddsd.dwWidth = sprites[1].ddsd.dwHeight =
			sprites[1].rect.right = sprites[1].rect.bottom = 256;
		sprites[2].ddsd.dwWidth = sprites[2].ddsd.dwHeight =
			sprites[2].rect.right = sprites[2].rect.bottom = 16;
		sprites[3].ddsd.dwWidth = sprites[3].ddsd.dwHeight =
			sprites[3].rect.right = sprites[3].rect.bottom = 16;
		sprites[4].ddsd.dwWidth = sprites[4].ddsd.dwHeight =
			sprites[4].rect.right = sprites[4].rect.bottom = 8;
		sprites[5].ddsd.dwWidth = sprites[5].ddsd.dwHeight =
			sprites[5].rect.right = sprites[5].rect.bottom = 6;
		ddinterface->CreateSurface(&sprites[0].ddsd, &sprites[0].surface, NULL);
		ddinterface->CreateSurface(&sprites[1].ddsd, &sprites[1].surface, NULL);
		ddinterface->CreateSurface(&sprites[2].ddsd, &sprites[2].surface, NULL);
		ddinterface->CreateSurface(&sprites[3].ddsd, &sprites[3].surface, NULL);
		ddinterface->CreateSurface(&sprites[4].ddsd, &sprites[4].surface, NULL);
		ddinterface->CreateSurface(&sprites[5].ddsd, &sprites[5].surface, NULL);
		DDCAPS ddcaps;
		ddcaps.dwSize = sizeof(DDCAPS);
		ddinterface->GetCaps(&ddcaps, NULL);
		DrawROPPatterns(ddsrender, sprites, backbuffers, ddver, bpp, ddcaps.dwRops,hWnd,palette);
		if (palette) palette->Release();
		break;
	case 8:
		ddsrender->GetSurfaceDesc(&ddsd);
		sprites[0].ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		sprites[0].ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		if (ddver > 3) sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC2);
		else sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC);
		sprites[0].ddsd.dwWidth = sprites[0].ddsd.dwHeight =
			sprites[0].rect.right = sprites[0].rect.bottom = 64;
		ddinterface->CreateSurface(&sprites[0].ddsd, &sprites[0].surface, NULL);
		DrawRotatedBlt(ddsrender, sprites);
		break;
	case 9:
		ddsrender->GetSurfaceDesc(&ddsd);
		sprites[0].ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		sprites[0].ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		if (ddver > 3) sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC2);
		else sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC);
		sprites[0].ddsd.dwWidth = sprites[0].ddsd.dwHeight =
			sprites[0].rect.right = sprites[0].rect.bottom = 255;
		ddinterface->CreateSurface(&sprites[0].ddsd, &sprites[0].surface, NULL);
		counter = 0;
		break;
	case 10:
	case 11:
		ddsrender->GetSurfaceDesc(&ddsd);
		sprites[0].ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		sprites[0].ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		if (ddver > 3) sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC2);
		else sprites[0].ddsd.dwSize = sizeof(DDSURFACEDESC);
		switch (bpp)
		{
		case 8:
			sprites[0].width = sprites[0].height = 16;
			break;
		case 15:
			sprites[0].width = 32;
			sprites[0].height = 7;
			break;
		case 16:
			sprites[0].width = 64;
			sprites[0].height = 7;
			break;
		case 24:
		case 32:
		default:
			sprites[0].width = 256;
			sprites[0].height = 7;
			break;
		}
		sprites[0].ddsd.dwWidth = sprites[0].width;
		sprites[0].ddsd.dwHeight = sprites[0].height;
		memcpy(&sprites[1], &sprites[0], sizeof(DDSPRITE));
		memcpy(&sprites[2], &sprites[0], sizeof(DDSPRITE));
		ddinterface->CreateSurface(&sprites[0].ddsd, &sprites[0].surface, NULL);
		ddinterface->CreateSurface(&sprites[1].ddsd, &sprites[1].surface, NULL);
		ddinterface->CreateSurface(&sprites[2].ddsd, &sprites[2].surface, NULL);
		sprites[1].surface->Lock(NULL, &sprites[1].ddsd, DDLOCK_WAIT, NULL);
		DrawColorKeyCompPatterns(sprites[1].ddsd, (unsigned char*)sprites[1].ddsd.lpSurface, bpp, 0);
		sprites[1].surface->Unlock(NULL);
		sprites[2].surface->Lock(NULL, &sprites[2].ddsd, DDLOCK_WAIT, NULL);
		DrawColorKeyCompPatterns(sprites[2].ddsd, (unsigned char*)sprites[2].ddsd.lpSurface, bpp, 1);
		sprites[2].surface->Unlock(NULL);
		counter = 0;
		break;
	case 12:
		MakeCube3D(5, 2);
		ZeroMemory(&material, sizeof(D3DMATERIAL7));
		material.ambient.r = 0.5f;
		material.ambient.g = 0.5f;
		material.ambient.b = 0.0f;
		material.diffuse.r = 1.0f;
		material.diffuse.g = 1.0f;
		material.diffuse.b = 1.0f;
		error = d3d7dev->SetMaterial(&material);
		error = d3d7dev->LightEnable(0, TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENT, 0x7f7f7f7f);
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
		matWorld = mat;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD, &matWorld);
		matView = mat;
		matView._43 = 10.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_VIEW, &matView);
		matProj = mat;
		matProj._11 = 2.0f;
		matProj._22 = 2.0f;
		matProj._34 = 1.0f;
		matProj._43 = -1.0f;
		matProj._44 = 0.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matProj);
		ZeroMemory(&lights[0], sizeof(D3DLIGHT7));
		lights[0].dltType = D3DLIGHT_DIRECTIONAL;
		lights[0].dcvDiffuse.r = 1;
		lights[0].dcvDiffuse.g = 0;
		lights[0].dcvDiffuse.b = 1;
		lights[0].dvDirection = D3DVECTOR(5, 5, 5);
		lights[0].dvRange = D3DLIGHT_RANGE_MAX;
		lights[0].dvAttenuation1 = 0.4f;
		error = d3d7dev->SetLight(0, &lights[0]);
		break;
	case 13:
		MakeCube3D(5, 2);
		cleartexformats();
		d3d7dev->EnumTextureFormats(gettexformat, NULL);
		gentexture(fmt_rgba4444, &textures[0], 256, 256, 0);
		gentexture(fmt_rgba1555, &textures[1], 256, 256, 0);
		gentexture(fmt_rgb565, &textures[2], 256, 256, 0);
		gentexture(fmt_rgba8888, &textures[3], 256, 256, 0);
		ZeroMemory(&material, sizeof(D3DMATERIAL7));
		material.ambient.r = 1.0f;
		material.ambient.g = 1.0f;
		material.ambient.b = 1.0f;
		material.diffuse.r = 1.0f;
		material.diffuse.g = 1.0f;
		material.diffuse.b = 1.0f;
		error = d3d7dev->SetMaterial(&material);
		error = d3d7dev->LightEnable(0, TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xFFFFFFFF);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
		matWorld = mat;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD, &matWorld);
		matView = mat;
		matView._43 = 10.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_VIEW, &matView);
		matProj = mat;
		matProj._11 = 2.0f;
		matProj._22 = 2.0f;
		matProj._34 = 1.0f;
		matProj._43 = -1.0f;
		matProj._44 = 0.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matProj);
		ZeroMemory(&lights[0], sizeof(D3DLIGHT7));
		lights[0].dltType = D3DLIGHT_DIRECTIONAL;
		lights[0].dcvDiffuse.r = 1;
		lights[0].dcvDiffuse.g = 0;
		lights[0].dcvDiffuse.b = 1;
		lights[0].dvDirection = D3DVECTOR(5, 5, 5);
		lights[0].dvRange = D3DLIGHT_RANGE_MAX;
		lights[0].dvAttenuation1 = 0.4f;
		error = d3d7dev->SetLight(0, &lights[0]);
		break;
	case 15:
		ZeroMemory(lights, 8 * sizeof(D3DLIGHT7));
		MakeLights();
	case 14:
		MakeCube3D(5, 8);
		ZeroMemory(&material, sizeof(D3DMATERIAL7));
		material.ambient.r = 1.0f;
		material.ambient.g = 1.0f;
		material.ambient.b = 1.0f;
		material.ambient.a = 1.0f;
		material.diffuse.r = 1.0f;
		material.diffuse.g = 1.0f;
		material.diffuse.b = 1.0f;
		material.diffuse.a = 1.0f;
		error = d3d7dev->SetMaterial(&material);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xffffffff);
		mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
		mat._12 = mat._13 = mat._14 = mat._41 = 0.0f;
		mat._21 = mat._23 = mat._24 = mat._42 = 0.0f;
		mat._31 = mat._32 = mat._34 = mat._43 = 0.0f;
		matWorld = mat;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_WORLD, &matWorld);
		matView = mat;
		matView._43 = 10.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_VIEW, &matView);
		matProj = mat;
		matProj._11 = 2.0f;
		matProj._22 = 2.0f;
		matProj._34 = 1.0f;
		matProj._43 = -1.0f;
		matProj._44 = 0.0f;
		error = d3d7dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matProj);
		texshaderstate = defaulttexshaderstate;
		break;
	default:
		break;
	}
}

void RunTestTimed(int test)
{
	if(stoptimer) return;
	DDSURFACEDESC2 ddsd;
	DDSCAPS2 ddscaps;
	DDBLTFX bltfx;
	HDC hDCdest, hDCsrc;
	RECT r1, r2;
	int x, y;
	POINT p;
	RECT srcrect, destrect;
	HRESULT error;
	D3DMATRIX mat;
	TCHAR message[256];
	TCHAR number[32];
	HBRUSH brush;
	BOOL out[7];
	bltfx.dwSize = sizeof(DDBLTFX);
	ZeroMemory(&ddscaps,sizeof(DDSCAPS2));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	MultiDirectDrawSurface *temp1 = NULL;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	if (d3dver >= 3) ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	float time;
	switch(test)
	{
	case 0: // Palette and gradients
	case 2: // GDI patterns
	case 7: // ROP patterns
	default:
		if(fullscreen)	ddsurface->Flip(NULL,DDFLIP_WAIT);
		break;
	case 4: // BltFast sprites
		if (backbuffers) ddsrender->GetAttachedSurface(&ddscaps, &temp1);
		else temp1 = ddsrender;
		bltfx.dwFillColor = 0;
		temp1->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &bltfx);
		if (backbuffers) temp1->Release();
		for(int i = 0; i < 16; i++)
		{
			sprites[i].x += sprites[i].xvelocity;
			if(sprites[i].xvelocity < 0 && sprites[i].x < 0) sprites[i].xvelocity = -sprites[i].xvelocity;
			if(sprites[i].xvelocity > 0 && (sprites[i].x + sprites[i].width) > width)
				sprites[i].xvelocity = -sprites[i].xvelocity;
			sprites[i].y += sprites[i].yvelocity;
			if(sprites[i].yvelocity < 0 && sprites[i].y < 0) sprites[i].yvelocity = -sprites[i].yvelocity;
			if(sprites[i].yvelocity > 0 && (sprites[i].y + sprites[i].height) > height)
				sprites[i].yvelocity = -sprites[i].yvelocity;
			if(sprites[i].surface)
			{
				if(backbuffers)	ddsrender->GetAttachedSurface(&ddscaps,&temp1);
				else temp1 = ddsrender;
				temp1->BltFast((DWORD)sprites[i].x,(DWORD)sprites[i].y,sprites[i].surface,&sprites[i].rect,sprites[i].bltflags);
				if(backbuffers) temp1->Release();
			}
		}
		if (fullscreen)
		{
			if (backbuffers && ddsrender) ddsrender->Flip(NULL, DDFLIP_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &r1);
			OffsetRect(&r1, p.x, p.y);
			SetRect(&r2, 0, 0, width, height);
			if (ddsurface && ddsrender) ddsurface->Blt(&r1, ddsrender, &r2, DDBLT_WAIT, NULL);
		}
		break;
	case 6: // Mouse pointer test
		if (backbuffers) ddsrender->GetAttachedSurface(&ddscaps, &temp1);
		else temp1 = ddsrender;
		temp1->GetDC(&hDCdest);
		brush = CreateSolidBrush(0);
		destrect.left = 0;
		destrect.right = width;
		destrect.top = 0;
		destrect.bottom = height;
		FillRect(hDCdest, &destrect, brush);
		DeleteObject(brush);
		_tcscpy(message, _T("Mouse pointer test"));
		displayfonts[0] = CreateFont(9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH, _T("Fixedsys"));
		displayfonts[1] = (HFONT)SelectObject(hDCdest, displayfonts[0]);
		SetTextColor(hDCdest, RGB(255,255,255));
		SetBkColor(hDCdest, RGB(0,0,255));
		TextOut(hDCdest, 0, 0, message, _tcslen(message));
		GetTextExtentPoint(hDCdest, _T("A"), 1, &textsize);
		_tcscpy(message, _T("Last mouse message: "));
		switch (lastmousemsg)
		{
		case WM_MOUSEMOVE:
			_tcscat(message, _T("WM_MOUSEMOVE "));
			break;
		case WM_LBUTTONDOWN:
			_tcscat(message, _T("WM_LBUTTONDOWN "));
			break;
		case WM_LBUTTONUP:
			_tcscat(message, _T("WM_LBUTTONUP "));
			break;
		case WM_LBUTTONDBLCLK:
			_tcscat(message, _T("WM_LBUTTONDBLCLK "));
			break;
		case WM_RBUTTONDOWN:
			_tcscat(message, _T("WM_RBUTTONDOWN "));
			break;
		case WM_RBUTTONUP:
			_tcscat(message, _T("WM_RBUTTONUP "));
			break;
		case WM_RBUTTONDBLCLK:
			_tcscat(message, _T("WM_RBUTTONDBLCLK "));
			break;
		case WM_MBUTTONDOWN:
			_tcscat(message, _T("WM_MBUTTONDOWN "));
			break;
		case WM_MBUTTONUP:
			_tcscat(message, _T("WM_MBUTTONUP "));
			break;
		case WM_MBUTTONDBLCLK:
			_tcscat(message, _T("WM_MBUTTONDBLCLK "));
			break;
		case WM_MOUSEWHEEL:
			_tcscat(message, _T("WM_MOUSEWHEEL "));
			break;
		case WM_XBUTTONDOWN:
			_tcscat(message, _T("WM_XBUTTONDOWN "));
			break;
		case WM_XBUTTONUP:
			_tcscat(message, _T("WM_XBUTTONUP "));
			break;
		case WM_XBUTTONDBLCLK:
			_tcscat(message, _T("WM_XBUTTONDBLCLK "));
			break;
		case WM_MOUSEHWHEEL:
			_tcscat(message, _T("WM_MOUSEHWHEEL "));
			break;
		default:
			_tcscat(message, _T("unknown "));
		}
		x = GET_X_LPARAM(lastmouselparam);
		y = GET_Y_LPARAM(lastmouselparam);
		_tcscat(message, _T("X="));
		_itot(x, number, 10);
		_tcscat(message, number);
		_tcscat(message, _T(" "));
		_tcscat(message, _T("Y="));
		_itot(y, number, 10);
		_tcscat(message, number);
		_tcscat(message, _T(" "));
		_tcscat(message, _T("Keys: "));
		if (lastmousewparam & MK_CONTROL) _tcscat(message, _T("CTRL "));
		if (lastmousewparam & MK_SHIFT) _tcscat(message, _T("SHIFT "));
		_tcscat(message, _T("Buttons: "));
		if (lastmousewparam & MK_LBUTTON) _tcscat(message, _T("L "));
		if (lastmousewparam & MK_MBUTTON) _tcscat(message, _T("M "));
		if (lastmousewparam & MK_RBUTTON) _tcscat(message, _T("R "));
		if (lastmousewparam & MK_XBUTTON1) _tcscat(message, _T("X1 "));
		if (lastmousewparam & MK_XBUTTON2) _tcscat(message, _T("X2 "));
		if ((x > width) || (y > height) || (x < 0) || (y < 0))
		{
			out[0] = TRUE;
			_tcscat(message, _T(" OUT OF BOUNDS"));
		}
		else out[0] = FALSE;
		if (out[0]) SetTextColor(hDCdest, RGB(255, 0, 0));
		else SetTextColor(hDCdest, RGB(255, 255, 255));
		TextOut(hDCdest, 0, textsize.cy, message, _tcslen(message));
		GetCursorPos(&p);
		_tcscpy(message, _T("GetCursorPos() position: "));
		_tcscat(message, _T("X="));
		_itot(p.x, number, 10);
		_tcscat(message, number);
		_tcscat(message, _T(" "));
		_tcscat(message, _T("Y="));
		_itot(p.y, number, 10);
		_tcscat(message, number);
		if ((p.x > width) || (p.y > height) || (p.x < 0) || (p.y < 0))
		{
			out[1] = TRUE;
			_tcscat(message, _T(" OUT OF BOUNDS"));
		}
		else out[1] = FALSE;
		if (out[1]) SetTextColor(hDCdest, RGB(255, 0, 0));
		else SetTextColor(hDCdest, RGB(255, 255, 255));
		SetBkColor(hDCdest, RGB(0, 255, 0));
		TextOut(hDCdest, 0, 2 * textsize.cy, message, _tcslen(message));
		SelectObject(hDCdest, displayfonts[1]);
		DeleteObject(displayfonts[0]);
		temp1->ReleaseDC(hDCdest);
		// Draw cursors
		if (!out[0])
		{
			destrect.left = GET_X_LPARAM(lastmouselparam);
			destrect.top = GET_Y_LPARAM(lastmouselparam);
			destrect.right = GET_X_LPARAM(lastmouselparam) + sprites[0].ddsd.dwWidth;
			if (destrect.right > width) destrect.right = width;
			destrect.bottom = GET_Y_LPARAM(lastmouselparam) + sprites[0].ddsd.dwHeight;
			if (destrect.bottom > height) destrect.bottom = height;
			srcrect.left = srcrect.top = 0;
			srcrect.right = destrect.right - destrect.left;
			srcrect.bottom = destrect.bottom - destrect.top;
			temp1->Blt(&destrect, sprites[0].surface, &srcrect, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
		}
		if (!out[1])
		{
			destrect.left = p.x;
			destrect.top = p.y;
			destrect.right = p.x + sprites[1].ddsd.dwWidth;
			if (destrect.right > width) destrect.right = width;
			destrect.bottom = p.y + sprites[1].ddsd.dwHeight;
			if (destrect.bottom > height) destrect.bottom = height;
			srcrect.left = srcrect.top = 0;
			srcrect.right = destrect.right - destrect.left;
			srcrect.bottom = destrect.bottom - destrect.top;
			temp1->Blt(&destrect, sprites[1].surface, &srcrect, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
		}
		if (fullscreen)
		{
			if (backbuffers && ddsrender) ddsrender->Flip(NULL, DDFLIP_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &r1);
			OffsetRect(&r1, p.x, p.y);
			SetRect(&r2, 0, 0, width, height);
			if (ddsurface && ddsrender) ddsurface->Blt(&r1, ddsrender, &r2, DDBLT_WAIT, NULL);
		}
		break;
	case 10: // Source Key Override test
	case 11: // Destination Key Override Test
		if (backbuffers) ddsrender->GetAttachedSurface(&ddscaps, &temp1);
		else temp1 = ddsrender;

		sprites[0].surface->Blt(NULL, sprites[1].surface, NULL, DDBLT_WAIT, NULL);
		bltfx.dwSize = sizeof(DDBLTFX);
		switch (bpp)
		{
		case 8:
			bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
				bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue = counter;
			counter++;
			if (counter > 255) counter = 0;
			r1.left = r1.top = 0;
			r1.right = r1.bottom = 16;
			break;
		case 15:
			switch (counter >> 5)
			{
			case 0:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 31) << 10;
				break;
			case 1:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 31) << 5;
				break;
			case 2:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 31);
				break;
			case 3:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 31) + ((counter & 31) << 5);
				break;
			case 4:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 31) + ((counter & 31) << 10);
				break;
			case 5:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					((counter & 31) << 5) + ((counter & 31) << 10);
				break;
			case 6:
			default:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 31) + ((counter & 31) << 5) + ((counter & 31) << 10);
				break;
			}
			counter++;
			if (counter > 223) counter = 0;
			r1.left = r1.top = 0;
			r1.right = 32;
			r1.bottom = 7;
			break;
		case 16:
			switch (counter >> 6)
			{
			case 0:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					((counter & 63) >> 1) << 11;
				break;
			case 1:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 63) << 5;
				break;
			case 2:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 63) >> 1;
				break;
			case 3:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					((counter & 63) >> 1) + ((counter & 63) << 5);
				break;
			case 4:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					((counter & 63) >> 1) + (((counter & 63) >> 1) << 11);
				break;
			case 5:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					((counter & 63) << 5) + (((counter & 63) >> 1) << 11);
				break;
			case 6:
			default:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					((counter & 63) >> 1) + ((counter & 63) << 5) + (((counter & 63) >> 1) << 11);
				break;
			}
			counter++;
			if (counter > 447) counter = 0;
			r1.left = r1.top = 0;
			r1.right = 64;
			r1.bottom = 7;
			break;
		case 24:
		case 32:
		default:
			switch (counter >> 8)
			{
			case 0:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 255) << 16;
				break;
			case 1:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 255) << 8;
				break;
			case 2:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					counter & 255;
				break;
			case 3:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 255) + ((counter & 255) << 8);
				break;
			case 4:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 255) + ((counter & 255) << 16);
				break;
			case 5:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					((counter & 255) << 8) + ((counter & 255) << 16);
				break;
			case 6:
			default:
				bltfx.ddckSrcColorkey.dwColorSpaceHighValue = bltfx.ddckSrcColorkey.dwColorSpaceLowValue =
					bltfx.ddckDestColorkey.dwColorSpaceHighValue = bltfx.ddckDestColorkey.dwColorSpaceLowValue =
					(counter & 255) + ((counter & 255) << 8) + ((counter & 255) << 16);
				break;
			}
			counter++;
			if (counter > 1791) counter = 0;
			r1.left = r1.top = 0;
			r1.right = 256;
			r1.bottom = 7;
			break;
		}
		if (test == 10)
			sprites[0].surface->Blt(NULL, sprites[2].surface, NULL, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &bltfx);
		else
			sprites[0].surface->Blt(NULL, sprites[2].surface, NULL, DDBLT_WAIT | DDBLT_KEYDESTOVERRIDE, &bltfx);
		temp1->GetDC(&hDCdest);
		sprites[0].surface->GetDC(&hDCsrc);
		if (ddver > 3) ddsd.dwSize = sizeof(DDSURFACEDESC2);
		else ddsd.dwSize = sizeof(DDSURFACEDESC);
		temp1->GetSurfaceDesc(&ddsd);
		StretchBlt(hDCdest, ((ddsd.dwWidth / 2) - 128), ((ddsd.dwHeight / 2) + 128), 256, -256,
			hDCsrc, 0, 0, r1.right, r1.bottom, SRCCOPY);
		sprites[0].surface->ReleaseDC(hDCsrc);
		SetBkColor(hDCdest, RGB(0, 0, 255));
		SetTextColor(hDCdest, RGB(255, 255, 255));
		if(test == 10)
			_tcscpy(message, _T("Source Color Key Override Test"));
		else
			_tcscpy(message, _T("Destination Color Key Override Test"));
		TextOut(hDCdest, 0, 0, message, _tcslen(message));
		_stprintf(message, _T("Color:  0x%08X    "), bltfx.ddckSrcColorkey.dwColorSpaceHighValue);
		TextOut(hDCdest, 0, 16, message, _tcslen(message));
		temp1->ReleaseDC(hDCdest);
		if (backbuffers) temp1->Release();
		if (fullscreen)
		{
			if (backbuffers && ddsrender) ddsrender->Flip(NULL, DDFLIP_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &r1);
			OffsetRect(&r1, p.x, p.y);
			SetRect(&r2, 0, 0, width, height);
			if (ddsurface && ddsrender) ddsurface->Blt(&r1, ddsrender, &r2, DDBLT_WAIT, NULL);
		}
		break;
	case 12:
		error = d3d7dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bgcolor, 1.0, 0);
		time = (float)clock() / (float)CLOCKS_PER_SEC;
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
		error = d3d7dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_VERTEX,vertices,numpoints,mesh,numindices,0);
		error = d3d7dev->EndScene();
		if (fullscreen)
		{
			if (backbuffers) ddsurface->Flip(NULL, DDFLIP_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &destrect);
			OffsetRect(&destrect, p.x, p.y);
			SetRect(&srcrect, 0, 0, width, height);
			if (ddsurface && ddsrender)error = ddsurface->Blt(&destrect, ddsrender, &srcrect, DDBLT_WAIT, NULL);
		}
		break;
	case 13:
		error = d3d7dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bgcolor, 1.0, 0);
		time = (float)clock() / (float)CLOCKS_PER_SEC;
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
		if (fullscreen)
		{
			if (backbuffers) ddsurface->Flip(NULL, DDFLIP_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &destrect);
			OffsetRect(&destrect, p.x, p.y);
			SetRect(&srcrect, 0, 0, width, height);
			if (ddsurface && ddsrender)error = ddsurface->Blt(&destrect, ddsrender, &srcrect, DDBLT_WAIT, NULL);
		}
		break;
	case 14:
		error = d3d7dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bgcolor, 1.0, 0);
		time = (float)clock() / (float)CLOCKS_PER_SEC;
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
		error = d3d7dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_LVERTEX,litvertices,numpoints,mesh,numindices,0);
		error = d3d7dev->EndScene();
		if (fullscreen)
		{
			if (backbuffers) ddsurface->Flip(NULL, DDFLIP_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &destrect);
			OffsetRect(&destrect, p.x, p.y);
			SetRect(&srcrect, 0, 0, width, height);
			if (ddsurface && ddsrender)error = ddsurface->Blt(&destrect, ddsrender, &srcrect, DDBLT_WAIT, NULL);
		}
		break;
	case 15:
		error = d3d7dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bgcolor, 1.0, 0);
		time = (float)clock() / (float)CLOCKS_PER_SEC;
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
		error = d3d7dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,FVF_COLORVERTEX,colorvertices,numpoints,mesh,numindices,0);
		error = d3d7dev->EndScene();
		if (fullscreen)
		{
			if (backbuffers) ddsurface->Flip(NULL, DDFLIP_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &destrect);
			OffsetRect(&destrect, p.x, p.y);
			SetRect(&srcrect, 0, 0, width, height);
			if (ddsurface && ddsrender)error = ddsurface->Blt(&destrect, ddsrender, &srcrect, DDBLT_WAIT, NULL);
		}
		break;
	}
}

void RunTestLooped(int test)
{
	randnum += rand(); // Improves randomness of "snow" patterns at certain resolutions
	HDC hdc;
	unsigned int i;
	POINT p;
	HPEN pen;
	HBRUSH brush;
	HANDLE tmphandle,tmphandle2;
	RECT srcrect,destrect;
	HRESULT error;
	DDSURFACEDESC2 ddsd;
	DDBLTFX bltfx;
	if(ddver > 3)ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	error = ddsrender->GetSurfaceDesc(&ddsd);
	MultiDirectDrawSurface *temp1 = NULL;
	DDSCAPS2 ddscaps;
	DWORD bitmask;
	ZeroMemory(&ddscaps,sizeof(DDSCAPS2));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	int op;
	switch(test)
	{
	case 1:
	default:
		if(backbuffers)
		{
			ddsrender->GetAttachedSurface(&ddscaps,&temp1);
			temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
		}
		else ddsrender->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
		
		for(i = 0; i < ((ddsd.lPitch * ddsd.dwHeight)/4); i++)
			((DWORD*)ddsd.lpSurface)[i] = rand32(randnum);

		if(backbuffers)
		{
			temp1->Unlock(NULL);
			ddsrender->Flip(NULL,DDFLIP_WAIT);
		}
		else ddsrender->Unlock(NULL);
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
		break;
	case 3:
		ddsrender->GetDC(&hdc);
		op = rand32(randnum) % 4;
		pen = CreatePen(rand32(randnum) % 5,0,RGB(rand32(randnum)%256,rand32(randnum)%256,rand32(randnum)%256));
		brush = CreateSolidBrush(RGB(rand32(randnum)%256,rand32(randnum)%256,rand32(randnum)%256));
		tmphandle = SelectObject(hdc,pen);
		tmphandle2 = SelectObject(hdc,brush);
		SetBkColor(hdc,RGB(rand32(randnum)%256,rand32(randnum)%256,rand32(randnum)%256));
		switch(op)
		{
		case 0:
		default:
			Rectangle(hdc,rand32(randnum)%ddsd.dwWidth,rand32(randnum)%ddsd.dwHeight,
				rand32(randnum)%ddsd.dwWidth,rand32(randnum)%ddsd.dwHeight);
			break;
		case 1:
			Ellipse(hdc,rand32(randnum)%ddsd.dwWidth,rand32(randnum)%ddsd.dwHeight,
				rand32(randnum)%ddsd.dwWidth,rand32(randnum)%ddsd.dwHeight);
			break;
		case 2:
			MoveToEx(hdc,rand32(randnum)%ddsd.dwWidth,rand32(randnum)%ddsd.dwHeight,NULL);
			LineTo(hdc,rand32(randnum)%ddsd.dwWidth,rand32(randnum)%ddsd.dwHeight);
			break;
		case 3:
			SetTextColor(hdc,RGB(rand32(randnum)%256,rand32(randnum)%256,rand32(randnum)%256));
			TextOut(hdc,rand32(randnum)%ddsd.dwWidth,rand32(randnum)%ddsd.dwHeight,_T("Text"),4);
			break;
		}
		SelectObject(hdc,tmphandle2);
		SelectObject(hdc,tmphandle);
		DeleteObject(brush);
		DeleteObject(pen);
		ddsrender->ReleaseDC(hdc);
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
		break;
	case 5:
		rndrect5:
		destrect.bottom = rand32(randnum)%ddsd.dwHeight;
		destrect.top = rand32(randnum)%ddsd.dwHeight;
		destrect.left = rand32(randnum)%ddsd.dwWidth;
		destrect.right = rand32(randnum)%ddsd.dwWidth;
		if((destrect.bottom < destrect.top) || (destrect.right < destrect.left)) goto rndrect5;
		bltfx.dwSize = sizeof(DDBLTFX);
		switch(bpp)
		{
		case 8:
			bltfx.dwFillColor = rand32(randnum) % 0xFF;
			break;
		case 15:
			bltfx.dwFillColor = rand32(randnum) % 0x7FFF;
			break;
		case 16:
			bltfx.dwFillColor = rand32(randnum) % 0xFFFF;
			break;
		case 24:
		case 32:
		default:
			bltfx.dwFillColor = rand32(randnum) % 0xFFFFFF;
			break;
		}
		ddsrender->Blt(&destrect,NULL,NULL,DDBLT_COLORFILL,&bltfx);
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
		break;
	case 9:
		bltfx.dwSize = sizeof(DDBLTFX);
		switch (bpp)
		{
		case 8:
			bitmask = 0xFF;
			break;
		case 15:
			bitmask = 0x7FFF;
			break;
		case 16:
			bitmask = 0xFFFF;
			break;
		case 24:
			bitmask = 0xFFFFFF;
			break;
		case 32:
		default:
			bitmask = 0xFFFFFFFF;
			break;
		}
		for (int y = 0; y < 255; y++)
		{
			for (int x = 0; x < 255; x++)
			{
				bltfx.dwFillColor = counter & bitmask;
				destrect.left = x;
				destrect.right = x + 1;
				destrect.top = y;
				destrect.bottom = y + 1;
				counter++;
				sprites[0].surface->Blt(&destrect, NULL, NULL, DDBLT_COLORFILL, &bltfx);
			}
		}
		ddsrender->Blt(NULL, sprites[0].surface, NULL, DDBLT_WAIT, NULL);
		if (!fullscreen)
		{
			p.x = 0;
			p.y = 0;
			ClientToScreen(hWnd, &p);
			GetClientRect(hWnd, &destrect);
			OffsetRect(&destrect, p.x, p.y);
			SetRect(&srcrect, 0, 0, width, height);
			if (ddsurface && ddsrender)error = ddsurface->Blt(&destrect, ddsrender, &srcrect, DDBLT_WAIT, NULL);
		}
		break;
	}
	if(temp1) temp1->Release();
}

void PopulateArgCombo(HWND hWnd)
{
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Diffuse"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Current"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Texture"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Factor"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Specular"));
}

void PopulateOpCombo(HWND hWnd, bool color)
{
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Disable"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Select Arg 1"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Select Arg 2"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Modulate"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Modulate 2x"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Modulate 4x"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Add"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Add Signed"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Add Signed 2x"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Subtract"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Add Smooth"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Blend Diffuse Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Blend Texture Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Blend Factor Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Blend Texture Alpha PM"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Blend Current Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Premodulate"));
	if (color)
	{
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Modulate Alpha Add Color"));
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Modulate Color Add Alpha"));
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Modulate Inv. Alpha Add Color"));
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Modulate Inv. Color Add Alpha"));
	}
	else
	{
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("(invalid)"));
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("(invalid)"));
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("(invalid)"));
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("(invalid)"));
	}
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Bump Env. Map"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Bump Env. Map Luminance"));
	if (color) SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Dot Product3"));
}

void PopulateBlendCombo(HWND hWnd, bool src)
{
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Zero"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("One"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Source Color"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Inv. Src. Color"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Source Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Inv. Src. Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Dest. Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Inv. Dest. Alpha"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Dest. Color"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Inv. Dest. Color"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Src. Alpha Sat."));
	if (src)
	{
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Both Src. Alpha"));
		SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Both Inv. Src. Alpha"));
	}
}

void PopulateCompareCombo(HWND hWnd)
{
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Never"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Less"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Equal"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Less or Equal"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Greater"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Not Equal"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Greater or Equal"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Always"));
}

void PopulateFogCombo(HWND hWnd)
{
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("None"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Exponential"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Exp. Squared"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Linear"));
}

void PopulateSourceCombo(HWND hWnd)
{
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Material"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Color 1"));
	SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)_T("Color 2"));
}

void strupper(TCHAR *str)
{
	TCHAR *ptr = str;
	while (*ptr != 0)
	{
		*ptr = _totupper(*ptr);
		ptr++;
	}
}

void paddwordzeroes(TCHAR *str)
{
	TCHAR str2[16];
	str2[0] = 0;
	int len = _tcslen(str);
	if (len < 8)
	{
		for (int i = 0; i < 8 - len; i++)
			_tcscat(str2, _T("0"));
		_tcscat(str2, str);
		_tcscpy(str, str2);
	}
}

HRESULT CALLBACK SelectTextureFormatCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
	if (lpDDPixFmt->dwFlags & (DDPF_LUMINANCE | DDPF_BUMPLUMINANCE | DDPF_BUMPDUDV)) return D3DENUMRET_OK;
	if (lpDDPixFmt->dwFourCC != 0) return D3DENUMRET_OK;
	if (!(lpDDPixFmt->dwFlags & DDPF_ALPHAPIXELS)) return D3DENUMRET_OK;
	if (lpDDPixFmt->dwRGBBitCount < 32) return D3DENUMRET_OK;
	memcpy(lpContext, lpDDPixFmt, sizeof(DDPIXELFORMAT));
	return D3DENUMRET_CANCEL;
}

void CreateSurfaceFromBitmap(MultiDirectDrawSurface **surface, DDSURFACEDESC2 *ddsd, HDC hdc, int width, int height)
{
	ddinterface->CreateSurface(ddsd, surface, NULL);
	HDC surfacedc;
	if (surface)
	{
		(*surface)->GetDC(&surfacedc);
		BitBlt(surfacedc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
		(*surface)->ReleaseDC(surfacedc);
	}
}

void SelectTexture(MultiDirectDrawSurface **surface, int type, DWORD colorkey, bool haskey, LPCTSTR file)
{
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	BITMAPV4HEADER bi;
	HBITMAP bitmap;
	HDC hdc;
	HDC hmemdc;
	HBITMAP holdbmp;
	HICON icon;
	VOID *bmpbits;
	ZeroMemory(&bi, sizeof(BITMAPV4HEADER));
	bi.bV4Size = sizeof(BITMAPV4HEADER);
	bi.bV4Planes = 1;
	bi.bV4BitCount = 32;
	bi.bV4V4Compression = BI_BITFIELDS;
	bi.bV4RedMask = 0x00FF0000;
	bi.bV4GreenMask = 0x0000FF00;
	bi.bV4BlueMask = 0x000000FF;
	bi.bV4AlphaMask = 0xFF000000;
	d3d7dev->EnumTextureFormats(SelectTextureFormatCallback, &ddsd.ddpfPixelFormat);
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;
	DDCOLORKEY ckey;
	ckey.dwColorSpaceHighValue = ckey.dwColorSpaceLowValue = colorkey;
	if (haskey) ddsd.dwFlags |= DDSD_CKSRCBLT;
	if (*surface)
	{
		d3d7dev->SetTexture(texshaderstate.currentstage, NULL);
		(*surface)->Release();
		*surface = NULL;
	}
	switch (type)
	{
	case 0:
	default:
		break;
	case 1:
		break;
	case 2:
		hdc = GetDC(NULL);
		bi.bV4Width = ddsd.dwWidth = bi.bV4Height = ddsd.dwHeight = 16;
		bitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bmpbits, NULL, 0);
		hmemdc = CreateCompatibleDC(hdc);
		ReleaseDC(NULL, hdc);
		holdbmp = (HBITMAP)SelectObject(hmemdc, bitmap);
		icon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DXGL), IMAGE_ICON, 16, 16, 0);
		DrawIconEx(hmemdc, 0, 0, icon, 16, 16, 0, NULL, DI_NORMAL);
		CreateSurfaceFromBitmap(surface, &ddsd, hmemdc, 16, 16);
		SelectObject(hmemdc, holdbmp);
		DeleteDC(hmemdc);
		DeleteObject(bitmap);
		if (*surface && haskey) (*surface)->SetColorKey(DDCKEY_SRCBLT, &ckey);
		if (*surface) d3d7dev->SetTexture(texshaderstate.currentstage, (LPDIRECTDRAWSURFACE7)(*surface)->GetSurface());
		break;
	case 3:
		hdc = GetDC(NULL);
		bi.bV4Width = ddsd.dwWidth = bi.bV4Height = ddsd.dwHeight = 256;
		bitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bmpbits, NULL, 0);
		hmemdc = CreateCompatibleDC(hdc);
		ReleaseDC(NULL, hdc);
		holdbmp = (HBITMAP)SelectObject(hmemdc, bitmap);
		icon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DXGL), IMAGE_ICON, 256, 256, 0);
		DrawIconEx(hmemdc, 0, 0, icon, 256, 256, 0, NULL, DI_NORMAL);
		CreateSurfaceFromBitmap(surface, &ddsd, hmemdc, 256, 256);
		SelectObject(hmemdc, holdbmp);
		DeleteDC(hmemdc);
		DeleteObject(bitmap);
		if (*surface && haskey) (*surface)->SetColorKey(DDCKEY_SRCBLT, &ckey);
		if (*surface) d3d7dev->SetTexture(texshaderstate.currentstage, (LPDIRECTDRAWSURFACE7)(*surface)->GetSurface());
		break;
	case 4:
		break;
	}
}

void SetShaderArg(HWND hWnd, UINT dropdown, UINT checkalpha, UINT checkinv, DWORD *texarg)
{
	DWORD arg = SendDlgItemMessage(hWnd, dropdown, CB_GETCURSEL, 0, 0);
	if (SendDlgItemMessage(hWnd, checkalpha, BM_GETCHECK, 0, 0) == BST_CHECKED)
		arg |= D3DTA_ALPHAREPLICATE;
	if (SendDlgItemMessage(hWnd, checkinv, BM_GETCHECK, 0, 0) == BST_CHECKED)
		arg |= D3DTA_COMPLEMENT;
	*texarg = arg;
}

INT_PTR CALLBACK TexShader7Proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HRESULT error;
	D3DVIEWPORT7 vp;
	HWND hDisplay;
	int number;
	float f;
	TCHAR tmpstring[MAX_PATH + 1];
	switch (Msg)
	{
	case WM_INITDIALOG:
		RECT r;
		DDSCAPS2 caps;
		DDSURFACEDESC2 ddsd;
		DDPIXELFORMAT ddpfz;
		testnum = 14;
		ddinterface = new MultiDirectDraw(7, &error, NULL);
		hDisplay = GetDlgItem(hWnd, IDC_DISPLAY);
		::hWnd = hDisplay;
		error = ddinterface->SetCooperativeLevel(hDisplay, DDSCL_NORMAL);
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		error = ddinterface->CreateSurface(&ddsd, &ddsurface, NULL);
		error = ddinterface->CreateClipper(0, &ddclipper, NULL);
		error = ddclipper->SetHWnd(0, hDisplay);
		error = ddsurface->SetClipper(ddclipper);
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		GetClientRect(hDisplay, &r);
		ddsd.dwWidth = r.right;
		ddsd.dwHeight = r.bottom;
		error = ddinterface->CreateSurface(&ddsd, &ddsrender, NULL);
		error = ddinterface->QueryInterface(IID_IDirect3D7, (VOID**)&d3d7);
		error = d3d7->EnumZBufferFormats(IID_IDirect3DRGBDevice, zcallback, &ddpfz);
		error = ddsrender->GetSurfaceDesc(&ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
		memcpy(&ddsd.ddpfPixelFormat, &ddpfz, sizeof(DDPIXELFORMAT));
		error = ddinterface->CreateSurface(&ddsd, &zbuffer, NULL);
		error = ddsrender->AddAttachedSurface(zbuffer);
		error = d3d7->CreateDevice(IID_IDirect3DHALDevice, (LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(), &d3d7dev);
		if (error != D3D_OK)
			error = d3d7->CreateDevice(IID_IDirect3DRGBDevice, (LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(), &d3d7dev);
		ddsrender->GetSurfaceDesc(&ddsd);
		vp.dvMaxZ = 1.0f;
		vp.dvMinZ = 0.0f;
		vp.dwX = vp.dwY = 0;
		vp.dwWidth = ddsd.dwWidth;
		vp.dwHeight = ddsd.dwHeight;
		error = d3d7dev->SetViewport(&vp);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
		InitTest(14);
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("None"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("Gradients"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("DXGL logo (small)"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("DXGL logo (large)"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("Texture file"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_SETCURSEL, 0, 0);
		SendDlgItemMessage(hWnd, IDC_TEXCOLORKEY, WM_SETTEXT, 0, (LPARAM)_T(""));
		PopulateArgCombo(GetDlgItem(hWnd, IDC_CARG1));
		PopulateArgCombo(GetDlgItem(hWnd, IDC_CARG2));
		PopulateArgCombo(GetDlgItem(hWnd, IDC_AARG1));
		PopulateArgCombo(GetDlgItem(hWnd, IDC_AARG2));
		SendDlgItemMessage(hWnd, IDC_CARG1, CB_SETCURSEL, D3DTA_TEXTURE, 0);
		SendDlgItemMessage(hWnd, IDC_CARG2, CB_SETCURSEL, D3DTA_CURRENT, 0);
		SendDlgItemMessage(hWnd, IDC_AARG1, CB_SETCURSEL, D3DTA_TEXTURE, 0);
		SendDlgItemMessage(hWnd, IDC_AARG2, CB_SETCURSEL, D3DTA_CURRENT, 0);
		PopulateOpCombo(GetDlgItem(hWnd, IDC_COLOROP), true);
		PopulateOpCombo(GetDlgItem(hWnd, IDC_ALPHAOP), false);
		SendDlgItemMessage(hWnd, IDC_COLOROP, CB_SETCURSEL, D3DTOP_MODULATE - 1, 0);
		SendDlgItemMessage(hWnd, IDC_ALPHAOP, CB_SETCURSEL, D3DTOP_SELECTARG1 - 1, 0);
		SendDlgItemMessage(hWnd, IDC_DIFFUSE, WM_SETTEXT, 0, (LPARAM)_T("FFFFFFFF"));
		SendDlgItemMessage(hWnd, IDC_SPECULAR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_FACTOR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_FOGCOLOR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_BGCOLOR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		PopulateBlendCombo(GetDlgItem(hWnd, IDC_SRCBLEND), true);
		PopulateBlendCombo(GetDlgItem(hWnd, IDC_DESTBLEND), true);
		SendDlgItemMessage(hWnd, IDC_SRCBLEND, CB_SETCURSEL, D3DBLEND_ONE - 1, 0);
		SendDlgItemMessage(hWnd, IDC_DESTBLEND, CB_SETCURSEL, D3DBLEND_ZERO - 1, 0);
		PopulateCompareCombo(GetDlgItem(hWnd, IDC_ALPHAFUNC));
		SendDlgItemMessage(hWnd, IDC_ALPHAFUNC, CB_SETCURSEL, D3DCMP_ALWAYS - 1, 0);
		PopulateFogCombo(GetDlgItem(hWnd, IDC_VERTEXFOGMODE));
		PopulateFogCombo(GetDlgItem(hWnd, IDC_PIXELFOGMODE));
		SendDlgItemMessage(hWnd, IDC_VERTEXFOGMODE, CB_SETCURSEL, D3DFOG_NONE, 0);
		SendDlgItemMessage(hWnd, IDC_PIXELFOGMODE, CB_SETCURSEL, D3DFOG_NONE, 0);
		SendDlgItemMessage(hWnd, IDC_FOGSTART, WM_SETTEXT, 0, (LPARAM)_T("0.0"));
		SendDlgItemMessage(hWnd, IDC_FOGEND, WM_SETTEXT, 0, (LPARAM)_T("1.0"));
		SendDlgItemMessage(hWnd, IDC_FOGDENSITY, WM_SETTEXT, 0, (LPARAM)_T("1.0"));
		SendDlgItemMessage(hWnd, IDC_SPINSTAGE, UDM_SETRANGE32, 0, 7);
		SendDlgItemMessage(hWnd, IDC_SPINALPHAREF, UDM_SETRANGE32, 0, 255);
		::width = ddsd.dwWidth;
		::height = ddsd.dwHeight;
		StartTimer(hWnd, WM_APP, 60);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_TEXSTAGE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_TEXSTAGE, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				number = _ttoi(tmpstring);
				if (number < 0) SendDlgItemMessage(hWnd, IDC_TEXSTAGE, WM_SETTEXT, 0, (LPARAM)_T("0"));
				if (number > 7) SendDlgItemMessage(hWnd, IDC_TEXSTAGE, WM_SETTEXT, 0, (LPARAM)_T("7"));
				if (number < 0) number = 0;
				if (number > 7) number = 7;
				texshaderstate.currentstage = number;
				_itot(texshaderstate.texstages[number].keycolor, tmpstring, 16);
				strupper(tmpstring);
				paddwordzeroes(tmpstring);
				if (texshaderstate.texstages[number].colorkey == FALSE) tmpstring[0] = 0;
				SendDlgItemMessage(hWnd, IDC_TEXCOLORKEY, WM_SETTEXT, 0, (LPARAM)tmpstring);
				SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_SETCURSEL, texshaderstate.texstages[number].texturetype, 0);
				SendDlgItemMessage(hWnd, IDC_TEXTUREFILE, WM_SETTEXT, 0, (LPARAM)texshaderstate.texstages[number].texturefile);
				SendDlgItemMessage(hWnd, IDC_CARG1, CB_SETCURSEL, texshaderstate.texstages[number].colorarg1 & D3DTA_SELECTMASK, 0);
				SendDlgItemMessage(hWnd, IDC_CARG2, CB_SETCURSEL, texshaderstate.texstages[number].colorarg2 & D3DTA_SELECTMASK, 0);
				SendDlgItemMessage(hWnd, IDC_AARG1, CB_SETCURSEL, texshaderstate.texstages[number].alphaarg1 & D3DTA_SELECTMASK, 0);
				SendDlgItemMessage(hWnd, IDC_AARG2, CB_SETCURSEL, texshaderstate.texstages[number].alphaarg2 & D3DTA_SELECTMASK, 0);
				if (texshaderstate.texstages[number].colorarg1 & D3DTA_ALPHAREPLICATE)
					SendDlgItemMessage(hWnd, IDC_CARG1A, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_CARG1A, BM_SETCHECK, BST_UNCHECKED, 0);
				if (texshaderstate.texstages[number].colorarg2 & D3DTA_ALPHAREPLICATE)
					SendDlgItemMessage(hWnd, IDC_CARG2A, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_CARG2A, BM_SETCHECK, BST_UNCHECKED, 0);
				if (texshaderstate.texstages[number].alphaarg1 & D3DTA_ALPHAREPLICATE)
					SendDlgItemMessage(hWnd, IDC_AARG1A, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_AARG1A, BM_SETCHECK, BST_UNCHECKED, 0);
				if (texshaderstate.texstages[number].alphaarg2 & D3DTA_ALPHAREPLICATE)
					SendDlgItemMessage(hWnd, IDC_AARG2A, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_AARG2A, BM_SETCHECK, BST_UNCHECKED, 0);
				if (texshaderstate.texstages[number].colorarg1 & D3DTA_COMPLEMENT)
					SendDlgItemMessage(hWnd, IDC_CARG1INV, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_CARG1INV, BM_SETCHECK, BST_UNCHECKED, 0);
				if (texshaderstate.texstages[number].colorarg2 & D3DTA_COMPLEMENT)
					SendDlgItemMessage(hWnd, IDC_CARG2INV, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_CARG2INV, BM_SETCHECK, BST_UNCHECKED, 0);
				if (texshaderstate.texstages[number].alphaarg1 & D3DTA_COMPLEMENT)
					SendDlgItemMessage(hWnd, IDC_AARG1INV, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_AARG1INV, BM_SETCHECK, BST_UNCHECKED, 0);
				if (texshaderstate.texstages[number].alphaarg2 & D3DTA_COMPLEMENT)
					SendDlgItemMessage(hWnd, IDC_AARG2INV, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_AARG2INV, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_COLOROP, CB_SETCURSEL, texshaderstate.texstages[number].colorop - 1, 0);
				SendDlgItemMessage(hWnd, IDC_ALPHAOP, CB_SETCURSEL, texshaderstate.texstages[number].alphaop - 1, 0);
			}
			break;
		case IDC_TEXTURE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				texshaderstate.texstages[number].texturetype =
					SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_GETCURSEL, 0, 0);
				SelectTexture(&texshaderstate.texstages[number].texture, texshaderstate.texstages[number].texturetype,
					texshaderstate.texstages[number].keycolor, texshaderstate.texstages[number].colorkey,
					texshaderstate.texstages[number].texturefile);
			}
			break;
		case IDC_TEXTUREFILE:
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				number = texshaderstate.currentstage;
				SendDlgItemMessage(hWnd, IDC_TEXTUREFILE, WM_GETTEXT, MAX_PATH + 1,
					(LPARAM)texshaderstate.texstages[number].texturefile);
				SelectTexture(&texshaderstate.texstages[number].texture, texshaderstate.texstages[number].texturetype,
					texshaderstate.texstages[number].keycolor, texshaderstate.texstages[number].colorkey,
					texshaderstate.texstages[number].texturefile);
			}
			break;
		case IDC_CARG1:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_CARG1, IDC_CARG1A, IDC_CARG1INV, &texshaderstate.texstages[number].colorarg1);
				d3d7dev->SetTextureStageState(number, D3DTSS_COLORARG1, texshaderstate.texstages[number].colorarg1);
			}
			break;
		case IDC_CARG1A:
		case IDC_CARG1INV:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_CARG1, IDC_CARG1A, IDC_CARG1INV, &texshaderstate.texstages[number].colorarg2);
				d3d7dev->SetTextureStageState(number, D3DTSS_COLORARG1, texshaderstate.texstages[number].colorarg2);
			}
			break;
		case IDC_CARG2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_CARG2, IDC_CARG2A, IDC_CARG2INV, &texshaderstate.texstages[number].colorarg2);
				d3d7dev->SetTextureStageState(number, D3DTSS_COLORARG2, texshaderstate.texstages[number].colorarg2);
			}
			break;
		case IDC_CARG2A:
		case IDC_CARG2INV:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_CARG2, IDC_CARG2A, IDC_CARG2INV, &texshaderstate.texstages[number].colorarg1);
				d3d7dev->SetTextureStageState(number, D3DTSS_COLORARG2, texshaderstate.texstages[number].colorarg1);
			}
			break;
		case IDC_AARG1:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_AARG1, IDC_AARG1A, IDC_AARG1INV, &texshaderstate.texstages[number].alphaarg1);
				d3d7dev->SetTextureStageState(number, D3DTSS_ALPHAARG1, texshaderstate.texstages[number].alphaarg1);
			}
			break;
		case IDC_AARG1A:
		case IDC_AARG1INV:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_AARG1, IDC_AARG1A, IDC_AARG1INV, &texshaderstate.texstages[number].alphaarg1);
				d3d7dev->SetTextureStageState(number, D3DTSS_ALPHAARG1, texshaderstate.texstages[number].alphaarg1);
			}
			break;
		case IDC_AARG2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_AARG2, IDC_AARG2A, IDC_AARG2INV, &texshaderstate.texstages[number].alphaarg2);
				d3d7dev->SetTextureStageState(number, D3DTSS_ALPHAARG2, texshaderstate.texstages[number].alphaarg2);
			}
			break;
		case IDC_AARG2A:
		case IDC_AARG2INV:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				number = texshaderstate.currentstage;
				SetShaderArg(hWnd, IDC_AARG2, IDC_AARG2A, IDC_AARG2INV, &texshaderstate.texstages[number].alphaarg2);
				d3d7dev->SetTextureStageState(number, D3DTSS_ALPHAARG2, texshaderstate.texstages[number].alphaarg2);
			}
			break;
		case IDC_COLOROP:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				texshaderstate.texstages[number].colorop = (D3DTEXTUREOP)(SendDlgItemMessage(hWnd, IDC_COLOROP, CB_GETCURSEL, 0, 0) + 1);
				d3d7dev->SetTextureStageState(number, D3DTSS_COLOROP, texshaderstate.texstages[number].colorop);
			}
			break;
		case IDC_ALPHAOP:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				texshaderstate.texstages[number].alphaop = (D3DTEXTUREOP)(SendDlgItemMessage(hWnd, IDC_ALPHAOP, CB_GETCURSEL, 0, 0) + 1);
				d3d7dev->SetTextureStageState(number, D3DTSS_ALPHAOP, texshaderstate.texstages[number].alphaop);
			}
			break;
		case IDC_BGCOLOR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_BGCOLOR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &bgcolor)) bgcolor = 0;
			}
			break;
		case IDC_DIFFUSE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_DIFFUSE, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				SetVertexColor(litvertices, colorvertices, numpoints, number);
			}
			break;
		case IDC_SPECULAR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_SPECULAR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				SetVertexSpecular(litvertices, colorvertices, numpoints, number);
			}
			break;
		case IDC_FACTOR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FACTOR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				d3d7dev->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, number);
			}
			break;
		case IDC_FOGCOLOR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGCOLOR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGCOLOR, number);
			}
			break;
		case IDC_TEXCOLORKEY:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_TEXCOLORKEY, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				number = texshaderstate.currentstage;
				if (tmpstring[0] == 0)
				{
					texshaderstate.texstages[number].colorkey = FALSE;
					texshaderstate.texstages[number].keycolor = 0;
				}
				else
				{
					texshaderstate.texstages[number].colorkey = TRUE;
					if (!_stscanf(tmpstring, _T("%x"), &texshaderstate.texstages[number].keycolor))
						texshaderstate.texstages[number].keycolor = 0;
				}
				SelectTexture(&texshaderstate.texstages[number].texture, texshaderstate.texstages[number].texturetype,
					texshaderstate.texstages[number].keycolor, texshaderstate.texstages[number].colorkey,
					texshaderstate.texstages[number].texturefile);
			}
		case IDC_ALPHABLEND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_ALPHABLEND, BM_GETCHECK, 0, 0) == BST_CHECKED)
					d3d7dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
				else d3d7dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
			}
			break;
		case IDC_SRCBLEND:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, SendDlgItemMessage(hWnd,
					IDC_SRCBLEND, CB_GETCURSEL, 0, 0) + 1);
			}
		case IDC_DESTBLEND:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, SendDlgItemMessage(hWnd,
					IDC_DESTBLEND, CB_GETCURSEL, 0, 0) + 1);
			}
			break;
		case IDC_ALPHATEST:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_ALPHATEST, BM_GETCHECK, 0, 0) == BST_CHECKED)
					d3d7dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
				else d3d7dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
			}
		case IDC_ALPHAFUNC:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, SendDlgItemMessage(hWnd,
					IDC_ALPHAFUNC, CB_GETCURSEL, 0, 0) + 1);
			}
			break;
		case IDC_ALPHAREF:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_ALPHAREF, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				number = _ttoi(tmpstring);
				if (number < 0) SendDlgItemMessage(hWnd, IDC_ALPHAREF, WM_SETTEXT, 0, (LPARAM)_T("0"));
				if (number > 255) SendDlgItemMessage(hWnd, IDC_ALPHAREF, WM_SETTEXT, 0, (LPARAM)_T("255"));
				if (number < 0) number = 0;
				if (number > 255) number = 255;
				if (d3d7dev) d3d7dev->SetRenderState(D3DRENDERSTATE_ALPHAREF, number);
			}
		case IDC_COLORKEY:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_COLORKEY, BM_GETCHECK, 0, 0) == BST_CHECKED)
					d3d7dev->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
				else d3d7dev->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
			}
			break;
		case IDC_FOGENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_FOGENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED)
					d3d7dev->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
				else d3d7dev->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
			}
			break;
		case IDC_VERTEXFOGMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGVERTEXMODE, SendDlgItemMessage(hWnd,
					IDC_VERTEXFOGMODE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_PIXELFOGMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, SendDlgItemMessage(hWnd,
					IDC_PIXELFOGMODE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_FOGSTART:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGSTART, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				f = (float)_ttof(tmpstring);
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGSTART, *((LPDWORD)(&f)));
			}
			break;
		case IDC_FOGEND:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGEND, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				f = (float)_ttof(tmpstring);
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGEND, *((LPDWORD)(&f)));
			}
			break;
		case IDC_FOGDENSITY:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGDENSITY, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				f = (float)_ttof(tmpstring);
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGDENSITY, *((LPDWORD)(&f)));
			}
			break;
		case IDCANCEL:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		default:
			return FALSE;
		}
		break;
	case WM_CLOSE:
		StopTimer();
		if (d3d7dev)
		{
			d3d7dev->Release();
			d3d7dev = NULL;
		}
		if (d3d7)
		{
			d3d7->Release();
			d3d7dev = NULL;
		}
		if (ddsrender)
		{
			ddsrender->Release();
			ddsrender = NULL;
		}
		if (ddsurface)
		{
			ddsurface->Release();
			ddsurface = NULL;
		}
		if (zbuffer)
		{
			zbuffer->Release();
			zbuffer = NULL;
		}
		if (ddclipper)
		{
			ddclipper->Release();
			ddclipper = NULL;
		}
		if (ddinterface)
		{
			ddinterface->Release();
			ddinterface = NULL;
		}
		if (mesh)
		{
			free(mesh);
			mesh = NULL;
		}
		if (vertices)
		{
			free(vertices);
			vertices = NULL;
		}
		if (litvertices)
		{
			free(litvertices);
			litvertices = NULL;
		}
		if (colorvertices)
		{
			free(colorvertices);
			colorvertices = NULL;
		}
		EndDialog(hWnd, IDCANCEL);
		break;
	case WM_APP:
		RunTestTimed(testnum);
		break;
	default:
		return FALSE;
	}
	return TRUE;


}

INT_PTR CALLBACK VertexShader7Proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HRESULT error;
	D3DVIEWPORT7 vp;
	HWND hDisplay;
	int number;
	float f;
	TCHAR tmpstring[MAX_PATH + 1];
	switch (Msg)
	{
	case WM_INITDIALOG:
		RECT r;
		DDSCAPS2 caps;
		DDSURFACEDESC2 ddsd;
		DDPIXELFORMAT ddpfz;
		testnum = 15;
		ddinterface = new MultiDirectDraw(7, &error, NULL);
		hDisplay = GetDlgItem(hWnd, IDC_DISPLAY);
		::hWnd = hDisplay;
		error = ddinterface->SetCooperativeLevel(hDisplay, DDSCL_NORMAL);
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		error = ddinterface->CreateSurface(&ddsd, &ddsurface, NULL);
		error = ddinterface->CreateClipper(0, &ddclipper, NULL);
		error = ddclipper->SetHWnd(0, hDisplay);
		error = ddsurface->SetClipper(ddclipper);
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		GetClientRect(hDisplay, &r);
		ddsd.dwWidth = r.right;
		ddsd.dwHeight = r.bottom;
		error = ddinterface->CreateSurface(&ddsd, &ddsrender, NULL);
		error = ddinterface->QueryInterface(IID_IDirect3D7, (VOID**)&d3d7);
		error = d3d7->EnumZBufferFormats(IID_IDirect3DRGBDevice, zcallback, &ddpfz);
		error = ddsrender->GetSurfaceDesc(&ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
		memcpy(&ddsd.ddpfPixelFormat, &ddpfz, sizeof(DDPIXELFORMAT));
		error = ddinterface->CreateSurface(&ddsd, &zbuffer, NULL);
		error = ddsrender->AddAttachedSurface(zbuffer);
		error = d3d7->CreateDevice(IID_IDirect3DHALDevice, (LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(), &d3d7dev);
		if (error != D3D_OK)
			error = d3d7->CreateDevice(IID_IDirect3DRGBDevice, (LPDIRECTDRAWSURFACE7)ddsrender->GetSurface(), &d3d7dev);
		ddsrender->GetSurfaceDesc(&ddsd);
		vp.dvMaxZ = 1.0f;
		vp.dvMinZ = 0.0f;
		vp.dwX = vp.dwY = 0;
		vp.dwWidth = ddsd.dwWidth;
		vp.dwHeight = ddsd.dwHeight;
		error = d3d7dev->SetViewport(&vp);
		error = d3d7dev->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
		InitTest(15);
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("None"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("Gradients"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("DXGL logo (small)"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("DXGL logo (large)"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_ADDSTRING, 0, (LPARAM)_T("Texture file"));
		SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_SETCURSEL, 0, 0);
		SendDlgItemMessage(hWnd, IDC_DIFFUSE, WM_SETTEXT, 0, (LPARAM)_T("FFFFFFFF"));
		SendDlgItemMessage(hWnd, IDC_SPECULAR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_FACTOR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_FOGCOLOR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_BGCOLOR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_AMBIENT, WM_SETTEXT, 0, (LPARAM)_T("FFFFFFFF"));
		SendDlgItemMessage(hWnd, IDC_EMISSIVE, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_MATAMBIENT, WM_SETTEXT, 0, (LPARAM)_T("FFFFFFFF"));
		SendDlgItemMessage(hWnd, IDC_MATDIFFUSE, WM_SETTEXT, 0, (LPARAM)_T("FFFFFFFF"));
		SendDlgItemMessage(hWnd, IDC_MATSPECULAR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		PopulateFogCombo(GetDlgItem(hWnd, IDC_VERTEXFOGMODE));
		PopulateFogCombo(GetDlgItem(hWnd, IDC_PIXELFOGMODE));
		SendDlgItemMessage(hWnd, IDC_VERTEXFOGMODE, CB_SETCURSEL, D3DFOG_NONE, 0);
		SendDlgItemMessage(hWnd, IDC_PIXELFOGMODE, CB_SETCURSEL, D3DFOG_NONE, 0);
		SendDlgItemMessage(hWnd, IDC_FOGSTART, WM_SETTEXT, 0, (LPARAM)_T("0.0"));
		SendDlgItemMessage(hWnd, IDC_FOGEND, WM_SETTEXT, 0, (LPARAM)_T("1.0"));
		SendDlgItemMessage(hWnd, IDC_FOGDENSITY, WM_SETTEXT, 0, (LPARAM)_T("1.0"));
		SendDlgItemMessage(hWnd, IDC_FILLMODE, CB_ADDSTRING, 0, (LPARAM)_T("Points"));
		SendDlgItemMessage(hWnd, IDC_FILLMODE, CB_ADDSTRING, 0, (LPARAM)_T("Wireframe"));
		SendDlgItemMessage(hWnd, IDC_FILLMODE, CB_ADDSTRING, 0, (LPARAM)_T("Solid"));
		SendDlgItemMessage(hWnd, IDC_FILLMODE, CB_SETCURSEL, 2, 0);
		SendDlgItemMessage(hWnd, IDC_SHADEMODE, CB_ADDSTRING, 0, (LPARAM)_T("Flat"));
		SendDlgItemMessage(hWnd, IDC_SHADEMODE, CB_ADDSTRING, 0, (LPARAM)_T("Gouraud"));
		SendDlgItemMessage(hWnd, IDC_SHADEMODE, CB_ADDSTRING, 0, (LPARAM)_T("Phong"));
		SendDlgItemMessage(hWnd, IDC_SHADEMODE, CB_SETCURSEL, 1, 0);
		SendDlgItemMessage(hWnd, IDC_CULLMODE, CB_ADDSTRING, 0, (LPARAM)_T("None"));
		SendDlgItemMessage(hWnd, IDC_CULLMODE, CB_ADDSTRING, 0, (LPARAM)_T("CW"));
		SendDlgItemMessage(hWnd, IDC_CULLMODE, CB_ADDSTRING, 0, (LPARAM)_T("CCW"));
		SendDlgItemMessage(hWnd, IDC_CULLMODE, CB_SETCURSEL, 2, 0);
		SendDlgItemMessage(hWnd, IDC_VERTEXCOLOR, BM_SETCHECK, BST_CHECKED, 0);
		PopulateSourceCombo(GetDlgItem(hWnd, IDC_DIFFUSESOURCE));
		PopulateSourceCombo(GetDlgItem(hWnd, IDC_SPECULARSOURCE));
		PopulateSourceCombo(GetDlgItem(hWnd, IDC_AMBIENTSOURCE));
		PopulateSourceCombo(GetDlgItem(hWnd, IDC_EMISSIVESOURCE));
		SendDlgItemMessage(hWnd, IDC_DIFFUSESOURCE, CB_SETCURSEL, D3DMCS_COLOR1, 0);
		SendDlgItemMessage(hWnd, IDC_SPECULARSOURCE, CB_SETCURSEL, D3DMCS_COLOR2, 0);
		SendDlgItemMessage(hWnd, IDC_AMBIENTSOURCE, CB_SETCURSEL, D3DMCS_MATERIAL, 0);
		SendDlgItemMessage(hWnd, IDC_EMISSIVESOURCE, CB_SETCURSEL, D3DMCS_MATERIAL, 0);
		SendDlgItemMessage(hWnd, IDC_SPINDETAIL, UDM_SETRANGE32, 2, 64);
		SendDlgItemMessage(hWnd, IDC_SPINDETAIL, UDM_SETPOS32, 0, 8);
		SendDlgItemMessage(hWnd, IDC_SPINLIGHT, UDM_SETRANGE32, 0, 7);
		SendDlgItemMessage(hWnd, IDC_LIGHTDIFFUSE, WM_SETTEXT, 0, (LPARAM)_T("00FFFFFF"));
		SendDlgItemMessage(hWnd, IDC_LIGHTAMBIENT, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_LIGHTSPECULAR, WM_SETTEXT, 0, (LPARAM)_T("00000000"));
		SendDlgItemMessage(hWnd, IDC_LIGHTRANGE, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_LIGHTFALLOFF, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_LIGHTTHETA, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_LIGHTPHI, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_LIGHTATTEN0, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_LIGHTATTEN1, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_LIGHTATTEN2, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_POWER, WM_SETTEXT, 0, (LPARAM)_T("0"));
		SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_ADDSTRING, 0, (LPARAM)_T("Point"));
		SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_ADDSTRING, 0, (LPARAM)_T("Spot"));
		SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_ADDSTRING, 0, (LPARAM)_T("Directional"));
		SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_ADDSTRING, 0, (LPARAM)_T("Parallel Point"));
		SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_ADDSTRING, 0, (LPARAM)_T("GL Spot"));
		SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_SETCURSEL, D3DLIGHT_DIRECTIONAL - 1, 0);
		SendDlgItemMessage(hWnd, IDC_LIGHTENABLED, BM_SETCHECK, BST_CHECKED, 0);
		::width = ddsd.dwWidth;
		::height = ddsd.dwHeight;
		vertexshaderstate.texture = NULL;
		vertexshaderstate.texturefile[0] = 0;
		vertexshaderstate.texturetype = 0;
		vertexshaderstate.currentlight = 0;
		StartTimer(hWnd, WM_APP, 60);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_TEXTURE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				number = texshaderstate.currentstage;
				vertexshaderstate.texturetype =
					SendDlgItemMessage(hWnd, IDC_TEXTURE, CB_GETCURSEL, 0, 0);
				SelectTexture(&vertexshaderstate.texture, vertexshaderstate.texturetype,
					0, FALSE, vertexshaderstate.texturefile);
				if ((vertexshaderstate.texturetype == 2) || (vertexshaderstate.texturetype == 3))
					d3d7dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHAPM);
				else d3d7dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			}
			break;
		case IDC_TEXTUREFILE:
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				number = texshaderstate.currentstage;
				SendDlgItemMessage(hWnd, IDC_TEXTUREFILE, WM_GETTEXT, MAX_PATH + 1,
					(LPARAM)vertexshaderstate.texturefile);
				SelectTexture(&vertexshaderstate.texture, vertexshaderstate.texturetype,
					0, FALSE, vertexshaderstate.texturefile);
			}
			break;
		case IDC_FOGENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_FOGENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED)
					d3d7dev->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
				else d3d7dev->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
			}
			break;
		case IDC_VERTEXFOGMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGVERTEXMODE, SendDlgItemMessage(hWnd,
					IDC_VERTEXFOGMODE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_PIXELFOGMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, SendDlgItemMessage(hWnd,
					IDC_PIXELFOGMODE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_FOGSTART:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGSTART, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				f = (float)_ttof(tmpstring);
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGSTART, *((LPDWORD)(&f)));
			}
			break;
		case IDC_FOGEND:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGEND, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				f = (float)_ttof(tmpstring);
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGEND, *((LPDWORD)(&f)));
			}
			break;
		case IDC_FOGDENSITY:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGDENSITY, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				f = (float)_ttof(tmpstring);
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGDENSITY, *((LPDWORD)(&f)));
			}
			break;
		case IDC_RANGEBASEDFOG:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_RANGEBASEDFOG, BM_GETCHECK, 0, 0) == BST_CHECKED)
					d3d7dev->SetRenderState(D3DRENDERSTATE_RANGEFOGENABLE, TRUE);
				else d3d7dev->SetRenderState(D3DRENDERSTATE_RANGEFOGENABLE, FALSE);
			}
			break;
		case IDC_FILLMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_FILLMODE, SendDlgItemMessage(hWnd,
					IDC_FILLMODE, CB_GETCURSEL, 0, 0) + 1);
			}
			break;
		case IDC_SHADEMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, SendDlgItemMessage(hWnd,
					IDC_SHADEMODE, CB_GETCURSEL, 0, 0) + 1);
			}
			break;
		case IDC_CULLMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_CULLMODE, SendDlgItemMessage(hWnd,
					IDC_CULLMODE, CB_GETCURSEL, 0, 0) + 1);
			}
			break;
		case IDC_DIFFUSE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_DIFFUSE, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				SetVertexColor(litvertices, colorvertices, numpoints, number);
			}
			break;
		case IDC_SPECULAR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_SPECULAR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				SetVertexSpecular(litvertices, colorvertices, numpoints, number);
			}
			break;
		case IDC_FACTOR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FACTOR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				d3d7dev->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, number);
			}
			break;
		case IDC_FOGCOLOR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_FOGCOLOR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				d3d7dev->SetRenderState(D3DRENDERSTATE_FOGCOLOR, number);
			}
			break;
		case IDC_BGCOLOR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_BGCOLOR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &bgcolor)) bgcolor = 0;
			}
			break;
		case IDC_AMBIENT:
		{
			SendDlgItemMessage(hWnd, IDC_AMBIENT, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
			if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
			d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENT, number);
		}
		break;
		case IDC_EMISSIVE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_EMISSIVE, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				material.emissive.b = (float)(number & 255) / 255.0;
				material.emissive.g = (float)((number >> 8) & 255) / 255.0;
				material.emissive.r = (float)((number >> 16) & 255) / 255.0;
				material.emissive.a = (float)((number >> 24) & 255) / 255.0;
				d3d7dev->SetMaterial(&material);
			}
			break;
		case IDC_MATAMBIENT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_MATAMBIENT, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				material.ambient.b = (float)(number & 255) / 255.0;
				material.ambient.g = (float)((number >> 8) & 255) / 255.0;
				material.ambient.r = (float)((number >> 16) & 255) / 255.0;
				material.ambient.a = (float)((number >> 24) & 255) / 255.0;
				d3d7dev->SetMaterial(&material);
			}
			break;
		case IDC_MATDIFFUSE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_MATDIFFUSE, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				material.diffuse.b = (float)(number & 255) / 255.0;
				material.diffuse.g = (float)((number >> 8) & 255) / 255.0;
				material.diffuse.r = (float)((number >> 16) & 255) / 255.0;
				material.diffuse.a = (float)((number >> 24) & 255) / 255.0;
				d3d7dev->SetMaterial(&material);
			}
			break;
		case IDC_MATSPECULAR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_MATSPECULAR, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				if (!_stscanf(tmpstring, _T("%x"), &number)) number = 0;
				material.specular.b = (float)(number & 255) / 255.0;
				material.specular.g = (float)((number >> 8) & 255) / 255.0;
				material.specular.r = (float)((number >> 16) & 255) / 255.0;
				material.specular.a = (float)((number >> 24) & 255) / 255.0;
				d3d7dev->SetMaterial(&material);
			}
			break;
		case IDC_POWER:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_POWER, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				material.power = (float)_ttof(tmpstring);
				d3d7dev->SetMaterial(&material);
			}
			break;
		case IDC_ENABLELIGHT:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_ENABLELIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
					for (int i = 0; i < 8; i++)
					{
						d3d7dev->SetLight(i, &lights[i]);
						d3d7dev->LightEnable(i, lightenable[i]);
					}
				}
				else d3d7dev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
			}
			break;
		case IDC_VERTEXCOLOR:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_VERTEXCOLOR, BM_GETCHECK, 0, 0) == BST_CHECKED)
					d3d7dev->SetRenderState(D3DRENDERSTATE_COLORVERTEX, TRUE);
				else d3d7dev->SetRenderState(D3DRENDERSTATE_COLORVERTEX, FALSE);
			}
			break;
		case IDC_DETAIL:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_DETAIL, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				number = _ttoi(tmpstring);
				if (number < 2) SendDlgItemMessage(hWnd, IDC_DETAIL, WM_SETTEXT, 0, (LPARAM)_T("2"));
				if (number > 64) SendDlgItemMessage(hWnd, IDC_DETAIL, WM_SETTEXT, 0, (LPARAM)_T("64"));
				MakeCube3D(5, number);
			}
			break;
		case IDC_DIFFUSESOURCE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, SendDlgItemMessage(hWnd,
					IDC_DIFFUSESOURCE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_SPECULARSOURCE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_SPECULARMATERIALSOURCE, SendDlgItemMessage(hWnd,
					IDC_SPECULARSOURCE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_AMBIENTSOURCE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, SendDlgItemMessage(hWnd,
					IDC_AMBIENTSOURCE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_EMISSIVESOURCE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				d3d7dev->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, SendDlgItemMessage(hWnd,
					IDC_EMISSIVESOURCE, CB_GETCURSEL, 0, 0));
			}
			break;
		case IDC_LIGHTNUMBER:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_LIGHTNUMBER, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				number = _ttoi(tmpstring);
				if (number < 0) SendDlgItemMessage(hWnd, IDC_LIGHTNUMBER, WM_SETTEXT, 0, (LPARAM)_T("0"));
				if (number > 7) SendDlgItemMessage(hWnd, IDC_LIGHTNUMBER, WM_SETTEXT, 0, (LPARAM)_T("7"));
				vertexshaderstate.currentlight = number;
				_itot(hexlightcolor[vertexshaderstate.currentlight].ambient, tmpstring, 16);
				strupper(tmpstring); paddwordzeroes(tmpstring);
				SendDlgItemMessage(hWnd, IDC_LIGHTAMBIENT, WM_SETTEXT, 0, (LPARAM)tmpstring);
				_itot(hexlightcolor[vertexshaderstate.currentlight].diffuse, tmpstring, 16);
				strupper(tmpstring); paddwordzeroes(tmpstring);
				SendDlgItemMessage(hWnd, IDC_LIGHTDIFFUSE, WM_SETTEXT, 0, (LPARAM)tmpstring);
				_itot(hexlightcolor[vertexshaderstate.currentlight].specular, tmpstring, 16);
				strupper(tmpstring); paddwordzeroes(tmpstring);
				SendDlgItemMessage(hWnd, IDC_LIGHTSPECULAR, WM_SETTEXT, 0, (LPARAM)tmpstring);
				SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_SETCURSEL, lights[vertexshaderstate.currentlight].dltType - 1, 0);
				if (lightenable[vertexshaderstate.currentlight])
					SendDlgItemMessage(hWnd, IDC_LIGHTENABLED, BM_SETCHECK, BST_CHECKED, 0);
				else SendDlgItemMessage(hWnd, IDC_LIGHTENABLED, BM_SETCHECK, BST_UNCHECKED, 0);
			}
			break;
		case IDC_LIGHTENABLED:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (SendDlgItemMessage(hWnd, IDC_LIGHTENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					lightenable[vertexshaderstate.currentlight] = TRUE;
					d3d7dev->LightEnable(vertexshaderstate.currentlight, TRUE);
				}
				else
				{
					lightenable[vertexshaderstate.currentlight] = FALSE;
					d3d7dev->LightEnable(vertexshaderstate.currentlight, FALSE);
				}
			}
			break;
		case IDC_LIGHTTYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				lights[vertexshaderstate.currentlight].dltType = (D3DLIGHTTYPE)
					(SendDlgItemMessage(hWnd, IDC_LIGHTTYPE, CB_GETCURSEL, 0, 0) + 1);
				d3d7dev->SetLight(vertexshaderstate.currentlight, &lights[vertexshaderstate.currentlight]);
			}
			break;
		case IDC_LIGHTRANGE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				SendDlgItemMessage(hWnd, IDC_LIGHTRANGE, WM_GETTEXT, MAX_PATH, (LPARAM)tmpstring);
				lights[vertexshaderstate.currentlight].dvRange = (float)_ttof(tmpstring);
				d3d7dev->SetLight(vertexshaderstate.currentlight, &lights[vertexshaderstate.currentlight]);
			}
			break;
		case IDCANCEL:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		default:
			return FALSE;
		}
		break;
	case WM_CLOSE:
		StopTimer();
		if (d3d7dev)
		{
			d3d7dev->Release();
			d3d7dev = NULL;
		}
		if (d3d7)
		{
			d3d7->Release();
			d3d7dev = NULL;
		}
		if (ddsrender)
		{
			ddsrender->Release();
			ddsrender = NULL;
		}
		if (ddsurface)
		{
			ddsurface->Release();
			ddsurface = NULL;
		}
		if (zbuffer)
		{
			zbuffer->Release();
			zbuffer = NULL;
		}
		if (ddclipper)
		{
			ddclipper->Release();
			ddclipper = NULL;
		}
		if (ddinterface)
		{
			ddinterface->Release();
			ddinterface = NULL;
		}
		if (mesh)
		{
			free(mesh);
			mesh = NULL;
		}
		if (vertices)
		{
			free(vertices);
			vertices = NULL;
		}
		if (litvertices)
		{
			free(litvertices);
			litvertices = NULL;
		}
		if (colorvertices)
		{
			free(colorvertices);
			colorvertices = NULL;
		}
		EndDialog(hWnd, IDCANCEL);
		break;
	case WM_APP:
		RunTestTimed(testnum);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

/*
void DDFlipTestWindow::OnQueryNewPalette(wxQueryNewPaletteEvent& event)
{
	//if(bpp == 8) ddsurface->SetPalette
}
*/