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
#include "surfacegen.h"

inline int NextMultipleOf4(int number){return ((number+3) & (~3));}

void DrawPalette(DDSURFACEDESC2 ddsd, unsigned char *buffer)  // Palette test
{
	DWORD x,y;
	DWORD color;
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
			if((ddsd.ddpfPixelFormat.dwRBitMask | ddsd.ddpfPixelFormat.dwGBitMask |
				ddsd.ddpfPixelFormat.dwBBitMask) == 0x7FFF)
			{
				for(y = 0; y < ddsd.dwHeight; y++)
				{
					for(x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x+((ddsd.lPitch/2)*y)] = ((x/(ddsd.dwWidth/256.)) + 256*floor((y/(ddsd.dwHeight/256.))))/2;
					}
				}
			}
			else
			{
				for(y = 0; y < ddsd.dwHeight; y++)
				{
					for(x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x+((ddsd.lPitch/2)*y)] = (x/(ddsd.dwWidth/256.)) + 256*floor((y/(ddsd.dwHeight/256.)));
					}
				}
			}
			break;
		case 24:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth*3; x+=3)
				{
					color = ((x/3)/(ddsd.dwWidth/4096.)) + 4096*floor((y/(ddsd.dwHeight/4096.)));
					buffer[x+(ddsd.lPitch*y)] = color & 0xFF;
					buffer[(x+1)+(ddsd.lPitch*y)] = (color >> 8) & 0xFF;
					buffer[(x+2)+(ddsd.lPitch*y)] = (color >> 16) & 0xFF;
				}
			}
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


void DrawGradient(HDC hdc, int left, int right, int top, int bottom, DWORD color)
{
	int x;
	int r,g,b;
	RECT rect;
	HBRUSH brushcolor;
	for(x = left; x < right; x++)
	{
		r = (x*(color & 0xff)) / (right-left);
		g = (x*((color >> 8) & 0xff)) / (right-left);
		b = (x*((color >> 16) & 0xff)) / (right-left);
		rect.left = x;
		rect.right = x+1;
		rect.top = top;
		rect.bottom = bottom;
		brushcolor = CreateSolidBrush(RGB(r,g,b));
		FillRect(hdc,&rect,brushcolor);
		DeleteObject(brushcolor);
	}
}

void DrawGradients(DDSURFACEDESC2 ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWPALETTE palette, int type, DWORD color) // Gradients
{
	DWORD colors[1024];
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
		palette->GetEntries(0,0,256,(LPPALETTEENTRY)colors);
		for(int i = 0; i < 256; i++)
			colors[i] = ((colors[i]&0x0000FF)<<16) | (colors[i]&0x00FF00) | ((colors[i]&0xFF0000)>>16);
		memcpy(bmi->bmiColors,colors,1024);
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
	switch(type)
	{
	case 1:
		DrawGradient(hdcmem,0,ddsd.dwWidth,0,ddsd.dwHeight / 7,0x0000FF);
		DrawGradient(hdcmem,0,ddsd.dwWidth,ddsd.dwHeight / 7, 2*(ddsd.dwHeight/7),0x00FF00);
		DrawGradient(hdcmem,0,ddsd.dwWidth,2*(ddsd.dwHeight/7),3*(ddsd.dwHeight/7),0xFF0000);
		DrawGradient(hdcmem,0,ddsd.dwWidth,3*(ddsd.dwHeight/7),4*(ddsd.dwHeight/7),0xFFFF00);
		DrawGradient(hdcmem,0,ddsd.dwWidth,4*(ddsd.dwHeight/7),5*(ddsd.dwHeight/7),0xFF00FF);
		DrawGradient(hdcmem,0,ddsd.dwWidth,5*(ddsd.dwHeight/7),6*(ddsd.dwHeight/7),0x00FFFF);
		DrawGradient(hdcmem,0,ddsd.dwWidth,6*(ddsd.dwHeight/7),ddsd.dwHeight,0xFFFFFF);
		break;
	default:
		DrawGradient(hdcmem,0,ddsd.dwWidth,0,ddsd.dwHeight,color);
	}
	for(int i = 0; i < ddsd.dwHeight; i++)
	{
		memcpy(buffer+(ddsd.lPitch*i),bits+(NextMultipleOf4(ddsd.lPitch)*i),ddsd.lPitch);
	}
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
	free(bmi);
}