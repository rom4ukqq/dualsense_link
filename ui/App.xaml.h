#pragma once

#include "MainWindow.xaml.h"

#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::DualSenseLinkUI::implementation {

struct App : AppT<App> {
    App();
    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

private:
    winrt::DualSenseLinkUI::MainWindow window_{nullptr};
};

} // namespace winrt::DualSenseLinkUI::implementation
