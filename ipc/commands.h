#pragma once

#include <cstdint>
#include <vector>

namespace dsl::ipc {

enum class CommandType : std::uint16_t {
    Unknown = 0,
    UiRequestStatus = 1,
    UiSetLiveStream = 2,
    UiShutdownService = 3,
    ServiceStatus = 100,
    ServiceRawInput = 101,
    ServiceInfo = 102
};

#pragma pack(push, 1)
struct CommandHeader {
    CommandType type{CommandType::Unknown};
    std::uint16_t payload_size{0};
};

struct LiveStreamPayload {
    std::uint8_t enabled{0};
    std::uint8_t reserved[3]{};
};

struct StatusPayload {
    std::uint8_t connection_state{0};
    std::uint8_t battery_percent{0};
    std::uint8_t left_stick_x{0};
    std::uint8_t left_stick_y{0};
    std::uint8_t right_stick_x{0};
    std::uint8_t right_stick_y{0};
    std::uint16_t buttons_mask{0};
    std::uint8_t dpad{0};
    std::uint8_t reserved{0};
};
#pragma pack(pop)

struct CommandMessage {
    CommandHeader header{};
    std::vector<std::uint8_t> payload{};
};

} // namespace dsl::ipc
