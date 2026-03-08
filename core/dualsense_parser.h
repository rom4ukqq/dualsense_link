#pragma once

#include "core/dualsense_reports.h"

#include <cstdint>
#include <span>
#include <vector>

namespace dsl::core {

enum class DpadDirection : std::uint8_t {
    Up = 0,
    UpRight = 1,
    Right = 2,
    DownRight = 3,
    Down = 4,
    DownLeft = 5,
    Left = 6,
    UpLeft = 7,
    Centered = 8
};

enum class BatteryChargeState : std::uint8_t {
    Unknown = 0,
    Discharging = 1,
    Charging = 2,
    Full = 3,
    NotCharging = 4
};

struct ParsedDualSenseState {
    DualSenseInputReport raw{};
    DpadDirection dpad{DpadDirection::Centered};
    bool square{false};
    bool cross{false};
    bool circle{false};
    bool triangle{false};
    bool l1{false};
    bool r1{false};
    bool l2_button{false};
    bool r2_button{false};
    bool create{false};
    bool options{false};
    bool l3{false};
    bool r3{false};
    bool ps{false};
    bool touchpad_click{false};
    bool mic{false};
    std::uint8_t battery_percent{0};
    BatteryChargeState charge_state{BatteryChargeState::Unknown};
};

class DualSenseParser {
public:
    bool TryParseBluetoothInput(
        const std::vector<std::uint8_t>& raw_packet,
        DualSenseInputReport& report) const;

    ParsedDualSenseState ParseState(const DualSenseInputReport& report) const;

private:
    static bool TryParseBtPacket(
        std::span<const std::uint8_t> raw_packet,
        DualSenseInputReport& report);

    static bool TryParseUsbPacket(
        std::span<const std::uint8_t> raw_packet,
        DualSenseInputReport& report);

    static bool ValidateBtInputCrc(std::span<const std::uint8_t> raw_packet);
    static std::uint32_t ReadLe32(const std::uint8_t* value);
    static DpadDirection DecodeDpad(std::uint8_t nibble);
    static std::uint8_t NormalizeBattery(std::uint8_t status_0);
    static BatteryChargeState DecodeChargeState(std::uint8_t status_0);
};

} // namespace dsl::core
