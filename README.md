# DualSense Link

DualSense Link is a Windows 10/11 HID bridge:

`Bluetooth DualSense -> service -> virtual wired DualSense -> games`

Stage 1 target is HID-first passthrough (input/output HID path), without controller USB audio.

## Repository Layout

- `core/` - DualSense report parsing, CRC, BT<->USB translation.
- `service/` - bridge runtime (Bluetooth I/O, driver bridge, HidHide integration, IPC).
- `driver/` - KMDF + VHF virtual HID driver skeleton.
- `ipc/` - named pipe protocol and server.
- `ui/` - WinUI 3 source (`App.xaml`, `MainWindow.xaml`, dashboard) and `ui/winui3/DualSenseLinkUI.vcxproj`.
- `shared/` - shared contracts (`driver_protocol.h`, common IDs/constants).
- `scripts/` - build/install/run flow.

## Service Modes

- `--background` - for UI-driven flow.
- `--console` - for diagnostics/manual commands.
- `--allow-fallback` - allows console virtual HID fallback when driver bridge is unavailable.

Default mode is `--background`.

## Driver Bridge Contract

Service and driver communicate via `shared/driver_protocol.h`:

- `IOCTL_DSL_GET_VERSION`
- `IOCTL_DSL_SUBMIT_INPUT_REPORT`
- `IOCTL_DSL_GET_OUTPUT_REPORT`
- `IOCTL_DSL_PUSH_OUTPUT_REPORT` (debug/test injection)

## UI IPC Contract

UI and service communicate via `\\.\pipe\DualSenseLink` (`ipc/commands.h`):

- status stream (`ServiceStatus`)
- raw/input info stream (`ServiceRawInput`, `ServiceInfo`)
- explicit bridge mode state (`ServiceBridgeStatus`: `driver-bridge` or `fallback`)
- HidHide status/toggle commands
- shutdown command

## Build and Run (PowerShell)

1. Preflight check:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\preflight.ps1
```

2. Build backend (CMake):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_backend.ps1 -Config Debug
```

3. Build WinUI 3 app (`.vcxproj`):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_ui.ps1 -Config Debug
```

If packages are already restored locally:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_ui.ps1 -Config Debug -SkipRestore
```

4. Run app:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_app.ps1 -Config Debug
```

Compatibility wrapper:

```powershell
.\run_ui.ps1
```

## Driver Install (Test-Signing)

Driver install script requires admin rights, WDK, and test-signing mode:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_driver.ps1
```

If test-signing is disabled:

```powershell
bcdedit /set testsigning on
```

Then reboot and rerun install.

## Current Stage Notes

- HID path is the focus: BT input to virtual wired HID, and output reports back to BT.
- WinUI 3 is the primary UI path.
- Win32 UI is kept only as legacy/diagnostic code path (`dsl_ui_legacy`, disabled by default in CMake).
- Controller audio profile passthrough is out of scope for Stage 1.
