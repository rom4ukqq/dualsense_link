#pragma once

#include <Windows.h>

#include <functional>
#include <string>

namespace dualsense_link::ui {

class ServiceProcess {
public:
    ServiceProcess() = default;
    ~ServiceProcess();

    bool EnsureRunning();
    void ShutdownIfLaunched(const std::function<bool()>& request_shutdown);

    [[nodiscard]] bool WasLaunchedByUi() const;
    [[nodiscard]] std::wstring StatusText() const;

private:
    [[nodiscard]] bool IsPipeReachable() const;
    [[nodiscard]] std::wstring ResolveServicePath() const;
    bool LaunchProcess(const std::wstring& executable_path);
    void CloseHandles();

    bool launched_by_ui_{false};
    bool service_detected_{false};
    std::wstring launch_path_{};
    PROCESS_INFORMATION process_info_{};
};

} // namespace dualsense_link::ui

