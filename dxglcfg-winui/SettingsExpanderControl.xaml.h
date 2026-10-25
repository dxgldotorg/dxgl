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

#pragma once

#include "SettingsExpanderControl.g.h"

namespace winrt::dxglcfg_winui::implementation
{
    struct SettingsExpanderControl : SettingsExpanderControlT<SettingsExpanderControl>
    {
        SettingsExpanderControl();

        hstring Header();
        void Header(hstring const &value);
        static Microsoft::UI::Xaml::DependencyProperty HeaderProperty();

        hstring Description();
        void Description(hstring const &valeu);
        static Microsoft::UI::Xaml::DependencyProperty DescriptionProperty();

        Microsoft::UI::Xaml::Controls::IconElement HeaderIcon();
        void HeaderIcon(Microsoft::UI::Xaml::Controls::IconElement const &value);
        static Microsoft::UI::Xaml::DependencyProperty HeaderIconProperty();

        winrt::Windows::Foundation::IInspectable HeaderContent();
        void HeaderContent(winrt::Windows::Foundation::IInspectable const &value);
        static Microsoft::UI::Xaml::DependencyProperty HeaderContentProperty();

        //static void OnPanelIndexChanged(DependencyObject const &d, winrt::Microsoft::UI::Xaml::DependencyPropertyChangedEventArgs const &e);
        //int32_t PanelIndex();
        //void PanelIndex(int32_t value);
        //static Microsoft::UI::Xaml::DependencyProperty PanelIndexProperty();

        //winrt::Microsoft::UI::Xaml::DependencyObjectCollection Panels();

    private:
        //winrt::Microsoft::UI::Xaml::DependencyObjectCollection paneldata;

    };
}

namespace winrt::dxglcfg_winui::factory_implementation
{
    struct SettingsExpanderControl : SettingsExpanderControlT<SettingsExpanderControl, implementation::SettingsExpanderControl>
    {
    };
}
