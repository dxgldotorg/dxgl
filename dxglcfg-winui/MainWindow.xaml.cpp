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
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "dxglcfg-winui.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::dxglcfg_winui::implementation
{
    MainWindow::MainWindow()
    {
        ExtendsContentIntoTitleBar(true);
        DXGLCFGWindow_Create(*this);
    }
    void MainWindow::DXGLCFG_PaneToggleRequested(IInspectable const &sender, IInspectable const &e)
    {
        this->MainNav().IsPaneOpen(!this->MainNav().IsPaneOpen());
    }
}
