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
#include "util.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Graphics::Imaging;

IAsyncOperation<IRandomAccessStream> ReadIconAsync(HICON icon, int dpi)
{
	ICONINFO iconinfo;
	HBITMAP hbitmap;
	BITMAP bitmap;
	BITMAPINFO bmi;
	HDC hdc;
	int result;
	if (!icon) throw hresult_error(E_INVALIDARG);
	ZeroMemory(&iconinfo, sizeof(ICONINFO));
	if (!GetIconInfo(icon, &iconinfo))
		throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
	bool alpha = true;
	hbitmap = iconinfo.hbmColor;
	if (!hbitmap)
	{
		hbitmap = iconinfo.hbmMask;
		alpha = false;
	}
	ZeroMemory(&bitmap, sizeof(BITMAP));
	GetObject(hbitmap, sizeof(BITMAP), &bitmap);
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bitmap.bmWidth;
	bmi.bmiHeader.biHeight = -bitmap.bmHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	hdc = GetDC(NULL);
	std::vector<uint8_t> pixelbuffer(bitmap.bmWidth * bitmap.bmHeight * 4);
	result = GetDIBits(hdc, hbitmap, 0, bitmap.bmHeight, pixelbuffer.data(), &bmi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);
	if (!result)
	{
		if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
		if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);
		throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
	}
	InMemoryRandomAccessStream stream;
	BitmapEncoder encoder = co_await BitmapEncoder::CreateAsync(BitmapEncoder::PngEncoderId(), stream);
	encoder.SetPixelData(BitmapPixelFormat::Bgra8, alpha ? BitmapAlphaMode::Premultiplied : BitmapAlphaMode::Ignore,
		static_cast<uint32_t>(bitmap.bmWidth), static_cast<uint32_t>(bitmap.bmHeight),
		static_cast<double>(dpi), static_cast<double>(dpi), pixelbuffer);
	co_await encoder.FlushAsync();
	stream.Seek(0);
	if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
	if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);
	co_return stream;
}