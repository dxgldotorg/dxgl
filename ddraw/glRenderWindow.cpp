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
#include "glDirectDraw.h"
#include "glRenderWindow.h"
#include "ddraw.h"
#include "hooks.h"

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

WNDCLASSEXA wndclass;
bool wndclasscreated = false;
bool hotkeyregistered = false;

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

glRenderWindow::glRenderWindow(int width, int height, bool fullscreen, HWND parent, glDirectDraw7 *glDD7, bool devwnd)
{
	DWORD threadid;
	ddInterface = glDD7;
	this->width = width;
	this->height = height;
	this->fullscreen = fullscreen;
	this->device = devwnd;
	hParentWnd = parent;
	ReadyEvent = CreateEvent(NULL,false,false,NULL);
	hThread = CreateThread(NULL,0,ThreadEntry,this,0,&threadid);
	WaitForObjectAndMessages(ReadyEvent);
	CloseHandle(ReadyEvent);
	ReadyEvent = NULL;
}

DWORD WINAPI glRenderWindow::ThreadEntry(void *entry)
{
	return ((glRenderWindow*)entry)->_Entry();
}

DWORD glRenderWindow::_Entry()
{
	char* windowname;
	if (device) windowname = "DirectDrawDeviceWnd";
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
	_GetClientRect(hParentWnd, &rectRender);
	dead = false;
	if (hParentWnd)
	{
		hWnd = CreateWindowA("DirectDrawDeviceWnd", windowname, WS_CHILD | WS_VISIBLE, 0, 0, rectRender.right - rectRender.left,
			rectRender.bottom - rectRender.top, hParentWnd, NULL, wndclass.hInstance, this);
		SetWindowPos(hWnd, HWND_TOP, 0, 0, rectRender.right, rectRender.bottom, SWP_SHOWWINDOW);
	}
	else
	{
		width = GetSystemMetrics(SM_CXSCREEN);
		height = GetSystemMetrics(SM_CYSCREEN);
		hWnd = CreateWindowExA(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
			"DirectDrawDeviceWnd", windowname, WS_POPUP, 0, 0, width, height, 0, 0, NULL, this);
		SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_SHOWWINDOW | SWP_NOACTIVATE);
	}
#ifdef _DEBUG
	if (RegisterHotKey(hWnd, 1, MOD_CONTROL, VK_CANCEL)) hotkeyregistered = true;
	else
	{
		TRACE_STRING("Failed to register hotkey.\n");
		Beep(120, 1000);
	}
#else
	if (dxglcfg.DebugTraceLevel)
	{
		if (RegisterHotKey(hWnd, 1, MOD_CONTROL, VK_CANCEL)) hotkeyregistered = true;
		else
		{
			TRACE_STRING("Failed to register hotkey.\n");
			Beep(120, 1000);
		}
	}
	#endif
	SetEvent(ReadyEvent);
	while((GetMessage(&Msg, NULL, 0, 0) > 0) && !dead)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return 0;
}

glRenderWindow::~glRenderWindow()
{
	#ifdef _DEBUG
	if(hotkeyregistered) UnregisterHotKey(hWnd,1);
	#endif
	SendMessage(hWnd,WM_CLOSE,0,0);
	WaitForSingleObject(hThread,INFINITE);
	CloseHandle(hThread);
}

DWORD WINAPI BeepThread(void *unused)
{
	Beep(3600,150);
	return 0;
}

LRESULT glRenderWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hParent;
	HCURSOR cursor;
	DWORD threadid;
	switch(msg)
	{
	case WM_CREATE:
		SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)this);
		return 0;
	case WM_SETCURSOR:
		hParent = GetParent(hwnd);
		cursor = (HCURSOR)GetClassLong(hParent,GCL_HCURSOR);
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
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		dead = true;
		return 0;
	case WM_HOTKEY:
		if (dxglcfg.DebugTraceLevel)
		{
			trace_end = TRUE;
			CreateThread(NULL, 0, BeepThread, NULL, 0, &threadid);
			UnregisterHotKey(hWnd, 1);
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
LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	glRenderWindow* instance = reinterpret_cast<glRenderWindow*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
	if(!instance)
	{
		if(msg == WM_CREATE)
			instance = reinterpret_cast<glRenderWindow*>(*(LONG_PTR*)lParam);
		else return DefWindowProc(hwnd,msg,wParam,lParam);
	}
	return instance->WndProc(hwnd,msg,wParam,lParam);
}
