#pragma once

#include <cstdint>
#include <vector>

namespace dsl::ipc {

enum class CommandType : std::uint16_t {
    Unknown = 0,
    UiRequestStatus = 1,
    UiSetLiveStream = 2,
    UiShutdownService = 3,
    UiRequestHidHideStatus = 4,
    UiSetHidHideEnabled = 5,
    ServiceStatus = 100,
    ServiceRawInput = 101,
    ServiceInfo = 102,
    ServiceHidHideStatus = 103,
    ServiceBridgeStatus = 104
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

struct HidHideTogglePayload {
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

struct HidHideStatusPayload {
    std::uint8_t installed{0};
    std::uint8_t service_running{0};
    std::uint8_t integration_enabled{0};
    std::uint8_t app_whitelisted{0};
    std::uint8_t device_hidden{0};
    std::uint8_t operation_ok{0};
    std::uint8_t reserved{0};
};

enum class BridgeMode : std::uint8_t {
    Unknown = 0,
    DriverBridge = 1,
    ConsoleFallback = 2
};

struct BridgeStatusPayload {
    BridgeMode mode{BridgeMode::Unknown};
    std::uint8_t connected{0};
    std::uint8_t reserved[2]{};
};
#pragma pack(pop)

struct CommandMessage {
    CommandHeader header{};
    std::vector<std::uint8_t> payload{};
};

} // namespace dsl::ipc
