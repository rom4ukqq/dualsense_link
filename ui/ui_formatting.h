#pragma once

#include "ipc/commands.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace dualsense_link::ui {

std::wstring Utf8ToWide(std::string_view text);
std::wstring ConnectionText(std::uint8_t state);
std::wstring DpadText(std::uint8_t value);
std::wstring ButtonsText(std::uint16_t mask);
std::wstring HidHideLabelText(const dsl::ipc::HidHideStatusPayload& payload);
std::string HidHideStatusLine(const dsl::ipc::HidHideStatusPayload& payload);

} // namespace dualsense_link::ui

