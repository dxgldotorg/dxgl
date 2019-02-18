// DXGL
// Copyright (C) 2018-2019 William Feely

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
#include "colorconv.h"

typedef void(*COLORCONVPROC) (size_t count, void *dest, void *src);
COLORCONVPROC colorconvproc[] =
{
	rgba8332torgba8888, // 0
	rgba8888torgba8332, // 1
	rgb565torgba8888,   // 2
	rgb565torgbx8888,   // 3
	rgbx8888torgb565,   // 4
	rgba1555torgba8888, // 5
	rgba8888torgba1555, // 6
	rgba4444torgba8888, // 7
	rgba8888torgba4444, // 8
	uyvytorgbx8888,     // 9
	rgbx8888touyvy,     // 10
	pal1topal8,         // 11
	pal2topal8,         // 12
	pal4topal8,         // 13
	pal8topal1,         // 14
	pal8topal2,         // 15
	pal8topal4,         // 16
};

__inline unsigned int _1to8(unsigned int input)
{
	return input * 255;
}

__inline unsigned int _2to8(unsigned int input)
{
	return input * 85;
}

__inline unsigned int _3to8(unsigned int input)
{
	return (input * 146 + 1) >> 2;
}

__inline unsigned int _4to8(unsigned int input)
{
	return input * 17;
}

__inline unsigned int _5to8(unsigned int input)
{
	return (input * 527 + 23) >> 6;
}

__inline unsigned int _6to8(unsigned int input)
{
	return (input * 259 + 33) >> 6;
}

_inline unsigned int _8to4(unsigned int input)
{
	return input >> 4;
}

_inline unsigned int _8to2(unsigned int input)
{
	return input >> 6;
}

_inline unsigned int _8to1(unsigned int input)
{
	return input >> 7;
}

void pal1topal8(size_t count, DWORD *dest, BYTE *src)
{
	size_t i;
	for (i = 0; i < (count >> 2); i += 2)
	{
		dest[i] = (_1to8(src[i >> 1] >> 7) | (_1to8((src[i >> 1] >> 6) & 1) << 8)
			| (_1to8((src[i >> 1] >> 5) & 1) << 16) | (_1to8((src[i >> 1] >> 4) & 1) << 24));
		dest[i + 1] = (_1to8((src[i >> 1] >> 3) & 1) | (_1to8((src[i >> 1] >> 2) & 1) << 8)
			| (_1to8((src[i >> 1] >> 1) & 1) << 16) | (_1to8((src[i >> 1]) & 1) << 24));
	}
	if (count & 4)
	{
		dest[count >> 2] = (_1to8(src[count >> 4] >> 7) | (_1to8((src[count >> 4] >> 6) & 1) << 8)
			| (_1to8((src[count >> 4] >> 5) & 1) << 16) | (_1to8((src[count >> 4] >> 4) & 1) << 24));
	}
}

void pal2topal8(size_t count, DWORD *dest, BYTE *src)
{
	size_t i;
	for (i = 0; i < (count >> 2); i ++)
		dest[i] = (_2to8(src[i] >> 6) | (_2to8((src[i] >> 4) & 3) << 8)
			| (_2to8((src[i] >> 2) & 3) << 16) | (_2to8((src[i] & 3)) << 24));
}

void pal4topal8(size_t count, WORD *dest, BYTE *src)
{
	size_t i;
	for (i = 0; i < (count >> 1); i++)
		dest[i] = (_4to8(src[i] >> 4) | (_4to8(src[i] & 15) << 8));
}

void pal8topal4(size_t count, BYTE *dest, WORD *src)
{
	size_t i;
	for (i = 0; i < count >> 1; i++)
		dest[i] = ((_8to4(src[i] & 255) << 4) | _8to4(src[i] >> 8));
}

void pal8topal2(size_t count, BYTE *dest, DWORD *src)
{
	size_t i;
	for (i = 0; i < count >> 2; i++)
		dest[i] = ((_8to2(src[i] & 255) << 6) | (_8to2((src[i] >> 8) & 255) << 4)
			| (_8to2((src[i] >> 16) & 255) << 2) | _8to2(src[i] >> 24));
}

void pal8topal1(size_t count, BYTE *dest, DWORD *src)
{
	size_t i;
	for (i = 0; i < (count >> 2); i += 2)
	{
		dest[i >> 1] = ((_8to1(src[i] & 255) << 7) | (_8to1((src[i] >> 8) & 255) << 6)
			| (_8to1((src[i] >> 16) & 255) << 5) | (_8to1(src[i] >> 24) << 4)
			| (_8to1(src[i + 1] & 255) << 3) | (_8to1((src[i + 1] >> 8) & 255) << 2)
			| (_8to1((src[i + 1]) >> 16) << 1) | _8to1(src[i + 1] >> 24));
	}
	if (count & 4)
	{
		dest[count >> 2] = ((_8to1(src[i] & 255) << 7) | (_8to1((src[i] >> 8) & 255) << 6)
			| (_8to1((src[i] >> 16) & 255) << 5) | (_8to1(src[i] >> 24) << 4));
	}
}

void rgba8332torgba8888(size_t count, DWORD *dest, WORD *src)
{
	size_t i;
	WORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = ((in & 0xFF00) << 16) | (_3to8((in & 0xE0) >> 5) << 16) |
			(_3to8((in & 0x1C) >> 2) << 8) | _2to8(in & 0x3);
	}
}

void rgba8888torgba8332(size_t count, WORD *dest, DWORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = ((in & 0xFF000000) >> 16) | ((in & 0xE00000) >> 16) |
			((in & 0xE000) >> 11) | ((in & 0xC0) >> 6);
	}
}

void rgb565torgba8888(size_t count, DWORD *dest, WORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = 0xFF000000 | (_5to8((in & 0xF800) >> 11) << 16) |
			(_6to8((in & 0x7E0) >> 5) << 8) | _5to8(in & 0x1F);
	}
}

void rgb565torgbx8888(size_t count, DWORD *dest, WORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = (_5to8((in & 0xF800) >> 11) << 16) |
			(_6to8((in & 0x7E0) >> 5) << 8) | _5to8(in & 0x1F);
	}
}

void rgbx8888torgb565(size_t count, WORD *dest, DWORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = ((in & 0xF80000) >> 8) | ((in & 0xFC00) >> 5)
			| ((in & 0xF8) >> 3);
	}
}

void rgba1555torgba8888(size_t count, DWORD *dest, WORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = (_1to8((in & 0x8000) >> 15) << 24) | (_5to8((in & 0x7C00) >> 10) << 16) |
			(_5to8((in & 0x3E0) >> 5) << 8) | _5to8(in & 0x1F);
	}
}

void rgba8888torgba1555(size_t count, WORD *dest, DWORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = ((in & 0x80000000) >> 16) | ((in & 0xF80000) >> 9) |
			((in & 0xF800) >> 6) | ((in & 0xF8) >> 3);
	}
}

void rgba4444torgba8888(size_t count, DWORD *dest, WORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = (_4to8((in & 0xF000) >> 12) << 24) | (_4to8((in & 0xF00) >> 8) << 16) |
			(_4to8((in & 0xF0) >> 4) << 8) | _4to8(in & 0xF);
	}
}

void rgba8888torgba4444(size_t count, WORD *dest, DWORD *src)
{
	size_t i;
	DWORD in;
	for (i = 0; i < count; i++)
	{
		in = src[i];
		dest[i] = ((in & 0xF0000000) >> 16) | ((in & 0xF00000) >> 12) |
			((in & 0xF000) >> 8) | ((in & 0xF0) >> 4);
	}
}

__inline DWORD yuvtorgb(DWORD y, DWORD u, DWORD v)
{
	float r, g, b;
	r = y + 1.402f * v;
	g = y - 0.344f * u - 0.714f * v;
	b = y + 1.772f * u;
	if (r > 255.0f) r = 255.0f;
	if (r < 0.0f) r = 0.0f;
	return ((DWORD)r << 16) | ((DWORD)g << 8) || (DWORD)b;
}

void uyvytorgbx8888(size_t count, DWORD *dest, DWORD *src)
{
	size_t i;
	DWORD in;
	DWORD y, u, v;
	for (i = 0; i < (count << 1); i++)
	{
		in = src[i];
		// first pixel
		y = (src[i] >> 8) & 0xFF;
		u = src[i] & 0xFF;
		v = (src[i] >> 16) & 0xFF;
		dest[i << 1] = yuvtorgb(y, u, v);
		// second pixel
		y = (src[i] >> 24) & 0xFF;
		dest[(i << 1)+1] = yuvtorgb(y, u, v);
	}
}

void rgbx8888touyvy(size_t count, DWORD *dest, DWORD *src)
{

}