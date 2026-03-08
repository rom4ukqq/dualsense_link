#include "core/dualsense_parser.h"

#include "core/crc32.h"

#include <algorithm>
#include <cstring>

namespace dsl::core {

bool DualSenseParser::TryParseBluetoothInput(
    const std::vector<std::uint8_t>& raw_packet,
    DualSenseInputReport& report) const {
    if(raw_packet.empty()) {
        return false;
    }

    const std::span<const std::uint8_t> packet(raw_packet.data(), raw_packet.size());
    if(packet.front() == kDualSenseInputReportBtId) {
        return TryParseBtPacket(packet, report);
    }
    if(packet.front() == kDualSenseInputReportUsbId) {
        return TryParseUsbPacket(packet, report);
    }
    return false;
}

ParsedDualSenseState DualSenseParser::ParseState(const DualSenseInputReport& report) const {
    ParsedDualSenseState state{};
    state.raw = report;
    state.dpad = DecodeDpad(report.buttons_1 & 0x0F);
    state.square = (report.buttons_1 & 0x10) != 0;
    state.cross = (report.buttons_1 & 0x20) != 0;
    state.circle = (report.buttons_1 & 0x40) != 0;
    state.triangle = (report.buttons_1 & 0x80) != 0;
    state.l1 = (report.buttons_2 & 0x01) != 0;
    state.r1 = (report.buttons_2 & 0x02) != 0;
    state.l2_button = (report.buttons_2 & 0x04) != 0;
    state.r2_button = (report.buttons_2 & 0x08) != 0;
    state.create = (report.buttons_2 & 0x10) != 0;
    state.options = (report.buttons_2 & 0x20) != 0;
    state.l3 = (report.buttons_2 & 0x40) != 0;
    state.r3 = (report.buttons_2 & 0x80) != 0;
    state.ps = (report.buttons_3 & 0x01) != 0;
    state.touchpad_click = (report.buttons_3 & 0x02) != 0;
    state.mic = (report.buttons_3 & 0x04) != 0;
    state.battery_percent = NormalizeBattery(report.status_0);
    state.charge_state = DecodeChargeState(report.status_0);
    return state;
}

bool DualSenseParser::TryParseBtPacket(
    const std::span<const std::uint8_t> raw_packet,
    DualSenseInputReport& report) {
    if(raw_packet.size() < kDualSenseInputReportBtSize) {
        return false;
    }
    const auto packet = raw_packet.first(kDualSenseInputReportBtSize);
    if(!ValidateBtInputCrc(packet)) {
        return false;
    }
    std::memcpy(&report, packet.data() + 2, sizeof(report));
    return true;
}

bool DualSenseParser::TryParseUsbPacket(
    const std::span<const std::uint8_t> raw_packet,
    DualSenseInputReport& report) {
    if(raw_packet.size() < kDualSenseInputReportUsbSize) {
        return false;
    }
    std::memcpy(&report, raw_packet.data() + 1, sizeof(report));
    return true;
}

bool DualSenseParser::ValidateBtInputCrc(const std::span<const std::uint8_t> raw_packet) {
    if(raw_packet.size() < kDualSenseInputReportBtSize) {
        return false;
    }
    const auto expected_crc = ReadLe32(raw_packet.data() + (kDualSenseInputReportBtSize - 4));
    const auto payload = raw_packet.first(kDualSenseInputReportBtSize - 4);
    const auto actual_crc = ComputeSeededDualSenseCrc(kDualSenseInputCrcSeed, payload);
    return actual_crc == expected_crc;
}

std::uint32_t DualSenseParser::ReadLe32(const std::uint8_t* value) {
    return static_cast<std::uint32_t>(value[0]) |
           (static_cast<std::uint32_t>(value[1]) << 8) |
           (static_cast<std::uint32_t>(value[2]) << 16) |
           (static_cast<std::uint32_t>(value[3]) << 24);
}

DpadDirection DualSenseParser::DecodeDpad(const std::uint8_t nibble) {
    if(nibble <= static_cast<std::uint8_t>(DpadDirection::UpLeft)) {
        return static_cast<DpadDirection>(nibble);
    }
    return DpadDirection::Centered;
}

std::uint8_t DualSenseParser::NormalizeBattery(const std::uint8_t status_0) {
    const auto raw_capacity = static_cast<std::uint8_t>(status_0 & 0x0F);
    const auto charge_state = DecodeChargeState(status_0);
    if(charge_state == BatteryChargeState::Full) {
        return 100;
    }
    if(charge_state == BatteryChargeState::Discharging || charge_state == BatteryChargeState::Charging) {
        return std::min<std::uint8_t>(100, static_cast<std::uint8_t>(raw_capacity * 10 + 5));
    }
    return 0;
}

BatteryChargeState DualSenseParser::DecodeChargeState(const std::uint8_t status_0) {
    const auto charging = static_cast<std::uint8_t>((status_0 >> 4) & 0x0F);
    switch(charging) {
        case 0x0:
            return BatteryChargeState::Discharging;
        case 0x1:
            return BatteryChargeState::Charging;
        case 0x2:
            return BatteryChargeState::Full;
        case 0xA:
        case 0xB:
            return BatteryChargeState::NotCharging;
        default:
            return BatteryChargeState::Unknown;
    }
}

} // namespace dsl::core
