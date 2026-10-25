#pragma once

#include <cstddef>
#include <cstdint>

namespace vcalc {

class VectorCalculator {
public:
    
    static std::int16_t sum(const std::int16_t* values, std::size_t size) noexcept;

    
    static std::int64_t sumToWide(const std::int16_t* values, std::size_t size) noexcept;

    
    static std::int16_t saturate(std::int64_t total) noexcept;
};

}
