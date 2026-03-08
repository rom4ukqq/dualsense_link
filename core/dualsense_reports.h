#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace dsl::core {

constexpr std::uint8_t kDualSenseInputReportUsbId = 0x01;
constexpr std::size_t kDualSenseInputReportUsbSize = 64;
constexpr std::uint8_t kDualSenseInputReportBtId = 0x31;
constexpr std::size_t kDualSenseInputReportBtSize = 78;

constexpr std::uint8_t kDualSenseOutputReportUsbId = 0x02;
constexpr std::size_t kDualSenseOutputReportUsbSize = 63;
constexpr std::uint8_t kDualSenseOutputReportBtId = 0x31;
constexpr std::size_t kDualSenseOutputReportBtSize = 78;

constexpr std::uint8_t kDualSenseBtOutputTag = 0x10;
constexpr std::uint8_t kDualSenseInputCrcSeed = 0xA1;
constexpr std::uint8_t kDualSenseOutputCrcSeed = 0xA2;

#pragma pack(push, 1)
struct DualSenseTouchPoint {
    std::uint8_t contact{};
    std::uint8_t x_lo{};
    std::uint8_t x_hi_y_lo{};
    std::uint8_t y_hi{};
};

// Main shared DualSense input payload without transport-specific headers/trailers.
struct DualSenseInputReport {
    std::uint8_t left_stick_x{};
    std::uint8_t left_stick_y{};
    std::uint8_t right_stick_x{};
    std::uint8_t right_stick_y{};
    std::uint8_t left_trigger{};
    std::uint8_t right_trigger{};
    std::uint8_t sequence_number{};
    std::uint8_t buttons_1{};
    std::uint8_t buttons_2{};
    std::uint8_t buttons_3{};
    std::uint8_t buttons_4{};
    std::uint8_t reserved_0[4]{};
    std::int16_t gyro_x{};
    std::int16_t gyro_y{};
    std::int16_t gyro_z{};
    std::int16_t accel_x{};
    std::int16_t accel_y{};
    std::int16_t accel_z{};
    std::uint32_t sensor_timestamp{};
    std::uint8_t reserved_1{};
    DualSenseTouchPoint touch_1{};
    DualSenseTouchPoint touch_2{};
    std::uint8_t reserved_2[12]{};
    std::uint8_t status_0{};
    std::uint8_t status_1{};
    std::uint8_t status_2{};
    std::uint8_t reserved_3[8]{};
};

struct DualSenseOutputReportCommon {
    std::uint8_t valid_flag_0{};
    std::uint8_t valid_flag_1{};
    std::uint8_t motor_right{};
    std::uint8_t motor_left{};
    std::uint8_t headphone_volume{};
    std::uint8_t speaker_volume{};
    std::uint8_t mic_volume{};
    std::uint8_t audio_control{};
    std::uint8_t mute_button_led{};
    std::uint8_t power_save_control{};
    std::uint8_t reserved_2[27]{};
    std::uint8_t audio_control_2{};
    std::uint8_t valid_flag_2{};
    std::uint8_t reserved_3[2]{};
    std::uint8_t lightbar_setup{};
    std::uint8_t led_brightness{};
    std::uint8_t player_leds{};
    std::uint8_t lightbar_red{};
    std::uint8_t lightbar_green{};
    std::uint8_t lightbar_blue{};
};

struct DualSenseOutputReportUsb {
    std::uint8_t report_id{};
    DualSenseOutputReportCommon common{};
    std::uint8_t reserved[15]{};
};

struct DualSenseOutputReportBt {
    std::uint8_t report_id{};
    std::uint8_t sequence_tag{};
    std::uint8_t tag{};
    DualSenseOutputReportCommon common{};
    std::uint8_t reserved[24]{};
    std::uint32_t crc32{};
};
#pragma pack(pop)

static_assert(sizeof(DualSenseTouchPoint) == 4, "Unexpected DualSenseTouchPoint size.");
static_assert(sizeof(DualSenseInputReport) == (kDualSenseInputReportUsbSize - 1), "Unexpected DualSenseInputReport size.");
static_assert(sizeof(DualSenseOutputReportCommon) == 47, "Unexpected DualSenseOutputReportCommon size.");
static_assert(sizeof(DualSenseOutputReportUsb) == kDualSenseOutputReportUsbSize, "Unexpected DualSenseOutputReportUsb size.");
static_assert(sizeof(DualSenseOutputReportBt) == kDualSenseOutputReportBtSize, "Unexpected DualSenseOutputReportBt size.");

struct DualSenseUsbInputReport {
    // USB-facing input report forwarded to the virtual HID layer.
    std::array<std::uint8_t, kDualSenseInputReportUsbSize> bytes{};
};

} // namespace dsl::core
