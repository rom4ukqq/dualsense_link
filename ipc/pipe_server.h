#pragma once

#include "ipc/commands.h"
#include "shared/common_types.h"

#include <Windows.h>

#include <atomic>
#include <functional>
#include <jthread>
#include <mutex>

namespace dsl::ipc {

class PipeServer {
public:
    using CommandCallback = std::function<void(const CommandMessage&)>;

    PipeServer() = default;
    ~PipeServer();

    bool Start();
    void Stop();

    void SetOnCommand(CommandCallback callback);
    bool SendStatus(const shared::ControllerStatus& status);

private:
    void Run();
    bool CreateAndConnectPipe();
    void HandleConnectedClient();
    void ClosePipe();
    void DispatchCommand(const CommandMessage& message);
    bool ReadMessage(CommandMessage& message);
    bool SendRaw(const void* data, std::size_t size);

    std::atomic<bool> running_{false};
    std::jthread worker_thread_;
    HANDLE pipe_{INVALID_HANDLE_VALUE};

    CommandCallback on_command_;
    mutable std::mutex callback_mutex_;
};

} // namespace dsl::ipc
