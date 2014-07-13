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

void InitTest2D(int test);
void RunTestTimed2D(int test);
void RunTestLooped2D(int test);
void RunTestMouse2D(int test, UINT Msg, WPARAM wParam, LPARAM lParam);


static MultiDirectDraw *ddinterface;
static MultiDirectDrawSurface *ddsurface;
static MultiDirectDrawSurface *ddsrender;
static IDirectDrawPalette *pal;
static LPDIRECTDRAWCLIPPER ddclipper;
static int width,height,bpp,refresh,backbuffers;
static double fps;
static bool fullscreen,resizable;
static HWND hWnd;
static int testnum;
static unsigned int randnum;
static int testtypes[] = {0,1,0,1,0,1,2};

static DDSPRITE sprites[16];

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
		if(pal)
		{
			pal->Release();
			pal = NULL;
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
		RunTestTimed2D(testnum);
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
		RunTestMouse2D(testnum,Msg,wParam,lParam);
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
	default:
		return DefWindowProc(hWnd,Msg,wParam,lParam);
	}
	return FALSE;
}

static int ddtestnum;
static int ddver;

void RunTestMouse2D(int test, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if(!ddsrender) return;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	DDBLTFX bltfx;
	unsigned char *surface;
	int bytes;
	unsigned int x,y;
	bool out = false;
	bool msgbottom = false;
	TCHAR message[256];
	message[0] = 0;
	HDC hDC;
	switch(test)
	{
	case 6:
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		ZeroMemory(&bltfx,sizeof(DDBLTFX));
		bltfx.dwSize = sizeof(DDBLTFX);
		bltfx.dwFillColor = 0;
		ddsrender->Blt(NULL,NULL,NULL,DDBLT_COLORFILL,&bltfx);
		if(ddver > 3)ddsd.dwSize = sizeof(DDSURFACEDESC2);
		else ddsd.dwSize = sizeof(DDSURFACEDESC);
		ddsrender->GetSurfaceDesc(&ddsd);
		switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
		{
		case 8:
			bytes=1;
			break;
		case 15:
		case 16:
			bytes=2;
			break;
		case 24:
			bytes=3;
			break;
		case 32:
		default:
			bytes=4;
		}
		_tcscpy(message,_T("Message: "));
		switch(Msg)
		{
		case WM_MOUSEMOVE:
			_tcscat(message,_T("WM_MOUSEMOVE "));
			break;
		case WM_LBUTTONDOWN:
			_tcscat(message,_T("WM_LBUTTONDOWN "));
			break;
		case WM_LBUTTONUP:
			_tcscat(message,_T("WM_LBUTTONUP "));
			break;
		case WM_LBUTTONDBLCLK:
			_tcscat(message,_T("WM_LBUTTONDBLCLK "));
			break;
		case WM_RBUTTONDOWN:
			_tcscat(message,_T("WM_RBUTTONDOWN "));
			break;
		case WM_RBUTTONUP:
			_tcscat(message,_T("WM_RBUTTONUP "));
			break;
		case WM_RBUTTONDBLCLK:
			_tcscat(message,_T("WM_RBUTTONDBLCLK "));
			break;
		case WM_MBUTTONDOWN:
			_tcscat(message,_T("WM_MBUTTONDOWN "));
			break;
		case WM_MBUTTONUP:
			_tcscat(message,_T("WM_MBUTTONUP "));
			break;
		case WM_MBUTTONDBLCLK:
			_tcscat(message,_T("WM_MBUTTONDBLCLK "));
			break;
		case WM_MOUSEWHEEL:
			_tcscat(message,_T("WM_MOUSEWHEEL "));
			break;
		case WM_XBUTTONDOWN:
			_tcscat(message,_T("WM_XBUTTONDOWN "));
			break;
		case WM_XBUTTONUP:
			_tcscat(message,_T("WM_XBUTTONUP "));
			break;
		case WM_XBUTTONDBLCLK:
			_tcscat(message,_T("WM_XBUTTONDBLCLK "));
			break;
		case WM_MOUSEHWHEEL:
			_tcscat(message,_T("WM_MOUSEHWHEEL "));
			break;
		default:
			_tcscat(message,_T("unknown "));
		}
		_tcscat(message,_T("Keys: "));
		if(wParam & MK_CONTROL) _tcscat(message, _T("CTRL "));
		if(wParam & MK_SHIFT) _tcscat(message,_T("SHIFT "));
		_tcscat(message,_T("Buttons: "));
		if(wParam & MK_LBUTTON) _tcscat(message,_T("L "));
		if(wParam & MK_MBUTTON) _tcscat(message,_T("M "));
		if(wParam & MK_RBUTTON) _tcscat(message,_T("R "));
		if(wParam & MK_XBUTTON1) _tcscat(message,_T("X1 "));
		if(wParam & MK_XBUTTON2) _tcscat(message,_T("X2 "));
		// Add X and Y
		ddsrender->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
		surface = (unsigned char *)ddsd.lpSurface;
		if((x > ddsd.dwWidth) || (y > ddsd.dwHeight))
		{
			out = true;
			_tcscat(message,_T(" OUT OF BOUNDS"));
		}
		else surface[(x*bytes)+(y*ddsd.lPitch)] = 0xFF;
		ddsrender->Unlock(NULL);
		ddsrender->GetDC(&hDC);
		if(out)SetBkColor(hDC,RGB(255,0,0));
		else SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		if(y > ddsd.dwHeight / 2) TextOut(hDC,0,0,message,_tcslen(message));
		else TextOut(hDC,0,ddsd.dwHeight-16,message,_tcslen(message));
		ddsrender->ReleaseDC(hDC);
		break;
	default:
		break;
	}
}

const TCHAR wndclassname2d[] = _T("DDTestWndClass");
void RunTest2D(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	double fps, bool fullscreen, bool resizable)
{
	ZeroMemory(sprites,16*sizeof(DDSPRITE));
	DDSURFACEDESC2 ddsd;
	BOOL done = false;
	::testnum = testnum;
	randnum = (unsigned int)time(NULL);
	ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
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
	ddtestnum = testnum;
	ddver = apiver;
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
	wc.lpszClassName = wndclassname2d;
	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL,_T("Can not register window class"),_T("Error"),MB_ICONEXCLAMATION|MB_OK);
		return;
	}
	if(resizable)
		hWnd = CreateWindowEx(WS_EX_APPWINDOW,wndclassname2d,_T("DDraw Test Window"),WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,CW_USEDEFAULT,width,height,NULL,NULL,hinstance,NULL);
	else if(!fullscreen)
		hWnd = CreateWindowEx(WS_EX_APPWINDOW,wndclassname2d,_T("DDraw Test Window"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
			CW_USEDEFAULT,CW_USEDEFAULT,width,height,NULL,NULL,hinstance,NULL);
#ifdef _DEBUG
	else hWnd = CreateWindowEx(0,wndclassname2d,_T("DDraw Test Window"),WS_POPUP,0,0,
		GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),NULL,NULL,hinstance,NULL);
#else
	else hWnd = CreateWindowEx(WS_EX_TOPMOST,wndclassname2d,_T("DDraw Test Window"),WS_POPUP,0,0,
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
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
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
	InitTest2D(testnum);
	if(!fullscreen) SendMessage(hWnd,WM_PAINT,0,0);
	if(testtypes[testnum] == 1)
	{
		while(!done)
		{
			if(PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			{
				if(Msg.message == WM_PAINT) RunTestLooped2D(testnum);
				else if(Msg.message  == WM_QUIT) done = TRUE;
				else
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}
			else
			{
				RunTestLooped2D(testnum);
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
	UnregisterClass(wndclassname2d,hinstance);
	StopTimer();
}


void InitTest2D(int test)
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
		DrawGradients(ddsd, (unsigned char *)ddsd.lpSurface, hWnd, palette, 1, 0);
		error = sprites[0].surface->Unlock(NULL);
		sprites[0].width = (float)ddsd.dwWidth;
		sprites[0].height = (float)ddsd.dwHeight;
		sprites[0].rect.left = sprites[0].rect.top = 0;
		sprites[0].rect.right = ddsd.dwWidth;
		sprites[0].rect.bottom = ddsd.dwHeight;
		for (int i = 1; i < 16; i++)
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
			DDCOLORKEY ckey;
			ckey.dwColorSpaceHighValue = ckey.dwColorSpaceLowValue = 0;
			if (i < 5) sprites[i].bltflags = 0;
			else if (i < 9)
			{
				sprites[i].bltflags = DDBLTFAST_SRCCOLORKEY;
				if (sprites[i].surface) sprites[i].surface->SetColorKey(DDCKEY_SRCBLT, &ckey);
			}
			else if (i < 13) sprites[i].bltflags = DDBLTFAST_DESTCOLORKEY;
			else sprites[i].bltflags = DDBLTFAST_SRCCOLORKEY | DDBLTFAST_DESTCOLORKEY;
			sprites[i].x = randfloat((float)ddsd.dwWidth);
			sprites[i].y = randfloat((float)ddsd.dwHeight);
			sprites[i].xvelocity = randfloat(5);
			sprites[i].yvelocity = randfloat(5);
		}
		break;
	case 7:
		if (!fullscreen) backbuffers = 0;
		ddsrender->GetSurfaceDesc(&ddsd);
		ddsrender->GetPalette(&palette);
		sprites[0].ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
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
	}
}

void RunTestTimed2D(int test)
{
	if(stoptimer) return;
	DDSCAPS2 ddscaps;
	ZeroMemory(&ddscaps,sizeof(DDSCAPS2));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	MultiDirectDrawSurface *temp1 = NULL;
	switch(test)
	{
	case 0: // Palette and gradients
	case 2: // GDI patterns
	default:
		if(fullscreen)	ddsurface->Flip(NULL,DDFLIP_WAIT);
		break;
	case 4: // FastBlt sprites
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
		if(backbuffers && ddsrender) ddsrender->Flip(NULL,DDFLIP_WAIT);
		break;
	}
}

void RunTestLooped2D(int test)
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
	}
	if(temp1) temp1->Release();
}

/*
void DDFlipTestWindow::OnQueryNewPalette(wxQueryNewPaletteEvent& event)
{
	//if(bpp == 8) ddsurface->SetPalette
}
*/