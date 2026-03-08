#include "PipeClient.h"
#include "service_process.h"
#include "ui_formatting.h"

#include <Windows.h>
#include <windowsx.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace {

constexpr wchar_t kWindowClassName[] = L"DualSenseLinkUiWindow";
constexpr UINT WM_DSL_STATUS = WM_APP + 1;
constexpr UINT WM_DSL_RAW = WM_APP + 2;
constexpr UINT WM_DSL_INFO = WM_APP + 3;
constexpr UINT WM_DSL_HIDHIDE = WM_APP + 4;
constexpr UINT WM_DSL_BRIDGE = WM_APP + 5;
constexpr int kMaxRawChars = 12000;

constexpr int kIdRefresh = 1001;
constexpr int kIdLiveOn = 1002;
constexpr int kIdLiveOff = 1003;
constexpr int kIdHidHideOn = 1004;
constexpr int kIdHidHideOff = 1005;

struct UiState {
    dualsense_link::ui::PipeClient pipe_client{};
    dualsense_link::ui::ServiceProcess service_process{};
    HWND service_text{nullptr};
    HWND connection_text{nullptr};
    HWND battery_text{nullptr};
    HWND dpad_text{nullptr};
    HWND buttons_text{nullptr};
    HWND sticks_text{nullptr};
    HWND hidhide_text{nullptr};
    HWND raw_input{nullptr};
    HWND refresh_button{nullptr};
    HWND live_on_button{nullptr};
    HWND live_off_button{nullptr};
    HWND hidhide_on_button{nullptr};
    HWND hidhide_off_button{nullptr};
};

void SetText(HWND control, const std::wstring& text) {
    if(control != nullptr) {
        SetWindowTextW(control, text.c_str());
    }
}

void TrimRawText(HWND edit) {
    const int len = GetWindowTextLengthW(edit);
    if(len <= kMaxRawChars) {
        return;
    }

    std::wstring text(len + 1, L'\0');
    GetWindowTextW(edit, text.data(), len + 1);
    text.resize(len);
    text = text.substr(len - (kMaxRawChars / 2));
    SetWindowTextW(edit, text.c_str());
}

void AppendRawLine(HWND edit, const std::wstring& line) {
    if(edit == nullptr) {
        return;
    }

    const auto len = GetWindowTextLengthW(edit);
    SendMessageW(edit, EM_SETSEL, len, len);

    std::wstring chunk = line;
    chunk += L"\r\n";
    SendMessageW(edit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(chunk.c_str()));
    TrimRawText(edit);
}

void UpdateStatus(UiState& state, const dsl::ipc::StatusPayload& payload) {
    SetText(state.connection_text, dualsense_link::ui::ConnectionText(payload.connection_state));
    SetText(state.battery_text, L"Battery: " + std::to_wstring(payload.battery_percent) + L"%");
    SetText(state.dpad_text, dualsense_link::ui::DpadText(payload.dpad));
    SetText(state.buttons_text, dualsense_link::ui::ButtonsText(payload.buttons_mask));

    const std::wstring sticks = L"Sticks: LX=" + std::to_wstring(payload.left_stick_x) +
                                L" LY=" + std::to_wstring(payload.left_stick_y) +
                                L" RX=" + std::to_wstring(payload.right_stick_x) +
                                L" RY=" + std::to_wstring(payload.right_stick_y);
    SetText(state.sticks_text, sticks);
}

void UpdateHidHideStatus(UiState& state, const dsl::ipc::HidHideStatusPayload& payload) {
    SetText(state.hidhide_text, dualsense_link::ui::HidHideLabelText(payload));
    AppendRawLine(
        state.raw_input,
        dualsense_link::ui::Utf8ToWide(dualsense_link::ui::HidHideStatusLine(payload)));
}

void UpdateBridgeInfo(UiState& state, const std::string& line) {
    if(line.find("virtual-hid:driver-bridge") != std::string::npos) {
        SetText(state.service_text, L"Service: connected | Bridge: driver");
        return;
    }
    if(line.find("virtual-hid:console-fallback") != std::string::npos) {
        SetText(state.service_text, L"Service: connected | Bridge: fallback");
    }
}

void UpdateBridgeStatus(UiState& state, const dsl::ipc::BridgeStatusPayload& payload) {
    if(payload.mode == dsl::ipc::BridgeMode::DriverBridge) {
        SetText(state.service_text, L"Service: connected | Bridge: driver");
        return;
    }
    if(payload.mode == dsl::ipc::BridgeMode::ConsoleFallback) {
        SetText(state.service_text, L"Service: connected | Bridge: fallback");
        return;
    }
    SetText(state.service_text, L"Service: connected | Bridge: unknown");
}

void ResizeLayout(UiState& state, const int width, const int height) {
    constexpr int margin = 12;
    constexpr int text_height = 22;
    constexpr int button_height = 28;
    constexpr int button_width = 88;
    constexpr int gap = 8;

    const int full = width - margin * 2;
    MoveWindow(state.service_text, margin, margin, full, text_height, TRUE);
    MoveWindow(state.connection_text, margin, margin + 24, full, text_height, TRUE);
    MoveWindow(state.battery_text, margin, margin + 48, full, text_height, TRUE);
    MoveWindow(state.dpad_text, margin, margin + 72, full, text_height, TRUE);
    MoveWindow(state.buttons_text, margin, margin + 96, full, text_height, TRUE);
    MoveWindow(state.sticks_text, margin, margin + 120, full, text_height, TRUE);
    MoveWindow(state.hidhide_text, margin, margin + 144, full, text_height, TRUE);

    const int buttons_y = margin + 172;
    MoveWindow(state.refresh_button, margin, buttons_y, button_width, button_height, TRUE);
    MoveWindow(state.live_on_button, margin + button_width + gap, buttons_y, button_width, button_height, TRUE);
    MoveWindow(
        state.live_off_button,
        margin + (button_width + gap) * 2,
        buttons_y,
        button_width,
        button_height,
        TRUE);
    MoveWindow(
        state.hidhide_on_button,
        margin + (button_width + gap) * 3,
        buttons_y,
        button_width,
        button_height,
        TRUE);
    MoveWindow(
        state.hidhide_off_button,
        margin + (button_width + gap) * 4,
        buttons_y,
        button_width,
        button_height,
        TRUE);

    const int raw_y = buttons_y + button_height + margin;
    const int raw_h = height - raw_y - margin;
    MoveWindow(state.raw_input, margin, raw_y, full, raw_h > 60 ? raw_h : 60, TRUE);
}

HWND CreateChild(
    HWND parent,
    const wchar_t* klass,
    const wchar_t* text,
    const DWORD style,
    const int id = 0,
    const DWORD ex_style = 0) {
    return CreateWindowExW(
        ex_style,
        klass,
        text,
        WS_CHILD | WS_VISIBLE | style,
        0,
        0,
        0,
        0,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        nullptr,
        nullptr);
}

void CreateControls(UiState& state, HWND hwnd) {
    state.service_text = CreateChild(hwnd, L"STATIC", L"Service: checking...", SS_LEFT);
    state.connection_text = CreateChild(hwnd, L"STATIC", L"Connection: Disconnected", SS_LEFT);
    state.battery_text = CreateChild(hwnd, L"STATIC", L"Battery: --%", SS_LEFT);
    state.dpad_text = CreateChild(hwnd, L"STATIC", L"D-Pad: Centered", SS_LEFT);
    state.buttons_text = CreateChild(hwnd, L"STATIC", L"Buttons: none", SS_LEFT);
    state.sticks_text = CreateChild(hwnd, L"STATIC", L"Sticks: LX=0 LY=0 RX=0 RY=0", SS_LEFT);
    state.hidhide_text = CreateChild(
        hwnd,
        L"STATIC",
        L"HidHide: installed=no, service=stopped, integration=off",
        SS_LEFT);

    state.refresh_button = CreateChild(hwnd, L"BUTTON", L"Refresh", BS_PUSHBUTTON, kIdRefresh);
    state.live_on_button = CreateChild(hwnd, L"BUTTON", L"Live ON", BS_PUSHBUTTON, kIdLiveOn);
    state.live_off_button = CreateChild(hwnd, L"BUTTON", L"Live OFF", BS_PUSHBUTTON, kIdLiveOff);
    state.hidhide_on_button = CreateChild(hwnd, L"BUTTON", L"HidHide ON", BS_PUSHBUTTON, kIdHidHideOn);
    state.hidhide_off_button = CreateChild(hwnd, L"BUTTON", L"HidHide OFF", BS_PUSHBUTTON, kIdHidHideOff);

    state.raw_input = CreateChild(
        hwnd,
        L"EDIT",
        L"",
        WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0,
        WS_EX_CLIENTEDGE);
}

void SetupPipeCallbacks(UiState& state, HWND hwnd) {
    state.pipe_client.SetOnStatus([hwnd](const dsl::ipc::StatusPayload& payload) {
        auto* copy = new dsl::ipc::StatusPayload(payload);
        PostMessageW(hwnd, WM_DSL_STATUS, 0, reinterpret_cast<LPARAM>(copy));
    });

    state.pipe_client.SetOnRawInput([hwnd](std::string_view line) {
        auto* copy = new std::string(line);
        PostMessageW(hwnd, WM_DSL_RAW, 0, reinterpret_cast<LPARAM>(copy));
    });

    state.pipe_client.SetOnHidHideStatus([hwnd](const dsl::ipc::HidHideStatusPayload& payload) {
        auto* copy = new dsl::ipc::HidHideStatusPayload(payload);
        PostMessageW(hwnd, WM_DSL_HIDHIDE, 0, reinterpret_cast<LPARAM>(copy));
    });

    state.pipe_client.SetOnBridgeStatus([hwnd](const dsl::ipc::BridgeStatusPayload& payload) {
        auto* copy = new dsl::ipc::BridgeStatusPayload(payload);
        PostMessageW(hwnd, WM_DSL_BRIDGE, 0, reinterpret_cast<LPARAM>(copy));
    });

    state.pipe_client.SetOnInfo([hwnd](std::string_view line) {
        auto* copy = new std::string("[info] ");
        copy->append(line.data(), line.size());
        PostMessageW(hwnd, WM_DSL_INFO, 0, reinterpret_cast<LPARAM>(copy));
    });
}

LRESULT HandleCommand(UiState& state, const int control_id) {
    if(control_id == kIdRefresh) {
        state.service_process.EnsureRunning();
        SetText(state.service_text, state.service_process.StatusText());
        state.pipe_client.RequestStatus();
        state.pipe_client.RequestHidHideStatus();
        return 0;
    }
    if(control_id == kIdLiveOn) {
        state.pipe_client.SetLiveStreamEnabled(true);
        return 0;
    }
    if(control_id == kIdLiveOff) {
        state.pipe_client.SetLiveStreamEnabled(false);
        return 0;
    }
    if(control_id == kIdHidHideOn) {
        state.pipe_client.SetHidHideEnabled(true);
        state.pipe_client.RequestHidHideStatus();
        return 0;
    }
    if(control_id == kIdHidHideOff) {
        state.pipe_client.SetHidHideEnabled(false);
        state.pipe_client.RequestHidHideStatus();
        return 0;
    }
    return 1;
}

bool HandleWindowMessage(UiState& state, HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param, LRESULT& result) {
    if(message == WM_CREATE) {
        CreateControls(state, hwnd);
        SetupPipeCallbacks(state, hwnd);
        state.service_process.EnsureRunning();
        SetText(state.service_text, state.service_process.StatusText());
        state.pipe_client.Start();
        state.pipe_client.RequestStatus();
        state.pipe_client.RequestHidHideStatus();
        result = 0;
        return true;
    }
    if(message == WM_SIZE) {
        ResizeLayout(state, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
        result = 0;
        return true;
    }
    if(message == WM_COMMAND && HandleCommand(state, LOWORD(w_param)) == 0) {
        result = 0;
        return true;
    }
    if(message == WM_DESTROY) {
        state.service_process.ShutdownIfLaunched(
            [&state]() { return state.pipe_client.RequestServiceShutdown(); });
        state.pipe_client.Stop();
        PostQuitMessage(0);
        result = 0;
        return true;
    }
    return false;
}

bool HandlePipeMessage(UiState& state, UINT message, LPARAM l_param, LRESULT& result) {
    if(message == WM_DSL_STATUS) {
        auto* payload = reinterpret_cast<dsl::ipc::StatusPayload*>(l_param);
        if(payload != nullptr) {
            UpdateStatus(state, *payload);
            delete payload;
        }
        result = 0;
        return true;
    }
    if(message == WM_DSL_HIDHIDE) {
        auto* payload = reinterpret_cast<dsl::ipc::HidHideStatusPayload*>(l_param);
        if(payload != nullptr) {
            UpdateHidHideStatus(state, *payload);
            delete payload;
        }
        result = 0;
        return true;
    }
    if(message == WM_DSL_BRIDGE) {
        auto* payload = reinterpret_cast<dsl::ipc::BridgeStatusPayload*>(l_param);
        if(payload != nullptr) {
            UpdateBridgeStatus(state, *payload);
            delete payload;
        }
        result = 0;
        return true;
    }
    if(message == WM_DSL_RAW || message == WM_DSL_INFO) {
        auto* text = reinterpret_cast<std::string*>(l_param);
        if(text != nullptr) {
            if(message == WM_DSL_INFO) {
                UpdateBridgeInfo(state, *text);
            }
            AppendRawLine(state.raw_input, dualsense_link::ui::Utf8ToWide(*text));
            delete text;
        }
        result = 0;
        return true;
    }
    return false;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param) {
    if(message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }

    auto* state = reinterpret_cast<UiState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if(state == nullptr) {
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }

    LRESULT result = 0;
    if(HandleWindowMessage(*state, hwnd, message, w_param, l_param, result)) {
        return result;
    }
    if(HandlePipeMessage(*state, message, l_param, result)) {
        return result;
    }
    return DefWindowProcW(hwnd, message, w_param, l_param);
}

ATOM RegisterMainClass(HINSTANCE instance) {
    WNDCLASSW klass{};
    klass.lpfnWndProc = WindowProc;
    klass.hInstance = instance;
    klass.lpszClassName = kWindowClassName;
    klass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    klass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    return RegisterClassW(&klass);
}

int RunUi(HINSTANCE instance, const int show) {
    RegisterMainClass(instance);

    UiState state{};
    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        L"DualSense Link UI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        620,
        nullptr,
        nullptr,
        instance,
        &state);
    if(hwnd == nullptr) {
        return 1;
    }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while(GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    return RunUi(instance, show_command);
}
