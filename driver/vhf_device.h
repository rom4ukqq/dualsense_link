#pragma once

#include "driver/driver.h"

#include <cstddef>
#include <cstdint>

#if DSL_DRIVER_HAS_WDK
constexpr std::size_t kDslOutputQueueCapacity = 16;
constexpr std::size_t kDslMaxOutputReportSize = 128;

typedef struct _DSL_OUTPUT_PACKET {
    std::uint8_t report[kDslMaxOutputReportSize];
    std::size_t report_size;
} DSL_OUTPUT_PACKET, *PDSL_OUTPUT_PACKET;

typedef struct _DSL_VHF_CONTEXT {
    VHFHANDLE handle;
    WDFSPINLOCK output_lock;
    DSL_OUTPUT_PACKET output_queue[kDslOutputQueueCapacity];
    std::size_t queue_head;
    std::size_t queue_tail;
    std::size_t queue_count;
} DSL_VHF_CONTEXT, *PDSL_VHF_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSL_VHF_CONTEXT, DslGetVhfContext);
#endif

NTSTATUS DslCreateVirtualDevice(WDFDEVICE device);
void DslDestroyVirtualDevice(WDFDEVICE device);
NTSTATUS send_input_report(WDFDEVICE device, const std::uint8_t* report, std::size_t report_size);
NTSTATUS receive_output_report(
    WDFDEVICE device,
    std::uint8_t* report,
    std::size_t report_size,
    std::size_t* bytes_copied);
NTSTATUS queue_output_report(WDFDEVICE device, const std::uint8_t* report, std::size_t report_size);
