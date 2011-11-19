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
#include <time.h>

void InitTest(int test);
void RunTestTimed(int test);
void RunTestLooped(int test);
inline unsigned int rand32(unsigned int &n)
{
    return n=(((unsigned int) 1103515245 * n) + (unsigned int) 12345) %
        (unsigned int) 0xFFFFFFFF;
}

	MultiDirectDraw *ddinterface;
	MultiDirectDrawSurface *ddsurface;
	MultiDirectDrawSurface *ddsrender;
	LPDIRECTDRAWCLIPPER ddclipper;
	int width,height,bpp,refresh,backbuffers;
	double fps;
	bool fullscreen,resizable;
	HWND hWnd;
	int testnum;
	unsigned int randnum;
	int testtypes[] = {0,1,0,1};


LRESULT CALLBACK DDWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	POINT p;
	RECT srcrect,destrect;
	HRESULT error;
	switch(Msg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		StopTimer();
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
		RunTestTimed(testnum);
		break;
	case WM_SIZE:
	case WM_PAINT:
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
		return FALSE;
	default:
		return DefWindowProc(hWnd,Msg,wParam,lParam);
	}
	return FALSE;
}



int ddtestnum;
int ddver;
const TCHAR wndclassname2d[] = _T("DDTestWndClass");
void RunTest2D(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	double fps, bool fullscreen, bool resizable)
{
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
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
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
	else hWnd = CreateWindowEx(WS_EX_TOPMOST,wndclassname2d,_T("DDraw Test Window"),WS_POPUP,0,0,
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
	}
	else
	{
		ddsrender = ddsurface;
		ddsrender->AddRef();
	}
	if(bpp == 8)
	{
		IDirectDrawPalette *pal;
		ddinterface->CreatePalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256,(LPPALETTEENTRY)&DefaultPalette,&pal,NULL);
		ddsrender->SetPalette(pal);
		pal->Release();
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
	else
	{
		StartTimer(hWnd,WM_APP,fps);
		while(GetMessage(&Msg, NULL, 0, 0) > 0)
		{
	        TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	UnregisterClass(wndclassname2d,hinstance);
	StopTimer();
}


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
	if(ddver > 3)ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	error = ddsrender->GetSurfaceDesc(&ddsd);
	switch(test)
	{
	case 0:
		if(!fullscreen) backbuffers = 0;
		buffer = (unsigned char *)malloc(ddsd.lPitch*ddsd.dwHeight);
		DrawPalette(ddsd,buffer);
		error = ddsrender->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
		memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
		error = ddsrender->Unlock(NULL);
		ddsrender->GetPalette(&palette);
		if(backbuffers > 0)
		{
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
			error = ddsrender->GetAttachedSurface(&ddscaps,&temp1);
			DrawGradients(ddsd,buffer,hWnd,palette,1,0);
			error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if(backbuffers > 1)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			DrawGradients(ddsd,buffer,hWnd,palette,0,0x0000FF);
			error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if(backbuffers > 2)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			DrawGradients(ddsd,buffer,hWnd,palette,0,0x00FF00);
			error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if(backbuffers > 3)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			DrawGradients(ddsd,buffer,hWnd,palette,0,0xFF0000);
			error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if(temp1) temp1->Release();
		free(buffer);
		if(palette) palette->Release();
		break;
	case 2:
		if(!fullscreen) backbuffers=0;
		error = ddsrender->GetDC(&hRenderDC);
		DrawGDIPatterns(ddsd,hRenderDC,0);
		ddsrender->ReleaseDC(hRenderDC);
		if(backbuffers > 0)
		{
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
			error = ddsrender->GetAttachedSurface(&ddscaps,&temp1);
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd,hRenderDC,1);
			temp1->ReleaseDC(hRenderDC);
		}
		if(backbuffers > 1)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd,hRenderDC,2);
			temp1->ReleaseDC(hRenderDC);
		}
		if(backbuffers > 2)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd,hRenderDC,3);
			temp1->ReleaseDC(hRenderDC);
		}
		if(backbuffers > 3)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd,hRenderDC,4);
			temp1->ReleaseDC(hRenderDC);
		}
		if(backbuffers > 4)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd,hRenderDC,5);
			temp1->ReleaseDC(hRenderDC);
		}
		if(backbuffers > 5)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd,hRenderDC,6);
			temp1->ReleaseDC(hRenderDC);
		}
		if(backbuffers > 6)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp2);
			temp1->Release();
			temp1 = temp2;
			temp1->GetDC(&hRenderDC);
			DrawGDIPatterns(ddsd,hRenderDC,7);
			temp1->ReleaseDC(hRenderDC);
		}
		if(temp1) temp1->Release();
		break;
	default:
		break;
	}
}

void RunTestTimed(int test)
{
	switch(test)
	{
	case 0:
	case 2:
	default:
		if(fullscreen)	ddsurface->Flip(NULL,DDFLIP_WAIT);
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
	}
	if(temp1) temp1->Release();
}

/*
void DDFlipTestWindow::OnQueryNewPalette(wxQueryNewPaletteEvent& event)
{
	//if(bpp == 8) ddsurface->SetPalette
}
*/