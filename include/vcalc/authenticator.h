#pragma once

#include <cstdint>
#include <string>

namespace vcalc {

class Authenticator {
public:
    
    std::uint64_t generateSalt() const;

    
    static std::string saltToHex(std::uint64_t salt);

    
    static std::string computeHash(const std::string& salt16,
                                   const std::string& password);

    
    static bool verify(const std::string& received,
                       const std::string& expected) noexcept;
};

}
