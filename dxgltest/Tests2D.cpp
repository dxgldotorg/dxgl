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

void InitTest(int test);
void RunTest(int test);

	MultiDirectDraw *ddinterface;
	MultiDirectDrawSurface *ddsurface;
	MultiDirectDrawSurface *ddsrender;
	LPDIRECTDRAWCLIPPER ddclipper;
	int width,height,bpp,refresh,backbuffers;
	double fps;
	bool fullscreen,resizable;
	HWND hWnd;
	int testnum;


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
		RunTest(testnum);
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
		return TRUE;
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
	::testnum = testnum;
	ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
	if(apiver > 3) ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	::fullscreen = fullscreen;
	::resizable = resizable;
	::width = width;
	::height = height;
	::bpp = bpp;
	::refresh = refresh;
	::backbuffers = backbuffers;
	::fps = fps;
	ddtestnum = testnum;
	ddver = apiver;
	HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(NULL);
	WNDCLASSEX wc;
	MSG Msg;
	ZeroMemory(&wc,sizeof(WNDCLASS));
	wc.cbSize = sizeof(WNDCLASSEX);
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
	else
		hWnd = CreateWindowEx(WS_EX_APPWINDOW,wndclassname2d,_T("DDraw Test Window"),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
			CW_USEDEFAULT,CW_USEDEFAULT,width,height,NULL,NULL,hinstance,NULL);

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
	UpdateWindow(hWnd);
	InitTest(testnum);
	if(!fullscreen) SendMessage(hWnd,WM_PAINT,0,0);
	StartTimer(hWnd,WM_APP,fps);
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
	UnregisterClass(wndclassname2d,hinstance);
	StopTimer();
}


void InitTest(int test)
{
	DDSURFACEDESC2 ddsd;
	DDSCAPS2 ddscaps;
	ZeroMemory(&ddscaps,sizeof(DDSCAPS2));
	LPDIRECTDRAWPALETTE palette;
	unsigned char *buffer;
	MultiDirectDrawSurface *temp1;
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
			error = temp1->GetAttachedSurface(&ddscaps,&temp1);
			DrawGradients(ddsd,buffer,hWnd,palette,0,0x0000FF);
			error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if(backbuffers > 2)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp1);
			DrawGradients(ddsd,buffer,hWnd,palette,0,0x00FF00);
			error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		if(backbuffers > 3)
		{
			ddscaps.dwCaps = DDSCAPS_FLIP;
			error = temp1->GetAttachedSurface(&ddscaps,&temp1);
			DrawGradients(ddsd,buffer,hWnd,palette,0,0xFF0000);
			error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = temp1->Unlock(NULL);
		}
		free(buffer);
		if(palette) palette->Release();
		break;
	default:
		break;
	}
}

void RunTest(int test)
{
	switch(test)
	{
	case 0:
	default:
		if(fullscreen)	ddsurface->Flip(NULL,DDBLT_WAIT);
		break;
	}
}

/*void DDFlipTestWindow::OnClose(wxCloseEvent& event)
{
	if(timer) delete timer;
	if(ddsrender) ddsrender->Release();
	if(ddsurface) ddsurface->Release();
	if(ddclipper) ddclipper->Release();
	if(ddinterface) ddinterface->Release();
	parentwnd->Enable();
	Destroy();
}

void DDFlipTestWindow::OnKey(wxKeyEvent& event)
{
	if(event.m_keyCode == WXK_ESCAPE) Close(true);
	else event.Skip();
}

void DDFlipTestWindow::OnPaint(wxPaintEvent& event)
{
	POINT p;
	RECT srcrect,destrect;
	LPDIRECTDRAWSURFACE temp1,temp2;
	if(dd_ready)
	{
		if(firstpaint)
		{
		}
		if(fullscreen)
		{
			ddsurface->Flip(NULL,DDBLT_WAIT);
		}
		else
		{
			p.x = 0;
			p.y = 0;
			::ClientToScreen(GetHandle(),&p);
			::GetClientRect(GetHandle(),&destrect);
			OffsetRect(&destrect,p.x,p.y);
			SetRect(&srcrect,0,0,width,height);
			error = ddsurface->Blt(&destrect,ddsrender,&srcrect,DDBLT_WAIT,NULL);
		}
	}
}

void DDFlipTestWindow::OnQueryNewPalette(wxQueryNewPaletteEvent& event)
{
	//if(bpp == 8) ddsurface->SetPalette
}
void DDFlipTestWindow::OnPaletteChanged(wxPaletteChangedEvent& event)
{

}*/
