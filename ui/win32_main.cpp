#include "PipeClient.h"

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
constexpr int kMaxRawChars = 12000;

constexpr int kIdRefresh = 1001;
constexpr int kIdLiveOn = 1002;
constexpr int kIdLiveOff = 1003;

struct UiState {
    dualsense_link::ui::PipeClient pipe_client{};
    HWND connection_text{nullptr};
    HWND battery_text{nullptr};
    HWND dpad_text{nullptr};
    HWND buttons_text{nullptr};
    HWND sticks_text{nullptr};
    HWND raw_input{nullptr};
    HWND refresh_button{nullptr};
    HWND live_on_button{nullptr};
    HWND live_off_button{nullptr};
};

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
    SetText(state.connection_text, ConnectionText(payload.connection_state));
    SetText(state.battery_text, L"Battery: " + std::to_wstring(payload.battery_percent) + L"%");
    SetText(state.dpad_text, DpadText(payload.dpad));
    SetText(state.buttons_text, ButtonsText(payload.buttons_mask));

    const std::wstring sticks = L"Sticks: LX=" + std::to_wstring(payload.left_stick_x) +
                                L" LY=" + std::to_wstring(payload.left_stick_y) +
                                L" RX=" + std::to_wstring(payload.right_stick_x) +
                                L" RY=" + std::to_wstring(payload.right_stick_y);
    SetText(state.sticks_text, sticks);
}

void ResizeLayout(UiState& state, const int width, const int height) {
    constexpr int margin = 12;
    constexpr int text_height = 22;
    constexpr int button_height = 28;
    constexpr int button_width = 88;
    constexpr int gap = 8;

    const int full = width - margin * 2;
    MoveWindow(state.connection_text, margin, margin, full, text_height, TRUE);
    MoveWindow(state.battery_text, margin, margin + 24, full, text_height, TRUE);
    MoveWindow(state.dpad_text, margin, margin + 48, full, text_height, TRUE);
    MoveWindow(state.buttons_text, margin, margin + 72, full, text_height, TRUE);
    MoveWindow(state.sticks_text, margin, margin + 96, full, text_height, TRUE);

    const int buttons_y = margin + 124;
    MoveWindow(state.refresh_button, margin, buttons_y, button_width, button_height, TRUE);
    MoveWindow(state.live_on_button, margin + button_width + gap, buttons_y, button_width, button_height, TRUE);
    MoveWindow(
        state.live_off_button,
        margin + (button_width + gap) * 2,
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
    state.connection_text = CreateChild(hwnd, L"STATIC", L"Connection: Disconnected", SS_LEFT);
    state.battery_text = CreateChild(hwnd, L"STATIC", L"Battery: --%", SS_LEFT);
    state.dpad_text = CreateChild(hwnd, L"STATIC", L"D-Pad: Centered", SS_LEFT);
    state.buttons_text = CreateChild(hwnd, L"STATIC", L"Buttons: none", SS_LEFT);
    state.sticks_text = CreateChild(hwnd, L"STATIC", L"Sticks: LX=0 LY=0 RX=0 RY=0", SS_LEFT);

    state.refresh_button = CreateChild(hwnd, L"BUTTON", L"Refresh", BS_PUSHBUTTON, kIdRefresh);
    state.live_on_button = CreateChild(hwnd, L"BUTTON", L"Live ON", BS_PUSHBUTTON, kIdLiveOn);
    state.live_off_button = CreateChild(hwnd, L"BUTTON", L"Live OFF", BS_PUSHBUTTON, kIdLiveOff);

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

    state.pipe_client.SetOnInfo([hwnd](std::string_view line) {
        auto* copy = new std::string("[info] ");
        copy->append(line.data(), line.size());
        PostMessageW(hwnd, WM_DSL_INFO, 0, reinterpret_cast<LPARAM>(copy));
    });
}

LRESULT HandleCommand(UiState& state, const int control_id) {
    if(control_id == kIdRefresh) {
        state.pipe_client.RequestStatus();
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
    return 1;
}

bool HandleWindowMessage(UiState& state, HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param, LRESULT& result) {
    if(message == WM_CREATE) {
        CreateControls(state, hwnd);
        SetupPipeCallbacks(state, hwnd);
        state.pipe_client.Start();
        state.pipe_client.RequestStatus();
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
    if(message == WM_DSL_RAW || message == WM_DSL_INFO) {
        auto* text = reinterpret_cast<std::string*>(l_param);
        if(text != nullptr) {
            AppendRawLine(state.raw_input, Utf8ToWide(*text));
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
