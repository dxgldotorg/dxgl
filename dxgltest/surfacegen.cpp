// DXGL
// Copyright (C) 2011-2013 William Feely

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

#ifndef GRADIENT_FILL_RECT_H
#define GRADIENT_FILL_RECT_H 0
#endif

inline int NextMultipleOf4(int number){return ((number+3) & (~3));}

const unsigned char dxgl_logo_bw[] = {
0x82,0x21,0x41,0x42,0x20,0x84,0x11,0x48,0x0a,0x30,0xe4,0x2e,0x92,0x50,0x91,
0x90,0x91,0x97,0x92,0x51,0xe4,0x2e,0x09,0x10,0x11,0x08,0x21,0x04,0x41,0x02,
0x81,0xe1
};


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
					buffer[x+(ddsd.lPitch*y)] = (unsigned char)((x/(ddsd.dwWidth/16.)) + 16*floor((y/(ddsd.dwHeight/16.))));
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
						buffer16[x+((ddsd.lPitch/2)*y)] = (unsigned short)((x/(ddsd.dwWidth/256.)) + 256*floor((y/(ddsd.dwHeight/256.))))/2;
					}
				}
			}
			else if((ddsd.ddpfPixelFormat.dwRBitMask | ddsd.ddpfPixelFormat.dwGBitMask |
				ddsd.ddpfPixelFormat.dwBBitMask) == 0xFFFF)
			{
				for(y = 0; y < ddsd.dwHeight; y++)
				{
					for(x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x+((ddsd.lPitch/2)*y)] = (unsigned short)((x/(ddsd.dwWidth/256.)) + 256*floor((y/(ddsd.dwHeight/256.))));
					}
				}
			}
			else
			{
				for(y = 0; y < ddsd.dwHeight; y++)
				{
					for(x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x+((ddsd.lPitch/2)*y)] = (unsigned short)((x/(ddsd.dwWidth/64.)) + 64*floor((y/(ddsd.dwHeight/64.))));
					}
				}
			}
			break;
		case 24:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth*3; x+=3)
				{
					color = (DWORD)(((x/3)/(ddsd.dwWidth/4096.)) + 4096*floor((y/(ddsd.dwHeight/4096.))));
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
					buffer32[x+((ddsd.lPitch/4)*y)] = (unsigned long)((x/(ddsd.dwWidth/4096.)) + 4096*floor((y/(ddsd.dwHeight/4096.))));
				}
			}
			return;
		default:
			return;
	}
}


void DrawGradient(HDC hdc, int left, int right, int top, int bottom, DWORD color, bool usegdi)
{
	int x;
	int r,g,b;
	RECT rect;
	HBRUSH brushcolor;
	TRIVERTEX vertex[2];
	GRADIENT_RECT grect;
	if(usegdi && gradientavailable)
	{
		vertex[0].x = left;
		vertex[1].x = right;
		vertex[0].y = top;
		vertex[1].y = bottom;
		vertex[0].Red = vertex[0].Green = vertex[0].Blue = 0;
		vertex[1].Red = (color & 0xff) << 8;
		vertex[1].Green = ((color >> 8) & 0xff) << 8;
		vertex[1].Blue = ((color >> 16) & 0xff) << 8;
		grect.UpperLeft = 0;
		grect.LowerRight = 1;
		_GradientFill(hdc,vertex,2,&grect,1,GRADIENT_FILL_RECT_H);
	}
	else
	{
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
}

void DrawGradients(DDSURFACEDESC2 ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWPALETTE palette, int type, DWORD color) // Gradients
{
	HDC hdcwin = GetDC(hwnd);
	DWORD colors[1024];
	int bitmode = BI_RGB;
	DWORD bitmasks[3];
	LPBYTE bits;
	BITMAPINFO *bmi = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER) + 1024);
	if(!bmi) return;
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
	bmi->bmiHeader.biBitCount = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
	HBITMAP bitmap = CreateDIBSection(hdcwin,bmi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	HDC hdcmem = CreateCompatibleDC(hdcwin);
	HGDIOBJ hbmold = SelectObject(hdcmem,bitmap);
	SetBkMode(hdcmem,TRANSPARENT);
	switch(type)
	{
	case 1:
		DrawGradient(hdcmem,0,ddsd.dwWidth,0,ddsd.dwHeight / 7,0x0000FF,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,ddsd.dwHeight / 7, 2*(ddsd.dwHeight/7),0x00FF00,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,2*(ddsd.dwHeight/7),3*(ddsd.dwHeight/7),0xFF0000,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,3*(ddsd.dwHeight/7),4*(ddsd.dwHeight/7),0xFFFF00,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,4*(ddsd.dwHeight/7),5*(ddsd.dwHeight/7),0xFF00FF,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,5*(ddsd.dwHeight/7),6*(ddsd.dwHeight/7),0x00FFFF,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,6*(ddsd.dwHeight/7),ddsd.dwHeight,0xFFFFFF,false);
		break;
	default:
		DrawGradient(hdcmem,0,ddsd.dwWidth,0,ddsd.dwHeight,color,false);
	}
	for(unsigned int i = 0; i < ddsd.dwHeight; i++)
	{
		memcpy(buffer+(ddsd.lPitch*i),bits+(NextMultipleOf4(ddsd.lPitch)*i),ddsd.lPitch);
	}
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
	ReleaseDC(hwnd,hdcwin);
	free(bmi);
}

const TCHAR ArialName[] = _T("Arial");
const TCHAR TimesName[] = _T("Times New Roman");
const TCHAR CourierName[] = _T("Courier New");
const TCHAR SSerifName[] = _T("MS Sans Serif");
const TCHAR SymbolName[] = _T("Symbol");
const TCHAR WingdingName[] = _T("Wingdings");

const TCHAR space[] = _T(" ");

const TCHAR regularname[] = _T(" Regular");
const TCHAR italicname[] = _T(" Italic");
const TCHAR boldname[] = _T(" Bold");

void DrawTextBlock(HDC hDC, DWORD x, DWORD &y, DWORD bold, BOOL italic, LPCTSTR font)
{
	TCHAR str[256];
	TCHAR num[32];
	TEXTMETRIC tm;
	HFONT DefaultFont;
	HFONT newfont;
	for(int i = -8; i > -25; i-=2)
	{
		newfont = CreateFont(i,0,0,0,bold,italic,FALSE,FALSE,DEFAULT_CHARSET,
			OUT_OUTLINE_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE,font);
		DefaultFont = (HFONT)SelectObject(hDC,newfont);
		_tcscpy(str,font);
		_tcscat(str,space);
		_itot(-i,num,10);
		_tcscat(str,num);
		if(!italic && bold == FW_NORMAL) _tcscat(str,regularname);
		if(bold == FW_BOLD) _tcscat(str,boldname);
		if(italic) _tcscat(str,italicname);
		TextOut(hDC,x,y,str,_tcslen(str));
		GetTextMetrics(hDC,&tm);
		y += tm.tmHeight;
		newfont = (HFONT)SelectObject(hDC,DefaultFont);
		DeleteObject(newfont);
		if(i == -14) i -= 2;
		if(i == -18) i -= 4;
	}
}

POINT AngleToPoint(int x, int y, int width, int height, double angle)
{
	POINT ret;
	ret.x = x + (int)((double)width*sin(angle));
	ret.y = y + (int)((double)height*cos(angle));
	return ret;
}

void CreatePolygon(POINT *array, int sides, int x, int y, int width, int height)
{
	double angle = 0.;
	for(int i = 0; i < sides; i++)
	{
		angle = ((atan(1.)*8.)/(double)sides)*(double)i;
		array[i] = AngleToPoint(x,y,width,height,angle);
	}
}

void CreatePattern(HDC hdc, HBRUSH *brush, HPEN *pen, HBITMAP *bmp, int number)
{
	switch(number)
	{
	case 0:
	default:
		*pen = CreatePen(PS_SOLID,1,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,0,0));
		break;
	case 1:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,255,0));
		break;
	case 2:
		*pen = CreatePen(PS_SOLID,3,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,0,255));
		break;
	case 3:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,255,255));
		break;
	case 4:
		*pen = CreatePen(PS_SOLID,1,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,0,255));
		break;
	case 5:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,255,0));
		break;
	case 6:
		*pen = CreatePen(PS_SOLID,3,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,255,255));
		break;
	case 7:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,0,255));
		break;
	case 8:
		SetBkColor(hdc,RGB(0,0,255));
		*pen = CreatePen(PS_DOT,1,RGB(255,255,0));
		*brush = CreateHatchBrush(HS_BDIAGONAL,RGB(0,255,255));
		break;
	case 9:
		SetBkColor(hdc,RGB(255,0,0));
		*pen = CreatePen(PS_DASH,1,RGB(0,255,255));
		*brush = CreateHatchBrush(HS_CROSS,RGB(255,0,255));
		break;
	case 10:
		SetBkColor(hdc,RGB(0,255,0));
		*pen = CreatePen(PS_DASHDOT,1,RGB(255,0,255));
		*brush = CreateHatchBrush(HS_DIAGCROSS,RGB(255,255,0));
		break;
	case 11:
		SetBkColor(hdc,RGB(0,0,255));
		*pen = CreatePen(PS_DASHDOTDOT,1,RGB(0,255,0));
		*brush = CreateHatchBrush(HS_FDIAGONAL,RGB(0,255,0));
		break;
	case 12:
		SetBkColor(hdc,RGB(0,255,0));
		*pen = CreatePen(PS_DOT,1,RGB(0,0,255));
		*brush = CreateHatchBrush(HS_HORIZONTAL,RGB(255,0,0));
		break;
	case 13:
		SetBkColor(hdc,RGB(0,0,0));
		*pen = CreatePen(PS_DASH,1,RGB(255,0,0));
		*brush = CreateHatchBrush(HS_VERTICAL,RGB(0,0,255));
		break;
	case 14:
		SetBkColor(hdc,RGB(255,255,255));
		*bmp = CreateBitmap(16,16,1,1,dxgl_logo_bw);
		*pen = CreatePen(PS_SOLID,2,RGB(192,192,192));
		*brush = CreatePatternBrush(*bmp);
		break;
	case 15:
		*bmp = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_DXGLINV));
		*pen = CreatePen(PS_SOLID,2,RGB(192,192,192));
		*brush = CreatePatternBrush(*bmp);
		break;
	}
}
void DestroyPattern(HBRUSH *brush, HPEN *pen, HBITMAP *bmp, int number)
{
	if(number >= 14) DeleteObject(*bmp);
	DeleteObject(*pen);
	DeleteObject(*brush);
	*bmp = NULL;
	*brush = NULL;
	*pen = NULL;
}

void DrawBitmap(HDC hDC, int x, int y, int w, int h, LPCTSTR bmp, DWORD method)
{
	BITMAP bminfo;
	HBITMAP bitmap = LoadBitmap(GetModuleHandle(NULL),bmp);
	HDC hdcmem = CreateCompatibleDC(hDC);
	HBITMAP hbmold = (HBITMAP)SelectObject(hdcmem,bitmap);
	GetObject(bitmap,sizeof(bminfo),&bminfo);
	if((w == bminfo.bmWidth) && (h == bminfo.bmHeight)) BitBlt(hDC,x,y,w,h,hdcmem,0,0,method);
	else StretchBlt(hDC,x,y,w,h,hdcmem,0,0,bminfo.bmWidth,bminfo.bmHeight,method);
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
}

void DrawGDIPatterns(DDSURFACEDESC2 ddsd, HDC hDC, int type)
{
	int i;
	DWORD x,x2;
	DWORD y,y2;
	int r,g,b;
	HPEN pen;
	HBRUSH brush;
	HBITMAP bmp;
	HANDLE tmphandle, tmphandle2;
	POINT points[18];
	RECT rect;
	switch(type)
	{
	case 0: // Text
		y = y2 = 16;
		SetBkColor(hDC,RGB(0,0,0));
		SetTextColor(hDC,RGB(255,0,0));
		DrawTextBlock(hDC,0,y,FW_NORMAL,FALSE,ArialName);
		SetTextColor(hDC,RGB(0,255,0));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_NORMAL,TRUE,ArialName);
		SetTextColor(hDC,RGB(0,0,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,FALSE,ArialName);
		SetTextColor(hDC,RGB(0,255,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,TRUE,ArialName);
		if(ddsd.dwWidth > 216)
		{
			y2 = y;
			y = 16;
			SetTextColor(hDC,RGB(255,0,255));
			DrawTextBlock(hDC,216,y,FW_NORMAL,FALSE,TimesName);
			SetTextColor(hDC,RGB(255,255,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,216,y,FW_NORMAL,TRUE,TimesName);
			SetTextColor(hDC,RGB(255,255,255));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,216,y,FW_BOLD,FALSE,TimesName);
			SetTextColor(hDC,RGB(255,127,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,216,y,FW_BOLD,TRUE,TimesName);
		}
		y = y2;
		SetTextColor(hDC,RGB(0,127,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_NORMAL,FALSE,CourierName);
		SetTextColor(hDC,RGB(127,255,0));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_NORMAL,TRUE,CourierName);
		SetTextColor(hDC,RGB(127,0,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,FALSE,CourierName);
		SetTextColor(hDC,RGB(255,0,127));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,TRUE,CourierName);
		y = y2;
		if(ddsd.dwWidth > 380)
		{
			SetTextColor(hDC,RGB(0,255,127));
			DrawTextBlock(hDC,380,y,FW_NORMAL,FALSE,SSerifName);
			SetTextColor(hDC,RGB(127,127,255));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,380,y,FW_NORMAL,TRUE,SSerifName);
			SetTextColor(hDC,RGB(255,127,127));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,380,y,FW_BOLD,FALSE,SSerifName);
			SetTextColor(hDC,RGB(255,127,255));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,380,y,FW_BOLD,TRUE,SSerifName);
		}
		y = 16;
		if(ddsd.dwWidth > 560)
		{
			SetTextColor(hDC,RGB(191,0,0));
			DrawTextBlock(hDC,560,y,FW_NORMAL,FALSE,SymbolName);
			SetTextColor(hDC,RGB(0,191,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,560,y,FW_NORMAL,TRUE,SymbolName);
			SetTextColor(hDC,RGB(0,0,191));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,560,y,FW_BOLD,FALSE,SymbolName);
			SetTextColor(hDC,RGB(0,191,191));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,560,y,FW_BOLD,TRUE,SymbolName);
		}
		if((ddsd.dwWidth > 720) && (ddsd.dwHeight > y))
		{
			SetTextColor(hDC,RGB(191,0,191));
			DrawTextBlock(hDC,720,y,FW_NORMAL,FALSE,WingdingName);
			SetTextColor(hDC,RGB(191,191,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,720,y,FW_NORMAL,TRUE,WingdingName);
			SetTextColor(hDC,RGB(191,191,191));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,720,y,FW_BOLD,FALSE,WingdingName);
			SetTextColor(hDC,RGB(127,127,127));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,720,y,FW_BOLD,TRUE,WingdingName);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 0:  Text"),15);
		break;
	case 1: // Lines
		r = g = b = 0;
		points[0].x = ddsd.dwWidth/2;
		points[0].y = ddsd.dwHeight/2;
		points[1].y = 0;
		for(x = 1; x <= 10; x++)
		{
			r = (int)(x * 25.5);
			points[1].x = (LONG)(ddsd.dwWidth * (x/10.));
			pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		for(y = 0; y <= 10; y++)
		{
			r = (int)((10-y) * 25.5);
			g = (int)(y * 25.5);
			points[1].y = (LONG)(ddsd.dwHeight * (y/10.));
			pen = CreatePen(PS_SOLID,y,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		for(x = 10; x > 0; x--)
		{
			g = (int)(x * 25.5);
			b = (int)((10-x) * 25.5);
			points[1].x = (LONG)(ddsd.dwWidth * (x/10.));
			pen = CreatePen(PS_SOLID,x,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
			if(x == 0) break;
		}
		SetBkColor(hDC,RGB(0,0,0));
		for(y = 10; y >= 0; y--)
		{
			r = g = (int)((10-y) * 25.5);
			points[1].y = (LONG)(ddsd.dwHeight * (y/10.));
			switch(y)
			{
			case 1:
			case 6:
			default:
				pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
				break;
			case 2:
			case 7:
				pen = CreatePen(PS_DASH,0,RGB(r,g,b));
				break;
			case 3:
			case 8:
				pen = CreatePen(PS_DOT,0,RGB(r,g,b));
				break;
			case 4:
			case 9:
				pen = CreatePen(PS_DASHDOT,0,RGB(r,g,b));
				break;
			case 5:
			case 10:
				pen = CreatePen(PS_DASHDOTDOT,0,RGB(r,g,b));
				break;
			}
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
			if(y == 0) break;
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 1:  Lines"),16);
		break;
	case 2: // Beziers
		points[0].x = 0;
		points[1].x = ddsd.dwWidth / 4;
		points[2].x = 3*(ddsd.dwWidth/4);
		points[3].x = ddsd.dwWidth;
		points[0].y = points[3].y = ddsd.dwHeight / 2;
		r = g = b = 0;
		for(i = 0; i < 50; i++)
		{
			r = (int)(i*5.1);
			points[1].y = (LONG)((ddsd.dwHeight/2.)+(i*(ddsd.dwHeight/30.)));
			points[2].y = (LONG)((ddsd.dwHeight/2.)-(i*(ddsd.dwHeight/30.)));
			pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			PolyBezier(hDC,points,4);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		r = 0;
		for(i = 0; i < 50; i++)
		{
			g = b = (int)(i*5.1);
			points[1].y = (LONG)((ddsd.dwHeight/2.)-(i*(ddsd.dwHeight/30.)));
			points[2].y = (LONG)((ddsd.dwHeight/2.)+(i*(ddsd.dwHeight/30.)));
			pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			PolyBezier(hDC,points,4);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 2:  Beziers"),18);
		break;
	case 3: // polygons
		x2 = (int)(ddsd.dwWidth / 8.1);
		y2 = (int)(ddsd.dwHeight / 8.1);
		for(i = 0; i < 16; i++)
		{
			x = (int)(((i&3) * (ddsd.dwWidth/4.))+(ddsd.dwWidth/8));
			y = (int)(((i>>2) * (ddsd.dwHeight/4.))+(ddsd.dwHeight/8));
			CreatePolygon(points,i+3,x,y,x2,y2);
			CreatePattern(hDC,&brush,&pen,&bmp,i);
			tmphandle = SelectObject(hDC,pen);
			tmphandle2 = SelectObject(hDC,brush);
			Polygon(hDC,points,i+3);
			SelectObject(hDC,tmphandle2);
			SelectObject(hDC,tmphandle);
			DestroyPattern(&brush,&pen,&bmp,i);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 3:  Polygons"),19);
		break;
	case 4: // rectangles
		for(i = 0; i < 16; i++)
		{
			rect.left = (int)((double)(i&3)*(double)ddsd.dwWidth/4.);
			rect.right = rect.left+(int)(ddsd.dwWidth/4.)-8;
			rect.top = (int)((double)(i>>2)*(double)ddsd.dwHeight/4);
			rect.bottom = rect.top+(int)(ddsd.dwHeight/4.)-8;
			CreatePattern(hDC,&brush,&pen,&bmp,i);
			tmphandle = SelectObject(hDC,pen);
			tmphandle2 = SelectObject(hDC,brush);
			if(i <= 7) FillRect(hDC,&rect,brush);
			else RoundRect(hDC,rect.left,rect.top,rect.right,rect.bottom,i+8,24-i);
			SelectObject(hDC,tmphandle2);
			SelectObject(hDC,tmphandle);
			DestroyPattern(&brush,&pen,&bmp,i);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 4:  Rectangles"),21);
		break;
	case 5: // ellipses
		for(i = 0; i < 16; i++)
		{
			rect.left = (int)((double)(i&3)*(double)ddsd.dwWidth/4.);
			rect.right = rect.left+(int)(ddsd.dwWidth/4.)-8;
			rect.top = (int)((double)(i>>2)*(double)ddsd.dwHeight/4);
			rect.bottom = rect.top+(int)(ddsd.dwHeight/4.)-8;
			x = (rect.right-rect.left)/4;
			y = (rect.bottom-rect.top)/4;
			CreatePattern(hDC,&brush,&pen,&bmp,i);
			tmphandle = SelectObject(hDC,pen);
			tmphandle2 = SelectObject(hDC,brush);
			switch(i)
			{
			case 0:
			default:
				Ellipse(hDC,rect.left,rect.top,rect.right,rect.bottom);
				break;
			case 1:
				Ellipse(hDC,rect.left,rect.top+y,rect.right,rect.bottom-y);
				break;
			case 2:
				Ellipse(hDC,rect.left+x,rect.top,rect.right-x,rect.bottom);
				break;
			case 3:
				Ellipse(hDC,rect.left+x,rect.top+y,rect.right-x,rect.bottom-y);
				break;
			case 4:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left+(2*x),rect.top,rect.left+(2*x),rect.bottom);
				break;
			case 5:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top+(2*y),rect.right,rect.top+(2*y));
				break;
			case 6:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top,rect.right,rect.bottom);
				break;
			case 7:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+x,rect.bottom);
				break;
			case 8:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+(2*x),rect.bottom);
				break;
			case 9:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top,rect.left,rect.bottom);
				break;
			case 10:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top,rect.right,rect.bottom);
				break;
			case 11:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+x,rect.bottom);
				break;
			case 12:
				RoundRect(hDC,rect.left,rect.top,rect.right,rect.bottom,ddsd.dwWidth/5,ddsd.dwHeight/5);
				break;
			case 13:
				RoundRect(hDC,rect.left,rect.top,rect.right,rect.bottom,ddsd.dwWidth/8,ddsd.dwHeight/8);
				break;
			case 14:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+x,rect.bottom);
				break;
			case 15:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left+x,rect.bottom,rect.right,rect.top);
				break;
			}
			SelectObject(hDC,tmphandle2);
			SelectObject(hDC,tmphandle);
			DestroyPattern(&brush,&pen,&bmp,i);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 5:  Ellipses"),19);
		break;
	case 6: // gradients
		DrawGradient(hDC,0,ddsd.dwWidth,0,ddsd.dwHeight / 7,0x0000FF,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,ddsd.dwHeight / 7, 2*(ddsd.dwHeight/7),0x00FF00,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,2*(ddsd.dwHeight/7),3*(ddsd.dwHeight/7),0xFF0000,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,3*(ddsd.dwHeight/7),4*(ddsd.dwHeight/7),0xFFFF00,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,4*(ddsd.dwHeight/7),5*(ddsd.dwHeight/7),0xFF00FF,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,5*(ddsd.dwHeight/7),6*(ddsd.dwHeight/7),0x00FFFF,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,6*(ddsd.dwHeight/7),ddsd.dwHeight,0xFFFFFF,gradientavailable);
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 6:  Gradients"),20);
		break;
	case 7: // bitmaps
		DrawBitmap(hDC,(int)(ddsd.dwWidth/8.)-8,(int)(ddsd.dwHeight/6.)-8,
			16,16,MAKEINTRESOURCE(IDB_DXGLINV),SRCCOPY);
		DrawBitmap(hDC,(int)((ddsd.dwWidth/4.)+(ddsd.dwWidth/8.))-32,(int)(ddsd.dwHeight/6.)-32,
			64,64,MAKEINTRESOURCE(IDB_DXGLINV64),SRCCOPY);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/2.),0,(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV),SRCCOPY);
		DrawBitmap(hDC,(int)(.75*ddsd.dwWidth),0,(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV64),SRCCOPY);
		SetBkColor(hDC,RGB(0,255,255));
		brush = CreateHatchBrush(HS_DIAGCROSS,RGB(128,0,128));
		tmphandle = SelectObject(hDC,brush);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/8.)-8,(int)(ddsd.dwHeight/2.)-8,
			16,16,MAKEINTRESOURCE(IDB_DXGLINV),MERGECOPY);
		DrawBitmap(hDC,(int)((ddsd.dwWidth/4.)+(ddsd.dwWidth/8.))-32,(int)(ddsd.dwHeight/2.)-32,
			64,64,MAKEINTRESOURCE(IDB_DXGLINV64),MERGECOPY);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/2.),(int)(ddsd.dwHeight/3.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV),MERGECOPY);
		DrawBitmap(hDC,(int)(.75*ddsd.dwWidth),(int)(ddsd.dwHeight/3.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV64),MERGECOPY);
		SelectObject(hDC,tmphandle);
		DeleteObject(brush);
		rect.left = 0;
		rect.right = ddsd.dwWidth;
		rect.top = (int)((ddsd.dwHeight/3.)*2.);
		rect.bottom = ddsd.dwHeight;
		FillRect(hDC,&rect,(HBRUSH) (COLOR_WINDOW+1));
		DrawBitmap(hDC,(int)(ddsd.dwWidth/8.)-8,(int)((5./6.)*ddsd.dwHeight)-8,
			16,16,MAKEINTRESOURCE(IDB_DXGLINV),SRCINVERT);
		DrawBitmap(hDC,(int)((ddsd.dwWidth/4.)+(ddsd.dwWidth/8.))-32,(int)((5./6.)*ddsd.dwHeight)-32,
			64,64,MAKEINTRESOURCE(IDB_DXGLINV64),SRCINVERT);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/2.),(int)((ddsd.dwHeight/3.)*2.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV),SRCINVERT);
		DrawBitmap(hDC,(int)(.75*ddsd.dwWidth),(int)((ddsd.dwHeight/3.)*2.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV64),SRCINVERT);
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 7:  Bitmaps"),18);
		break;
	}
}
