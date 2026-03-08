#include "core/packet_translation.h"

#include "core/crc32.h"

#include <algorithm>
#include <atomic>
#include <cstring>

namespace dsl::core {

namespace {

std::uint8_t NextSequenceTag() {
    static std::atomic<std::uint8_t> sequence{0};
    const auto current = static_cast<std::uint8_t>(sequence.fetch_add(1) & 0x0F);
    return static_cast<std::uint8_t>(current << 4);
}

void WriteLe32(std::uint8_t* out, const std::uint32_t value) {
    out[0] = static_cast<std::uint8_t>(value & 0xFF);
    out[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    out[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    out[3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
}

DualSenseOutputReportUsb NormalizeUsbOutput(const std::span<const std::uint8_t> usb_output_report) {
    DualSenseOutputReportUsb normalized{};
    normalized.report_id = kDualSenseOutputReportUsbId;
    if(usb_output_report.empty()) {
        return normalized;
    }
    if(usb_output_report.front() == kDualSenseOutputReportUsbId) {
        const auto copy_size = std::min<std::size_t>(usb_output_report.size(), sizeof(normalized));
        std::memcpy(&normalized, usb_output_report.data(), copy_size);
        return normalized;
    }
    const auto payload_size = std::min<std::size_t>(usb_output_report.size(), sizeof(normalized) - 1);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&normalized) + 1, usb_output_report.data(), payload_size);
    return normalized;
}

} // namespace

DualSenseUsbInputReport PacketTranslation::ToUsbInputReport(const DualSenseInputReport& bt_report) {
    DualSenseUsbInputReport usb{};
    usb.bytes.fill(0);
    usb.bytes[0] = kDualSenseInputReportUsbId;
    std::memcpy(usb.bytes.data() + 1, &bt_report, sizeof(bt_report));
    return usb;
}

std::vector<std::uint8_t> PacketTranslation::ToBluetoothOutputReport(
    const std::span<const std::uint8_t> usb_output_report) {
    if(usb_output_report.empty()) {
        return {};
    }

    const auto usb = NormalizeUsbOutput(usb_output_report);
    DualSenseOutputReportBt bt{};
    bt.report_id = kDualSenseOutputReportBtId;
    bt.sequence_tag = NextSequenceTag();
    bt.tag = kDualSenseBtOutputTag;
    std::memcpy(&bt.common, &usb.common, sizeof(bt.common));

    std::vector<std::uint8_t> packet(sizeof(bt));
    std::memcpy(packet.data(), &bt, sizeof(bt));

    const auto payload = std::span<const std::uint8_t>(packet.data(), packet.size() - 4);
    const auto crc = ComputeSeededDualSenseCrc(kDualSenseOutputCrcSeed, payload);
    WriteLe32(packet.data() + packet.size() - 4, crc);
    return packet;
}

} // namespace dsl::core
