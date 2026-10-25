#include "vcalc/vector_calculator.h"

#include <limits>

namespace vcalc {

std::int64_t VectorCalculator::sumToWide(const std::int16_t* values,
                                         std::size_t size) noexcept {
    std::int64_t total = 0;
    for (std::size_t index = 0; index < size; ++index) {
        total += values[index];
    }
    return total;
}

std::int16_t VectorCalculator::saturate(std::int64_t total) noexcept {
    if (total > std::numeric_limits<std::int16_t>::max()) {
        return std::numeric_limits<std::int16_t>::max();
    }
    if (total < std::numeric_limits<std::int16_t>::min()) {
        return std::numeric_limits<std::int16_t>::min();
    }
    return static_cast<std::int16_t>(total);
}

std::int16_t VectorCalculator::sum(const std::int16_t* values,
                                   std::size_t size) noexcept {
    return saturate(sumToWide(values, size));
}

}
