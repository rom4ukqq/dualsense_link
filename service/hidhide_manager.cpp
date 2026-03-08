#include "service/hidhide_manager.h"

#include <cfgmgr32.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <winsvc.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <string_view>
#include <utility>

namespace dsl::service {

namespace {

class ScHandle {
public:
    explicit ScHandle(SC_HANDLE value = nullptr) : value_(value) {
    }
    ~ScHandle() {
        if(value_ != nullptr) {
            CloseServiceHandle(value_);
        }
    }
    ScHandle(const ScHandle&) = delete;
    ScHandle& operator=(const ScHandle&) = delete;
    [[nodiscard]] SC_HANDLE Get() const {
        return value_;
    }

private:
    SC_HANDLE value_{nullptr};
};

std::wstring ToLower(std::wstring value) {
    for(auto& c : value) {
        if(c >= L'A' && c <= L'Z') {
            c = static_cast<wchar_t>(c - L'A' + L'a');
        }
    }
    return value;
}

bool IsSamePath(std::wstring_view a, std::wstring_view b) {
    return ToLower(std::wstring(a)) == ToLower(std::wstring(b));
}

std::wstring GetHidInterfacePath(HDEVINFO info, const GUID& hid_guid, const DWORD index, SP_DEVINFO_DATA* dev_data) {
    SP_DEVICE_INTERFACE_DATA interface_data{};
    interface_data.cbSize = sizeof(interface_data);
    if(!SetupDiEnumDeviceInterfaces(info, nullptr, &hid_guid, index, &interface_data)) {
        return {};
    }

    DWORD required = 0;
    SetupDiGetDeviceInterfaceDetailW(info, &interface_data, nullptr, 0, &required, nullptr);
    if(required == 0) {
        return {};
    }

    std::vector<std::byte> buffer(required);
    auto* detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data());
    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    SP_DEVINFO_DATA local_dev{};
    local_dev.cbSize = sizeof(local_dev);
    if(!SetupDiGetDeviceInterfaceDetailW(info, &interface_data, detail, required, nullptr, &local_dev)) {
        return {};
    }
    if(dev_data != nullptr) {
        *dev_data = local_dev;
    }
    return detail->DevicePath;
}

} // namespace

HidHideStatus HidHideManager::QueryServiceStatus() const {
    HidHideStatus status{};
    ScHandle scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if(scm.Get() == nullptr) {
        return status;
    }

    ScHandle service(OpenServiceW(scm.Get(), L"HidHide", SERVICE_QUERY_STATUS));
    if(service.Get() == nullptr) {
        return status;
    }

    status.installed = true;
    SERVICE_STATUS_PROCESS process_status{};
    DWORD bytes = 0;
    const auto ok = QueryServiceStatusEx(
        service.Get(),
        SC_STATUS_PROCESS_INFO,
        reinterpret_cast<LPBYTE>(&process_status),
        sizeof(process_status),
        &bytes);
    status.running = ok && process_status.dwCurrentState == SERVICE_RUNNING;
    return status;
}

std::wstring HidHideManager::ResolveDeviceInstanceId(const std::wstring_view interface_path) const {
    GUID hid_guid{};
    HidD_GetHidGuid(&hid_guid);
    HDEVINFO info = SetupDiGetClassDevsW(
        &hid_guid,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(info == INVALID_HANDLE_VALUE) {
        return {};
    }

    std::wstring instance_id;
    for(DWORD i = 0;; ++i) {
        SP_DEVINFO_DATA dev_data{};
        dev_data.cbSize = sizeof(dev_data);
        const auto path = GetHidInterfacePath(info, hid_guid, i, &dev_data);
        if(path.empty()) {
            if(GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            }
            continue;
        }
        if(!IsSamePath(path, interface_path)) {
            continue;
        }

        std::array<wchar_t, 512> buffer{};
        if(CM_Get_Device_IDW(dev_data.DevInst, buffer.data(), static_cast<ULONG>(buffer.size()), 0) == CR_SUCCESS) {
            instance_id = buffer.data();
        }
        break;
    }

    SetupDiDestroyDeviceInfoList(info);
    return instance_id;
}

std::wstring HidHideManager::ResolveCurrentExecutablePath() const {
    std::array<wchar_t, MAX_PATH> buffer{};
    const DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if(len == 0 || len >= buffer.size()) {
        return {};
    }
    return std::wstring(buffer.data(), len);
}

std::wstring HidHideManager::FindHidHideCliPath() const {
    constexpr std::array<std::wstring_view, 4> candidates = {
        LR"(C:\Program Files\Nefarius Software Solutions\HidHide\x64\HidHideCLI.exe)",
        LR"(C:\Program Files\Nefarius Software Solutions e.U.\HidHide\x64\HidHideCLI.exe)",
        LR"(C:\Program Files\HidHide\x64\HidHideCLI.exe)",
        LR"(HidHideCLI.exe)"
    };
    for(const auto path : candidates) {
        if(path == L"HidHideCLI.exe") {
            return std::wstring(path);
        }
        if(std::filesystem::exists(path)) {
            return std::wstring(path);
        }
    }
    return {};
}

bool HidHideManager::RunCli(const std::wstring& cli_path, const std::vector<std::wstring>& args) const {
    std::wstring command = L"\"";
    command += cli_path;
    command += L"\"";
    for(const auto& arg : args) {
        command += L" \"";
        command += arg;
        command += L"\"";
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::vector<wchar_t> mutable_command(command.begin(), command.end());
    mutable_command.push_back(L'\0');
    const auto created = CreateProcessW(
        nullptr,
        mutable_command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startup,
        &process);
    if(!created) {
        return false;
    }

    WaitForSingleObject(process.hProcess, 5000);
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exit_code == 0;
}

bool HidHideManager::TryRunCli(
    const std::wstring& cli_path,
    const std::initializer_list<std::vector<std::wstring>>& variants) const {
    for(const auto& args : variants) {
        if(RunCli(cli_path, args)) {
            return true;
        }
    }
    return false;
}

bool HidHideManager::EnsureAppWhitelisted(const std::wstring& cli_path, const std::wstring& exe_path) {
    const bool ok = TryRunCli(cli_path, {{{L"--app-reg", exe_path}}, {{L"--app-add", exe_path}}});
    app_whitelisted_ = app_whitelisted_ || ok;
    return ok;
}

bool HidHideManager::SetDeviceHidden(const std::wstring& cli_path, const std::wstring_view instance_id, const bool hidden) {
    if(instance_id.empty()) {
        return false;
    }
    const bool ok = hidden
                        ? TryRunCli(
                              cli_path,
                              {{{L"--dev-hide", std::wstring(instance_id)}}, {{L"--dev-reg", std::wstring(instance_id)}}})
                        : TryRunCli(
                              cli_path,
                              {{{L"--dev-unhide", std::wstring(instance_id)}}, {{L"--dev-unreg", std::wstring(instance_id)}}});
    if(ok) {
        device_hidden_ = hidden;
    }
    return ok;
}

bool HidHideManager::SetCloakEnabled(const std::wstring& cli_path, const bool enabled) {
    return enabled ? TryRunCli(cli_path, {{{L"--cloak-on"}}, {{L"--enable"}}})
                   : TryRunCli(cli_path, {{{L"--cloak-off"}}, {{L"--disable"}}});
}

void HidHideManager::UpdateCachedFlags(HidHideStatus* status) const {
    status->app_whitelisted = app_whitelisted_;
    status->device_hidden = device_hidden_;
    status->operation_ok = operation_ok_;
    status->device_interface_path = controller_interface_path_;
    status->device_instance_id = controller_instance_id_;
}

HidHideStatus HidHideManager::QueryStatus() const {
    auto status = QueryServiceStatus();
    std::scoped_lock lock(mutex_);
    status.integration_enabled = integration_enabled_;
    UpdateCachedFlags(&status);
    return status;
}

bool HidHideManager::SetIntegrationEnabled(const bool enabled) {
    const auto base_status = QueryServiceStatus();
    std::scoped_lock lock(mutex_);
    integration_enabled_ = enabled;
    operation_ok_ = false;
    if(!base_status.installed || !base_status.running) {
        return false;
    }

    const auto cli_path = FindHidHideCliPath();
    if(cli_path.empty()) {
        return false;
    }
    if(controller_instance_id_.empty() && !controller_interface_path_.empty()) {
        controller_instance_id_ = ResolveDeviceInstanceId(controller_interface_path_);
    }

    const auto exe_path = ResolveCurrentExecutablePath();
    const bool app_ok = !exe_path.empty() && EnsureAppWhitelisted(cli_path, exe_path);
    const bool device_ok = SetDeviceHidden(cli_path, controller_instance_id_, enabled);
    const bool cloak_ok = SetCloakEnabled(cli_path, enabled);
    operation_ok_ = app_ok && device_ok && cloak_ok;
    return operation_ok_;
}

bool HidHideManager::IsIntegrationEnabled() const {
    std::scoped_lock lock(mutex_);
    return integration_enabled_;
}

HidHideStatus HidHideManager::GetLastStatus() const {
    std::scoped_lock lock(mutex_);
    HidHideStatus status{};
    status.integration_enabled = integration_enabled_;
    UpdateCachedFlags(&status);
    return status;
}

void HidHideManager::SetControllerInterfacePath(std::wstring interface_path) {
    std::scoped_lock lock(mutex_);
    controller_interface_path_ = std::move(interface_path);
    controller_instance_id_.clear();
}

} // namespace dsl::service
