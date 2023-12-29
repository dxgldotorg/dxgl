// DXGL
// Copyright (C) 2012-2020 William Feely

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
#include "glDirectDraw.h"
#include "glRenderWindow.h"
#include "ddraw.h"
#include "hooks.h"

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

extern "C" {

static WNDCLASSEXA wndclass;
static BOOL wndclasscreated = FALSE;
static BOOL hotkeyregistered = FALSE;

void WaitForObjectAndMessages(HANDLE object)
{
	MSG Msg;
	while(1)
	{
		switch(MsgWaitForMultipleObjects(1,&object,FALSE,INFINITE,QS_ALLINPUT))
		{
		case WAIT_OBJECT_0:
			return;
		case WAIT_OBJECT_0+1:
			while(PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		}
	}

}

HRESULT glRenderWindow_Create(int width, int height, BOOL fullscreen,
	HWND parent, glDirectDraw7 *glDD7, BOOL devwnd, glRenderWindow **renderwnd)
{
	glRenderWindow *This = (glRenderWindow*)malloc(sizeof(glRenderWindow));
	if (!This) return E_OUTOFMEMORY;
	DWORD threadid;
	This->ddInterface = glDD7;
	This->width = width;
	This->height = height;
	This->fullscreen = fullscreen;
	This->device = devwnd;
	This->hParentWnd = parent;
	This->ReadyEvent = CreateEvent(NULL,false,false,NULL);
	This->hWnd = NULL;
	This->hThread = CreateThread(NULL,0,glRenderWindow_ThreadEntry,This,0,&threadid);
	*renderwnd = This;
	WaitForObjectAndMessages(This->ReadyEvent);
	CloseHandle(This->ReadyEvent);
	This->ReadyEvent = NULL;
	return S_OK;
}

DWORD WINAPI glRenderWindow_ThreadEntry(void *entry)
{
	return glRenderWindow__Entry((glRenderWindow*)entry);
}

DWORD glRenderWindow__Entry(glRenderWindow *This)
{
	char* windowname;
	if (This->device) windowname = "DirectDrawDeviceWnd";
	else windowname = "Renderer";
	MSG Msg;
	if (!wndclasscreated)
	{
		wndclass.cbSize = sizeof(WNDCLASSEXA);
		wndclass.style = CS_DBLCLKS;
		wndclass.lpfnWndProc = RenderWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = (HINSTANCE)GetModuleHandle(NULL);
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "DirectDrawDeviceWnd";
		wndclass.hIconSm = NULL;
		RegisterClassExA(&wndclass);
		wndclasscreated = true;
	}
	RECT rectRender;
	_GetClientRect(This->hParentWnd, &rectRender);
	This->dead = FALSE;
	if (This->hParentWnd)
	{
		This->hWnd = CreateWindowA("DirectDrawDeviceWnd", windowname, WS_CHILD | WS_VISIBLE, 0, 0, rectRender.right - rectRender.left,
			rectRender.bottom - rectRender.top, This->hParentWnd, NULL, wndclass.hInstance, This);
		SetWindowPos(This->hWnd, HWND_TOP, 0, 0, rectRender.right, rectRender.bottom, SWP_SHOWWINDOW);
	}
	else
	{
		This->width = GetSystemMetrics(SM_CXSCREEN);
		This->height = GetSystemMetrics(SM_CYSCREEN);
		This->hWnd = CreateWindowExA(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
			"DirectDrawDeviceWnd", windowname, WS_POPUP, 0, 0, This->width, This->height, 0, 0, NULL, This);
		SetWindowPos(This->hWnd, HWND_TOP, 0, 0, This->width, This->height, SWP_SHOWWINDOW | SWP_NOACTIVATE);
	}
#ifdef _DEBUG
	if (RegisterHotKey(This->hWnd, 1, MOD_CONTROL, VK_CANCEL)) hotkeyregistered = true;
	else
	{
		TRACE_STRING("Failed to register hotkey.\n");
		Beep(120, 1000);
	}
#else
	if (dxglcfg.DebugTraceLevel)
	{
		if (RegisterHotKey(This->hWnd, 1, MOD_CONTROL, VK_CANCEL)) hotkeyregistered = true;
		else
		{
			TRACE_STRING("Failed to register hotkey.\n");
			Beep(120, 1000);
		}
	}
	#endif
	SetEvent(This->ReadyEvent);
	while((GetMessage(&Msg, NULL, 0, 0) > 0) && !This->dead)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return 0;
}

void glRenderWindow_Delete(glRenderWindow *This)
{
	if(hotkeyregistered) UnregisterHotKey(This->hWnd,1);
	SendMessage(This->hWnd,WM_CLOSE,0,0);
	WaitForSingleObject(This->hThread,INFINITE);
	CloseHandle(This->hThread);
	free(This);
}

DWORD WINAPI BeepThread(void *unused)
{
	Beep(3600,150);
	return 0;
}

LRESULT glRenderWindow_WndProc(glRenderWindow *This, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hParent;
	HCURSOR cursor;
	DWORD threadid;
	switch(msg)
	{
	case WM_CREATE:
		SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)This);
		return 0;
	case WM_SETCURSOR:
		hParent = GetParent(hwnd);
		cursor = (HCURSOR)GetClassLongPtr(hParent,GCLP_HCURSOR);
		SetCursor(cursor);
		return SendMessage(hParent,msg,wParam,lParam);
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
	case WM_MOUSEHWHEEL:
		hParent = GetParent(hwnd);
		return SendMessage(hParent,msg,wParam,lParam);
	case WM_CLOSE:
		DestroyWindow(This->hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		This->dead = TRUE;
		return 0;
	case WM_HOTKEY:
		if (dxglcfg.DebugTraceLevel)
		{
			trace_end = TRUE;
			CreateThread(NULL, 0, BeepThread, NULL, 0, &threadid);
			UnregisterHotKey(This->hWnd, 1);
			hotkeyregistered = false;
			dxglcfg.DebugTraceLevel = 0;
		}
		else
		{
			#ifdef _DEBUG
			Beep(3600, 150);
			DebugBreak();
			#endif
		}
	default:
		return DefWindowProc(hwnd,msg,wParam,lParam);
	}
	return 0;
}





// Render Window event handler
static LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	glRenderWindow* instance = reinterpret_cast<glRenderWindow*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
	if(!instance)
	{
		if (msg == WM_CREATE)
			instance = (glRenderWindow*)(*(LONG_PTR*)lParam);
		else return DefWindowProc(hwnd,msg,wParam,lParam);
	}
	return glRenderWindow_WndProc(instance, hwnd, msg, wParam, lParam);
}

}