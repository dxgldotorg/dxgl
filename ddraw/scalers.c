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
#include "scalers.h"
#include <WinDef.h>


void ScaleNearest8(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch)
{
	BYTE *d = (BYTE *)dest;
	BYTE *s = (BYTE *)src;
	int rx = (int)((sw<<16)/dw)+1;
	int ry = (int)((sh<<16)/dh)+1;
	int x,y;
	int x2,y2;
	int b1,b2;
	for(y = 0; y < dh; y++)
	{
		b1 = y*outpitch;
		y2 = ((y*ry)>>16);
		b2 = y2*inpitch;
		for(x = 0; x < dw; x++)
		{
			x2 = ((x*rx)>>16);
			y2 = ((y*ry)>>16);
			d[b1+x] = s[b2+x2];
		}
	}
}
void ScaleNearest16(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch)
{
	WORD *d = (WORD *)dest;
	WORD *s = (WORD *)src;
	int rx = (int)((sw<<16)/dw)+1;
	int ry = (int)((sh<<16)/dh)+1;
	int x,y;
	int x2,y2;
	int b1,b2;
	for(y = 0; y < dh; y++)
	{
		b1 = y*outpitch;
		y2 = ((y*ry)>>16);
		b2 = y2*inpitch;
		for(x = 0; x < dw; x++)
		{
			x2 = ((x*rx)>>16);
			y2 = ((y*ry)>>16);
			d[b1+x] = s[b2+x2];
		}
	}
}
void ScaleNearest24(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch)
{
	RGBTRIPLE *d = (RGBTRIPLE *)dest;
	RGBTRIPLE *s = (RGBTRIPLE *)src;
	char *d8 = (char*)dest;
	char *s8 = (char*)src;
	int rx = (int)((sw<<16)/dw)+1;
	int ry = (int)((sh<<16)/dh)+1;
	int x,y;
	int x2,y2;
	for(y = 0; y < dh; y++)
	{
		d8 = (y*outpitch)+(char*)dest;
		y2 = ((y*ry)>>16);
		s8 = (y2*inpitch)+(char*)src;
		d = (RGBTRIPLE*)d8;
		s = (RGBTRIPLE*)s8;
		for(x = 0; x < dw; x++)
		{
			x2 = ((x*rx)>>16);
			y2 = ((y*ry)>>16);
			d[x] = s[x2];
		}
	}
}
void ScaleNearest32(void *dest, void *src, int dw, int dh, int sw, int sh, int inpitch, int outpitch)
{
	DWORD *d = (DWORD *)dest;
	DWORD *s = (DWORD *)src;
	int rx = (int)((sw<<16)/dw)+1;
	int ry = (int)((sh<<16)/dh)+1;
	int x,y;
	int x2,y2;
	int b1,b2;
	for(y = 0; y < dh; y++)
	{
		b1 = y*outpitch;
		y2 = ((y*ry)>>16);
		b2 = y2*inpitch;
		for(x = 0; x < dw; x++)
		{
			x2 = ((x*rx)>>16);
			d[b1+x] = s[b2+x2];
		}
	}
}
