// DXGL
// Copyright (C) 2011-2025 William Feely

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
#include "glTexture.h"
#include "glUtil.h"
#include "DXGLTexture.h"
#include "DXGLRenderer.h"
#include "glDirectDraw.h"
#include "glDirectDrawClipper.h"
#include "glRenderer.h"
#pragma warning(disable: 4996)
#include "ddraw.h"

static const DDSURFACEDESC2 ddsdclipper =
{
	sizeof(DDSURFACEDESC2),
	DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT,
	1,
	256,
	256 * 4,
	0,
	0,
	0,
	0,
	NULL,
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{
		sizeof(DDPIXELFORMAT),
		DDPF_RGB | DDPF_ALPHAPIXELS,
		0,
		16,
		0xF00,
		0xF0,
		0xF,
		0xF000,
	},
	{
		DDSCAPS_OFFSCREENPLAIN,
		0,
		0,
		0
	},
	0,
};
glDirectDrawClipperVtbl glDirectDrawClipper_iface =
{
	glDirectDrawClipper_QueryInterface,
	glDirectDrawClipper_AddRef,
	glDirectDrawClipper_Release,
	glDirectDrawClipper_GetClipList,
	glDirectDrawClipper_GetHWnd,
	glDirectDrawClipper_Initialize,
	glDirectDrawClipper_IsClipListChanged,
	glDirectDrawClipper_SetClipList,
	glDirectDrawClipper_SetHWnd
};

HRESULT glDirectDrawClipper_CreateNoInit(LPDIRECTDRAWCLIPPER *lplpDDClipper)
{
	glDirectDrawClipper *newclipper;
	TRACE_ENTER(1, 14, lplpDDClipper);
	if (!lplpDDClipper) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	newclipper = (glDirectDrawClipper*)malloc(sizeof(glDirectDrawClipper));
	if (!newclipper) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	ZeroMemory(newclipper, sizeof(glDirectDrawClipper));
	newclipper->refcount = 1;
	newclipper->initialized = false;
	newclipper->lpVtbl = &glDirectDrawClipper_iface;
	newclipper->creator = NULL;
	if (lplpDDClipper) *lplpDDClipper = (LPDIRECTDRAWCLIPPER)newclipper;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;	
}

HRESULT glDirectDrawClipper_Create(DWORD dwFlags, glDirectDraw7 *parent, LPDIRECTDRAWCLIPPER *lplpDDClipper)
{
	glDirectDrawClipper *newclipper;
	TRACE_ENTER(3, 9, dwFlags, 14, parent, 14, lplpDDClipper);
	if (!lplpDDClipper) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (dwFlags) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	newclipper = (glDirectDrawClipper*)malloc(sizeof(glDirectDrawClipper));
	if (!newclipper) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	ZeroMemory(newclipper, sizeof(glDirectDrawClipper));
	newclipper->refcount = 1;
	newclipper->initialized = false;
	newclipper->lpVtbl = &glDirectDrawClipper_iface;
	newclipper->creator = NULL;
	glDirectDrawClipper_Initialize(newclipper, (LPDIRECTDRAW)parent, dwFlags);
	if (lplpDDClipper) *lplpDDClipper = (LPDIRECTDRAWCLIPPER)newclipper;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawClipper_QueryInterface(glDirectDrawClipper *This, REFIID riid, LPVOID* obp)
{
	TRACE_ENTER(3,14,This,24,&riid,14,obp);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!obp) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		glDirectDrawClipper_AddRef(This);
		*obp = This;
		TRACE_VAR("*obp",14,*obp);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawClipper)
	{
		*obp = This;
		glDirectDrawClipper_AddRef(This);
		TRACE_VAR("*obp", 14, *obp);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}
ULONG WINAPI glDirectDrawClipper_AddRef(glDirectDrawClipper *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	This->refcount++;
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}
ULONG WINAPI glDirectDrawClipper_Release(glDirectDrawClipper *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	This->refcount--;
	ret = This->refcount;
	if (This->refcount == 0)
	{
		if (This->cliplist) free(This->cliplist);
		if (This->vertices) free(This->vertices);
		if (This->indices) free(This->indices);
		if (This->glDD7) glDirectDraw7_DeleteClipper(This->glDD7, This);
		if (This->creator) This->creator->Release();
		if (This->texture.initialized) glTexture_Release(&This->texture, FALSE);
		if (This->glDD7) IDXGLRenderer_FreePointer(This->glDD7->renderer, This);
	};
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirectDrawClipper_GetClipList(glDirectDrawClipper *This, LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize)
{
	int error;
	POINT origin;
	HDC hdc;
	HRGN rgnrect;
	HRGN rgncliplist;
	RGNDATA *rgn;
	DWORD rgnsize;
	TRACE_ENTER(4,14,This,26,lpRect,14,lpClipList,14,lpdwSize);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpdwSize) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!This->clipsize || This->hWnd)
	{
		if (!This->hWnd) TRACE_RET(HRESULT, 23, DDERR_NOCLIPLIST);
		hdc = GetDC(This->hWnd);
		if (!hdc) TRACE_RET(HRESULT, 23, DDERR_GENERIC);
		rgncliplist = CreateRectRgn(0, 0, 0, 0);
		if (!rgncliplist)
		{
			ReleaseDC(This->hWnd, hdc);
			TRACE_RET(HRESULT, 23, DDERR_GENERIC);
		}
		error = GetRandomRgn(hdc, rgncliplist, SYSRGN);
		if (error == -1)
		{
			DeleteObject(rgncliplist);
			ReleaseDC(This->hWnd, hdc);
			TRACE_RET(HRESULT, 23, DDERR_GENERIC);
		}
		if (GetVersion() & 0x80000000)
		{
			GetDCOrgEx(hdc, &origin);
			OffsetRgn(rgncliplist, origin.x, origin.y);
		}
		ReleaseDC(This->hWnd, hdc);
		if (lpRect)
		{
			rgnrect = CreateRectRgnIndirect(lpRect);
			if (CombineRgn(rgncliplist, rgnrect, rgncliplist, RGN_AND) == ERROR)
			{
				DeleteObject(rgnrect);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			}
			DeleteObject(rgnrect);
		}
		rgnsize = GetRegionData(rgncliplist, 0, NULL);
		rgn = (RGNDATA*)malloc(rgnsize);
		if (!rgn)
		{
			DeleteObject(rgncliplist);
			TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
		}
		GetRegionData(rgncliplist, rgnsize, rgn);
		if (!lpClipList)
		{
			*lpdwSize = sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT));
			free(rgn);
			DeleteObject(rgncliplist);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
		else
		{
			if (*lpdwSize < (sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT))))
			{
				free(rgn);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_REGIONTOOSMALL);
			}
			*lpdwSize = sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT));
#ifdef _MSC_VER
			__try
			{
#endif
				memcpy(lpClipList, rgn, sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT)));
#ifdef _MSC_VER
			}
			__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
			{
				free(rgn);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_INVALIDCLIPLIST);
			}
#endif
			free(rgn);
			DeleteObject(rgncliplist);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}
	if (!lpClipList)
	{
		if (lpRect)
		{
			rgnrect = CreateRectRgnIndirect(lpRect);
			rgncliplist = ExtCreateRegion(NULL, (sizeof(RGNDATAHEADER) + (This->cliplist->rdh.nCount*sizeof(RECT))),
				This->cliplist);
			if (CombineRgn(rgncliplist, rgnrect, rgncliplist, RGN_AND) == ERROR)
			{
				DeleteObject(rgnrect);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			}
			rgnsize = GetRegionData(rgncliplist, 0, NULL);
			rgn = (RGNDATA*)malloc(rgnsize);
			if (!rgn)
			{
				DeleteObject(rgnrect);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
			}
			GetRegionData(rgncliplist, rgnsize, rgn);
			*lpdwSize = sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT));
			free(rgn);
			DeleteObject(rgnrect);
			DeleteObject(rgncliplist);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
		else
		{
			*lpdwSize = sizeof(RGNDATAHEADER) + (This->clipsize*sizeof(RECT));
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}
	else
	{
		if (lpRect)
		{
			rgnrect = CreateRectRgnIndirect(lpRect);
			rgncliplist = ExtCreateRegion(NULL, (sizeof(RGNDATAHEADER) + (This->cliplist->rdh.nCount*sizeof(RECT))),
				This->cliplist);
			if (CombineRgn(rgncliplist, rgnrect, rgncliplist, RGN_AND) == ERROR)
			{
				DeleteObject(rgnrect);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			}
			rgnsize = GetRegionData(rgncliplist, 0, NULL);
			rgn = (RGNDATA*)malloc(rgnsize);
			if (!rgn)
			{
				DeleteObject(rgnrect);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
			}
			GetRegionData(rgncliplist, rgnsize, rgn);
			if (*lpdwSize < (sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT))))
			{
				free(rgn);
				DeleteObject(rgnrect);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_REGIONTOOSMALL);
			}
			*lpdwSize = sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT));
#ifdef _MSC_VER
			__try
			{
#endif
				memcpy(lpClipList, rgn, sizeof(RGNDATAHEADER) + (rgn->rdh.nCount*sizeof(RECT)));
#ifdef _MSC_VER
			}
			__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
			{
				free(rgn);
				DeleteObject(rgnrect);
				DeleteObject(rgncliplist);
				TRACE_RET(HRESULT, 23, DDERR_INVALIDCLIPLIST);
			}
#endif
			free(rgn);
			DeleteObject(rgnrect);
			DeleteObject(rgncliplist);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
		else
		{
			if(*lpdwSize < (sizeof(RGNDATAHEADER) + (This->clipsize*sizeof(RECT))))
				TRACE_RET(HRESULT,23,DDERR_REGIONTOOSMALL);
			*lpdwSize = sizeof(RGNDATAHEADER) + (This->clipsize*sizeof(RECT));
#ifdef _MSC_VER
			__try
			{
#endif
				memcpy(lpClipList, This->cliplist, sizeof(RGNDATAHEADER) + (This->clipsize*sizeof(RECT)));
#ifdef _MSC_VER
			}
			__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
			{
				TRACE_RET(HRESULT, 23, DDERR_INVALIDCLIPLIST);
			}
#endif
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}
	FIXME("IDirectDrawClipper::GetClipList: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper_GetHWnd(glDirectDrawClipper *This, HWND FAR *lphWnd)
{
	TRACE_ENTER(2,14,This,14,lphWnd);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lphWnd) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(!This->hWnd) TRACE_RET(HRESULT,23,DDERR_NOHWND);
	*lphWnd = This->hWnd;
	TRACE_VAR("*lphWnd",13,*lphWnd);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper_Initialize(glDirectDrawClipper *This, LPDIRECTDRAW lpDD, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDD,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (dwFlags) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(This->initialized) TRACE_RET(HRESULT,23,DDERR_ALREADYINITIALIZED);
	This->glDD7 = (glDirectDraw7*)lpDD;
	if(This->glDD7) This->hasparent = true;
	else This->hasparent = false;
	This->hWnd = NULL;
	This->cliplist = NULL;
	This->vertices = NULL;
	This->indices = NULL;
	This->hascliplist = false;
	This->maxsize = This->clipsize = 0;
	This->refcount = 1;
	This->initialized = true;
	This->dirty = true;
	This->cliplistchanged = TRUE;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper_IsClipListChanged(glDirectDrawClipper *This, BOOL FAR *lpbChanged)
{
	WINDOWPLACEMENT newpos;
	TRACE_ENTER(2,14,This,14,lpbChanged);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpbChanged) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	newpos.length = sizeof(WINDOWPLACEMENT);
	if (This->hWnd) GetWindowPlacement(This->hWnd, &newpos);
	if (memcmp(&This->lastpos, &newpos, sizeof(WINDOWPLACEMENT)))
	{
		This->cliplistchanged = TRUE;
		memcpy(&This->lastpos, &newpos, sizeof(WINDOWPLACEMENT));
	}
	*lpbChanged = This->cliplistchanged;
	This->cliplistchanged = FALSE;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper_SetClipList(glDirectDrawClipper *This, LPRGNDATA lpClipList, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpClipList,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (dwFlags) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(This->hWnd) TRACE_RET(HRESULT,23,DDERR_CLIPPERISUSINGHWND);
	bool memfail;
	DWORD i;
	if(lpClipList)
	{
		if(lpClipList->rdh.dwSize != sizeof(RGNDATAHEADER)) TRACE_RET(HRESULT,23,DDERR_INVALIDCLIPLIST);
		if(lpClipList->rdh.iType != RDH_RECTANGLES) TRACE_RET(HRESULT,23,DDERR_INVALIDCLIPLIST);
		if(lpClipList->rdh.nRgnSize != lpClipList->rdh.nCount * sizeof(RECT))
			TRACE_RET(HRESULT,23,DDERR_INVALIDCLIPLIST);
		if(!This->cliplist)
		{
			memfail = false;
			This->maxsize = lpClipList->rdh.nCount;
			This->cliplist = (RGNDATA*)malloc(sizeof(RGNDATAHEADER)+(This->maxsize*sizeof(RECT)));
			if(!This->cliplist) memfail = true;
			if(!memfail) This->vertices = (BltVertex*)malloc(This->maxsize*4*sizeof(BltVertex));
			if(!This->vertices) memfail = true;
			if(!memfail) This->indices = (GLushort*)malloc(This->maxsize*6*sizeof(GLushort));
			if(!This->indices) memfail = true;
			if(memfail)
			{
				if(This->vertices)
				{
					free(This->vertices);
					This->vertices = NULL;
				}
				if(This->cliplist)
				{
					free(This->cliplist);
					This->cliplist = NULL;
				}
				This->maxsize = 0;
				TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
			}
		}
		if(lpClipList->rdh.nCount > This->maxsize)
		{
			memfail = false;
			RGNDATA *newcliplist = NULL;
			BltVertex *newvertices = NULL;
			GLushort *newindices = NULL;
			newcliplist = (RGNDATA*)realloc(This->cliplist,sizeof(RGNDATAHEADER)+(lpClipList->rdh.nCount*sizeof(RECT)));
			if(!newcliplist) memfail = true;
			else This->cliplist = newcliplist;
			if(!memfail) newvertices = (BltVertex*)realloc(This->vertices,lpClipList->rdh.nCount*4*sizeof(BltVertex));
			if(!newvertices) memfail = true;
			else This->vertices = newvertices;
			if(!memfail) newindices = (GLushort*)realloc(This->indices,lpClipList->rdh.nCount*6*sizeof(GLushort));
			if(!newindices) memfail = true;
			else This->indices = newindices;
			if(memfail) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
			This->maxsize = lpClipList->rdh.nCount;
		}
		This->clipsize = lpClipList->rdh.nCount;
		memcpy(This->cliplist,lpClipList,sizeof(RGNDATAHEADER)+(lpClipList->rdh.nCount*sizeof(RECT)));
		RECT *buffer = (RECT*)This->cliplist->Buffer;
		for(i = 0; i < lpClipList->rdh.nCount; i++)
		{
			This->vertices[(i*4)+1].x = This->vertices[(i*4)+3].x = (GLfloat)buffer[i].left;
			This->vertices[i*4].x = This->vertices[(i*4)+2].x = (GLfloat)buffer[i].right;
			This->vertices[i*4].y = This->vertices[(i*4)+1].y = (GLfloat)buffer[i].top;
			This->vertices[(i*4)+2].y = This->vertices[(i*4)+3].y = (GLfloat)buffer[i].bottom;
			// 0 1 2 2 1 3
			This->indices[i*6] = (GLushort)i*4;
			This->indices[(i*6)+1] = This->indices[(i*6)+4] = (GLushort)(i*4)+1;
			This->indices[(i*6)+2] = This->indices[(i*6)+3] = (GLushort)(i*4)+2;
			This->indices[(i*6)+5] = (GLushort)(i*4)+3;
		}
		for(i = 0; i < (4*lpClipList->rdh.nCount); i++)
		{
			This->vertices[i].s = This->vertices[i].t = 0.0f;
		}
	}
	else This->clipsize = 0;
	This->dirty = true;
	This->cliplistchanged = TRUE;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper_SetHWnd(glDirectDrawClipper *This, DWORD dwFlags, HWND hWnd)
{
	TRACE_ENTER(3,14,This,9,dwFlags,13,hWnd);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (dwFlags) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	This->hWnd = hWnd;
	This->lastpos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWnd, &This->lastpos);
	This->cliplistchanged = TRUE;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

void glDirectDrawClipper_CreateTexture(glDirectDrawClipper *This, glTexture *texture, glRenderer *renderer)
{
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, &ddsdclipper, sizeof(DDSURFACEDESC2));
	ddsd.dwWidth = texture->levels[0].ddsd.dwWidth;
	ddsd.lPitch = NextMultipleOf4(ddsd.dwWidth * 2);
	ddsd.dwHeight = texture->levels[0].ddsd.dwHeight;
	glTexture_Create(&ddsd, &This->texture, renderer, FALSE, 0);
	This->texture.freeonrelease = FALSE;
}