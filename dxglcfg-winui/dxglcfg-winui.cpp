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

#include "common.h"
#include "dxglcfg-winui.h"

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

DesktopWindowXamlSource xamlsource = nullptr;

void (*_RunDXGLTest)(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	int filter, int msaa, double fps, bool fullscreen, bool resizable, BOOL is3d, BOOL softd3d, HWND parent) = NULL;

HWND islandwnd = NULL;

void CreateXamlWindow(HWND hwnd, HINSTANCE hinstance)
{
	RECT r;
	xamlsource = DesktopWindowXamlSource();
	auto interop = xamlsource.as<IDesktopWindowXamlSourceNative>();
	check_hresult(interop->AttachToWindow(hwnd));
	check_hresult(interop->get_WindowHandle(&islandwnd));
	GetClientRect(hwnd, &r);
	SetWindowPos(islandwnd, NULL, 0, 0, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);

	// FIXME:  Replace with real code
	winrt::Windows::UI::Xaml::Controls::Grid mainGrid;
	winrt::Windows::UI::Xaml::Controls::TextBlock textBlock;

	textBlock.Text(L"Coming soon...");
	textBlock.HorizontalAlignment(winrt::Windows::UI::Xaml::HorizontalAlignment::Center);
	textBlock.VerticalAlignment(winrt::Windows::UI::Xaml::VerticalAlignment::Center);

	mainGrid.Children().Append(textBlock);

	// 7. Inject the UWP container framework straight into the island source
	xamlsource.Content(mainGrid);

}

LRESULT CALLBACK DXGLConfigWinUIWndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
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
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// All painting occurs here, between BeginPaint and EndPaint.

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BTNSHADOW));

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
	WindowsXamlManager xamlmanager = WindowsXamlManager::InitializeForCurrentThread();
	_RunDXGLTest = (void(*)(int, int, int, int, int, int, int, int, int, double, bool, bool, BOOL, BOOL, HWND))rundxgltest;
	ZeroMemory(&wndclass, sizeof(WNDCLASS));
	wndclass.lpfnWndProc = DXGLConfigWinUIWndProc;
	wndclass.hInstance = hinstance;
	wndclass.lpszClassName = wndclassname;
	RegisterClass(&wndclass);
	hwnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP,wndclassname,dxglcfgname,WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hinstance, NULL);
	if (!hwnd) return 0;
	ShowWindow(hwnd, SW_SHOWNORMAL);
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 1;
}
