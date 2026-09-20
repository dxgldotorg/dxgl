// DXGL
// Copyright (C) 2026 William Feely

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

#include "pch.h"
#include "dxglcfg-winui.h"
#include "resource.h"

using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt;


#ifdef _M_X64
static const TCHAR installdir[] = _T("InstallDir_x64");
static const TCHAR regglobal[] = _T("Global_x64");
static const TCHAR profilespath[] = _T("Software\\DXGL\\Profiles_x64");
static const TCHAR profilespath2[] = _T("Software\\DXGL\\Profiles_x64\\");
static const TCHAR dxglcfgname[] = _T("DXGL Config (x64)");
#else
static const TCHAR installdir[] = _T("InstallDir");
static const TCHAR regglobal[] = _T("Global");
static const TCHAR profilespath[] = _T("Software\\DXGL\\Profiles");
static const TCHAR profilespath2[] = _T("Software\\DXGL\\Profiles\\");
static const TCHAR dxglcfgname[] = _T("DXGL Config");
#endif

//Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xamlsource = nullptr;

HBRUSH hbrDarkBackground = NULL;
HBRUSH hbrLightBackground = NULL;
HBRUSH *hbrBackground;

BOOL darkmode = FALSE;

void (*_RunDXGLTest)(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	int filter, int msaa, double fps, bool fullscreen, bool resizable, BOOL is3d, BOOL softd3d, HWND parent) = NULL;

HWND islandwnd = NULL;
HMODULE hDxglcfgWinui = NULL;

void CreateXamlWindow(HWND hwnd, HINSTANCE hinstance)
{
	/*RECT r;
	HRSRC hRes;
	HGLOBAL hLoad;
	const char *data;
	DWORD size;
	std::string utf8str;
	std::wstring wstr;
	int wcharsize;
	xamlsource = DesktopWindowXamlSource();
	auto interop = xamlsource.as<IDesktopWindowXamlSourceNative>();
	check_hresult(interop->AttachToWindow(hwnd));
	check_hresult(interop->get_WindowHandle(&islandwnd));
	GetClientRect(hwnd, &r);
	SetWindowPos(islandwnd, NULL, 0, 0, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);
	if (!hDxglcfgWinui) hDxglcfgWinui = GetModuleHandle(_T("dxglcfg-winui.dll"));
	hRes = FindResource(hDxglcfgWinui, MAKEINTRESOURCE(IDR_DXGLCFG_WINUI_MAIN), L"XAML");
	hLoad = LoadResource(hDxglcfgWinui, hRes);
	data = (const char*)LockResource(hLoad);
	size = SizeofResource(hDxglcfgWinui, hRes);
	if (data && size)
	{
		utf8str.assign(data, size);
		wcharsize = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), -1, NULL, 0);
		wstr.resize(wcharsize-1, 0);
		MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), -1, &wstr[0], wcharsize);
	}
	if (!wstr.empty())
	{
		winrt::Windows::UI::Xaml::UIElement xamlRoot =
			winrt::Windows::UI::Xaml::Markup::XamlReader::Load(wstr).as<winrt::Windows::UI::Xaml::UIElement>();
		xamlsource.Content(xamlRoot);
	}*/
}

LRESULT CALLBACK DXGLConfigWinUIWndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	switch (Msg)
	{
	case WM_CREATE:
		CreateXamlWindow(hwnd, GetModuleHandle(NULL));
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		if (islandwnd) SetWindowPos(islandwnd, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER|SWP_SHOWWINDOW);
		return 0;
	case WM_ERASEBKGND:
		GetClientRect(hwnd, &r);
		FillRect((HDC)wParam, &r, hbrDarkBackground);
		return 1;
	case WM_NCCALCSIZE:
		return 0;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// All painting occurs here, between BeginPaint and EndPaint.

		FillRect(hdc, &ps.rcPaint, hbrDarkBackground);

		EndPaint(hwnd, &ps);
	}
	return 0;

	}
	return DefWindowProc(hwnd, Msg, wParam, lParam);
}
int WINAPI RunDXGLConfigWinUI(void *rundxgltest)
{
	LPCTSTR wndclassname = _T("DXGL Config WinUI");
	HWND hwnd;
	WNDCLASS wndclass;
	HINSTANCE hinstance = GetModuleHandle(NULL);
	MSG msg;
	//WindowsXamlManager xamlmanager = WindowsXamlManager::InitializeForCurrentThread();
	_RunDXGLTest = (void(*)(int, int, int, int, int, int, int, int, int, double, bool, bool, BOOL, BOOL, HWND))rundxgltest;
	hbrDarkBackground = CreateSolidBrush(RGB(32, 32, 32));
	hbrLightBackground = CreateSolidBrush(RGB(243, 243, 243));
	ZeroMemory(&wndclass, sizeof(WNDCLASS));
	wndclass.lpfnWndProc = DXGLConfigWinUIWndProc;
	wndclass.hInstance = hinstance;
	wndclass.lpszClassName = wndclassname;
	RegisterClass(&wndclass);
	hwnd = CreateWindowEx(0,wndclassname,dxglcfgname,WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hinstance, NULL);
	if (!hwnd) return 0;
	ShowWindow(hwnd, SW_SHOWNORMAL);
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DeleteObject(hbrDarkBackground);
	hbrDarkBackground = NULL;
	DeleteObject(hbrLightBackground);
	hbrLightBackground = NULL;
	return 1;
}
