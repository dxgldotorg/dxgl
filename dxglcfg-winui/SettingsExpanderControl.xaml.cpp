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
#include "SettingsExpanderControl.xaml.h"
#if __has_include("SettingsExpanderControl.g.cpp")
#include "SettingsExpanderControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation::Collections;


namespace winrt::dxglcfg_winui::implementation
{
    SettingsExpanderControl::SettingsExpanderControl()
    {
        //paneldata = DependencyObjectCollection();
    }

    hstring SettingsExpanderControl::Header()
    {
        return unbox_value<hstring>(GetValue(HeaderProperty()));
    }
    void SettingsExpanderControl::Header(hstring const &value)
    {
        SetValue(HeaderProperty(), box_value(value));
    }
    DependencyProperty SettingsExpanderControl::HeaderProperty()
    {
        static DependencyProperty headerproperty =
            DependencyProperty::Register(L"Header", xaml_typename<hstring>(),
                xaml_typename<dxglcfg_winui::SettingsExpanderControl>(),
                PropertyMetadata(box_value(L"")));
        return headerproperty;
    }

    hstring SettingsExpanderControl::Description()
    {
        return unbox_value<hstring>(GetValue(DescriptionProperty()));
    }
    void SettingsExpanderControl::Description(hstring const &value)
    {
        SetValue(DescriptionProperty(), box_value(value));
    }
    DependencyProperty SettingsExpanderControl::DescriptionProperty()
    {
        static DependencyProperty descriptionproperty =
            DependencyProperty::Register(L"Description", xaml_typename<hstring>(),
                xaml_typename<dxglcfg_winui::SettingsExpanderControl>(),
                PropertyMetadata(box_value(L"")));
        return descriptionproperty;
    }
    
    IconElement SettingsExpanderControl::HeaderIcon()
    {
        return unbox_value<IconElement>(GetValue(HeaderIconProperty()));
    }
    void SettingsExpanderControl::HeaderIcon(IconElement const &value)
    {
        SetValue(HeaderIconProperty(), box_value(value));
    }
    DependencyProperty SettingsExpanderControl::HeaderIconProperty()
    {
        static DependencyProperty iconproperty =
            DependencyProperty::Register(L"HeaderIcon", xaml_typename<IconElement>(),
                xaml_typename<dxglcfg_winui::SettingsExpanderControl>(),
                PropertyMetadata(nullptr));
        return iconproperty;
    }

    IInspectable SettingsExpanderControl::HeaderContent()
    {
        return unbox_value<IInspectable>(GetValue(HeaderContentProperty()));
    }
    void SettingsExpanderControl::HeaderContent(IInspectable const &value)
    {
        SetValue(HeaderContentProperty(), box_value(value));
    }
    DependencyProperty SettingsExpanderControl::HeaderContentProperty()
    {
        static DependencyProperty contentproperty =
            DependencyProperty::Register(L"HeaderContent", xaml_typename<IInspectable>(),
                xaml_typename<dxglcfg_winui::SettingsExpanderControl>(),
                PropertyMetadata(nullptr));
        return contentproperty;
    }

    /*int32_t SettingsExpanderControl::PanelIndex()
    {
        return unbox_value<int32_t>(GetValue(PanelIndexProperty()));
    }
    void SettingsExpanderControl::PanelIndex(int32_t value)
    {
        SetValue(PanelIndexProperty(), box_value(value));
    }

    void SettingsExpanderControl::OnPanelIndexChanged(DependencyObject const &d, DependencyPropertyChangedEventArgs const &e)
    {
        auto expander = d.as<dxglcfg_winui::implementation::SettingsExpanderControl>();
        int32_t index = unbox_value<int32_t>(e.NewValue());
        auto slidercontainer = expander->FindName(L"SliderContainer").as<Grid>();
        auto internalslider = expander->FindName(L"InternalPanelSlider").as<FlipView>();
        if (slidercontainer && internalslider)
        {
            internalslider.SelectedIndex(0);
        }
    }
    DependencyProperty SettingsExpanderControl::PanelIndexProperty()
    {
        static DependencyProperty panelindexproperty =
            DependencyProperty::Register(L"PanelIndex", xaml_typename<int32_t>(),
                xaml_typename<dxglcfg_winui::SettingsExpanderControl>(),
                PropertyMetadata(box_value(0), PropertyChangedCallback(&OnPanelIndexChanged)));
        return panelindexproperty;
    }

    DependencyObjectCollection SettingsExpanderControl::Panels()
    {
        return paneldata;
    }*/
}
