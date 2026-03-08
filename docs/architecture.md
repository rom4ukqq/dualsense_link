# DualSense Link Architecture (Stage 1: HID-first)

## Goal

Expose a Bluetooth-connected DualSense as a wired USB-style DualSense device for Windows games.

Audio over the controller USB profile is intentionally out of scope for this stage.

## Data Flow

1. DualSense connected via Bluetooth HID.
2. `dsl_service` opens the HID interface and reads input reports continuously.
3. `core/dualsense_parser` validates/parses reports (including BT CRC where required).
4. `core/packet_translation` maps BT format to USB report format.
5. Service sends USB-style input report to KMDF/VHF driver bridge.
6. Windows and games see a wired-style controller device.

Reverse path:

1. Game sends output report (rumble/lightbar/triggers) to virtual device.
2. Driver exposes output queue via IOCTL.
3. Service reads output, translates USB -> BT, and writes to Bluetooth controller.

## Components

- `core/`: parsing and report translation logic.
- `service/`: runtime orchestration, bridge mode management, HidHide integration.
- `driver/`: KMDF + VHF virtual HID skeleton.
- `ipc/`: named-pipe command/status stream for UI.
- `ui/`: WinUI 3 app (`ui/winui3/DualSenseLinkUI.vcxproj`) with status dashboard.
- `shared/`: cross-layer constants and contracts.

## Service Modes

- `--background` (default): UI-driven mode; no interactive console.
- `--console`: diagnostic mode with console commands.
- `--allow-fallback`: allows console virtual HID fallback if driver bridge is unavailable.

In background mode, driver bridge is required by default.

## Driver Contract

Single shared contract file: `shared/driver_protocol.h`.

IOCTLs:

- `IOCTL_DSL_GET_VERSION`
- `IOCTL_DSL_SUBMIT_INPUT_REPORT`
- `IOCTL_DSL_GET_OUTPUT_REPORT`
- `IOCTL_DSL_PUSH_OUTPUT_REPORT` (debug/test injection path)

The driver keeps output reports in a bounded, lock-protected queue to make IOCTL behavior predictable.

## UI Contract

IPC protocol is defined in `ipc/commands.h`.

Important service->UI messages:

- `ServiceStatus`
- `ServiceRawInput`
- `ServiceInfo`
- `ServiceHidHideStatus`
- `ServiceBridgeStatus` (`driver-bridge`, `fallback`, `unknown`)

This makes bridge mode explicit instead of inferring it from free-form log lines.

## HidHide Integration

Service handles HidHide operations (install/running checks, app whitelist, hide/unhide + cloak toggles).
UI only sends toggle/status commands and displays resulting state.

## Build/Run Separation

- Backend (`service/core/ipc/driver` skeleton): CMake.
- WinUI 3 frontend: standalone Visual Studio project (`.vcxproj`).
- PowerShell scripts provide unified flow:
  - `preflight.ps1`
  - `build_backend.ps1`
  - `build_ui.ps1`
  - `install_driver.ps1`
  - `run_app.ps1`
