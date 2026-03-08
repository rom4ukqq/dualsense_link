#include "ipc/pipe_server.h"

#include <utility>

namespace dsl::ipc {

PipeServer::~PipeServer() {
    Stop();
}

bool PipeServer::Start() {
    if(running_.exchange(true)) {
        return true;
    }
    worker_thread_ = std::jthread([this](std::stop_token) { Run(); });
    return true;
}

void PipeServer::Stop() {
    if(!running_.exchange(false)) {
        return;
    }

    if(pipe_ != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(pipe_);
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
    }

    if(worker_thread_.joinable()) {
        worker_thread_.request_stop();
        worker_thread_.join();
    }
}

void PipeServer::SetOnCommand(CommandCallback callback) {
    std::scoped_lock lock(callback_mutex_);
    on_command_ = std::move(callback);
}

bool PipeServer::SendStatus(const shared::ControllerStatus& status) {
    StatusPayload payload{};
    payload.connection_state = static_cast<std::uint8_t>(status.connection);
    payload.battery_percent = status.battery_percent;

    CommandHeader header{};
    header.type = CommandType::GetStatus;
    header.payload_size = sizeof(payload);
    return SendRaw(&header, sizeof(header)) && SendRaw(&payload, sizeof(payload));
}

void PipeServer::Run() {
    while(running_.load()) {
        if(!CreateAndConnectPipe()) {
            continue;
        }
        HandleConnectedClient();
        ClosePipe();
    }
}

bool PipeServer::CreateAndConnectPipe() {
    pipe_ = CreateNamedPipeW(
        shared::kPipeName.data(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        nullptr);

    if(pipe_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    const bool connected = ConnectNamedPipe(pipe_, nullptr) != FALSE ||
                           GetLastError() == ERROR_PIPE_CONNECTED;
    if(!connected) {
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
        return false;
    }
    return true;
}

void PipeServer::HandleConnectedClient() {
    while(running_.load()) {
        CommandMessage message{};
        if(!ReadMessage(message)) {
            break;
        }
        DispatchCommand(message);
    }
}

void PipeServer::ClosePipe() {
    if(pipe_ == INVALID_HANDLE_VALUE) {
        return;
    }
    FlushFileBuffers(pipe_);
    DisconnectNamedPipe(pipe_);
    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
}

void PipeServer::DispatchCommand(const CommandMessage& message) {
    CommandCallback callback;
    {
        std::scoped_lock lock(callback_mutex_);
        callback = on_command_;
    }
    if(callback) {
        callback(message);
    }
}

bool PipeServer::ReadMessage(CommandMessage& message) {
    DWORD bytes_read = 0;
    if(!ReadFile(pipe_, &message.header, sizeof(message.header), &bytes_read, nullptr)) {
        return false;
    }
    if(bytes_read != sizeof(message.header)) {
        return false;
    }

    if(message.header.payload_size == 0) {
        return true;
    }

    message.payload.resize(message.header.payload_size);
    if(!ReadFile(pipe_, message.payload.data(), message.header.payload_size, &bytes_read, nullptr)) {
        return false;
    }
    return bytes_read == message.header.payload_size;
}

bool PipeServer::SendRaw(const void* data, const std::size_t size) {
    if(pipe_ == INVALID_HANDLE_VALUE || data == nullptr || size == 0) {
        return false;
    }

    DWORD written = 0;
    if(!WriteFile(pipe_, data, static_cast<DWORD>(size), &written, nullptr)) {
        return false;
    }
    return written == size;
}

} // namespace dsl::ipc
