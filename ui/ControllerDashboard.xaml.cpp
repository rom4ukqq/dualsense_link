#include "ControllerDashboard.xaml.h"

namespace winrt::DualSenseLinkUI::implementation {

ControllerDashboard::ControllerDashboard() {
    InitializeComponent();
}

void ControllerDashboard::SetDpadText(const std::wstring_view text) {
    DpadStateText().Text(winrt::hstring(text));
}

void ControllerDashboard::SetFaceButtonsText(const std::wstring_view text) {
    FaceButtonsText().Text(winrt::hstring(text));
}

void ControllerDashboard::SetSticksText(const std::wstring_view text) {
    SticksText().Text(winrt::hstring(text));
}

} // namespace winrt::DualSenseLinkUI::implementation
