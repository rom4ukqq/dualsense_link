#pragma once

#include "ipc/commands.h"

#include <Windows.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

namespace dualsense_link::ui {

class PipeClient {
public:
    using StatusCallback = std::function<void(const dsl::ipc::StatusPayload&)>;
    using TextCallback = std::function<void(std::string_view)>;

    PipeClient() = default;
    ~PipeClient();

    bool Start();
    void Stop();

    bool RequestStatus();
    bool SetLiveStreamEnabled(bool enabled);

    void SetOnStatus(StatusCallback callback);
    void SetOnRawInput(TextCallback callback);
    void SetOnInfo(TextCallback callback);

private:
    void RunReadLoop();
    void DispatchMessage(const dsl::ipc::CommandMessage& message);
    bool SendCommand(dsl::ipc::CommandType type, std::span<const std::uint8_t> payload);
    bool ReadMessage(dsl::ipc::CommandMessage& message);
    void ClosePipe();

    std::atomic<bool> running_{false};
    std::jthread read_thread_;
    HANDLE pipe_{INVALID_HANDLE_VALUE};
    std::mutex write_mutex_;
    std::mutex callback_mutex_;

    StatusCallback on_status_;
    TextCallback on_raw_input_;
    TextCallback on_info_;
};

} // namespace dualsense_link::ui
