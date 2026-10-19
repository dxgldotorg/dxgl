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
#include "SettingsCardControl.xaml.h"
#if __has_include("SettingsCardControl.g.cpp")
#include "SettingsCardControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::dxglcfg_winui::implementation
{
    SettingsCardControl::SettingsCardControl()
    {
    }

    hstring SettingsCardControl::Header()
    {
        return unbox_value<hstring>(GetValue(HeaderProperty()));
    }
    void SettingsCardControl::Header(hstring const &value)
    {
        SetValue(HeaderProperty(), box_value(value));
    }
    DependencyProperty SettingsCardControl::HeaderProperty()
    {
        return DependencyProperty::Register(L"Header", xaml_typename<hstring>(),
            xaml_typename<dxglcfg_winui::SettingsCardControl>(),
            PropertyMetadata(box_value(L"")));
    }

    hstring SettingsCardControl::Description()
    {
        return unbox_value<hstring>(GetValue(DescriptionProperty()));
    }
    void SettingsCardControl::Description(hstring const &value)
    {
        SetValue(HeaderProperty(), box_value(value));
    }
    DependencyProperty SettingsCardControl::DescriptionProperty()
    {
        return DependencyProperty::Register(L"Description", xaml_typename<hstring>(),
            xaml_typename<dxglcfg_winui::SettingsCardControl>(),
            PropertyMetadata(box_value(L"")));
    }

    IconElement SettingsCardControl::HeaderIcon()
    {
        return unbox_value<IconElement>(GetValue(HeaderIconProperty()));
    }
    void SettingsCardControl::HeaderIcon(IconElement const &value)
    {
        SetValue(HeaderIconProperty(), box_value(value));
    }
    DependencyProperty SettingsCardControl::HeaderIconProperty()
    {
        return DependencyProperty::Register(L"HeaderIcon", xaml_typename<IconElement>(),
            xaml_typename<dxglcfg_winui::SettingsCardControl>(),
            PropertyMetadata(nullptr));
    }

    IInspectable SettingsCardControl::HeaderContent()
    {
        return unbox_value<IInspectable>(GetValue(HeaderContentProperty()));
    }
    void SettingsCardControl::HeaderContent(IInspectable const &value)
    {
        SetValue(HeaderContentProperty(), box_value(value));
    }
    DependencyProperty SettingsCardControl::HeaderContentProperty()
    {
        return DependencyProperty::Register(L"HeaderContent", xaml_typename<IInspectable>(),
            xaml_typename<dxglcfg_winui::SettingsCardControl>(),
            PropertyMetadata(nullptr));
    }
}
