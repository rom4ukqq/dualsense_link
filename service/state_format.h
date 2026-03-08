#pragma once

#include "core/dualsense_parser.h"
#include "ipc/commands.h"
#include "shared/common_types.h"

#include <string>

namespace dsl::service {

std::string FormatStateLine(const core::ParsedDualSenseState& state);
ipc::StatusPayload BuildStatusPayload(
    const core::ParsedDualSenseState& state,
    shared::ConnectionState connection);

} // namespace dsl::service

