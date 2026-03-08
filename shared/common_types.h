#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace dsl::shared {

enum class ConnectionState : std::uint8_t {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2
};

struct StickState {
    std::uint8_t x{0};
    std::uint8_t y{0};
};

struct ControllerStatus {
    ConnectionState connection{ConnectionState::Disconnected};
    std::uint8_t battery_percent{0};
    StickState left_stick{};
    StickState right_stick{};
    std::array<bool, 16> buttons{};
};

constexpr std::wstring_view kPipeName = LR"(\\.\pipe\DualSenseLink)";
constexpr std::uint16_t kSonyVendorId = 0x054C;
constexpr std::uint16_t kDualSensePid = 0x0CE6;
constexpr std::uint16_t kDualSenseEdgePid = 0x0DF2;

} // namespace dsl::shared
