#include "MainWindow.xaml.h"

namespace winrt::DualSenseLinkUI::implementation {

MainWindow::MainWindow() {
    InitializeComponent();
}

void MainWindow::UpdateControllerStatus(const std::wstring_view connection_state, const int battery_percent) {
    ConnectionStateText().Text(winrt::hstring(connection_state));
    BatteryText().Text(L"Battery: " + std::to_wstring(battery_percent) + L"%");
}

void MainWindow::AppendRawInputLine(const std::wstring_view raw_line) {
    auto current = RawInputTextBox().Text();
    current = current + winrt::hstring(raw_line) + L"\n";
    RawInputTextBox().Text(current);
}

} // namespace winrt::DualSenseLinkUI::implementation
