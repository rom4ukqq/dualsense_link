#pragma once

#include "App.xaml.g.h"
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

namespace winrt::DualSenseLinkUI::factory_implementation {

struct App : AppT<App, implementation::App> {
};

} // namespace winrt::DualSenseLinkUI::factory_implementation
