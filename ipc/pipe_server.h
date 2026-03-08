#pragma once

#include "ipc/commands.h"
#include "shared/common_types.h"

#include <Windows.h>

#include <atomic>
#include <functional>
#include <string_view>
#include <mutex>
#include <span>
#include <thread>

namespace dsl::ipc {

class PipeServer {
public:
    using CommandCallback = std::function<void(const CommandMessage&)>;

    PipeServer() = default;
    ~PipeServer();

    bool Start();
    void Stop();

    void SetOnCommand(CommandCallback callback);
    bool SendStatus(const StatusPayload& status);
    bool SendRawInput(std::string_view utf8_line);
    bool SendInfo(std::string_view utf8_line);

private:
    void Run();
    bool CreateAndConnectPipe();
    void HandleConnectedClient();
    void ClosePipe();
    void DispatchCommand(const CommandMessage& message);
    bool SendMessage(CommandType type, std::span<const std::uint8_t> payload);
    bool ReadMessage(CommandMessage& message);
    bool SendRaw(const void* data, std::size_t size);

    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::jthread worker_thread_;
    HANDLE pipe_{INVALID_HANDLE_VALUE};
    std::mutex write_mutex_;

    CommandCallback on_command_;
    mutable std::mutex callback_mutex_;
};

} // namespace dsl::ipc
