#pragma once

#include "service/controller_proxy.h"

#include <cstdint>
#include <mutex>

namespace dsl::service {

class ConsoleVirtualHidClient final : public IVirtualHidClient {
public:
    bool SendInputReport(std::span<const std::uint8_t> report) override;
    void SetOutputReportHandler(OutputReportHandler handler) override;

    void EmitTestRumbleCommand();
    [[nodiscard]] std::uint64_t ReportCount() const;
    [[nodiscard]] std::size_t LastSize() const;

private:
    mutable std::mutex mutex_;
    OutputReportHandler output_handler_;
    std::uint64_t report_count_{0};
    std::size_t last_size_{0};
};

} // namespace dsl::service

