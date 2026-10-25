#include "vcalc/logger.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace vcalc {

Logger::Logger(std::string fileName) : fileName_(std::move(fileName)) {
    if (fileName_.empty()) {
        throw std::runtime_error("Не задано имя файла журнала");
    }
}

void Logger::logError(const std::string& message, bool critical,
                      const std::string& parameters) const {
    std::ofstream log(fileName_, std::ios::app);
    if (!log) {
        throw std::runtime_error("Невозможно открыть файл журнала: " + fileName_);
    }

    log << timestamp()
        << " | " << (critical ? "CRITICAL" : "NONCRITICAL")
        << " | message=" << message;
    if (!parameters.empty()) {
        log << " | parameters=" << parameters;
    }
    log << '\n';

    if (!log) {
        throw std::runtime_error("Ошибка записи в файл журнала: " + fileName_);
    }
}

std::string Logger::timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_r(&time, &local);

    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

}
