#include "core/crc32.h"

namespace dsl::core {

std::uint32_t Crc32(const std::uint32_t initial, const std::span<const std::uint8_t> data) {
    std::uint32_t crc = initial;
    for(const auto byte : data) {
        crc ^= byte;
        for(int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = (crc & 1u) ? 0xEDB88320u : 0u;
            crc = (crc >> 1u) ^ mask;
        }
    }
    return crc;
}

std::uint32_t ComputeSeededDualSenseCrc(
    const std::uint8_t seed,
    const std::span<const std::uint8_t> data) {
    const std::span<const std::uint8_t> seed_span(&seed, 1);
    const std::uint32_t after_seed = Crc32(0xFFFFFFFFu, seed_span);
    return ~Crc32(after_seed, data);
}

} // namespace dsl::core
