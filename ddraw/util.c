// DXGL
// Copyright (C) 2013-2017 William Feely

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
#include "util.h"
#include "const.h"

#ifdef _MSC_VER
#pragma optimize("g", off)
#endif
extern DXGLCFG dxglcfg;
/**
  * Tests if a pointer is valid for reading from.  Uses SEH on Visual C++,
  * non-recommended Windows API on other systems.
  * @param ptr
  *  Pointer to test for validity.
  * @param size
  *  Size of block to check
  * @return
  *  Returns non-zero if the pointer is valid, or zero if an error occurs.
  */
char IsReadablePointer(void *ptr, LONG_PTR size)
{
	char a;
	char *ptr2 = ptr;
	if(!ptr) return 0;
#ifdef _MSC_VER
	__try
	{
		a = ptr2[0];
		if (!a) a++;
		if (size > 1) a = ptr2[size-1];
		if (!a) a++;
		return a;
	}
	__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
	{
		return 0;
	}
#else
	if(IsBadReadPtr(ptr,size)) return 0;
	else return 1;
#endif
}

/**
* Tests if a pointer is valid for writing to.  Uses SEH on Visual C++,
* non-recommended Windows API on other systems.
* @param ptr
*  Pointer to test for validity.
* @param size
*  Size of block to check
* @param preserve
*  TRUE to preserve the contents of the pointer.
* @return
*  Returns false if the pointer is valid, or true if an error occurs.
*/
char IsWritablePointer(void *ptr, LONG_PTR size, BOOL preserve)
{
	char a;
	char *ptr2 = ptr;
	if (!ptr) return 0;
#ifdef _MSC_VER
	__try
	{
		if (preserve) a = ptr2[0];
		else a = 1;
		ptr2[0] = a + 1;
		if (preserve) ptr2[0] = a;
		if (size > 1)
		{
			if (preserve) a = ptr2[size-1];
			ptr2[size-1] = a + 1;
			if (preserve) ptr2[size-1] = a;
		}
		return 1;
	}
	__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
	{
		return 0;
	}
#else
	if (IsBadWritePtr(ptr, 1)) return 0;
	else return 1;
#endif
}	

void AndMem(void *dest, const void *a, const void *b, size_t size)
{
	// Buffer must be multiple of 4 and should be DWORD aligned
	size_t i;
	for (i = 0; i < size>>2; i++)
	{
		((DWORD*)dest)[i] = ((DWORD*)a)[i] & ((DWORD*)b)[i];
	}
}

const DDBLTFX cmp_fx =
{
	0xFFFFFFFF, // dwSize
	0x00000180, // dwDDFX & (DDBLTFX_ZBUFFERBASEDEST | DDBLTFX_ZBUFFERRANGE)
	0xFFFFFFFF, // dwROP
	0xFFFFFFFF, // dwDDROP
	0x00000000, // dwRotationAngle
	0xFFFFFFFF, // dwZBufferOpCode
	0x00000000, // dwZBufferLow
	0x00000000, // dwZBufferHigh
	0x00000000, // dwZBufferBaseDest
	0xFFFFFFFF, // dwZDestConstBitDepth
	0x00000000, // dwZDestConst and lpDDSZBufferDest
	0xFFFFFFFF, // dwZSrcConstBitDepth
	0x00000000, // dwZSrcConst and lpDDSZBufferSrc
	0xFFFFFFFF, // dwAlphaEdgeBlendBitDepth
	0x00000000, // dwAlphaEdgeBlend
	0x00000000, // dwReserved
	0xFFFFFFFF, // dwAlphaDestConstBitDepth
	0x00000000, // dwAlphaDestConst and lpDDSAlphaDest
	0xFFFFFFFF, // dwAlphaSrcConstBitDepth
	0x00000000, // dwAlphaSrcConst and lpDDSAlphaSrc
	0x00000000, // dwFillColor, dwFillDepth, dwFillPixel, and lpDDSPattern
	{0,0},      // ddckDestColorkey
	{0,0}       // ddckSrcColorkey
};

BOOL comp_bltfx(DDBLTFX *a, DDBLTFX *b, DWORD flags)
{
	DDBLTFX comp_mask;
	DDBLTFX comp_a;
	DDBLTFX comp_b;
	if (!a && !b) return FALSE;  // If both have no BLTFX
	if ((!a && b) || (a && !b)) return TRUE;  // One has BLTFX but not other
	memcpy(&comp_mask, &cmp_fx, sizeof(DDBLTFX));
	if(flags & DDBLT_ROP)
	{
		if (rop_texture_usage[(a->dwROP >> 16) & 0xFF] & 4)
			comp_mask.lpDDSPattern = (LPDIRECTDRAWSURFACE)_PTR_MASK;
	}
	if (flags & DDBLT_ZBUFFERDESTOVERRIDE) comp_mask.lpDDSZBufferDest = (LPDIRECTDRAWSURFACE)_PTR_MASK;
	if (flags & DDBLT_ZBUFFERSRCOVERRIDE) comp_mask.lpDDSZBufferSrc = (LPDIRECTDRAWSURFACE)_PTR_MASK;
	if (flags & DDBLT_ALPHADESTSURFACEOVERRIDE) comp_mask.lpDDSAlphaDest = (LPDIRECTDRAWSURFACE)_PTR_MASK;
	if (flags & DDBLT_ALPHASRCSURFACEOVERRIDE) comp_mask.lpDDSAlphaSrc = (LPDIRECTDRAWSURFACE)_PTR_MASK;
	AndMem(&comp_a, a, &comp_mask, sizeof(DDBLTFX));
	AndMem(&comp_b, b, &comp_mask, sizeof(DDBLTFX));
	if (!memcmp(&comp_a, &comp_b, sizeof(DDBLTFX))) return FALSE;
	else return TRUE;
}

BOOL IsAlphaCKey()
{
	if ((dxglcfg.BltScale == 2) || (dxglcfg.BltScale == 3) || (dxglcfg.BltScale == 4))
		return TRUE;
	else return FALSE;
}

WNDCLASSEX wndclassdxgltemp =
{
	sizeof(WNDCLASSEX),
	0,
	DefWindowProc,
	0,0,
	NULL,
	NULL,NULL,
	(HBRUSH)COLOR_WINDOW,
	NULL,
	_T("DXGLTempSizingWindowClass"),
	NULL
};
ATOM wndclassdxgltempatom = NULL;

void RegisterDXGLTempWindowClass()
{
	if (!wndclassdxgltempatom)
	{
		if (!wndclassdxgltemp.hInstance) wndclassdxgltemp.hInstance = GetModuleHandle(NULL);
		if (!wndclassdxgltemp.hCursor) wndclassdxgltemp.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclassdxgltempatom = RegisterClassEx(&wndclassdxgltemp);
	}
}

void UnregisterDXGLTempWindowClass()
{
	if (wndclassdxgltempatom)
	{
		UnregisterClass(wndclassdxgltemp.lpszClassName, GetModuleHandle(NULL));
		wndclassdxgltempatom = NULL;
	}
}
