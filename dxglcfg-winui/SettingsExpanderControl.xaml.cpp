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

    IInspectable SettingsExpanderControl::ExpandableContent()
    {
        return unbox_value<IInspectable>(GetValue(ExpandableContentProperty()));
    }
    void SettingsExpanderControl::ExpandableContent(IInspectable const &value)
    {
        SetValue(ExpandableContentProperty(), box_value(value));
    }
    DependencyProperty SettingsExpanderControl::ExpandableContentProperty()
    {
        static DependencyProperty expandablecontentproperty =
            DependencyProperty::Register(L"ExpandableContent", xaml_typename<IInspectable>(),
                xaml_typename<dxglcfg_winui::SettingsExpanderControl>(),
                PropertyMetadata(nullptr));
        return expandablecontentproperty;
    }

    bool SettingsExpanderControl::IsExpanded()
    {
        return unbox_value<bool>(GetValue(IsExpandedProperty()));
    }
    void SettingsExpanderControl::IsExpanded(bool value)
    {
        SetValue(IsExpandedProperty(), box_value<bool>(value));
    }
    DependencyProperty SettingsExpanderControl::IsExpandedProperty()
    {
        static DependencyProperty isexpandedproperty =
            DependencyProperty::Register(L"IsExpanded", xaml_typename<bool>(),
                xaml_typename<dxglcfg_winui::SettingsExpanderControl>(),
                PropertyMetadata(box_value(false), PropertyChangedCallback(&OnIsExpandedChanged)));
        return isexpandedproperty;
    }
    void SettingsExpanderControl::OnIsExpandedChanged(DependencyObject const& d, DependencyPropertyChangedEventArgs const& e)
    {
        auto control = d.as<dxglcfg_winui::SettingsExpanderControl>();
        auto expander = winrt::get_self<SettingsExpanderControl>(control);
        bool isExpanded = unbox_value<bool>(e.NewValue());
        auto sliderContainer = expander->FindName(L"Sliderontainer").as<Grid>();
        if (sliderContainer)
        {
            sliderContainer.Visibility(isExpanded ? Visibility::Visible : Visibility::Collapsed);
        }
    }
}
