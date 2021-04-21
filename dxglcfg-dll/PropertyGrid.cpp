// DXGL
// Copyright (C) 2021 William Feely

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

#include <Windows.h>
#include <tchar.h>
#define IN_DXGLCFG_DLL
#include "PropertyGrid.h"
using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

extern "C" {extern ATOM wndclass; }

LRESULT CALLBACK PropertyGridWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndControl;
	PropertyGrid ^ PropGrid;
	RECT wndrect;
	switch (uMsg)
	{
	case WM_CREATE:
		PropGrid = gcnew PropertyGrid();
		hwndControl = (HWND)PropGrid->Handle.ToPointer();
		SetParent(hwndControl, hWnd);
		GetWindowRect(hWnd, &wndrect);
		PropGrid->Size = Size(wndrect.right-wndrect.left, wndrect.bottom-wndrect.top);
//		PropGrid->HelpVisible = false;
//		PropGrid->ToolbarVisible = false;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

extern "C" BOOL RegisterPropertyGrid()
{
	WNDCLASSEX propgridclass;
	ZeroMemory(&propgridclass, sizeof(WNDCLASSEX));
	propgridclass.cbSize = sizeof(WNDCLASSEX);
	propgridclass.lpszClassName = _T("PropertyGridClass");
	propgridclass.hInstance = GetModuleHandle(NULL);
	propgridclass.lpfnWndProc = PropertyGridWndProc;
	wndclass = RegisterClassEx(&propgridclass);
	return FALSE;
}