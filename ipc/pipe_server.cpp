#include "ipc/pipe_server.h"

#include <cstring>
#include <utility>
#include <vector>

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
    connected_ = false;
    ClosePipe();

    if(worker_thread_.joinable()) {
        worker_thread_.request_stop();
        worker_thread_.join();
    }
}

void PipeServer::SetOnCommand(CommandCallback callback) {
    std::scoped_lock lock(callback_mutex_);
    on_command_ = std::move(callback);
}

bool PipeServer::SendStatus(const StatusPayload& status) {
    const auto payload = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(&status),
        sizeof(status));
    return SendMessage(CommandType::ServiceStatus, payload);
}

bool PipeServer::SendRawInput(const std::string_view utf8_line) {
    const auto payload = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(utf8_line.data()),
        utf8_line.size());
    return SendMessage(CommandType::ServiceRawInput, payload);
}

bool PipeServer::SendInfo(const std::string_view utf8_line) {
    const auto payload = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(utf8_line.data()),
        utf8_line.size());
    return SendMessage(CommandType::ServiceInfo, payload);
}

void PipeServer::Run() {
    while(running_.load()) {
        if(!CreateAndConnectPipe()) {
            continue;
        }
        connected_ = true;
        HandleConnectedClient();
        connected_ = false;
        ClosePipe();
    }
}

bool PipeServer::CreateAndConnectPipe() {
    pipe_ = CreateNamedPipeW(
        shared::kPipeName.data(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        8192,
        8192,
        0,
        nullptr);

    if(pipe_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    const bool connected = ConnectNamedPipe(pipe_, nullptr) != FALSE ||
                           GetLastError() == ERROR_PIPE_CONNECTED;
    if(connected) {
        return true;
    }

    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
    return false;
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
    std::scoped_lock write_lock(write_mutex_);
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

bool PipeServer::SendMessage(const CommandType type, const std::span<const std::uint8_t> payload) {
    if(!connected_.load()) {
        return false;
    }

    CommandHeader header{};
    header.type = type;
    header.payload_size = static_cast<std::uint16_t>(payload.size());

    std::scoped_lock lock(write_mutex_);
    if(!SendRaw(&header, sizeof(header))) {
        return false;
    }
    if(payload.empty()) {
        return true;
    }
    return SendRaw(payload.data(), payload.size());
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
        message.payload.clear();
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
