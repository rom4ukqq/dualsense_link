#include "ui_formatting.h"

#include <Windows.h>

namespace dualsense_link::ui {

std::wstring Utf8ToWide(const std::string_view text) {
    if(text.empty()) {
        return {};
    }

    const int needed = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0);
    if(needed <= 0) {
        return std::wstring(text.begin(), text.end());
    }

    std::wstring wide(needed, L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        static_cast<int>(text.size()),
        wide.data(),
        needed);
    return wide;
}

std::wstring ConnectionText(const std::uint8_t state) {
    if(state == 2) {
        return L"Connection: Connected";
    }
    if(state == 1) {
        return L"Connection: Connecting";
    }
    return L"Connection: Disconnected";
}

std::wstring DpadText(const std::uint8_t value) {
    switch(value) {
        case 0: return L"D-Pad: Up";
        case 1: return L"D-Pad: UpRight";
        case 2: return L"D-Pad: Right";
        case 3: return L"D-Pad: DownRight";
        case 4: return L"D-Pad: Down";
        case 5: return L"D-Pad: DownLeft";
        case 6: return L"D-Pad: Left";
        case 7: return L"D-Pad: UpLeft";
        default: return L"D-Pad: Centered";
    }
}

std::wstring ButtonsText(const std::uint16_t mask) {
    std::wstring text = L"Buttons:";
    if(mask & (1u << 0)) { text += L" Square"; }
    if(mask & (1u << 1)) { text += L" Cross"; }
    if(mask & (1u << 2)) { text += L" Circle"; }
    if(mask & (1u << 3)) { text += L" Triangle"; }
    if(text == L"Buttons:") {
        text += L" none";
    }
    return text;
}

std::wstring HidHideLabelText(const dsl::ipc::HidHideStatusPayload& payload) {
    const wchar_t* installed = payload.installed != 0 ? L"yes" : L"no";
    const wchar_t* running = payload.service_running != 0 ? L"running" : L"stopped";
    const wchar_t* integration = payload.integration_enabled != 0 ? L"on" : L"off";
    const wchar_t* app = payload.app_whitelisted != 0 ? L"yes" : L"no";
    const wchar_t* hidden = payload.device_hidden != 0 ? L"yes" : L"no";
    const wchar_t* op = payload.operation_ok != 0 ? L"ok" : L"n/a";
    return std::wstring(L"HidHide: installed=") + installed + L", service=" + running +
           L", integration=" + integration + L", app=" + app + L", hidden=" + hidden +
           L", op=" + op;
}

std::string HidHideStatusLine(const dsl::ipc::HidHideStatusPayload& payload) {
    const char* installed = payload.installed != 0 ? "yes" : "no";
    const char* running = payload.service_running != 0 ? "running" : "stopped";
    const char* integration = payload.integration_enabled != 0 ? "on" : "off";
    const char* app = payload.app_whitelisted != 0 ? "yes" : "no";
    const char* hidden = payload.device_hidden != 0 ? "yes" : "no";
    const char* op = payload.operation_ok != 0 ? "ok" : "n/a";
    return std::string("[hidhide] installed:") + installed + " service:" + running +
           " integration:" + integration + " app:" + app + " hidden:" + hidden + " op:" + op;
}

} // namespace dualsense_link::ui
