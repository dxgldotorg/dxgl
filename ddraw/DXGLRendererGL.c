// DXGL
// Copyright (C) 2022 William Feely

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
#include "DXGLQueue.h"
#include "DXGLRenderer.h"
#include "DXGLRendererGL.h"
#include "DXGLTexture.h"
#include <WbemCli.h>
#include <oleauto.h>
#include "const.h"
#include "fourcc.h"

static BOOL(WINAPI *_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX lpBuffer) = NULL;
static HMODULE hKernel32;
static MEMORYSTATUS memstatus;
static MEMORYSTATUSEX memstatusex;

extern DXGLCFG dxglcfg;
extern DWORD gllock;

static WNDCLASSEXA wndclass;
static BOOL wndclasscreated = FALSE;

static IDXGLRendererVtbl vtbl =
{
	DXGLRendererGL_QueryInterface,
	DXGLRendererGL_AddRef,
	DXGLRendererGL_Release,
	DXGLRendererGL_GetAttachedDevice,
	DXGLRendererGL_SetAttachedDevice
};

DWORD WINAPI DXGLRendererGL_MainThread(LPDXGLRENDERERGL This);

HRESULT DXGLRendererGL_Create(GUID *guid, LPDXGLRENDERERGL *out)
{
	HANDLE waithandles[2];
	IDXGLRendererGL *This;
	DWORD waitobject;
	HRESULT ret;
	// Allocate a renderer object
	This = malloc(sizeof(IDXGLRendererGL));
	if (!This) return DDERR_OUTOFMEMORY;
	ZeroMemory(This, sizeof(IDXGLRendererGL));
	This->base.lpVtbl = &vtbl;
	This->refcount = 1;

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
	windowname = "DXGL OpenGL Renderer";
	BOOL exch = TRUE;
	exch = InterlockedExchange(&wndclasscreated, exch);
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
	VARIANT adapterram;
	IWbemLocator *wbemloc;
	IWbemServices *wbemsvc;
	IEnumWbemClassObject *wbemenum;
	IWbemClassObject *wbemclsobj;
	BSTR cimv2;
	BSTR wql;
	BSTR wqlquery;
	BOOL corecontext = FALSE;
	DXGLQueueCmd currcmd;
	ULONG_PTR queuepos, pixelpos, vertexpos, indexpos;
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
						ret = CoSetProxyBlanket(wbemloc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
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
									if (ret == 0) break;
									ret = wbemclsobj->lpVtbl->Get(wbemclsobj, L"AdapterRAM", 0, &adapterram, 0, 0);
									dxglcfg.VideoRAM = adapterram.llVal;
									VariantClear(&adapterram);
									wbemclsobj->lpVtbl->Release(wbemclsobj);
								}
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
			dxglcfg.VertexBufferSize = 4194302;
		else if (dxglcfg.VideoRAM > 500000000)
			dxglcfg.VertexBufferSize = 2097152;
		else if (dxglcfg.VideoRAM > 250000000)
			dxglcfg.VertexBufferSize = 1048576;
		else if (dxglcfg.VideoRAM > 120000000)
			dxglcfg.VertexBufferSize = 524288;
		else if (dxglcfg.VideoRAM > 60000000)
			dxglcfg.VertexBufferSize = 262144;
		else dxglcfg.VertexBufferSize = 131072;
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
	This->commandqueue->commandsize = dxglcfg.CmdBufferSize;
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
	This->ext.glGenBuffers(1, &This->commandqueue[1].pixelbuffer);
	This->ext.glGenBuffers(1, &This->commandqueue[0].vertexbuffer);
	This->ext.glGenBuffers(1, &This->commandqueue[1].vertexbuffer);
	This->ext.glGenBuffers(1, &This->commandqueue[0].indexbuffer);
	This->ext.glGenBuffers(1, &This->commandqueue[1].indexbuffer);
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
		LeaveCriticalSection(&This->cs);
		queuepos = pixelpos = vertexpos = indexpos = 0;
		while (This->running)
		{
			memcpy(&currcmd, &This->currentqueue->commands[queuepos], sizeof(DXGLQueueCmd));
			switch (currcmd.command)
			{
			case QUEUEOP_QUIT:  // End render device
				This->shutdown = TRUE;
				This->running = FALSE;
				break;
			case QUEUEOP_NULL:  // Do nothing
				break;
			case QUEUEOP_RESET:  // Rese device 
				DXGLRendererGL__Reset(This);
				break;
			case QUEUEOP_MAKETEXTURE:  // Create a texture or surface
				break;
			}
			// Move queue forward
			queuepos += currcmd.size;
			if (queuepos >= This->currentqueue->commandpos)
			{
				// Flip buffers or end execution
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

HRESULT WINAPI DXGLRendererGL_Reset(LPDXGLRENDERERGL This)
{
	DXGLPostQueueCmd cmd;
	DXGLQueueCmd cmddata;
	cmddata.command = QUEUEOP_RESET;
	cmddata.size = sizeof(DXGLQueueCmd);
	cmd.data = &cmddata;
	return DXGLRendererGL_PostCommand(This, &cmd);
}
HRESULT WINAPI DXGLRendererGL_PostCommand(LPDXGLRENDERERGL This, DXGLPostQueueCmd *cmd)
{
	DXGLQueue tmp;
	EnterCriticalSection(&This->cs);
	
	LeaveCriticalSection(&This->cs);
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
