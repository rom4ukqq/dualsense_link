#pragma once

#include <cstdint>
#include <vector>

namespace dsl::ipc {

enum class CommandType : std::uint16_t {
    Unknown = 0,
    GetStatus = 1,
    SubscribeRawInput = 2,
    SetPassthroughEnabled = 3,
    ShutdownService = 4
};

#pragma pack(push, 1)
struct CommandHeader {
    CommandType type{CommandType::Unknown};
    std::uint16_t payload_size{0};
};

struct StatusPayload {
    std::uint8_t connection_state{0};
    std::uint8_t battery_percent{0};
    std::uint8_t reserved[2]{};
};
#pragma pack(pop)

struct CommandMessage {
    CommandHeader header{};
    std::vector<std::uint8_t> payload{};
};

} // namespace dsl::ipc
