# DualSense Link Architecture

## Goal
Expose a Bluetooth DualSense controller to Windows games as a wired USB DualSense.

## Data Flow
1. Physical controller is connected over Bluetooth HID.
2. `dsl_service` discovers and opens the HID interface.
3. Incoming Bluetooth reports are parsed in `core/dualsense_parser`.
4. Parsed reports are translated into USB-shaped packets in `core/packet_translation`.
5. USB-shaped packets are forwarded to a virtual HID device implemented by KMDF + VHF driver code.
6. Games consume input from the virtual device as if it were a wired controller.

## Components
- `service/`: long-running bridge process and Bluetooth HID I/O.
- `core/`: parsing and packet translation logic.
- `driver/`: KMDF/VHF virtual HID skeleton.
- `ipc/`: named pipe IPC for UI and diagnostics.
- `ui/`: WinUI 3 skeleton with status, battery, and raw report view.
- `shared/`: cross-module constants and common structures.

## UI IPC
- UI sends `UiRequestStatus` and `UiSetLiveStream`.
- Service sends `ServiceStatus`, `ServiceRawInput`, and `ServiceInfo`.
- Transport is duplex named pipe `\\.\pipe\DualSenseLink`.

## UI Runtime Note
- `dsl_ui` is now a buildable desktop window target in CMake (Win32 host) that uses the same IPC client.
- Existing WinUI 3 XAML files are preserved as a parallel skeleton for the next Windows App SDK integration step.

## Reverse Path (Output Reports)
1. Game sends output report (rumble/lightbar) to virtual device.
2. Driver/service transport receives output report.
3. `ControllerProxy` maps USB output format back to Bluetooth format.
4. `BluetoothManager::WriteOutputReport` forwards packet to physical controller.

## Protocol Notes
- Bluetooth input reports use `ReportID=0x31`, total `78` bytes, with CRC32 in the last 4 bytes.
- Bluetooth output reports use `ReportID=0x31`, include `seq_tag` + `tag=0x10`, and require CRC32.
- USB input reports use `ReportID=0x01` and `64` bytes total.
- USB output reports use `ReportID=0x02` and `63` bytes total.

## Notes
- Driver code includes stub fallback when WDK headers are not installed.
- Real production descriptor and report mapping must be validated against full DualSense HID spec.
