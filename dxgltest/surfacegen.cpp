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
#include "surfacegen.h"

void GenScreen0(DDSURFACEDESC ddsd, unsigned char *buffer)  // Palette test
{
	DWORD x,y;
	unsigned short *buffer16 = (unsigned short*) buffer;
	unsigned long *buffer32 = (unsigned long*) buffer;
	switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
	{
		case 8:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth; x++)
				{
					buffer[x+(ddsd.lPitch*y)] = (x/(ddsd.dwWidth/16.)) + 16*floor((y/(ddsd.dwHeight/16.)));
				}
			}
			break;
		case 16:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth; x++)
				{
					buffer16[x+((ddsd.lPitch/2)*y)] = (x/(ddsd.dwWidth/256.)) + 256*floor((y/(ddsd.dwHeight/256.)));
				}
			}
			break;
		case 24:
			return;
		case 32:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth; x++)
				{
					buffer32[x+((ddsd.lPitch/4)*y)] = (x/(ddsd.dwWidth/4096.)) + 4096*floor((y/(ddsd.dwHeight/4096.)));
				}
			}
			return;
		default:
			return;
	}
}




void GenScreen1(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface) // Gradients
{
	OSVERSIONINFOA verinfo;
	verinfo.dwOSVersionInfoSize = sizeof(verinfo);
	GetVersionExA(&verinfo);
	bool gradientavailable;
	if(verinfo.dwMajorVersion > 4) gradientavailable = true;
	else if(verinfo.dwMajorVersion >= 4 && verinfo.dwMinorVersion >= 1) gradientavailable = true;
	else gradientavailable = false;
	int bitmode = BI_RGB;
	DWORD bitmasks[3];
	LPBYTE bits;
	BITMAPINFO *bmi = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER) + 1024);
	ZeroMemory(bmi,sizeof(BITMAPINFOHEADER) + 1024);
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		//GetSystemPaletteEntries(GetDC(hwnd),0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		LPDIRECTDRAWPALETTE pal;
		surface->GetPalette(&pal);
		pal->GetEntries(0,0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		pal->Release();
	}
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
		{
			bitmode = BI_BITFIELDS;
			bitmasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
			bitmasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
			bitmasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
			memcpy(bmi->bmiColors,bitmasks,3*sizeof(DWORD));
		}
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = ddsd.lPitch / (ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
	bmi->bmiHeader.biHeight = 0-ddsd.dwHeight;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biCompression = bitmode;
	bmi->bmiHeader.biBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
	HBITMAP bitmap = CreateDIBSection(GetDC(hwnd),bmi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	HDC hdcmem = CreateCompatibleDC(GetDC(hwnd));
	HGDIOBJ hbmold = SelectObject(hdcmem,bitmap);
	SetBkMode(hdcmem,TRANSPARENT);
	DWORD x;
	int r,g,b;
	RECT rect;
	HBRUSH color;
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = (x*255)/ddsd.dwWidth;
		g = 0;
		b = 0;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 0;
		rect.bottom = ddsd.dwHeight / 7;
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = 0;
		g = (x*255)/ddsd.dwWidth;
		b = 0;
		rect.left = x;
		rect.right = x + 1;
		rect.top = ddsd.dwHeight / 7;
		rect.bottom = 2*(ddsd.dwHeight / 7.);
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = 0;
		g = 0;
		b = (x*255)/ddsd.dwWidth;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 2*(ddsd.dwHeight / 7.);
		rect.bottom = 3*(ddsd.dwHeight / 7.);
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = 0;
		g = (x*255)/ddsd.dwWidth;
		b = (x*255)/ddsd.dwWidth;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 3*(ddsd.dwHeight / 7.);
		rect.bottom = 4*(ddsd.dwHeight / 7.);
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = (x*255)/ddsd.dwWidth;
		g = 0;
		b = (x*255)/ddsd.dwWidth;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 4*(ddsd.dwHeight / 7.);
		rect.bottom = 5*(ddsd.dwHeight / 7.);
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = (x*255)/ddsd.dwWidth;
		g = (x*255)/ddsd.dwWidth;
		b = 0;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 5*(ddsd.dwHeight / 7.);
		rect.bottom = 6*(ddsd.dwHeight / 7.);
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = (x*255)/ddsd.dwWidth;
		g = (x*255)/ddsd.dwWidth;
		b = (x*255)/ddsd.dwWidth;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 6*(ddsd.dwHeight / 7.);
		rect.bottom = 7*(ddsd.dwHeight / 7.);
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	memcpy(buffer,bits,ddsd.lPitch*ddsd.dwHeight);
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
	free(bmi);
}
void GenScreen2(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface)
{
	OSVERSIONINFOA verinfo;
	verinfo.dwOSVersionInfoSize = sizeof(verinfo);
	GetVersionExA(&verinfo);
	bool gradientavailable;
	if(verinfo.dwMajorVersion > 4) gradientavailable = true;
	else if(verinfo.dwMajorVersion >= 4 && verinfo.dwMinorVersion >= 1) gradientavailable = true;
	else gradientavailable = false;
	int bitmode = BI_RGB;
	DWORD bitmasks[3];
	LPBYTE bits;
	BITMAPINFO *bmi = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER) + 1024);
	ZeroMemory(bmi,sizeof(BITMAPINFOHEADER) + 1024);
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		//GetSystemPaletteEntries(GetDC(hwnd),0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		LPDIRECTDRAWPALETTE pal;
		surface->GetPalette(&pal);
		pal->GetEntries(0,0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		pal->Release();
	}
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
		{
			bitmode = BI_BITFIELDS;
			bitmasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
			bitmasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
			bitmasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
			memcpy(bmi->bmiColors,bitmasks,3*sizeof(DWORD));
		}
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = ddsd.lPitch / (ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
	bmi->bmiHeader.biHeight = 0-ddsd.dwHeight;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biCompression = bitmode;
	bmi->bmiHeader.biBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
	HBITMAP bitmap = CreateDIBSection(GetDC(hwnd),bmi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	HDC hdcmem = CreateCompatibleDC(GetDC(hwnd));
	HGDIOBJ hbmold = SelectObject(hdcmem,bitmap);
	SetBkMode(hdcmem,TRANSPARENT);
	DWORD x;
	int r,g,b;
	RECT rect;
	HBRUSH color;
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = (x*255)/ddsd.dwWidth;
		g = 0;
		b = 0;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 0;
		rect.bottom = ddsd.dwHeight;
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	memcpy(buffer,bits,ddsd.lPitch*ddsd.dwHeight);
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
	free(bmi);
}
void GenScreen3(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface)
{
	OSVERSIONINFOA verinfo;
	verinfo.dwOSVersionInfoSize = sizeof(verinfo);
	GetVersionExA(&verinfo);
	bool gradientavailable;
	if(verinfo.dwMajorVersion > 4) gradientavailable = true;
	else if(verinfo.dwMajorVersion >= 4 && verinfo.dwMinorVersion >= 1) gradientavailable = true;
	else gradientavailable = false;
	int bitmode = BI_RGB;
	DWORD bitmasks[3];
	LPBYTE bits;
	BITMAPINFO *bmi = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER) + 1024);
	ZeroMemory(bmi,sizeof(BITMAPINFOHEADER) + 1024);
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		//GetSystemPaletteEntries(GetDC(hwnd),0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		LPDIRECTDRAWPALETTE pal;
		surface->GetPalette(&pal);
		pal->GetEntries(0,0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		pal->Release();
	}
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
		{
			bitmode = BI_BITFIELDS;
			bitmasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
			bitmasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
			bitmasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
			memcpy(bmi->bmiColors,bitmasks,3*sizeof(DWORD));
		}
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = ddsd.lPitch / (ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
	bmi->bmiHeader.biHeight = 0-ddsd.dwHeight;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biCompression = bitmode;
	bmi->bmiHeader.biBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
	HBITMAP bitmap = CreateDIBSection(GetDC(hwnd),bmi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	HDC hdcmem = CreateCompatibleDC(GetDC(hwnd));
	HGDIOBJ hbmold = SelectObject(hdcmem,bitmap);
	SetBkMode(hdcmem,TRANSPARENT);
	DWORD x;
	int r,g,b;
	RECT rect;
	HBRUSH color;
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = 0;
		g = (x*255)/ddsd.dwWidth;
		b = 0;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 0;
		rect.bottom = ddsd.dwHeight;
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	memcpy(buffer,bits,ddsd.lPitch*ddsd.dwHeight);
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
	free(bmi);
}
void GenScreen4(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface)
{
	OSVERSIONINFOA verinfo;
	verinfo.dwOSVersionInfoSize = sizeof(verinfo);
	GetVersionExA(&verinfo);
	bool gradientavailable;
	if(verinfo.dwMajorVersion > 4) gradientavailable = true;
	else if(verinfo.dwMajorVersion >= 4 && verinfo.dwMinorVersion >= 1) gradientavailable = true;
	else gradientavailable = false;
	int bitmode = BI_RGB;
	DWORD bitmasks[3];
	LPBYTE bits;
	BITMAPINFO *bmi = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER) + 1024);
	ZeroMemory(bmi,sizeof(BITMAPINFOHEADER) + 1024);
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		//GetSystemPaletteEntries(GetDC(hwnd),0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		LPDIRECTDRAWPALETTE pal;
		surface->GetPalette(&pal);
		pal->GetEntries(0,0,256,(LPPALETTEENTRY)&bmi->bmiColors);
		pal->Release();
	}
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
		{
			bitmode = BI_BITFIELDS;
			bitmasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
			bitmasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
			bitmasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
			memcpy(bmi->bmiColors,bitmasks,3*sizeof(DWORD));
		}
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = ddsd.lPitch / (ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
	bmi->bmiHeader.biHeight = 0-ddsd.dwHeight;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biCompression = bitmode;
	bmi->bmiHeader.biBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
	HBITMAP bitmap = CreateDIBSection(GetDC(hwnd),bmi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	HDC hdcmem = CreateCompatibleDC(GetDC(hwnd));
	HGDIOBJ hbmold = SelectObject(hdcmem,bitmap);
	SetBkMode(hdcmem,TRANSPARENT);
	DWORD x;
	int r,g,b;
	RECT rect;
	HBRUSH color;
	for(x = 0; x < ddsd.dwWidth; x++)
	{
		r = 0;
		g = 0;
		b = (x*255)/ddsd.dwWidth;
		rect.left = x;
		rect.right = x + 1;
		rect.top = 0;
		rect.bottom = ddsd.dwHeight;
		color = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdcmem,&rect,color);
		DeleteObject(color);
	}
	memcpy(buffer,bits,ddsd.lPitch*ddsd.dwHeight);
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
	free(bmi);
}

void GenScreen5(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen6(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen7(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen8(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen9(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
void GenScreen10(DDSURFACEDESC ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWSURFACE surface);
