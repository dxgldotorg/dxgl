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
#include "resource.h"
#include "../dxglcfg/resource.h"
#include "util.h"
#pragma comment(lib, "comctl32.lib")
#include "MainWindow.xaml.h"
#include "dxglcfg-winui.h"

#include <WindowsAppSDK-VersionInfo.h>

using namespace winrt;
using namespace winrt::Microsoft::UI::Windowing;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::XamlTypeInfo;
using namespace winrt::Microsoft::UI::Xaml::Markup;
using namespace winrt::Microsoft::UI::Xaml::Media::Imaging;
using namespace winrt::Windows::UI::Xaml::Interop;
using namespace winrt::Windows::Storage::Streams;

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

DXGLCFG currcfg;

HBRUSH hbrDarkBackground = NULL;
HBRUSH hbrLightBackground = NULL;
HBRUSH *hbrBackground;

BOOL darkmode = FALSE;
BOOL accentborder = FALSE;
DWORD accentcolor = 0;

void (*_RunDXGLTest)(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	int filter, int msaa, double fps, bool fullscreen, bool resizable, BOOL is3d, BOOL softd3d, HWND parent) = NULL;

HMODULE hDxglcfg = NULL;  // HMODULE for parent EXE to extract resources from
HMODULE hDxglcfgWinui = NULL;

winrt::Microsoft::UI::Xaml::UIElement xamlroot{ nullptr };


LRESULT CALLBACK FixTopBorder(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR refdata)
{
	switch (msg)
	{
	case WM_NCCALCSIZE:
		if (wParam)
		{
			LRESULT ret = DefSubclassProc(hwnd, msg, wParam, lParam);
			NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS*)lParam;
			UINT dpi = GetDpiForWindow(hwnd);
			params->rgrc[0].top -= MulDiv(1, dpi, 96);
			return ret;
		}
		else return DefSubclassProc(hwnd, msg, wParam, lParam);
	case WM_SETTINGCHANGE:
		if (lParam)
		{
			if (!_tcscmp((TCHAR*)lParam, _T("ImmersiveColorSet")))
			{
				GetThemeInfo(&darkmode, &accentborder, &accentcolor, &currcfg);

				return TRUE;
			}
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	case WM_DESTROY:
		xamlroot = nullptr;
		break;
	default:
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

winrt::fire_and_forget DXGLCFGWindow_Create(winrt::dxglcfg_winui::implementation::MainWindow &window)
{
	HICON appicon;
	HWND hwnd;
	window.Title(dxglcfgname);
	window.SystemBackdrop(Media::MicaBackdrop());
	if (!hDxglcfg) hDxglcfg = GetModuleHandle(NULL);
	try
	{
		appicon = (HICON)LoadImage(hDxglcfg, MAKEINTRESOURCE(IDI_DXGL), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		auto windowNative = window.try_as<IWindowNative>();
		winrt::check_hresult(windowNative->get_WindowHandle(&hwnd));
		SetWindowSubclass(hwnd, FixTopBorder, 1, 0);
		IRandomAccessStream stream = co_await ReadIconAsync(appicon, GetDpiForWindow(hwnd));
		auto bitmapsource = BitmapImage();
		co_await bitmapsource.SetSourceAsync(stream);
		ImageIconSource titlebaricon;
		titlebaricon.ImageSource(bitmapsource);
		window.DXGLCFGTitlebar().IconSource(titlebaricon);
		DestroyIcon(appicon);
	}
	catch (hresult_error const& error) {}
	switch (currcfg.DarkMode)
	{
	case 0:
	default:
		window.DXGLCFGMainGrid().RequestedTheme(ElementTheme::Default);
		break;
	case 1:
		window.DXGLCFGMainGrid().RequestedTheme(ElementTheme::Dark);
		break;
	case 2:
		window.DXGLCFGMainGrid().RequestedTheme(ElementTheme::Light);
		break;
	}
	window.AppWindow().TitleBar().PreferredHeightOption(TitleBarHeightOption::Tall);
	window.DXGLCFGTitlebar().Title(dxglcfgname);
#ifdef _DEBUG
	window.DXGLCFGTitlebar().Subtitle(L"Debug");
#endif
	window.MainNav().SelectedItem(window.MainNav().MenuItems().GetAt(0));

	window.Activate();
}

int WINAPI RunDXGLConfigWinUI(void *rundxgltest, unsigned int tab, DWORD app)
{
	int result;
	GetCurrentConfig(&currcfg, FALSE);
	PACKAGE_VERSION minver = { WINDOWSAPPSDK_RUNTIME_VERSION_UINT64 };
	HRESULT error = MddBootstrapInitialize2(WINDOWSAPPSDK_RELEASE_MAJORMINOR,
		WINDOWSAPPSDK_RELEASE_VERSION_TAG_W,
		minver,	MddBootstrapInitializeOptions_None);
	if (FAILED(error)) return 0;
	/*winrt::init_apartment(winrt::apartment_type::single_threaded);
	Application::Start([](auto&&) {make<DXGLCFGApp>(); });*/
	result = wWinMain(0, 0, 0, 0);
	MddBootstrapShutdown();
	if (result) return 0;
	else return 1;
}
