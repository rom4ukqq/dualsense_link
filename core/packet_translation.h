#pragma once

#include "core/dualsense_reports.h"

#include <cstdint>
#include <span>
#include <vector>

namespace dsl::core {

class PacketTranslation {
public:
    static DualSenseUsbInputReport ToUsbInputReport(const DualSenseInputReport& bt_report);

    static std::vector<std::uint8_t> ToBluetoothOutputReport(
        std::span<const std::uint8_t> usb_output_report);
};

} // namespace dsl::core
