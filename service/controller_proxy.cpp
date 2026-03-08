#include "service/controller_proxy.h"

#include "core/packet_translation.h"

namespace dsl::service {

ControllerProxy::ControllerProxy(
    std::shared_ptr<core::DualSenseParser> parser,
    std::shared_ptr<IVirtualHidClient> virtual_hid)
    : parser_(std::move(parser)), virtual_hid_(std::move(virtual_hid)) {
}

ControllerProxy::~ControllerProxy() {
    Stop();
}

void ControllerProxy::Start(BluetoothManager& manager) {
    manager_ = &manager;

    manager_->SetOnInputReportReceived([this](const std::vector<std::uint8_t>& packet) {
        HandleBluetoothInputReport(packet);
    });

    virtual_hid_->SetOutputReportHandler([this](const std::vector<std::uint8_t>& report) {
        HandleVirtualOutputReport(report);
    });
}

void ControllerProxy::Stop() {
    if(manager_ != nullptr) {
        manager_->SetOnInputReportReceived({});
        manager_ = nullptr;
    }
    virtual_hid_->SetOutputReportHandler({});
}

void ControllerProxy::SetOnStateUpdated(StateCallback callback) {
    std::scoped_lock lock(state_mutex_);
    on_state_updated_ = std::move(callback);
}

core::ParsedDualSenseState ControllerProxy::GetLastState() const {
    std::scoped_lock lock(state_mutex_);
    return last_state_;
}

void ControllerProxy::HandleBluetoothInputReport(const std::vector<std::uint8_t>& raw_packet) {
    core::DualSenseInputReport bt_report{};
    if(!parser_->TryParseBluetoothInput(raw_packet, bt_report)) {
        return;
    }

    const auto parsed_state = parser_->ParseState(bt_report);
    const auto usb_input = core::PacketTranslation::ToUsbInputReport(bt_report);
    virtual_hid_->SendInputReport(usb_input.bytes);

    StateCallback callback;
    {
        std::scoped_lock lock(state_mutex_);
        last_state_ = parsed_state;
        callback = on_state_updated_;
    }
    if(callback) {
        callback(parsed_state);
    }
}

void ControllerProxy::HandleVirtualOutputReport(const std::vector<std::uint8_t>& usb_output_report) {
    if(manager_ == nullptr) {
        return;
    }
    const auto bt_output = core::PacketTranslation::ToBluetoothOutputReport(usb_output_report);
    if(!bt_output.empty()) {
        manager_->WriteOutputReport(bt_output);
    }
}

} // namespace dsl::service
