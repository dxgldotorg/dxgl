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
#include "DXGLRenderer.h"
#include "DXGLRendererGL.h"
#include <WbemCli.h>
#include <oleauto.h>

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
	DXGLRendererGL_Release
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
	EnterCriticalSection(&dll_cs);
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
		LeaveCriticalSection(&dll_cs);
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
		LeaveCriticalSection(&dll_cs);
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
		LeaveCriticalSection(&dll_cs);
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
		LeaveCriticalSection(&dll_cs);
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
				LeaveCriticalSection(&dll_cs);
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
				LeaveCriticalSection(&dll_cs);
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
	LeaveCriticalSection(&dll_cs);
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
	// Thread initialization done
	SetEvent(This->SyncEvent);

	// Main loop

	// Begin shutdown

	// Final shutdown
	wglMakeCurrent(0, 0);
	wglDeleteContext(This->hrc);
	ReleaseDC(This->hWndContext, This->hdc);
	SendMessage(This->hWndContext, WM_CLOSE, 0, 0);
	WaitForSingleObject(This->WindowThreadHandle, INFINITE);
	return DD_OK;
}