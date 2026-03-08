#include "service/console_virtual_hid_client.h"

#include <utility>

namespace dsl::service {

bool ConsoleVirtualHidClient::SendInputReport(const std::span<const std::uint8_t> report) {
    std::scoped_lock lock(mutex_);
    last_size_ = report.size();
    ++report_count_;
    return true;
}

void ConsoleVirtualHidClient::SetOutputReportHandler(OutputReportHandler handler) {
    std::scoped_lock lock(mutex_);
    output_handler_ = std::move(handler);
}

void ConsoleVirtualHidClient::EmitTestRumbleCommand() {
    OutputReportHandler handler_copy;
    {
        std::scoped_lock lock(mutex_);
        handler_copy = output_handler_;
    }
    if(handler_copy) {
        handler_copy({0x02, 0xFF, 0x40, 0x40});
    }
}

std::uint64_t ConsoleVirtualHidClient::ReportCount() const {
    std::scoped_lock lock(mutex_);
    return report_count_;
}

std::size_t ConsoleVirtualHidClient::LastSize() const {
    std::scoped_lock lock(mutex_);
    return last_size_;
}

} // namespace dsl::service

