#include "PipeClient.h"

#include "shared/common_types.h"

#include <chrono>
#include <cstring>
#include <string>
#include <utility>

namespace dualsense_link::ui {

PipeClient::~PipeClient() {
    Stop();
}

bool PipeClient::Start() {
    if(running_.exchange(true)) {
        return true;
    }
    read_thread_ = std::jthread([this](std::stop_token) { RunReadLoop(); });
    return true;
}

void PipeClient::Stop() {
    if(!running_.exchange(false)) {
        return;
    }
    ClosePipe();
    if(read_thread_.joinable()) {
        read_thread_.request_stop();
        read_thread_.join();
    }
}

bool PipeClient::RequestStatus() {
    return SendCommand(dsl::ipc::CommandType::UiRequestStatus, {});
}

bool PipeClient::SetLiveStreamEnabled(const bool enabled) {
    dsl::ipc::LiveStreamPayload payload{};
    payload.enabled = enabled ? 1 : 0;
    const auto bytes = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(&payload),
        sizeof(payload));
    return SendCommand(dsl::ipc::CommandType::UiSetLiveStream, bytes);
}

void PipeClient::SetOnStatus(StatusCallback callback) {
    std::scoped_lock lock(callback_mutex_);
    on_status_ = std::move(callback);
}

void PipeClient::SetOnRawInput(TextCallback callback) {
    std::scoped_lock lock(callback_mutex_);
    on_raw_input_ = std::move(callback);
}

void PipeClient::SetOnInfo(TextCallback callback) {
    std::scoped_lock lock(callback_mutex_);
    on_info_ = std::move(callback);
}

void PipeClient::RunReadLoop() {
    while(running_.load()) {
        if(pipe_ == INVALID_HANDLE_VALUE) {
            pipe_ = CreateFileW(
                dsl::shared::kPipeName.data(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            if(pipe_ == INVALID_HANDLE_VALUE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                continue;
            }
            RequestStatus();
        }

        dsl::ipc::CommandMessage message{};
        if(!ReadMessage(message)) {
            ClosePipe();
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            continue;
        }
        DispatchMessage(message);
    }
}

void PipeClient::DispatchMessage(const dsl::ipc::CommandMessage& message) {
    StatusCallback status_cb;
    TextCallback raw_cb;
    TextCallback info_cb;
    {
        std::scoped_lock lock(callback_mutex_);
        status_cb = on_status_;
        raw_cb = on_raw_input_;
        info_cb = on_info_;
    }

    if(message.header.type == dsl::ipc::CommandType::ServiceStatus &&
       message.payload.size() >= sizeof(dsl::ipc::StatusPayload)) {
        dsl::ipc::StatusPayload payload{};
        std::memcpy(&payload, message.payload.data(), sizeof(payload));
        if(status_cb) {
            status_cb(payload);
        }
        return;
    }

    if(message.header.type == dsl::ipc::CommandType::ServiceRawInput && raw_cb) {
        raw_cb(std::string_view(reinterpret_cast<const char*>(message.payload.data()), message.payload.size()));
        return;
    }

    if(message.header.type == dsl::ipc::CommandType::ServiceInfo && info_cb) {
        info_cb(std::string_view(reinterpret_cast<const char*>(message.payload.data()), message.payload.size()));
    }
}

bool PipeClient::SendCommand(const dsl::ipc::CommandType type, const std::span<const std::uint8_t> payload) {
    if(pipe_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    dsl::ipc::CommandHeader header{};
    header.type = type;
    header.payload_size = static_cast<std::uint16_t>(payload.size());

    std::scoped_lock lock(write_mutex_);
    DWORD written = 0;
    if(!WriteFile(pipe_, &header, sizeof(header), &written, nullptr) || written != sizeof(header)) {
        return false;
    }
    if(payload.empty()) {
        return true;
    }
    return WriteFile(pipe_, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr) &&
           written == payload.size();
}

bool PipeClient::ReadMessage(dsl::ipc::CommandMessage& message) {
    if(pipe_ == INVALID_HANDLE_VALUE) {
        return false;
    }

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

void PipeClient::ClosePipe() {
    std::scoped_lock lock(write_mutex_);
    if(pipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
    }
}

} // namespace dualsense_link::ui
