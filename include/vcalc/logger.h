#pragma once

#include <string>

namespace vcalc {

class Logger {
public:
    explicit Logger(std::string fileName);

    
    void logError(const std::string& message, bool critical,
                  const std::string& parameters = "") const;

    
    static std::string timestamp();

    const std::string& fileName() const noexcept { return fileName_; }

private:
    std::string fileName_;
};

}
