#pragma once

#include <cstdint>
#include <span>

namespace dsl::core {

std::uint32_t Crc32(std::uint32_t initial, std::span<const std::uint8_t> data);

std::uint32_t ComputeSeededDualSenseCrc(
    std::uint8_t seed,
    std::span<const std::uint8_t> data);

} // namespace dsl::core
