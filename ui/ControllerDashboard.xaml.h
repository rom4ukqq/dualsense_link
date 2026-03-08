#pragma once

#include "ControllerDashboard.xaml.g.h"

#include <string_view>
#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::DualSenseLinkUI::implementation {

struct ControllerDashboard : ControllerDashboardT<ControllerDashboard> {
    ControllerDashboard();

    void SetDpadText(std::wstring_view text);
    void SetFaceButtonsText(std::wstring_view text);
    void SetSticksText(std::wstring_view text);
};

} // namespace winrt::DualSenseLinkUI::implementation

namespace winrt::DualSenseLinkUI::factory_implementation {

struct ControllerDashboard : ControllerDashboardT<ControllerDashboard, implementation::ControllerDashboard> {
};

} // namespace winrt::DualSenseLinkUI::factory_implementation
