#pragma once

#include <cstdint>

namespace dsl::shared::driver_protocol {

#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED 0
#endif

#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS 0
#endif

#ifndef FILE_READ_DATA
#define FILE_READ_DATA 0x0001
#endif

#ifndef FILE_WRITE_DATA
#define FILE_WRITE_DATA 0x0002
#endif

#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) \
    ((((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method)))
#endif

constexpr wchar_t kUserDevicePath[] = LR"(\\.\DualSenseLinkVhf)";
constexpr wchar_t kKernelDeviceName[] = LR"(\Device\DualSenseLinkVhf)";
constexpr wchar_t kDosDeviceName[] = LR"(\DosDevices\DualSenseLinkVhf)";

constexpr std::uint32_t kInterfaceVersion = 1;
constexpr std::uint32_t kMaxReportSize = 128;

constexpr std::uint32_t kDeviceType = 0xA5D5;
constexpr std::uint32_t IOCTL_DSL_SUBMIT_INPUT_REPORT =
    CTL_CODE(kDeviceType, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA);
constexpr std::uint32_t IOCTL_DSL_GET_OUTPUT_REPORT =
    CTL_CODE(kDeviceType, 0x802, METHOD_BUFFERED, FILE_READ_DATA);
constexpr std::uint32_t IOCTL_DSL_GET_VERSION =
    CTL_CODE(kDeviceType, 0x803, METHOD_BUFFERED, FILE_READ_DATA);
constexpr std::uint32_t IOCTL_DSL_PUSH_OUTPUT_REPORT =
    CTL_CODE(kDeviceType, 0x804, METHOD_BUFFERED, FILE_WRITE_DATA);

#pragma pack(push, 1)
struct InputReportRequest {
    std::uint32_t report_size{0};
    std::uint8_t report[kMaxReportSize]{};
};

struct OutputReportResponse {
    std::uint32_t report_size{0};
    std::uint8_t report[kMaxReportSize]{};
};

struct OutputReportPushRequest {
    std::uint32_t report_size{0};
    std::uint8_t report[kMaxReportSize]{};
};

struct DriverVersionResponse {
    std::uint32_t interface_version{0};
    std::uint32_t flags{0};
};
#pragma pack(pop)

} // namespace dsl::shared::driver_protocol
