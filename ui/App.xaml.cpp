#include "App.xaml.h"

namespace winrt::DualSenseLinkUI::implementation {

App::App() {
    InitializeComponent();
}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
    window_ = winrt::DualSenseLinkUI::MainWindow();
    window_.Activate();
}

} // namespace winrt::DualSenseLinkUI::implementation
