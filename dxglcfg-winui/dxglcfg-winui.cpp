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
#include "../dxglcfg/resource.h"
#include "util.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Hosting;
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

DispatcherQueueController queuecontroller = nullptr;
DesktopWindowXamlSource xamlsource = nullptr;

HBRUSH hbrDarkBackground = NULL;
HBRUSH hbrLightBackground = NULL;
HBRUSH *hbrBackground;

BOOL darkmode = FALSE;

void (*_RunDXGLTest)(int testnum, int width, int height, int bpp, int refresh, int backbuffers, int apiver,
	int filter, int msaa, double fps, bool fullscreen, bool resizable, BOOL is3d, BOOL softd3d, HWND parent) = NULL;

HMODULE hDxglcfg = NULL;  // HMODULE for parent EXE to extract resources from
HMODULE hDxglcfgWinui = NULL;

class App : public ApplicationT<App, IXamlMetadataProvider>
{
public:
	winrt::Windows::Foundation::IAsyncAction OnLaunched(LaunchActivatedEventArgs const&)
	{
		HICON appicon;
		HWND hwnd;
		HRSRC hRes;
		HGLOBAL hLoad;
		const char *data;
		DWORD size;
		std::string utf8str;
		std::wstring wstr;
		int wcharsize;
		window = Window();
		window.Title(dxglcfgname);
		Resources().MergedDictionaries().Append(XamlControlsResources());
		if (!hDxglcfgWinui) hDxglcfgWinui = GetModuleHandle(_T("dxglcfg-winui.dll"));
		hRes = FindResource(hDxglcfgWinui, MAKEINTRESOURCE(IDR_DXGLCFG_WINUI_MAIN), L"XAML");
		hLoad = LoadResource(hDxglcfgWinui, hRes);
		data = (const char*)LockResource(hLoad);
		size = SizeofResource(hDxglcfgWinui, hRes);
		if (data && size)
		{
			utf8str.assign(data, size);
			wcharsize = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), -1, NULL, 0);
			wstr.resize(wcharsize - 1, 0);
			MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), -1, &wstr[0], wcharsize);
		}
		if (!wstr.empty())
		{
			winrt::Microsoft::UI::Xaml::UIElement xamlRoot =
				winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(wstr).as<winrt::Microsoft::UI::Xaml::UIElement>();
			auto maingrid = xamlRoot.as<winrt::Microsoft::UI::Xaml::Controls::Grid>();
			window.Content(maingrid);
			window.SystemBackdrop(winrt::Microsoft::UI::Xaml::Media::MicaBackdrop());

			auto titlebar = maingrid.FindName(L"DXGLCFGTitlebar").try_as<winrt::Microsoft::UI::Xaml::Controls::TitleBar>();
			titlebar.Title(dxglcfgname);
			//auto titlebaricon = maingrid.FindName(L"TitlebarIcon").try_as<winrt::Microsoft::UI::Xaml::Controls::ImageIconSource>();
			if (!hDxglcfg) hDxglcfg = GetModuleHandle(NULL);
			try
			{
				appicon = (HICON)LoadImage(hDxglcfg, MAKEINTRESOURCE(IDI_DXGL), IMAGE_ICON,
					GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
				auto windowNative = window.as<IWindowNative>();
				winrt::check_hresult(windowNative->get_WindowHandle(&hwnd));
				IRandomAccessStream stream = co_await ReadIconAsync(appicon, GetDpiForWindow(hwnd));
				auto bitmapsource = BitmapImage();
				co_await bitmapsource.SetSourceAsync(stream);
				ImageIconSource titlebaricon;
				titlebaricon.ImageSource(bitmapsource);
				titlebar.IconSource(titlebaricon);
				DestroyIcon(appicon);
			}
			catch (hresult_error const& error)
			{

			}

		}
		window.ExtendsContentIntoTitleBar(true);
		window.Activate();
	}
	IXamlType GetXamlType(TypeName const& type)
	{
		return provider.GetXamlType(type);
	}
	IXamlType GetXamlType(hstring const& fullname)
	{
		return provider.GetXamlType(fullname);
	}
	com_array<XmlnsDefinition> GetXmlnsDefinitions()
	{
		return provider.GetXmlnsDefinitions();
	}
private:
	Window window{ nullptr };
	XamlControlsXamlMetaDataProvider provider;
};

int WINAPI RunDXGLConfigWinUI(void *rundxgltest)
{
	LPCTSTR wndclassname = _T("DXGL Config WinUI");
	HWND hwnd;
	WNDCLASS wndclass;
	HINSTANCE hinstance = GetModuleHandle(NULL);
	MSG msg;
	PACKAGE_VERSION minver = { WINDOWSAPPSDK_RUNTIME_VERSION_UINT64 };
	HRESULT error = MddBootstrapInitialize2(WINDOWSAPPSDK_RELEASE_MAJORMINOR, 
		WINDOWSAPPSDK_RELEASE_VERSION_TAG_W,
		minver,	MddBootstrapInitializeOptions_None);
	if (FAILED(error)) return 0;
	winrt::init_apartment(winrt::apartment_type::single_threaded);
	Application::Start([](auto&&) {make<App>(); });
	MddBootstrapShutdown();
	return 1;
}
