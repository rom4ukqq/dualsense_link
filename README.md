# DualSense Link

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.md)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue)](https://www.microsoft.com)

**DualSense Link** enables a Bluetooth-connected Sony DualSense Wireless Controller to appear as a wired USB DualSense controller on Windows, supporting full features: adaptive triggers, haptics, gyro, touchpad, LED, and battery reporting.

---

## 🔹 Features

- Full DualSense passthrough via Bluetooth  
- Adaptive triggers & HD haptics support  
- Gyroscope & accelerometer  
- Touchpad and button input  
- LED / lightbar control  
- Battery level monitoring  
- WinUI 3 GUI for live controller status and configuration  

---

## 📦 Project Structure

```
dualsense-link/
│
├─ driver/       # KMDF driver using Virtual HID Framework (VHF)
├─ core/         # Packet translation & DualSense report parsing
├─ service/      # Background service handling Bluetooth & driver communication
├─ ipc/          # Named Pipes for UI ↔ service communication
├─ ui/           # WinUI 3 application
├─ shared/       # Shared headers & structures
└─ docs/         # Documentation and protocol references
```

---

## ⚙️ How it works

1. Connect a Sony DualSense controller via Bluetooth.  
2. The **backend service** reads Bluetooth HID reports from the controller.  
3. Reports are **translated** from Bluetooth format to USB HID format.  
4. The **Virtual HID driver (VHF)** exposes a virtual wired DualSense device to Windows.  
5. Games see the virtual device and support **full DualSense features**.  
6. The **WinUI 3 GUI** communicates with the service to display controller status and manage settings.

```
Controller (Bluetooth)
        ↓
Backend service
        ↓
Virtual HID Driver (VHF)
        ↓
Windows sees wired DualSense
        ↓
Games
```

---

## 🖥 GUI Features

- Live controller visualization with pressed buttons highlighted  
- Battery level and connection status  
- Adjustable LED color  
- Trigger force and haptic intensity settings  
- Debug panel with raw HID report stream  

---

## 🚀 Getting Started

1. Clone the repository:  
```bash
git clone https://github.com/yourusername/dualsense-link.git
```

2. Open the solution in Visual Studio (requires WDK for driver).  

3. Build `driver`, `service`, and `ui`.  

4. Connect your DualSense controller via Bluetooth.  

5. Launch the UI to see live status and configure settings.

---

## 📄 License

DualSense Link is licensed under the MIT License. See [LICENSE](LICENSE.md) for details.

