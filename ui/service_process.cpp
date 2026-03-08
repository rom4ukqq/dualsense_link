#include "service_process.h"

#include "shared/common_types.h"

#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>

namespace dualsense_link::ui {

namespace {

constexpr std::wstring_view kServiceExeName = L"dsl_service.exe";

} // namespace

ServiceProcess::~ServiceProcess() {
    ShutdownIfLaunched([] { return false; });
    CloseHandles();
}

bool ServiceProcess::EnsureRunning() {
    if(IsPipeReachable()) {
        service_detected_ = true;
        return true;
    }

    const auto path = ResolveServicePath();
    if(path.empty()) {
        return false;
    }

    if(!LaunchProcess(path)) {
        return false;
    }

    launch_path_ = path;
    launched_by_ui_ = true;
    for(int i = 0; i < 30; ++i) {
        if(IsPipeReachable()) {
            service_detected_ = true;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

void ServiceProcess::ShutdownIfLaunched(const std::function<bool()>& request_shutdown) {
    if(!launched_by_ui_) {
        return;
    }

    request_shutdown();
    if(process_info_.hProcess != nullptr) {
        WaitForSingleObject(process_info_.hProcess, 2000);
    }
    launched_by_ui_ = false;
    service_detected_ = false;
}

bool ServiceProcess::WasLaunchedByUi() const {
    return launched_by_ui_;
}

std::wstring ServiceProcess::StatusText() const {
    if(!service_detected_) {
        return L"Service: not running";
    }
    if(launched_by_ui_) {
        return L"Service: started by UI";
    }
    return L"Service: external";
}

bool ServiceProcess::IsPipeReachable() const {
    if(WaitNamedPipeW(dsl::shared::kPipeName.data(), 80)) {
        return true;
    }
    return GetLastError() == ERROR_SEM_TIMEOUT;
}

std::wstring ServiceProcess::ResolveServicePath() const {
    std::vector<wchar_t> path(MAX_PATH, L'\0');
    const DWORD len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if(len == 0 || len >= path.size()) {
        return {};
    }

    std::filesystem::path ui_path(path.data());
    const auto ui_dir = ui_path.parent_path();
    const auto candidate_a = ui_dir / kServiceExeName;
    if(std::filesystem::exists(candidate_a)) {
        return candidate_a.wstring();
    }

    const auto candidate_b = ui_dir.parent_path() / kServiceExeName;
    if(std::filesystem::exists(candidate_b)) {
        return candidate_b.wstring();
    }

    const auto candidate_c = ui_dir.parent_path().parent_path() / kServiceExeName;
    if(std::filesystem::exists(candidate_c)) {
        return candidate_c.wstring();
    }
    return {};
}

bool ServiceProcess::LaunchProcess(const std::wstring& executable_path) {
    std::wstring command = L"\"";
    command += executable_path;
    command += L"\" --background";

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};
    if(!CreateProcessW(
           executable_path.c_str(),
           command.data(),
           nullptr,
           nullptr,
           FALSE,
           CREATE_NO_WINDOW,
           nullptr,
           nullptr,
           &startup_info,
           &process_info)) {
        return false;
    }

    CloseHandles();
    process_info_ = process_info;
    return true;
}

void ServiceProcess::CloseHandles() {
    if(process_info_.hThread != nullptr) {
        CloseHandle(process_info_.hThread);
        process_info_.hThread = nullptr;
    }
    if(process_info_.hProcess != nullptr) {
        CloseHandle(process_info_.hProcess);
        process_info_.hProcess = nullptr;
    }
    process_info_.dwProcessId = 0;
    process_info_.dwThreadId = 0;
}

} // namespace dualsense_link::ui
