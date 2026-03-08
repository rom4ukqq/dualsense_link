#pragma once

#include "driver/driver.h"

#include <cstddef>
#include <cstdint>

#if DSL_DRIVER_HAS_WDK
typedef struct _DSL_VHF_CONTEXT {
    VHFHANDLE handle;
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
