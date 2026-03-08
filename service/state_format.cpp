#include "service/state_format.h"

#include <cstdint>
#include <sstream>

namespace dsl::service {

namespace {

std::uint16_t BuildButtonsMask(const core::ParsedDualSenseState& state) {
    std::uint16_t mask = 0;
    mask |= static_cast<std::uint16_t>(state.square) << 0;
    mask |= static_cast<std::uint16_t>(state.cross) << 1;
    mask |= static_cast<std::uint16_t>(state.circle) << 2;
    mask |= static_cast<std::uint16_t>(state.triangle) << 3;
    mask |= static_cast<std::uint16_t>(state.l1) << 4;
    mask |= static_cast<std::uint16_t>(state.r1) << 5;
    mask |= static_cast<std::uint16_t>(state.l2_button) << 6;
    mask |= static_cast<std::uint16_t>(state.r2_button) << 7;
    mask |= static_cast<std::uint16_t>(state.create) << 8;
    mask |= static_cast<std::uint16_t>(state.options) << 9;
    mask |= static_cast<std::uint16_t>(state.l3) << 10;
    mask |= static_cast<std::uint16_t>(state.r3) << 11;
    mask |= static_cast<std::uint16_t>(state.ps) << 12;
    mask |= static_cast<std::uint16_t>(state.touchpad_click) << 13;
    mask |= static_cast<std::uint16_t>(state.mic) << 14;
    return mask;
}

} // namespace

std::string FormatStateLine(const core::ParsedDualSenseState& state) {
    std::ostringstream line;
    line << "LX:" << static_cast<int>(state.raw.left_stick_x)
         << " LY:" << static_cast<int>(state.raw.left_stick_y)
         << " RX:" << static_cast<int>(state.raw.right_stick_x)
         << " RY:" << static_cast<int>(state.raw.right_stick_y)
         << " Battery:" << static_cast<int>(state.battery_percent) << "% "
         << "Cross:" << state.cross << " Circle:" << state.circle
         << " Square:" << state.square << " Triangle:" << state.triangle;
    return line.str();
}

ipc::StatusPayload BuildStatusPayload(
    const core::ParsedDualSenseState& state,
    const shared::ConnectionState connection) {
    ipc::StatusPayload payload{};
    payload.connection_state = static_cast<std::uint8_t>(connection);
    payload.battery_percent = state.battery_percent;
    payload.left_stick_x = state.raw.left_stick_x;
    payload.left_stick_y = state.raw.left_stick_y;
    payload.right_stick_x = state.raw.right_stick_x;
    payload.right_stick_y = state.raw.right_stick_y;
    payload.buttons_mask = BuildButtonsMask(state);
    payload.dpad = static_cast<std::uint8_t>(state.dpad);
    return payload;
}

} // namespace dsl::service

