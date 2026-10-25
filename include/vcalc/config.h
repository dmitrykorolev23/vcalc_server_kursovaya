#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace vcalc {

class Config {
public:
    
    bool parseArgs(int argc, char* argv[]);

    
    void printHelp(std::ostream& out) const;

    const std::string& databaseFileName() const noexcept { return databaseFileName_; }
    const std::string& logFileName() const noexcept { return logFileName_; }
    std::uint16_t port() const noexcept { return port_; }

private:
    std::string databaseFileName_{"/etc/vcalc.conf"};
    std::string logFileName_{"/var/log/vcalc.log"};
    std::uint16_t port_{33333};
};

}
