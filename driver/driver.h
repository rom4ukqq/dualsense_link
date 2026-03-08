#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32) && __has_include(<ntddk.h>) && __has_include(<wdf.h>) && __has_include(<vhf.h>)
#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>
#define DSL_DRIVER_HAS_WDK 1
#else
#define DSL_DRIVER_HAS_WDK 0
using WDFDRIVER = void*;
using WDFDEVICE = void*;
using WDFOBJECT = void*;
using NTSTATUS = long;
constexpr NTSTATUS STATUS_SUCCESS = 0;
constexpr NTSTATUS STATUS_NOT_SUPPORTED = static_cast<NTSTATUS>(0xC00000BBL);
#endif

#if DSL_DRIVER_HAS_WDK
extern "C" DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD DslEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP DslEvtDeviceContextCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DslEvtIoDeviceControl;

NTSTATUS DslCreateDefaultQueue(WDFDEVICE device);
#endif
