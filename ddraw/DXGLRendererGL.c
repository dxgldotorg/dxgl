// DXGL
// Copyright (C) 2023-2025 William Feely

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
#include "glUtil.h"
#include "DXGLQueue.h"
#include "DXGLRenderer.h"
#include "DXGLRendererGL.h"
#include "DXGLTexture.h"
#include "DXGLTextureGL.h"
#include "util.h"
#include <WbemCli.h>
#include <oleauto.h>
#include "const.h"
#include "fourcc.h"
#include "spinlock.h"
#include "ShaderManager.h"
#include "string.h"
#include "ShaderGen2D.h"


static BOOL(WINAPI *_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX lpBuffer) = NULL;
static HMODULE hKernel32;
static MEMORYSTATUS memstatus;
static MEMORYSTATUSEX memstatusex;

extern DXGLCFG dxglcfg;
extern DWORD gllock;

static WNDCLASSEXA wndclass;
static BOOL wndclasscreated = FALSE;

static const WORD bltindices[6] = { 0,1,2,1,2,3 };

static const struct IDXGLRendererVtbl vtbl =
{
	DXGLRendererGL_QueryInterface,
	DXGLRendererGL_AddRef,
	DXGLRendererGL_Release,
	DXGLRendererGL_GetAttachedDevice,
	DXGLRendererGL_SetAttachedDevice,
	DXGLRendererGL_GetCaps,
	DXGLRendererGL_Reset,
	DXGLRendererGL_PostCommand,
	DXGLRendererGL_Break,
	DXGLRendererGL_FreePointer,
	DXGLRendererGL_SetCooperativeLevel,
	DXGLRendererGL_CreateTexture,
	DXGLRendererGL_DeleteTexture,
	DXGLRendererGL_SetTexture,
	DXGLRendererGL_SetTarget,
	DXGLRendererGL_Lock,
	DXGLRendererGL_Unlock, // Unlock
	NULL, // Clear
	DXGLRendererGL_SetRenderState,
	DXGLRendererGL_SetFVF,
	DXGLRendererGL_DrawPrimitives2D,
	NULL, // DrawPrimitives
	DXGLRendererGL_SwapBuffers, // SwapBuffers
	DXGLRendererGL_Sync,
	DXGLRendererGL_GetWindow,
	DXGLRendererGL_SetWindowSize//Fixme:  Fill in these functions.
};

DWORD WINAPI DXGLRendererGL_MainThread(LPDXGLRENDERERGL This);

HRESULT DXGLRendererGL_Create(GUID *guid, LPDXGLRENDERERGL *out, int index)
{
	HANDLE waithandles[2];
	IDXGLRendererGL *This;
	DWORD waitobject;
	HRESULT ret;
	WCHAR RenderThreadName[27] = L"GL Render Thread #00000000";
	// Allocate a renderer object
	_itow(index, &RenderThreadName[18], 10);
	This = (IDXGLRendererGL*)malloc(sizeof(IDXGLRendererGL));
	if (!This) return DDERR_OUTOFMEMORY;
	ZeroMemory(This, sizeof(IDXGLRendererGL));
	This->synclock = 0;
	This->base.lpVtbl = &vtbl;
	This->refcount = 1;
	This->rendermode = -1;

	// Start the render thread
	This->SyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!This->SyncEvent)
	{
		free(This);
		return DDERR_OUTOFMEMORY;
	}
	This->StartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!This->StartEvent)
	{
		CloseHandle(This->SyncEvent);
		free(This);
		return DDERR_OUTOFMEMORY;
	}
	__try
	{
		InitializeCriticalSection(&This->cs);
	}
	__except (1)
	{
		CloseHandle(This->StartEvent);
		CloseHandle(This->SyncEvent);
		free(This);
		return DDERR_OUTOFMEMORY;
	}
	This->ThreadHandle = CreateThread(NULL, 0, DXGLRendererGL_MainThread, This, 0, &This->ThreadID);
	if (!This->ThreadHandle)
	{
		DeleteCriticalSection(&This->cs);
		CloseHandle(This->StartEvent);
		CloseHandle(This->SyncEvent);
		free(This);
		return DDERR_OUTOFMEMORY;
	}
	SetThreadName(This->ThreadHandle, RenderThreadName);
	waithandles[0] = This->SyncEvent;
	waithandles[1] = This->ThreadHandle;
	waitobject = WaitForMultipleObjects(2, waithandles, FALSE, INFINITE);
	// Thread exited prematurely
	if (waitobject == WAIT_OBJECT_0 + 1)
	{
		GetExitCodeThread(This->ThreadHandle, &ret);
		CloseHandle(This->ThreadHandle);
		DeleteCriticalSection(&This->cs);
		CloseHandle(This->StartEvent);
		CloseHandle(This->SyncEvent);
		free(This);
		return ret;
	}
	// Thread is ready
	*out = This;
	return DD_OK;
}

HRESULT WINAPI DXGLRendererGL_QueryInterface(LPDXGLRENDERERGL This, REFIID riid, LPVOID *ppvObject)
{
	if (!This) return DDERR_INVALIDOBJECT;
	if (!riid) return DDERR_INVALIDPARAMS;
	if (!ppvObject) return DDERR_INVALIDPARAMS;
	if ((!memcmp(riid, &IID_IUnknown, sizeof(GUID)))
		|| (!memcmp(riid, &IID_IUnknown, sizeof(GUID))))
	{
		DXGLRendererGL_AddRef(This);
		*ppvObject = (LPUNKNOWN)This;
		return DD_OK;
	}
	else return E_NOINTERFACE;
}

ULONG WINAPI DXGLRendererGL_AddRef(LPDXGLRENDERERGL This)
{
	if (!This) return 0;
	return InterlockedIncrement(&This->refcount);
}

ULONG WINAPI DXGLRendererGL_Release(LPDXGLRENDERERGL This)
{
	ULONG ret;
	if (!This) return 0;
	ret = InterlockedDecrement(&This->refcount);
	if (!ret)
	{
		// Cleanup

		free(This);
	}
	return ret;
}

//GL Render Window event handler
static LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPDXGLRENDERERGL This = (LPDXGLRENDERERGL)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (msg == WM_CLOSE)  // Prevent destroying parent
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	if(!This) return DefWindowProc(hwnd, msg, wParam, lParam);
	if (This->hWndTarget)
	{
		//TODO:  Port over glRenderWindow handler
		PostMessage(This->hWndTarget, msg, wParam, lParam);
		return 0;
	}
	else
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}


DWORD WINAPI DXGLRendererGL_WindowThread(LPDXGLRENDERERGL This)
{
	MSG Msg;
	char *windowname;
	BOOL exch = TRUE;
	windowname = "DXGL OpenGL Renderer";
	exch = InterlockedExchange((LONG*) &wndclasscreated, (LONG)exch);
	if (!exch)
	{
		wndclass.cbSize = sizeof(WNDCLASSEXA);
		wndclass.style = CS_DBLCLKS;
		wndclass.lpfnWndProc = RenderWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = (HINSTANCE)GetModuleHandle(_T("ddraw.dll"));
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "DirectDrawDeviceWnd";
		wndclass.hIconSm = NULL;
		RegisterClassExA(&wndclass);
	}
	This->hWndContext = CreateWindowA("DirectDrawDeviceWnd", windowname, WS_POPUP,
		0, 0, 16, 16, NULL, NULL, GetModuleHandle(_T("ddraw.dll")), NULL);
	if (!This->hWndContext) return DDERR_OUTOFMEMORY;
	SetWindowLongPtr(This->hWndContext, GWLP_USERDATA, (LONG_PTR)This);
	SetEvent(This->StartEvent);

	// Run the window loop
	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return 0;
}

DWORD WINAPI DXGLRendererGL_MainThread(LPDXGLRENDERERGL This)
{
	UINT numpf;
	HANDLE waithandles[2];
	DWORD waitobject;
	HRESULT ret;
	HGLRC hrcinitial;
	HWND hwndinitial;
	HDC hdcinitial;
	BOOL stopthread = FALSE;
	GLint vidmemnv;
	VARIANT adapterram = {0};
	IWbemLocator *wbemloc = NULL;
	IWbemServices *wbemsvc = NULL;
	IEnumWbemClassObject *wbemenum = NULL;
	IWbemClassObject *wbemclsobj = NULL;
	BSTR cimv2;
	BSTR wql;
	BSTR wqlquery;
	BOOL corecontext = FALSE;
	DXGLQueueCmdDecoder *currcmd;
	ULONG_PTR queuepos, pixelpos, vertexpos, indexpos;
	void *tmp;
	WCHAR WindowThreadName[26] = L"GL Window Thread #00000000";
	PWSTR RenderThreadName;
	int index;
	LONG lastcmd;
	int pfattribs[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_FULL_ACCELERATION_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		0,0,0,0,0,0,0
	};
	int pfattribs10bit[] =  // For later 10-bit output support
	{
		WGL_RED_BITS_ARB, 10,
		WGL_GREEN_BITS_ARB, 10,
		WGL_BLUE_BITS_ARB, 10,
		WGL_ALPHA_BITS_ARB, 2,
		0
	};
	int glattribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	This->WindowThreadHandle = CreateThread(NULL, 0, DXGLRendererGL_WindowThread, This, 0, &This->WindowThreadID);
	if (!This->WindowThreadHandle) return DDERR_OUTOFMEMORY;
	RenderThreadName = GetThreadName(GetCurrentThread());
	if (RenderThreadName)
	{
		index = _wtoi(&RenderThreadName[18]);
		_itow(index, &WindowThreadName[18], 10);
		SetThreadName(This->WindowThreadHandle, WindowThreadName);
		LocalFree(RenderThreadName);
	}
	waithandles[0] = This->StartEvent; // Temporarily use the start event to wait for the window thread
	waithandles[1] = This->WindowThreadHandle;
	waitobject = WaitForMultipleObjects(2, waithandles, FALSE, INFINITE);
	// Thread exited prematurely
	if (waitobject == WAIT_OBJECT_0 + 1)
	{
		GetExitCodeThread(This->ThreadHandle, &ret);
		CloseHandle(This->WindowThreadHandle);
		return ret;
	}

	// Initialize OpenGL (first pass)
	hwndinitial = CreateWindowA("DirectDrawDeviceWnd", "GLInit", WS_POPUP,
		0, 0, 16, 16, NULL, NULL, GetModuleHandle(_T("ddraw.dll")), NULL);
	if (!hwndinitial)
	{
		SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
		WaitForSingleObject(This->WindowThreadHandle, INFINITE);
		return DDERR_OUTOFMEMORY;
	}
	ZeroMemory(&This->pfd, sizeof(PIXELFORMATDESCRIPTOR));
	This->pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	This->pfd.nVersion = 1;
	if (dxglcfg.SingleBufferDevice)
	{
		This->pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
		pfattribs[5] = GL_FALSE;
	}
	else This->pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	This->pfd.iPixelType = PFD_TYPE_RGBA;
	This->pfd.cColorBits = 32;
	This->pfd.iLayerType = PFD_MAIN_PLANE;
	InterlockedIncrement(&gllock);
	hdcinitial = GetDC(hwndinitial);
	if (!hdcinitial)
	{
		DEBUG("DXGLRendererGL_MainThread: Can not create hDC\n");
		InterlockedDecrement((LONG*)&gllock);
		DestroyWindow(hwndinitial);
		SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
		WaitForSingleObject(This->WindowThreadHandle,INFINITE);
		return DDERR_GENERIC;
	}
	This->pf = ChoosePixelFormat(hdcinitial, &This->pfd);
	if (!This->pf)
	{
		DEBUG("DXGLRendererGL_MainThread: Can not get pixelformat\n");
		InterlockedDecrement((LONG*)&gllock);
		ReleaseDC(hwndinitial, hdcinitial);
		DestroyWindow(hwndinitial);
		SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
		WaitForSingleObject(This->WindowThreadHandle, INFINITE);
		return DDERR_GENERIC;
	}
	if (!SetPixelFormat(hdcinitial, This->pf, &This->pfd))
		DEBUG("DXGLRendererGL_MainThread: Can not set pixelformat\n");
	hrcinitial = wglCreateContext(hdcinitial);
	if (!hrcinitial)
	{
		DEBUG("DXGLRendererGL_MainThread: Can not create GL context\n");
		InterlockedDecrement((LONG*)&gllock);
		ReleaseDC(hwndinitial, hdcinitial);
		DestroyWindow(hwndinitial);
		SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
		WaitForSingleObject(This->WindowThreadHandle, INFINITE);
		return DDERR_GENERIC;
	}
	if (!wglMakeCurrent(hdcinitial, hrcinitial))
	{
		DEBUG("DXGLRendererGL_MainThread: Can not activate GL context\n");
		wglDeleteContext(hrcinitial);
		hrcinitial = NULL;
		InterlockedDecrement((LONG*)&gllock);
		ReleaseDC(hwndinitial, hdcinitial);
		DestroyWindow(hwndinitial);
		SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
		WaitForSingleObject(This->WindowThreadHandle, INFINITE);
		return DDERR_GENERIC;
	}
	glExtensions_Init(&This->ext, hdcinitial, FALSE);
	glUtil_Create(&This->ext, &This->util);
	if (This->ext.WGLEXT_ARB_create_context || This->ext.WGLEXT_ARB_pixel_format)
	{
		if (This->ext.WGLEXT_ARB_pixel_format)
		{
			This->hdc = GetDC(This->hWndContext);
			This->ext.wglChoosePixelFormatARB(This->hdc, pfattribs, NULL, 1, &This->pf, &numpf);
		}
		DescribePixelFormat(This->hdc, This->pf, sizeof(PIXELFORMATDESCRIPTOR), &This->pfd);
		SetPixelFormat(This->hdc, This->pf, &This->pfd);
		if (This->ext.WGLEXT_ARB_create_context && (This->ext.glver_major >= 3))
		{
			glattribs[1] = This->ext.glver_major;
			glattribs[3] = This->ext.glver_minor;
			corecontext = TRUE;
			if (!This->ext.WGLEXT_ARB_create_context_profile) glattribs[4] = 0;
			This->hrc = This->ext.wglCreateContextAttribsARB(This->hdc, 0, glattribs);
			if (!This->hrc)
			{
				DEBUG("DXGLRendererGL_MainThraed: Failed to create OpenGL Core Context\n");
				wglMakeCurrent(0, 0);
				wglDeleteContext(hrcinitial);
				hrcinitial = NULL;
				ReleaseDC(This->hWndContext, This->hdc);
				InterlockedDecrement((LONG*)&gllock);
				ReleaseDC(hwndinitial, hdcinitial);
				DestroyWindow(hwndinitial);
				SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
				WaitForSingleObject(This->WindowThreadHandle, INFINITE);
				return DDERR_GENERIC;
			}
		}
		else
		{
			This->hrc = wglCreateContext(This->hdc);
			if (!This->hrc)
			{
				DEBUG("DXGLRendererGL_MainThraed: Failed to create OpenGL Main Context\n");
				wglMakeCurrent(0, 0);
				wglDeleteContext(hrcinitial);
				hrcinitial = NULL;
				ReleaseDC(This->hWndContext, This->hdc);
				InterlockedDecrement((LONG*)&gllock);
				ReleaseDC(hwndinitial, hdcinitial);
				DestroyWindow(hwndinitial);
				SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
				WaitForSingleObject(This->WindowThreadHandle, INFINITE);
				return DDERR_GENERIC;
			}
		}
		wglMakeCurrent(0, 0);
		wglDeleteContext(hrcinitial);
		ReleaseDC(hwndinitial, hdcinitial);
		DestroyWindow(hwndinitial);
		wglMakeCurrent(This->hdc, This->hrc);
		InterlockedDecrement((LONG*)&gllock);
	}
	else
	{
		This->hdc = GetDC(This->hWndContext);
		This->hrc = hrcinitial;
		SetPixelFormat(This->hdc, This->pf, &This->pfd);
		wglMakeCurrent(This->hdc, This->hrc);
		ReleaseDC(hwndinitial, hdcinitial);
		DestroyWindow(hwndinitial);
		InterlockedDecrement((LONG*)&gllock);
	}
	glExtensions_Init(&This->ext, This->hdc, corecontext);
	// Create shader manager
	ShaderManager_Init(&This->ext, &This->shaders);

	// Populate ddcaps
	ZeroMemory(&This->ddcaps, sizeof(DDCAPS_DX7));
	This->ddcaps.dwSize = sizeof(DDCAPS_DX7);
	This->ddcaps.dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL | DDCAPS_BLTFOURCC |
		DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY | DDCAPS_GDI | DDCAPS_PALETTE | DDCAPS_CANBLTSYSMEM |
		DDCAPS_3D | DDCAPS_CANCLIP | DDCAPS_CANCLIPSTRETCHED | DDCAPS_READSCANLINE |
		DDCAPS_OVERLAY | DDCAPS_OVERLAYSTRETCH;
	This->ddcaps.dwCaps2 = DDCAPS2_CANRENDERWINDOWED | DDCAPS2_WIDESURFACES | DDCAPS2_NOPAGELOCKREQUIRED |
		DDCAPS2_FLIPINTERVAL | DDCAPS2_FLIPNOVSYNC | DDCAPS2_NONLOCALVIDMEM;
	This->ddcaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT | DDCKEYCAPS_DESTBLT |
		/*DDCKEYCAPS_SRCOVERLAY | */DDCKEYCAPS_DESTOVERLAY;
	This->ddcaps.dwFXCaps = DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKY |
		DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY | DDFXCAPS_BLTMIRRORLEFTRIGHT |
		DDFXCAPS_BLTMIRRORUPDOWN | DDFXCAPS_BLTROTATION90;
	This->ddcaps.dwFXAlphaCaps = 0;
	This->ddcaps.dwPalCaps = DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT |
		DDPCAPS_PRIMARYSURFACE | DDPCAPS_ALLOW256;
	This->ddcaps.dwSVCaps = 0;
	This->ddcaps.dwAlphaBltConstBitDepths = 0;
	This->ddcaps.dwAlphaBltPixelBitDepths = 0;
	This->ddcaps.dwAlphaBltSurfaceBitDepths = 0;
	This->ddcaps.dwAlphaOverlayConstBitDepths = 0;
	This->ddcaps.dwAlphaOverlayPixelBitDepths = 0;
	This->ddcaps.dwAlphaOverlaySurfaceBitDepths = 0;
	This->ddcaps.dwZBufferBitDepths = DDBD_16 | DDBD_24 | DDBD_32;
	This->ddcaps.dwVidMemTotal = 0; // Todo:  Prefill with memory status
	This->ddcaps.dwVidMemFree = 0;
	This->ddcaps.dwMaxVisibleOverlays = 16;
	This->ddcaps.dwCurrVisibleOverlays = 0;
	This->ddcaps.dwNumFourCCCodes = GetNumFOURCC();
	This->ddcaps.dwAlignBoundarySrc = 0;
	This->ddcaps.dwAlignSizeSrc = 0;
	This->ddcaps.dwAlignBoundaryDest = 0;
	This->ddcaps.dwAlignSizeDest = 0;
	This->ddcaps.dwAlignStrideAlign = 0;
	if ((This->ext.glver_major >= 3) || This->ext.GLEXT_EXT_gpu_shader4)
		memcpy(This->ddcaps.dwRops, supported_rops, DD_ROP_SPACE * sizeof(DWORD));
	else memcpy(This->ddcaps.dwRops, supported_rops_gl2, DD_ROP_SPACE * sizeof(DWORD));
	This->ddcaps.dwMinOverlayStretch = 1;
	This->ddcaps.dwMaxOverlayStretch = 2147483647;
	This->ddcaps.dwMinLiveVideoStretch = 1;
	This->ddcaps.dwMaxLiveVideoStretch = 2147483647;
	This->ddcaps.dwMinHwCodecStretch = 1;
	This->ddcaps.dwMaxHwCodecStretch = 2147483647;
	This->ddcaps.ddsOldCaps.dwCaps = This->ddcaps.ddsCaps.dwCaps =
		DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP |
		DDSCAPS_FRONTBUFFER | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PALETTE |
		DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE |
		DDSCAPS_NONLOCALVIDMEM | DDSCAPS_LOCALVIDMEM | DDSCAPS_TEXTURE |
		DDSCAPS_MIPMAP | DDSCAPS_OVERLAY;
	This->ddcaps.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;

	// Populate d3dcaps
	This->d3ddesc6.dwSize = sizeof(D3DDEVICEDESC);
	This->d3ddesc6.dwFlags = D3DDD_COLORMODEL | D3DDD_DEVCAPS | D3DDD_TRANSFORMCAPS |
		D3DDD_BCLIPPING | D3DDD_LIGHTINGCAPS | D3DDD_LINECAPS | D3DDD_TRICAPS |
		D3DDD_DEVICERENDERBITDEPTH | D3DDD_DEVICEZBUFFERBITDEPTH | D3DDD_MAXVERTEXCOUNT;
	This->d3ddesc6.dcmColorModel = D3DCOLOR_RGB;
	This->d3ddesc6.dwDevCaps = This->d3ddesc7.dwDevCaps =
		D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_EXECUTESYSTEMMEMORY |
		D3DDEVCAPS_EXECUTEVIDEOMEMORY | D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_CANBLTSYSTONONLOCAL |
		D3DDEVCAPS_TEXTURESYSTEMMEMORY | D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TEXTURENONLOCALVIDMEM |
		D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY | D3DDEVCAPS_HWRASTERIZATION;
	This->d3ddesc6.dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
	This->d3ddesc6.dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
	This->d3ddesc6.bClipping = TRUE;
	This->d3ddesc6.dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
	This->d3ddesc6.dlcLightingCaps.dwCaps = D3DLIGHTCAPS_DIRECTIONAL | D3DLIGHTCAPS_POINT | D3DLIGHTCAPS_SPOT;
	This->d3ddesc6.dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
	This->d3ddesc6.dlcLightingCaps.dwNumLights = 8;
	This->d3ddesc6.dpcLineCaps.dwSize= sizeof(D3DPRIMCAPS);
	This->d3ddesc6.dpcLineCaps.dwMiscCaps = D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLNONE |
		D3DPMISCCAPS_MASKZ;
	This->d3ddesc6.dpcLineCaps.dwRasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_FOGRANGE |
		D3DPRASTERCAPS_FOGTABLE | D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_MIPMAPLODBIAS |
		D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_WFOG | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_ZFOG |
		D3DPRASTERCAPS_ZTEST;
	This->d3ddesc6.dpcLineCaps.dwZCmpCaps = This->d3ddesc6.dpcLineCaps.dwAlphaCmpCaps =
		D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_GREATEREQUAL |
		D3DPCMPCAPS_LESS | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_NEVER | D3DPCMPCAPS_NOTEQUAL;
	This->d3ddesc6.dpcLineCaps.dwSrcBlendCaps = This->d3ddesc6.dpcLineCaps.dwDestBlendCaps =
		D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
		D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA |
		D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR | D3DPBLENDCAPS_INVDESTCOLOR |
		D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA;
	This->d3ddesc6.dpcLineCaps.dwShadeCaps = D3DPSHADECAPS_ALPHAFLATBLEND | D3DPSHADECAPS_ALPHAGOURAUDBLEND |
		D3DPSHADECAPS_COLORFLATRGB | D3DPSHADECAPS_COLORGOURAUDRGB | D3DPSHADECAPS_FOGFLAT | D3DPSHADECAPS_FOGGOURAUD;
	This->d3ddesc6.dpcLineCaps.dwTextureCaps = D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_PERSPECTIVE |
		D3DPTEXTURECAPS_TRANSPARENCY;
	This->d3ddesc6.dpcLineCaps.dwTextureFilterCaps = D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR |
		D3DPTFILTERCAPS_MIPNEAREST | D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST |
		D3DPTFILTERCAPS_LINEARMIPLINEAR | D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | 
		D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MINFPOINT;
	This->d3ddesc6.dpcLineCaps.dwTextureBlendCaps = D3DPTBLENDCAPS_ADD | D3DPTBLENDCAPS_COPY |
		D3DPTBLENDCAPS_DECAL | D3DPTBLENDCAPS_DECALALPHA | D3DPTBLENDCAPS_MODULATE |
		D3DPTBLENDCAPS_MODULATEALPHA;
	This->d3ddesc6.dpcLineCaps.dwTextureAddressCaps = D3DPTADDRESSCAPS_CLAMP |
		D3DPTADDRESSCAPS_INDEPENDENTUV | D3DPTADDRESSCAPS_MIRROR | D3DPTADDRESSCAPS_WRAP;
	This->d3ddesc6.dpcLineCaps.dwStippleWidth = 0;
	This->d3ddesc6.dpcLineCaps.dwStippleHeight = 0;
	memcpy(&This->d3ddesc6.dpcTriCaps, &This->d3ddesc6.dpcLineCaps, sizeof(D3DPRIMCAPS));
	memcpy(&This->d3ddesc7.dpcLineCaps, &This->d3ddesc6.dpcLineCaps, sizeof(D3DPRIMCAPS));
	memcpy(&This->d3ddesc7.dpcTriCaps, &This->d3ddesc6.dpcLineCaps, sizeof(D3DPRIMCAPS));
	This->d3ddesc6.dwDeviceRenderBitDepth = This->d3ddesc7.dwDeviceRenderBitDepth =
		DDBD_16 | DDBD_24 | DDBD_32;
	This->d3ddesc6.dwDeviceZBufferBitDepth = This->d3ddesc7.dwDeviceZBufferBitDepth =
		DDBD_16 | DDBD_24 | DDBD_32;
	This->d3ddesc6.dwMaxBufferSize = 0;
	This->d3ddesc6.dwMaxVertexCount = 65535;
	This->d3ddesc6.dwMinTextureWidth = This->d3ddesc7.dwMinTextureHeight = 1;
	This->d3ddesc6.dwMaxTextureWidth = This->d3ddesc6.dwMaxTextureHeight =
		This->d3ddesc7.dwMaxTextureWidth = This->d3ddesc7.dwMaxTextureHeight =
		This->ext.maxtexturesize > 65536 ? 65536 : This->ext.maxtexturesize;
	This->d3ddesc6.dwMaxStippleWidth = This->d3ddesc6.dwMaxStippleHeight = 0;
	This->d3ddesc6.dwMaxTextureRepeat = This->d3ddesc6.dwMaxTextureAspectRatio =
		This->d3ddesc7.dwMaxTextureRepeat = This->d3ddesc7.dwMaxTextureAspectRatio =
		This->ext.maxtexturesize > 65536 ? 65536 : This->ext.maxtexturesize;
	This->d3ddesc6.dwMaxAnisotropy = This->d3ddesc7.dwMaxAnisotropy = 1; // FIXME:  Add anisotropic filtering support
	This->d3ddesc6.dvGuardBandLeft = This->d3ddesc6.dvGuardBandTop =
		This->d3ddesc7.dvGuardBandLeft = This->d3ddesc7.dvGuardBandTop = -1.0E8f;
	This->d3ddesc6.dvGuardBandRight = This->d3ddesc6.dvGuardBandBottom =
		This->d3ddesc7.dvGuardBandRight = This->d3ddesc7.dvGuardBandBottom = 1.0E8f;
	This->d3ddesc6.dvExtentsAdjust = This->d3ddesc7.dvExtentsAdjust = 0.0f;
	This->d3ddesc6.dwStencilCaps = This->d3ddesc7.dwStencilCaps = 0;
	This->d3ddesc6.dwFVFCaps = This->d3ddesc7.dwFVFCaps = 8; // 8 stages
	This->d3ddesc6.dwTextureOpCaps = This->d3ddesc7.dwTextureOpCaps =
		D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_SELECTARG2 | D3DTEXOPCAPS_MODULATE |
		D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_MODULATE4X | D3DTEXOPCAPS_ADD |
		D3DTEXOPCAPS_ADDSIGNED | D3DTEXOPCAPS_ADDSIGNED2X | D3DTEXOPCAPS_SUBTRACT |
		D3DTEXOPCAPS_ADDSMOOTH | D3DTEXOPCAPS_BLENDDIFFUSEALPHA | D3DTEXOPCAPS_BLENDTEXTUREALPHA |
		D3DTEXOPCAPS_BLENDTEXTUREALPHAPM | D3DTEXOPCAPS_BLENDCURRENTALPHA;
	This->d3ddesc6.wMaxTextureBlendStages = This->d3ddesc7.wMaxTextureBlendStages = 8;
	This->d3ddesc6.wMaxSimultaneousTextures = This->d3ddesc7.wMaxSimultaneousTextures = 8;
	This->d3ddesc7.dwMaxActiveLights = 8;
	This->d3ddesc7.dvMaxVertexW = 1E10f;
	This->d3ddesc7.wMaxUserClipPlanes = 0;
	This->d3ddesc7.wMaxVertexBlendMatrices = 0;
	This->d3ddesc7.dwVertexProcessingCaps =
		D3DVTXPCAPS_DIRECTIONALLIGHTS | D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_VERTEXFOG;
	This->d3ddesc7.dwReserved1 = This->d3ddesc7.dwReserved2 =
		This->d3ddesc7.dwReserved3 = This->d3ddesc7.dwReserved4 = 0;

	// TODO:  Complete initialization
	if (!hKernel32) hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	if (hKernel32)
	{
		if (!_GlobalMemoryStatusEx) _GlobalMemoryStatusEx = GetProcAddress(hKernel32, "GlobalMemoryStatusEx");
		memstatusex.dwLength = sizeof(MEMORYSTATUSEX);
		_GlobalMemoryStatusEx(&memstatusex);
		dxglcfg.SystemRAM = memstatusex.ullTotalPhys;
	}
	else
	{
		memstatus.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus(&memstatus);
		dxglcfg.SystemRAM = memstatus.dwTotalPhys;
	}
	// If NVIDIA get video RAM from GL
	if (This->ext.GLEXT_NVX_gpu_memory_info)
	{
		glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &vidmemnv);
		dxglcfg.VideoRAM = (ULONGLONG)vidmemnv * 1024;
	}
	else
	{
		// Try Win32_videocontroller
		ret = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(ret)) dxglcfg.VideoRAM = 16777216;  // Worst case scenario of COM blows up.
		else
		{
			ret = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
				RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
			if (FAILED(ret)) dxglcfg.VideoRAM = 16777216;
			else
			{
				ret = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID*)&wbemloc);
				if (FAILED(ret)) dxglcfg.VideoRAM = 16777216;
				else
				{
					cimv2 = SysAllocString(L"ROOT\\CIMV2");
					ret = wbemloc->lpVtbl->ConnectServer(wbemloc, cimv2,
						NULL, NULL, 0, NULL, 0, 0, &wbemsvc);
					SysFreeString(cimv2);
					if (FAILED(ret)) dxglcfg.VideoRAM = 16777216;
					else
					{
						ret = CoSetProxyBlanket(wbemsvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
							RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
						if(FAILED(ret)) dxglcfg.VideoRAM = 16777216;
						else
						{
							wql = SysAllocString(L"WQL");
							wqlquery = SysAllocString(L"select * from Win32_VideoController");
							ret = wbemsvc->lpVtbl->ExecQuery(wbemsvc, wql, wqlquery,
								WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
								NULL, &wbemenum);
							SysFreeString(wqlquery);
							SysFreeString(wql);
							if (FAILED(ret)) dxglcfg.VideoRAM = 16777216;
							else
							{
								while (wbemenum)
								{
									wbemenum->lpVtbl->Next(wbemenum, WBEM_INFINITE, 1, &wbemclsobj, &ret);
									if (adapterram.llVal) break;
									if (ret == 0) break;
									ret = wbemclsobj->lpVtbl->Get(wbemclsobj, L"AdapterRAM", 0, &adapterram, 0, 0);
									dxglcfg.VideoRAM = adapterram.llVal;
									VariantClear(&adapterram);
									wbemclsobj->lpVtbl->Release(wbemclsobj);
									wbemclsobj = NULL;
								}
								if (!dxglcfg.VideoRAM) dxglcfg.VideoRAM = 16777216;
							}
						}
					}
				}
				if (wbemenum) wbemenum->lpVtbl->Release(wbemenum);
				if(wbemsvc) wbemsvc->lpVtbl->Release(wbemsvc);
				if(wbemloc) wbemloc->lpVtbl->Release(wbemloc);
			}
		}

	}
	// Generate command queues
	ZeroMemory(This->commandqueue, 2 * sizeof(DXGLQueue));
	if (!dxglcfg.CmdBufferSize)
	{
		if (dxglcfg.SystemRAM > 800000000)
			dxglcfg.CmdBufferSize = 16777216;
		else if (dxglcfg.SystemRAM > 120000000)
			dxglcfg.CmdBufferSize = 8388608;
		else if (dxglcfg.SystemRAM > 60000000)
			dxglcfg.CmdBufferSize = 4194304;
		else dxglcfg.CmdBufferSize = 2097152;
	}
	if (!dxglcfg.UnpackBufferSize)
	{
		if (dxglcfg.VideoRAM > 1000000000)
			dxglcfg.UnpackBufferSize = 67108864;
		else if (dxglcfg.VideoRAM > 250000000)
			dxglcfg.UnpackBufferSize = 33554432;
		else if (dxglcfg.VideoRAM > 120000000)
			dxglcfg.UnpackBufferSize = 16777216;
		else dxglcfg.UnpackBufferSize = 8388608;
	}
	if (!dxglcfg.VertexBufferSize)
	{
		if (dxglcfg.VideoRAM > 1000000000)
			dxglcfg.VertexBufferSize = 16777216;
		else if (dxglcfg.VideoRAM > 500000000)
			dxglcfg.VertexBufferSize = 8388608;
		else if (dxglcfg.VideoRAM > 250000000)
			dxglcfg.VertexBufferSize = 4194302;
		else if (dxglcfg.VideoRAM > 120000000)
			dxglcfg.VertexBufferSize = 2097152;
		else if (dxglcfg.VideoRAM > 60000000)
			dxglcfg.VertexBufferSize = 1048576;
		else dxglcfg.VertexBufferSize = 524288;
	}
	if (!dxglcfg.IndexBufferSize)
	{
		if (dxglcfg.VideoRAM > 1000000000)
			dxglcfg.IndexBufferSize = 4194302;
		else if (dxglcfg.VideoRAM > 500000000)
			dxglcfg.IndexBufferSize = 2097152;
		else if (dxglcfg.VideoRAM > 250000000)
			dxglcfg.IndexBufferSize = 1048576;
		else if (dxglcfg.VideoRAM > 120000000)
			dxglcfg.IndexBufferSize = 524288;
		else if (dxglcfg.VideoRAM > 60000000)
			dxglcfg.IndexBufferSize = 262144;
		else dxglcfg.IndexBufferSize = 131072;
	}
	This->commandqueue[0].commands = (char*)malloc(dxglcfg.CmdBufferSize);
	if (!This->commandqueue[0].commands)
	{
		wglMakeCurrent(0, 0);
		wglDeleteContext(This->hrc);
		ReleaseDC(This->hWndContext, This->hdc);
		SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
		WaitForSingleObject(This->WindowThreadHandle, INFINITE);
		return DDERR_OUTOFMEMORY;
	}
	This->commandqueue->commandsize = This->commandqueue[1].commandsize = dxglcfg.CmdBufferSize;
	This->commandqueue[1].commands = (char*)malloc(dxglcfg.CmdBufferSize);
	if (!This->commandqueue[1].commands)
	{
		free(This->commandqueue[0].commands);
		wglMakeCurrent(0, 0);
		wglDeleteContext(This->hrc);
		ReleaseDC(This->hWndContext, This->hdc);
		SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
		WaitForSingleObject(This->WindowThreadHandle, INFINITE);
		return DDERR_OUTOFMEMORY;
	}
	This->ext.glGenBuffers(1, &This->commandqueue[0].pixelbuffer);
	This->ext.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, This->commandqueue[0].pixelbuffer);
	This->ext.glBufferData(GL_PIXEL_UNPACK_BUFFER, dxglcfg.UnpackBufferSize, NULL, GL_DYNAMIC_DRAW);
	This->commandqueue[0].pixelbuffersize = dxglcfg.UnpackBufferSize;
	This->commandqueue[0].pixelbufferptr = This->ext.glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	This->ext.glGenBuffers(1, &This->commandqueue[1].pixelbuffer);
	This->ext.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, This->commandqueue[1].pixelbuffer);
	This->ext.glBufferData(GL_PIXEL_UNPACK_BUFFER, dxglcfg.UnpackBufferSize, NULL, GL_DYNAMIC_DRAW);
	This->commandqueue[1].pixelbuffersize = dxglcfg.UnpackBufferSize;
	This->commandqueue[1].pixelbufferptr = This->ext.glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	This->ext.glGenBuffers(1, &This->commandqueue[0].vertexbuffer);
	This->ext.glBindBuffer(GL_ARRAY_BUFFER, This->commandqueue[0].vertexbuffer);
	This->ext.glBufferData(GL_ARRAY_BUFFER, dxglcfg.VertexBufferSize, NULL, GL_DYNAMIC_DRAW);
	This->commandqueue[0].vertexbuffersize = dxglcfg.VertexBufferSize;
	This->commandqueue[0].vertexbufferptr = This->ext.glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	This->ext.glGenBuffers(1, &This->commandqueue[1].vertexbuffer);
	This->ext.glBindBuffer(GL_ARRAY_BUFFER, This->commandqueue[1].vertexbuffer);
	This->ext.glBufferData(GL_ARRAY_BUFFER, dxglcfg.VertexBufferSize, NULL, GL_DYNAMIC_DRAW);
	This->commandqueue[1].vertexbuffersize = dxglcfg.VertexBufferSize;
	This->commandqueue[1].vertexbufferptr = This->ext.glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	This->ext.glGenBuffers(1, &This->commandqueue[0].indexbuffer);
	This->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, This->commandqueue[0].indexbuffer);
	This->ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER, dxglcfg.IndexBufferSize, NULL, GL_DYNAMIC_DRAW);
	This->commandqueue[0].indexbuffersize = dxglcfg.IndexBufferSize;
	This->commandqueue[0].indexbufferptr = This->ext.glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	This->ext.glGenBuffers(1, &This->commandqueue[1].indexbuffer);
	This->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, This->commandqueue[1].indexbuffer);
	This->ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER, dxglcfg.IndexBufferSize, NULL, GL_DYNAMIC_DRAW);
	This->commandqueue[1].indexbuffersize = dxglcfg.IndexBufferSize;
	This->commandqueue[1].indexbufferptr = This->ext.glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	This->queueindexread = 0;
	This->queueindexwrite = 0;

	// Thread initialization done
	This->running = FALSE;
	This->shutdown = FALSE;
	SetEvent(This->SyncEvent);

	// Main loop
	while (This->shutdown == FALSE)
	{
		// Wait for startup
		if (!This->running) WaitForSingleObject(This->StartEvent, INFINITE);
		EnterCriticalSection(&This->cs);
		This->running = TRUE;
		This->currentqueue = &This->commandqueue[This->queueindexread];
		This->currentqueue->busy = TRUE;
		This->ext.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, This->currentqueue->pixelbuffer);
		This->ext.glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		This->currentqueue->pixelbufferptr = NULL;
		This->ext.glBindBuffer(GL_ARRAY_BUFFER, This->currentqueue->pixelbuffer);
		This->ext.glUnmapBuffer(GL_ARRAY_BUFFER);
		This->currentqueue->vertexbufferptr = NULL;
		This->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, This->currentqueue->pixelbuffer);
		This->ext.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		This->ext.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		LeaveCriticalSection(&This->cs);
		queuepos = pixelpos = vertexpos = indexpos = 0;
		while (This->running)
		{
			lastcmd = currcmd->cmd.command;
			currcmd = (DXGLQueueCmdDecoder*)&This->currentqueue->commands[queuepos];
			switch (currcmd->cmd.command)
			{
			case QUEUEOP_QUIT:  // End render device
				This->shutdown = TRUE;
				This->running = FALSE;
				break;
			case QUEUEOP_NULL:  // Do nothing
				break;
			case QUEUEOP_RESET:  // Reset device 
				DXGLRendererGL__Reset(This);
				break;
			case QUEUEOP_CREATETEXTURE:  // Create a texture or surface
				DXGLTextureGL__FinishCreate(currcmd->createtexture.out, This, &This->ext, &This->util);
				break;
			case QUEUEOP_DELETETEXTURE:  // Delete one or more textures or surfaces
				for (index = 0; index < currcmd->deletetexture.count; index++)
					glDeleteTextures(1, &currcmd->deletetexture.texture[index].glhandle);
				break;
			case QUEUEOP_SETTEXTURE:
				DXGLRendererGL__SetTexture(This, currcmd->settexture.level, currcmd->settexture.texture);
				break;
			case QUEUEOP_SETTARGET:
				DXGLRendererGL__SetTarget(This, currcmd->settarget.texture, currcmd->settarget.miplevel);
				break;
			case QUEUEOP_SETRENDERSTATE:
				DXGLRendererGL__SetRenderState(This, &currcmd->setrenderstate.state);
				break;
			case QUEUEOP_EXPANDBUFFERS: // Expand the buffers, write error to start of queue
				// Command should only be placed by itself then wait until it finished
				if (currcmd->expandbuffers.size > This->currentqueue->commandsize)
				{
					tmp = realloc(This->currentqueue->commands, currcmd->expandbuffers.size);
					if (!tmp)
					{
						*((DWORD*)This->currentqueue->commands) = DDERR_OUTOFMEMORY;
						break;
					}
					else
					{
						This->currentqueue->commandsize = currcmd->expandbuffers.size;
						This->currentqueue->commands = tmp;
					}
				}
				if (currcmd->expandbuffers.pixelsize > This->currentqueue->pixelbuffersize)
				{
					glUtil_ClearErrors(&This->util);
					
					This->ext.glBindBuffer(GL_PIXEL_PACK_BUFFER, This->currentqueue->pixelbuffer);
					This->ext.glBufferData(GL_PIXEL_PACK_BUFFER, currcmd->expandbuffers.pixelsize, NULL, GL_DYNAMIC_DRAW);
					if (glGetError() != GL_NO_ERROR)
					{
						*((DWORD*)This->currentqueue->commands) = DDERR_OUTOFVIDEOMEMORY;
						break;
					}
					else This->currentqueue->pixelbuffersize = currcmd->expandbuffers.pixelsize;
				}
				if (currcmd->expandbuffers.vertexsize > This->currentqueue->vertexbuffersize)
				{
					glUtil_ClearErrors(&This->util);

					This->ext.glBindBuffer(GL_ARRAY_BUFFER, This->currentqueue->vertexbuffer);
					This->ext.glBufferData(GL_ARRAY_BUFFER, currcmd->expandbuffers.vertexsize, NULL, GL_DYNAMIC_DRAW);
					if (glGetError() != GL_NO_ERROR)
					{
						*((DWORD*)This->currentqueue->commands) = DDERR_OUTOFVIDEOMEMORY;
						break;
					}
					else This->currentqueue->vertexbuffersize = currcmd->expandbuffers.vertexsize;
				}
				if (currcmd->expandbuffers.indexsize > This->currentqueue->indexbuffersize)
				{
					glUtil_ClearErrors(&This->util);

					This->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, This->currentqueue->indexbuffer);
					This->ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER, currcmd->expandbuffers.indexsize, NULL, GL_DYNAMIC_DRAW);
					if (glGetError() != GL_NO_ERROR)
					{
						*((DWORD*)This->currentqueue->commands) = DDERR_OUTOFVIDEOMEMORY;
						break;
					}
					else This->currentqueue->indexbuffersize = currcmd->expandbuffers.indexsize;
				}
				*((DWORD*)This->currentqueue->commands) = DD_OK;
				break;
			case QUEUEOP_LOCK:  // Lock a texture
				DXGLTextureGL__Lock(currcmd->lock.texture, &This->ext, currcmd->lock.miplevel, currcmd->lock.ptr);
				break;
			case QUEUEOP_UNLOCK: // Unlock a texture
				DXGLTextureGL__Unlock(currcmd->unlock.texture, &This->ext, currcmd->unlock.miplevel);
				break;
			case QUEUEOP_SETWINDOWSIZE: // Set the window viewport size
				glUtil_SetViewport(&This->util, currcmd->setwindowsize.r.left, currcmd->setwindowsize.r.top,
					currcmd->setwindowsize.r.right - currcmd->setwindowsize.r.left,
					currcmd->setwindowsize.r.bottom - currcmd->setwindowsize.r.top);
				break;
			case QUEUEOP_SETFVF:
				if (currcmd->setfvf.fvf == This->fvf) break;
				This->fvf = currcmd->setfvf.fvf;
				break;
			default:            // Unknown, probably will crash
				FIXME("Detected an unknown command.")
				break;
			}
			// Move queue forward
			queuepos += currcmd->cmd.size;
			if (queuepos >= This->currentqueue->commandwrite)
			{
				// Flip buffers or end execution
				EnterCriticalSection(&This->cs);
				This->ext.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, This->currentqueue->pixelbuffer);
				This->currentqueue->pixelbufferptr = This->ext.glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
				This->ext.glBindBuffer(GL_ARRAY_BUFFER, This->currentqueue->pixelbuffer);
				This->ext.glUnmapBuffer(GL_ARRAY_BUFFER);
				This->currentqueue->vertexbufferptr = This->ext.glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
				This->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, This->currentqueue->pixelbuffer);
				This->currentqueue->indexbufferptr = This->ext.glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
				This->currentqueue->busy = FALSE;
				This->currentqueue->filled = FALSE;
				This->currentqueue->commandwrite = This->currentqueue->pixelbufferwrite =
					This->currentqueue->vertexbufferwrite = This->currentqueue->indexbufferwrite = 0;
				This->queueindexread = (This->queueindexread ^ 1) & 1;
				This->currentqueue = &This->commandqueue[This->queueindexread];
				if (!This->currentqueue->filled)
				{
					if (This->waitsync && ((This->syncptr == 0) || (This->syncptr == 1)))
					{
						This->waitsync = 0;
						SetEvent(This->SyncEvent);
					}
					This->running = FALSE;
				}
				else if (This->waitsync && (This->syncptr == 1))
				{
					This->waitsync = 0;
					SetEvent(This->SyncEvent);
				}
				queuepos = pixelpos = vertexpos = indexpos = 0;
				LeaveCriticalSection(&This->cs);
			}
		}
	}


	// Begin shutdown

	// Final shutdown
	wglMakeCurrent(0, 0);
	wglDeleteContext(This->hrc);
	ReleaseDC(This->hWndContext, This->hdc);
	SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
	WaitForSingleObject(This->WindowThreadHandle, INFINITE);
	return DD_OK;
}

HRESULT WINAPI DXGLRendererGL_GetAttachedDevice(LPDXGLRENDERERGL This, struct glDirectDraw7 **glDD7)
{
	*glDD7 = This->glDD7;
	return DD_OK;
}

HRESULT WINAPI DXGLRendererGL_SetAttachedDevice(LPDXGLRENDERERGL This, struct glDirectDraw7 *glDD7)
{
	This->glDD7 = glDD7;
	if (!This->glDD7) {}
	return DD_OK;
}

HRESULT WINAPI DXGLRendererGL_GetCaps(LPDXGLRENDERERGL This, DWORD index, void *output)
{
	//TODO:  Retrieve memory status
	DWORD size;
	if (!output) return DDERR_INVALIDPARAMS;
	switch (index)
	{
	case 0: // DDraw caps
		size = ((LPDDCAPS)output)->dwSize;
		if (size > sizeof(DDCAPS_DX7)) size = sizeof(DDCAPS_DX7);
		memcpy(output, &This->ddcaps, size);
		return DD_OK;
	case 1: // D3D6 caps
		size = ((LPD3DDEVICEDESC)output)->dwSize;
		if (size > sizeof(D3DDEVICEDESC)) size = sizeof(D3DDEVICEDESC);
		memcpy(output, &This->d3ddesc6, size);
		((LPD3DDEVICEDESC)output)->dwSize = size;
		return DD_OK;
	case 2: // D3D7 caps
		memcpy(output, &This->d3ddesc7, sizeof(D3DDEVICEDESC7));
		return DD_OK;
	default:
		return DDERR_INVALIDPARAMS;
	}
}

//Resets the renderer
HRESULT WINAPI DXGLRendererGL_Reset(LPDXGLRENDERERGL This)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmd cmddata;
	cmddata.command = QUEUEOP_RESET;
	cmddata.size = sizeof(DXGLQueueCmd);
	cmd.data = &cmddata;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Syncs, with an optional address of a resource to wait for
HRESULT WINAPI DXGLRendererGL_Sync(LPDXGLRENDERERGL This, void *ptr)
{
	BOOL running;
	EnterCriticalSection(&This->cs);
	running = This->running;
	LeaveCriticalSection(&This->cs);
	if (running)
	{
		This->waitsync = TRUE;
		This->syncptr = ptr;
	syncwait:
		WaitForSingleObject(This->SyncEvent, 10);
		if (This->waitsync && This->running) goto syncwait;
	}
	return DD_OK;
}

// Locks a texture surface and returns a pointer to read or write its contents
HRESULT WINAPI DXGLRendererGL_Lock(LPDXGLRENDERERGL This, DXGLTexture *texture, GLuint miplevel, BYTE **pointer)
{
	HRESULT error;
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdLock cmddata;
	if (!texture) return DDERR_INVALIDOBJECT;
	if (miplevel >= texture->mipcount) return DDERR_INVALIDPARAMS;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.size = sizeof(DXGLQueueCmdLock);
	cmddata.command = QUEUEOP_LOCK;
	cmddata.texture = texture;
	cmddata.miplevel = miplevel;
	cmddata.ptr = pointer;
	error = DXGLRendererGL_PostCommand(This, &cmd);
	if (FAILED(error)) return error;
	DXGLRendererGL_Sync(This, 0);
}

// Unlocks a texture surface and commits the contents to the texture
HRESULT WINAPI DXGLRendererGL_Unlock(LPDXGLRENDERERGL This, DXGLTexture *texture, GLuint miplevel)
{
	HRESULT error;
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdUnlock cmddata;
	if (!texture) return DDERR_INVALIDOBJECT;
	if (miplevel >= texture->mipcount) return DDERR_INVALIDPARAMS;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.size = sizeof(DXGLQueueCmdLock);
	cmddata.command = QUEUEOP_UNLOCK;
	cmddata.texture = texture;
	cmddata.miplevel = miplevel;
	error = DXGLRendererGL_PostCommand(This, &cmd);
	if (FAILED(error)) return error;
	DXGLRendererGL_Sync(This, 0);
}

HRESULT WINAPI DXGLRendererGL_SetWindowSize(LPDXGLRENDERERGL This, RECT *r)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdSetWindowSize cmddata;
	if (!r) return DDERR_INVALIDPARAMS;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.size = sizeof(DXGLQueueCmdSetWindowSize);
	cmddata.command = QUEUEOP_SETWINDOWSIZE;
	memcpy(&cmddata.r, r, sizeof(RECT));
	return DXGLRendererGL_PostCommand(This, &cmd);
}

void DXGLRendererGL_FlipWrite(LPDXGLRENDERERGL This)
{
	EnterCriticalSection(&This->cs);
	This->queueindexwrite = (This->queueindexwrite ^ 1) & 1;
	LeaveCriticalSection(&This->cs);
	//if (This->commandqueue[This->queueindexwrite].busy) DXGLRendererGL_Sync(This, 1);
}

HRESULT WINAPI DXGLRendererGL_PostCommand(LPDXGLRENDERERGL This, DXGLPostQueueCmd* cmd)
{
	return DXGLRendererGL_PostCommand2(This, cmd, FALSE);
}


HRESULT WINAPI DXGLRendererGL_PostCommand2(LPDXGLRENDERERGL This, DXGLPostQueueCmd *cmd, BOOL inner)
{
	GLuint tmpbuffer;
	ULONG_PTR newsize;
	void *tmp;
	DXGLQueueCmdDecoder *cmdptr;
	DXGLQueue* queue;
	DXGLQueueCmdExpandBuffers expandbufferscmd =
	{ QUEUEOP_EXPANDBUFFERS,sizeof(DXGLQueueCmdExpandBuffers),0,0,0,0 };
	DXGLPostQueueCmd expandbufferscmd2 = { &expandbufferscmd,NULL,0,NULL,0,NULL,0 };
	BOOL fliprequired = FALSE;
	BOOL expandrequired = FALSE;
	WORD indices[6];
	int i;
	EnterCriticalSection(&This->cs);
	// Check if running
	if (!This->running)
	{
		if (This->queueindexread != This->queueindexwrite) DXGLRendererGL_FlipWrite(This);
	}
	queue = &This->commandqueue[This->queueindexwrite];
	if (!inner)
	{
		if (cmd->data->size > queue->commandsize - queue->commandwrite)
		{
			if (cmd->data->size > queue->commandsize / 2)
			{
				expandbufferscmd.cmdsize =
					cmd->data->size > (queue->commandsize + 2097152) ?
					NextMultipleOf1024(cmd->data->size) : (queue->commandsize + 2097152);
				expandrequired = TRUE;
			}
			fliprequired = TRUE;
		}
		if (cmd->pixelsize > queue->pixelbuffersize - queue->pixelbufferwrite)
		{
			if (cmd->pixelsize > queue->pixelbuffersize / 2)
			{
				expandbufferscmd.pixelsize =
					cmd->pixelsize > (queue->pixelbuffersize + 2097152) ?
					NextMultipleOf1024(cmd->pixelsize) : (queue->pixelbuffersize + 2097152);
				expandrequired = TRUE;
			}
			fliprequired = TRUE;
		}
		if (cmd->vertexsize > queue->vertexbuffersize - queue->vertexbufferwrite)
		{
			if (cmd->vertexsize > queue->vertexbuffersize / 2)
			{
				expandbufferscmd.vertexsize =
					cmd->vertexsize > (queue->vertexbuffersize + 2097152) ?
					NextMultipleOf1024(cmd->vertexsize) : (queue->vertexbuffersize + 2097152);
				expandrequired = TRUE;
			}
			fliprequired = TRUE;
		}
		if (cmd->indexsize > queue->indexbuffersize - queue->indexbufferwrite)
		{
			if (cmd->indexsize > queue->indexbuffersize / 2)
			{
				expandbufferscmd.indexsize =
					cmd->indexsize > (queue->indexbuffersize + 2097152) ?
					NextMultipleOf1024(cmd->indexsize) : (queue->indexbuffersize + 2097152);
				expandrequired = TRUE;
			}
			fliprequired = TRUE;
		}
		if (fliprequired) DXGLRendererGL_FlipWrite(This);
		queue = &This->commandqueue[This->queueindexwrite];
		if (expandrequired)
		{
			// Expand both buffers
			LeaveCriticalSection(&This->cs);
			DXGLRendererGL_PostCommand2(This, &expandbufferscmd2, TRUE);
			DXGLRendererGL_Sync(This, 0);
			if (FAILED(*((HRESULT*)queue->commands))) return *((DWORD*)queue->commands);
			DXGLRendererGL_FlipWrite(This);
			queue = &This->commandqueue[This->queueindexwrite];
			DXGLRendererGL_PostCommand2(This, &expandbufferscmd2, TRUE);
			DXGLRendererGL_Sync(This, 0);
			if (FAILED(*((HRESULT*)queue->commands))) return *((DWORD*)queue->commands);
			EnterCriticalSection(&This->cs);
			DXGLRendererGL_FlipWrite(This);
			queue = &This->commandqueue[This->queueindexwrite];
		}
	}

	// Append commmand to buffer
	switch (cmd->data->command)
	{
	case QUEUEOP_DRAWPRIMITIVES2D:
		if (queue->lastcommand.command == QUEUEOP_DRAWPRIMITIVES2D)
		{   // Append to previous command
			cmdptr = queue->commands + queue->lastcommand.ptr;
			memcpy(indices, bltindices, 6 * sizeof(WORD));
			for (i = 0; i < 6; i++)
				indices[i] += cmdptr->drawprimitives.vertexcount;
			cmdptr->drawprimitives.vertexcount += 4;
			cmdptr->drawprimitives.indexcount += 6;
			memcpy(queue->vertexbufferptr + queue->vertexbufferwrite,
				cmdptr->drawprimitives2d.vertices, cmdptr->drawprimitives2d.vertexcount * sizeof(D3DTLVERTEX));
			queue->vertexbufferwrite += 4 * sizeof(D3DTLVERTEX);
			memcpy(queue->indexbufferptr + queue->indexbufferwrite, indices, 6 * sizeof(WORD));
			queue->indexbufferwrite += 6 * sizeof(WORD);
		}
		else
		{
			queue->lastcommand.command = cmd->data->command;
			queue->lastcommand.ptr = queue->commandwrite;
			cmdptr = queue->commands + queue->commandwrite;
			cmdptr->drawprimitives.command = QUEUEOP_DRAWPRIMITIVES2D;
			cmdptr->drawprimitives.size = sizeof(DXGLQueueCmdDrawPrimitives);
			cmdptr->drawprimitives.type = D3DPT_TRIANGLELIST;
			cmdptr->drawprimitives.vertexcount = 4;
			cmdptr->drawprimitives.indexcount = 6;
			cmdptr->drawprimitives.vertices = queue->vertexbufferwrite;
			cmdptr->drawprimitives.indices = queue->indexbufferwrite;
			queue->commandwrite += cmd->data->size;
			memcpy(queue->vertexbufferptr + queue->vertexbufferwrite,
				cmdptr->drawprimitives2d.vertices, cmdptr->drawprimitives2d.vertexcount * sizeof(D3DTLVERTEX));
			queue->vertexbufferwrite += 4 * sizeof(D3DTLVERTEX);
			memcpy(queue->indexbufferptr + queue->indexbufferwrite, bltindices, 6 * sizeof(WORD));
			queue->indexbufferwrite += 6 * sizeof(WORD);
		}
		break;
	default:
		queue->lastcommand.command = cmd->data->command;
		memcpy(queue->commands + queue->commandwrite, cmd->data, cmd->data->size);
		queue->commandwrite += cmd->data->size;
		if (cmd->pixelsize)
		{
			memcpy(queue->pixelbufferptr + queue->pixelbufferwrite, cmd->pixelbuffer, cmd->pixelsize);
			queue->pixelbufferwrite += cmd->pixelsize;
		}
		if (cmd->vertexsize)
		{
			memcpy(queue->vertexbufferptr + queue->vertexbufferwrite, cmd->vertexbuffer, cmd->vertexsize);
			queue->vertexbufferwrite += cmd->vertexsize;
		}
		if (cmd->indexsize)
		{
			memcpy(queue->indexbufferptr + queue->indexbufferwrite, cmd->indexbuffer, cmd->indexsize);
			queue->indexbufferwrite += cmd->indexsize;
		}
		break;
	}
	if (!This->running)
	{
		This->running = TRUE;
		SetEvent(This->StartEvent);
	}
	LeaveCriticalSection(&This->cs);
}

// Posts a break command when using GDebugger
HRESULT WINAPI DXGLRendererGL_Break(LPDXGLRENDERERGL This)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmd cmddata;
	cmddata.command = QUEUEOP_BREAK;
	cmddata.size = sizeof(DXGLQueueCmd);
	cmd.data = &cmddata;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Posts a command to free a pointer, ensuring all uses are finished
HRESULT WINAPI DXGLRendererGL_FreePointer(LPDXGLRENDERERGL This, void *ptr)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdFreePointer cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_FREEPOINTER;
	cmddata.size = sizeof(DXGLQueueCmdFreePointer);
	cmddata.count = 1;
	cmddata.ptr = ptr;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Sets the DDraw Cooperative Level
HRESULT WINAPI DXGLRendererGL_SetCooperativeLevel(LPDXGLRENDERERGL This, HWND hWnd, DWORD dwFlags)
{
	BOOL exclusive;
	DWORD winver = GetVersion();
	DWORD winvermajor = (DWORD)(LOBYTE(LOWORD(winver)));
	DWORD winverminor = (DWORD)(HIBYTE(LOWORD(winver)));
	DEVMODE devmode;
	RECT wndrect;
	HWND hTempWnd;
	WINDOWPLACEMENT wndplace;
	int screenx, screeny;
	DXGLRendererGL_Sync(This, 0);
	if (((hWnd != This->hWndTarget) && This->hWndTarget) || (This->hWndTarget && (dwFlags & DDSCL_NORMAL)))
	{
		if (This->winstyle)
		{
			SetWindowLongPtrA(This->hWndTarget, GWL_STYLE, This->winstyle);
			SetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE, This->winstyleex);
			ShowWindow(This->hWndTarget, SW_RESTORE);
			This->winstyle = This->winstyleex = 0;
			SetWindowPos(This->hWndTarget, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		}
	}
	if (This->hWndTarget) UninstallDXGLHook(This->hWndTarget);
	This->hWndTarget = hWnd;
	if (!This->winstyle && !This->winstyleex)
	{
		This->winstyle = GetWindowLongPtrA(hWnd, GWL_STYLE);
		This->winstyleex = GetWindowLongPtrA(hWnd, GWL_EXSTYLE);
	}
	exclusive = FALSE;
	if (dwFlags & DDSCL_ALLOWMODEX)
	{
		// Validate flags
		if (dwFlags & (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE))
		{
			DEBUG("IDirectDraw::SetCooperativeLevel: Mode X not supported, ignoring\n");
		}
		else DEBUG("IDirectDraw::SetCooperativeLevel: DDSCL_ALLOWMODEX without DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE\n");
	}
	if (dwFlags & DDSCL_EXCLUSIVE)
		exclusive = TRUE;
	else exclusive = FALSE;
	if (dwFlags & DDSCL_FULLSCREEN)
		This->fullscreen = TRUE;
	else This->fullscreen = FALSE;
	/*if (dwFlags & DDSCL_CREATEDEVICEWINDOW)
	{
		if (!exclusive) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		This->devwnd = true;
	}*/
	if (dwFlags & DDSCL_SETDEVICEWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETDEVICEWINDOW unsupported\n");
	if (dwFlags & DDSCL_SETFOCUSWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETFOCUSWINDOW unsupported\n");
	InstallDXGLHook(hWnd, (LPDIRECTDRAW7)This);
	EnableWindowScaleHook(FALSE);
	ZeroMemory(&devmode, sizeof(DEVMODE));
	devmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	if (This->fullscreen)
	{
		This->width = This->vidwidth = This->restorewidth = devmode.dmPelsWidth;
		This->height = This->vidheight = This->restoreheight = devmode.dmPelsHeight;
	}
	else
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		if ((winvermajor > 4) || ((winvermajor == 4) && (winverminor >= 1)))
		{
			This->width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			This->height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		}
		else
		{ // Windows versions below 4.1 don't support multi-monitor
			This->width = devmode.dmPelsWidth;
			This->height = devmode.dmPelsHeight;
		}
		This->vidwidth = This->restorewidth = (DWORD)((float)This->width / dxglcfg.WindowScaleX);
		This->vidheight = This->restoreheight = (DWORD)((float)This->height / dxglcfg.WindowScaleY);
		if ((dxglcfg.WindowScaleX != 1.0f) || (dxglcfg.WindowScaleY != 1.0f))
		{
			GetWindowRect(hWnd, &rect);
			EnableWindowScaleHook(TRUE);
			SetWindowPos(hWnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING);
		}
	}
	This->vidbpp = This->restorebpp = devmode.dmBitsPerPel;
	This->vidrefresh = This->restorerefresh = devmode.dmDisplayFrequency;
	This->cooplevel = dwFlags;

	if (This->fullscreen)
	{
		switch (dxglcfg.fullmode)
		{
		case 0:    // Fullscreen
			This->winstyle = GetWindowLongPtrA(This->hWndTarget, GWL_STYLE);
			This->winstyleex = GetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE, This->winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWndTarget, GWL_STYLE, (This->winstyle | WS_POPUP) & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER));
			ShowWindow(This->hWndTarget, SW_MAXIMIZE);
			break;
		case 1:    // Non-exclusive Fullscreen
		case 5:    // Windowed borderless scaled
			This->winstyle = GetWindowLongPtrA(This->hWndTarget, GWL_STYLE);
			This->winstyleex = GetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE, This->winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWndTarget, GWL_STYLE, This->winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(This->hWndTarget, SW_MAXIMIZE);
			break;
		case 2:     // Windowed non-resizable
			This->winstyle = GetWindowLongPtrA(This->hWndTarget, GWL_STYLE);
			This->winstyleex = GetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE, This->winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(This->hWndTarget, GWL_STYLE, (This->winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP));
			ShowWindow(This->hWndTarget, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (This->winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(This->winstyleex | WS_EX_APPWINDOW));
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (This->winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(This->winstyleex | WS_EX_APPWINDOW));
				wndrect.right -= wndrect.left;
				wndrect.left = 0;
				wndrect.bottom -= wndrect.top;
				wndrect.top = 0;
			}
			else if (dxglcfg.WindowPosition == 3)
			{
				if (!wndclassdxgltempatom) RegisterDXGLTempWindowClass();
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (This->winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(This->winstyleex | WS_EX_APPWINDOW));
				hTempWnd = CreateWindow(wndclassdxgltemp.lpszClassName, _T("DXGL Sizing Window"),
					(This->winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE),
					CW_USEDEFAULT, CW_USEDEFAULT, wndrect.right - wndrect.left, wndrect.bottom - wndrect.top, NULL, NULL,
					GetModuleHandle(NULL), NULL);
				GetWindowRect(hTempWnd, &wndrect);
				DestroyWindow(hTempWnd);
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
				AdjustWindowRectEx(&wndrect, (This->winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(This->winstyleex | WS_EX_APPWINDOW));
			}
			SetWindowPos(This->hWndTarget, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			break;
		case 3:     // Windowed resizable
			This->winstyle = GetWindowLongPtrA(This->hWndTarget, GWL_STYLE);
			This->winstyleex = GetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE, This->winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(This->hWndTarget, GWL_STYLE, (This->winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP);
			ShowWindow(This->hWndTarget, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, This->winstyle | WS_OVERLAPPEDWINDOW, FALSE, (This->winstyleex | WS_EX_APPWINDOW));
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, This->winstyle | WS_OVERLAPPEDWINDOW, FALSE, (This->winstyleex | WS_EX_APPWINDOW));
				wndrect.right -= wndrect.left;
				wndrect.left = 0;
				wndrect.bottom -= wndrect.top;
				wndrect.top = 0;
			}
			else if (dxglcfg.WindowPosition == 3)
			{
				if (!wndclassdxgltempatom) RegisterDXGLTempWindowClass();
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (This->winstyle | WS_OVERLAPPEDWINDOW), FALSE, (This->winstyleex | WS_EX_APPWINDOW));
				hTempWnd = CreateWindow(wndclassdxgltemp.lpszClassName, _T("DXGL Sizing Window"),
					((This->winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_POPUP | WS_VISIBLE)), CW_USEDEFAULT, CW_USEDEFAULT,
					wndrect.right - wndrect.left, wndrect.bottom - wndrect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
				GetWindowRect(hTempWnd, &wndrect);
				DestroyWindow(hTempWnd);
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
				AdjustWindowRectEx(&wndrect, This->winstyle | WS_OVERLAPPEDWINDOW, FALSE, (This->winstyleex | WS_EX_APPWINDOW));
			}
			wndplace.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(This->hWndTarget, &wndplace);
			wndplace.flags = WPF_ASYNCWINDOWPLACEMENT;
			if (dxglcfg.WindowMaximized == 1) wndplace.showCmd = SW_SHOWMAXIMIZED;
			else wndplace.showCmd = SW_SHOWNORMAL;
			wndplace.rcNormalPosition = wndrect;
			SetWindowPlacement(This->hWndTarget, &wndplace);
			break;
		case 4:     // Windowed borderless
			This->winstyle = GetWindowLongPtrA(This->hWndTarget, GWL_STYLE);
			This->winstyleex = GetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWndTarget, GWL_EXSTYLE, This->winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWndTarget, GWL_STYLE, This->winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(This->hWndTarget, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
			}
			else if (dxglcfg.WindowPosition == 3)
			{
				if (!wndclassdxgltempatom) RegisterDXGLTempWindowClass();
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				hTempWnd = CreateWindow(wndclassdxgltemp.lpszClassName, _T("DXGL Sizing Window"),
					This->winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP | WS_VISIBLE), CW_USEDEFAULT, CW_USEDEFAULT,
					wndrect.right - wndrect.left, wndrect.bottom - wndrect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
				GetWindowRect(hTempWnd, &wndrect);
				DestroyWindow(hTempWnd);
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
			}
			SetWindowPos(This->hWndTarget, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			break;
		}
	}
	SetWindowPos(This->hWndTarget, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	// Adjust render window
	if (!This->hWndTarget)
	{
		SetWindowLongPtr(This->hWndContext, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
		SetParent(This->hWndContext, NULL);
		SetWindowPos(This->hWndContext, HWND_TOPMOST, 0, 0, This->width, This->height, SWP_SHOWWINDOW);
	}
	else
	{
		SetWindowLongPtr(This->hWndContext, GWL_EXSTYLE, 0);
		SetParent(This->hWndContext, This->hWndTarget);
		SetWindowPos(This->hWndContext, HWND_TOP, 0, 0, This->width, This->height, SWP_SHOWWINDOW);
	}
}

// Creates one or more textures, determined by ddsd
HRESULT WINAPI DXGLRendererGL_CreateTexture(LPDXGLRENDERERGL This, LPDDSURFACEDESC2 desc, DWORD bpp, DXGLTexture *out)
{
	HRESULT error;
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdCreateTexture cmddata;
	error = DXGLTextureGL_Create(desc, bpp, &This->ext, out);
	if(FAILED(error)) return error;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_CREATETEXTURE;
	cmddata.size = sizeof(DXGLQueueCmdCreateTexture);
	memcpy(&cmddata.desc, &desc, sizeof(DDSURFACEDESC2));
	cmddata.out = out;
	error = DXGLRendererGL_PostCommand(This, &cmd);
	if (SUCCEEDED(error)) DXGLRendererGL_Sync(This, NULL);
	return error;
}

// Posts a command to delete a texture, ensuring all uses are finished
// Post this command before freeing the buffer holding the texture structures
HRESULT WINAPI DXGLRendererGL_DeleteTexture(LPDXGLRENDERERGL This, DXGLTexture *texture)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdDeleteTexture cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_DELETETEXTURE;
	cmddata.size = sizeof(DXGLQueueCmdDeleteTexture);
	cmddata.count = 1;
	cmddata.texture = texture;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Sets the current texture in a texture unit
HRESULT WINAPI DXGLRendererGL_SetTexture(LPDXGLRENDERERGL This, GLuint level, DXGLTexture *texture)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdSetTexture cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_SETTEXTURE;
	cmddata.size = sizeof(DXGLQueueCmdSetTexture);
	cmddata.level = level;
	cmddata.texture = texture;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Sets the current render target
HRESULT WINAPI DXGLRendererGL_SetTarget(LPDXGLRENDERERGL This, DXGLTexture *texture, GLuint miplevel)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdSetTarget cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_SETTARGET;
	cmddata.size = sizeof(DXGLQueueCmdSetTarget);
	cmddata.texture = texture;
	cmddata.miplevel = miplevel;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Sets a render state value or the 2D render state
HRESULT WINAPI DXGLRendererGL_SetRenderState(LPDXGLRENDERERGL This, DXGLRenderState *state)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdSetRenderState cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_SETRENDERSTATE;
	cmddata.size = sizeof(DXGLQueueCmdSetRenderState);
	memcpy(&cmddata.state, state, sizeof(DXGLRenderState));
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Sets the current vertex format in Direct3D FVF format
HRESULT WINAPI DXGLRendererGL_SetFVF(LPDXGLRENDERERGL This, DWORD fvf)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdSetFVF cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_SETFVF;
	cmddata.size = sizeof(DXGLQueueCmdSetFVF);
	cmddata.fvf = fvf;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Draws vertices in 2D space.  Subsequent calls will be combined for performance.
HRESULT WINAPI DXGLRendererGL_DrawPrimitives2D(LPDXGLRENDERERGL This, D3DPRIMITIVETYPE type, const BYTE *vertices, DWORD vertexcount)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdDrawPrimitives2D cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmd.vertexsize = 4 * sizeof(D3DTLVERTEX);
	cmd.indexsize = 6 * sizeof(WORD);
	cmddata.command = QUEUEOP_DRAWPRIMITIVES2D;
	cmddata.size = sizeof(DXGLQueueCmdDrawPrimitives2D);
	cmddata.type = type;
	cmddata.vertices = vertices;
	cmddata.vertexcount = vertexcount;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Flips the OpenGL buffers at the specified sync interval
HRESULT WINAPI DXGLRendererGL_SwapBuffers(LPDXGLRENDERERGL This, int interval)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmdSwapBuffers cmddata;
	ZeroMemory(&cmd, sizeof(DXGLPostQueueCmd));
	cmd.data = &cmddata;
	cmddata.command = QUEUEOP_SWAPBUFFERS;
	cmddata.size = sizeof(DXGLQueueCmdSwapBuffers);
	cmddata.interval = interval;
	return DXGLRendererGL_PostCommand(This, &cmd);
}

// Retrieves the window handle associated with the OpenGL context
HRESULT WINAPI DXGLRendererGL_GetWindow(LPDXGLRENDERERGL This, HWND *hWnd)
{
	*hWnd = This->hWndContext;
	return DD_OK;
}

// Resets OpenGL state
void DXGLRendererGL__Reset(LPDXGLRENDERERGL This)
{
	This->depthwrite = GL_TRUE;
	This->depthtest = GL_FALSE;
	This->depthcomp = GL_LESS;
	This->alphacomp = GL_ALWAYS;
	This->scissor = FALSE;
	This->scissorx = 0;
	This->scissory = 0;
	This->scissorwidth = This->width;
	This->scissorheight = This->height;
	This->clearr = This->clearg = This->clearb = This->cleara = 0.0f;
	This->cleardepth = 1.0;
	This->clearstencil = 0;
	This->blendsrc = GL_ONE;
	This->blenddest = GL_ZERO;
	This->blendenabled = GL_FALSE;
	This->polymode = GL_FILL;
	This->shademode = GL_SMOOTH;
	This->texlevel = 0;
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_SCISSOR_TEST);
	glScissor(0, 0, This->width, This->height);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0);
	glClearStencil(0);
	glBlendFunc(GL_ONE, GL_ZERO);
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (This->ext.glver_major < 3) glShadeModel(GL_SMOOTH);
	This->ext.glActiveTexture(GL_TEXTURE0);
}

void DXGLRendererGL__SetTexture(LPDXGLRENDERERGL This, GLuint level, DXGLTexture *texture)
{
	if (level > 31) return; // bounds checking
	if (This->textures[level] == texture) return;
	This->ext.glActiveTexture(GL_TEXTURE0 + level);
	if (texture) glBindTexture(GL_TEXTURE_2D, texture->glhandle);
	else glBindTexture(GL_TEXTURE_2D, 0);
}

void DXGLRendererGL__SetTarget(LPDXGLRENDERERGL This, DXGLTexture *texture, GLuint miplevel)
{
	if (!texture && (This->target == 0)) return;
	if ((texture->levels[miplevel].FBO == This->target) && This->target) return;
	if (texture)
	{
		if (This->ext.GLEXT_ARB_framebuffer_object)
		{
			if (!texture->levels[miplevel].FBO)
			{
				This->ext.glGenFramebuffers(1,&texture->levels[miplevel].FBO);
				This->ext.glBindFramebuffer(GL_FRAMEBUFFER, texture->levels[miplevel].FBO);
				This->ext.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture->gltarget, texture->glhandle, miplevel);
			}
			else
			{
				This->ext.glBindFramebuffer(GL_FRAMEBUFFER, texture->levels[miplevel].FBO);
			}
		}
		else if(This->ext.GLEXT_EXT_framebuffer_object)
		{
			if (!texture->levels[miplevel].FBO)
			{
				This->ext.glGenFramebuffersEXT(1, &texture->levels[miplevel].FBO);
				This->ext.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, texture->levels[miplevel].FBO);
				This->ext.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, texture->gltarget, texture->glhandle, miplevel);
			}
			else
			{
				This->ext.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, texture->levels[miplevel].FBO);
			}
		}
	}
	else
	{
		if (This->ext.GLEXT_ARB_framebuffer_object) This->ext.glBindFramebuffer(GL_FRAMEBUFFER, 0);
		else if (This->ext.GLEXT_EXT_framebuffer_object) This->ext.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

void DXGLRendererGL__SetRenderState(LPDXGLRENDERERGL This, DXGLRenderState* state)
{
	unsigned __int64 shaderid;
	DXGLRenderState2D* state2D = &state->state.state2D;
	switch (state->mode)
	{
	case DXGLRENDERMODE_STATICSHADER: // Legacy static shader
		if ((This->rendermode != DXGLRENDERMODE_STATICSHADER || This->staticshader != state->state.staticindex))
		{
			This->rendermode = DXGLRENDERMODE_STATICSHADER;
			ShaderManager_SetShader(&This->shaders, state->state.staticindex, NULL, 0);
		}
		break;
	case DXGLRENDERMODE_STATE2D: // 2D State
		if ((This->rendermode == DXGLRENDERMODE_STATE2D) && !memcmp(&This->renderstate2D, &state->state.state2D, sizeof(DXGLRenderState2D)))
			break;
		This->rendermode = DXGLRENDERMODE_STATE2D;
		if ((state2D->flags & DDBLT_ROP) && (state2D->bltfx.dwSize == sizeof(DDBLTFX)))
		{
			shaderid = PackROPBits(state2D->bltfx.dwROP, state2D->flags);
		}
		else shaderid = state2D->flags & 0xF2FAADFF;
		if (state2D->srcformat.dwSize == sizeof(DDPIXELFORMAT))
			shaderid |= ((unsigned long long)state2D->blttypesrc << 32);
		if(state2D->flags & 0x80000000)
			shaderid != ((unsigned long long)state2D->blttypedest << 40);
		ShaderManager_SetShader(&This->shaders, shaderid, NULL, 1);
		memcpy(&This->renderstate2D, &state->state.state2D, sizeof(DXGLRenderState2D));
		break;
	case DXGLRENDERMODE_STATE3D: // 3D State
		break;
	case DXGLRENDERMODE_POSTSHADER: // Future:  Custom shaders
		break;
	}
}

void DXGLRendererGL__DrawPrimitives(LPDXGLRENDERERGL This, D3DPRIMITIVETYPE type, const BYTE* vertices, DWORD vertexcount, const WORD* indices, DWORD indexcount)
{
}
