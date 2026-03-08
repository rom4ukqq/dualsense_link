#pragma once

#include <Windows.h>

#include <mutex>
#include <string>
#include <vector>

namespace dsl::service {

struct HidHideStatus {
    bool installed{false};
    bool running{false};
    bool integration_enabled{false};
    bool app_whitelisted{false};
    bool device_hidden{false};
    bool operation_ok{false};
    std::wstring device_interface_path{};
    std::wstring device_instance_id{};
};

class HidHideManager {
public:
    HidHideManager() = default;
    HidHideStatus QueryStatus() const;
    bool SetIntegrationEnabled(bool enabled);
    bool IsIntegrationEnabled() const;
    HidHideStatus GetLastStatus() const;
    void SetControllerInterfacePath(std::wstring interface_path);

private:
    HidHideStatus QueryServiceStatus() const;
    std::wstring ResolveDeviceInstanceId(std::wstring_view interface_path) const;
    std::wstring ResolveCurrentExecutablePath() const;
    std::wstring FindHidHideCliPath() const;
    bool EnsureAppWhitelisted(const std::wstring& cli_path, const std::wstring& exe_path);
    bool SetDeviceHidden(const std::wstring& cli_path, std::wstring_view instance_id, bool hidden);
    bool SetCloakEnabled(const std::wstring& cli_path, bool enabled);
    bool TryRunCli(
        const std::wstring& cli_path,
        const std::initializer_list<std::vector<std::wstring>>& variants) const;
    bool RunCli(const std::wstring& cli_path, const std::vector<std::wstring>& args) const;
    void UpdateCachedFlags(HidHideStatus* status) const;

    mutable std::mutex mutex_;
    std::wstring controller_interface_path_{};
    std::wstring controller_instance_id_{};
    bool integration_enabled_{false};
    bool app_whitelisted_{false};
    bool device_hidden_{false};
    bool operation_ok_{false};
};

} // namespace dsl::service
