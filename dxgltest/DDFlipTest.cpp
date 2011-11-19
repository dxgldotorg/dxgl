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

#include "stdafx.h"
#include "tests.h"
#include "surfacegen.h"

class DDFlipTestWindow : public wxFrame
{
public:
	DDFlipTestWindow(const wxString& title, const wxPoint& pos, const wxSize& size, wxWindow *parent,
		int bpp, int refresh, int backbuffers, bool fullscreen, bool resizable);
	void InitDD();
	void OnClose(wxCloseEvent& event);
	void OnKey(wxKeyEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnQueryNewPalette(wxQueryNewPaletteEvent& event);
	void OnPaletteChanged(wxPaletteChangedEvent& event);
	DECLARE_EVENT_TABLE()

private:
	LPDIRECTDRAW2 ddinterface;
	LPDIRECTDRAWSURFACE ddsurface;
	LPDIRECTDRAWSURFACE ddsrender;
	LPDIRECTDRAWCLIPPER ddclipper;
	wxWindow *parentwnd;
	int width,height,bpp,refresh,backbuffers;
	bool fullscreen,resizable;
	wxTimer *timer;
	bool firstpaint;
	bool dd_ready;
};

BEGIN_EVENT_TABLE(DDFlipTestWindow,wxFrame)
	EVT_CLOSE(DDFlipTestWindow::OnClose)
	EVT_PAINT(DDFlipTestWindow::OnPaint)
	EVT_KEY_DOWN(DDFlipTestWindow::OnKey)
	EVT_TIMER(wxID_HIGHEST,DDFlipTestWindow::OnTimer)
	EVT_QUERY_NEW_PALETTE(DDFlipTestWindow::OnQueryNewPalette)
	EVT_PALETTE_CHANGED(DDFlipTestWindow::OnPaletteChanged)
END_EVENT_TABLE()

void RunDDFlipTest(int width, int height, int bpp, int refresh, int backbuffers, bool fullscreen, bool resizable, wxWindow *parent)
{
	DDFlipTestWindow *window = new DDFlipTestWindow(_("DDraw Flipping test"),wxDefaultPosition,wxSize(width,height),parent,bpp,refresh,backbuffers,fullscreen,resizable);
	window->Show(true);
	window->InitDD();
}

void DDFlipTestWindow::InitDD()
{
	LPDIRECTDRAW ddinterface1;
	HRESULT error = DirectDrawCreate(NULL,&ddinterface1,NULL);
	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd,sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	error = ddinterface1->QueryInterface(IID_IDirectDraw2,(void**)&ddinterface);
	ddinterface1->Release();
	if(fullscreen) error = ddinterface->SetCooperativeLevel(GetHandle(),DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
	else error = ddinterface->SetCooperativeLevel(GetHandle(),DDSCL_NORMAL);
	if(fullscreen) error = ddinterface->SetDisplayMode(width,height,bpp,refresh,0);
	else
	{
		if(!resizable) SetWindowStyle(GetWindowStyle() - wxRESIZE_BORDER);
		SetClientSize(width,height);
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
		error = ddclipper->SetHWnd(0,GetHandle());
		error = ddsurface->SetClipper(ddclipper);
		ZeroMemory(&ddsd,sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
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
	if(fullscreen && backbuffers > 0)
	{
		timer = new wxTimer(this,wxID_HIGHEST);
		timer->Start(1000);
	}
	if(bpp == 8)
	{
		IDirectDrawPalette *pal;
		ddinterface->CreatePalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256,(LPPALETTEENTRY)&DefaultPalette,&pal,NULL);
		ddsrender->SetPalette(pal);
		pal->Release();
	}
	firstpaint=true;
	dd_ready=true;
}

DDFlipTestWindow::DDFlipTestWindow(const wxString& title, const wxPoint& pos, const wxSize& size, wxWindow *parent,
		int bpp, int refresh, int backbuffers, bool fullscreen, bool resizable)
		: wxFrame(parent,-1,title,pos,size)
{
	firstpaint=false;
	dd_ready=false;
	this->width = size.GetX();
	this->height = size.GetY();
	this->bpp = bpp;
	this->refresh = refresh;
	this->backbuffers = backbuffers;
	this->fullscreen = fullscreen;
	this->resizable = resizable;
	ddclipper = NULL;
	ddinterface = NULL;
	ddsurface = NULL;
	ddsrender = NULL;
	parentwnd = parent;
	parentwnd->Disable();
	timer = NULL;
}

void DDFlipTestWindow::OnClose(wxCloseEvent& event)
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
	HRESULT error;
	DDSURFACEDESC ddsd;
	LPDIRECTDRAWSURFACE temp1,temp2;
	DDSCAPS ddscaps;
	unsigned char *buffer;
	if(dd_ready)
	{
		if(firstpaint)
		{
			if(!fullscreen) backbuffers = 0;
			ddsd.dwSize = sizeof(ddsd);
			error = ddsrender->GetSurfaceDesc(&ddsd);
			buffer = (unsigned char *)malloc(ddsd.lPitch*ddsd.dwHeight);
			GenScreen0(ddsd,buffer);
			error = ddsrender->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
			memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
			error = ddsrender->Unlock(NULL);
			if(backbuffers > 0)
			{
				ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
				error = ddsrender->GetAttachedSurface(&ddscaps,&temp1);
				GenScreen1(ddsd,buffer,GetHandle(),ddsrender);
				error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
				memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
				error = temp1->Unlock(NULL);
			}
			if(backbuffers > 1)
			{
				ddscaps.dwCaps = DDSCAPS_FLIP;
				error = temp1->GetAttachedSurface(&ddscaps,&temp1);
				GenScreen2(ddsd,buffer,GetHandle(),ddsrender);
				error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
				memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
				error = temp1->Unlock(NULL);
			}
			if(backbuffers > 2)
			{
				ddscaps.dwCaps = DDSCAPS_FLIP;
				error = temp1->GetAttachedSurface(&ddscaps,&temp1);
				GenScreen3(ddsd,buffer,GetHandle(),ddsrender);
				error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
				memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
				error = temp1->Unlock(NULL);
			}
			if(backbuffers > 3)
			{
				ddscaps.dwCaps = DDSCAPS_FLIP;
				error = temp1->GetAttachedSurface(&ddscaps,&temp1);
				GenScreen4(ddsd,buffer,GetHandle(),ddsrender);
				error = temp1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
				memcpy(ddsd.lpSurface,buffer,ddsd.lPitch*ddsd.dwHeight);
				error = temp1->Unlock(NULL);
			}
			free(buffer);
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

void DDFlipTestWindow::OnTimer(wxTimerEvent& event)
{
	ddsurface->Flip(NULL,DDBLT_WAIT);
}

void DDFlipTestWindow::OnQueryNewPalette(wxQueryNewPaletteEvent& event)
{
	//if(bpp == 8) ddsurface->SetPalette
}
void DDFlipTestWindow::OnPaletteChanged(wxPaletteChangedEvent& event)
{
	
}
