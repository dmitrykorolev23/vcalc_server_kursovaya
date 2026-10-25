#include "vcalc/user_database.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace vcalc {
namespace {

std::string trim(std::string value) {
    const auto isSpace = [](unsigned char character) { return std::isspace(character) != 0; };
    std::size_t first = 0;
    while (first < value.size() && isSpace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && isSpace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return value.substr(first, last - first);
}

}

UserDatabase::UserDatabase(std::string fileName) : fileName_(std::move(fileName)) {}

void UserDatabase::load() {
    std::ifstream input(fileName_);
    if (!input) {
        throw std::runtime_error("Не удалось открыть базу клиентов: " + fileName_);
    }

    std::unordered_map<std::string, std::string> loaded;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::string record = trim(line);
        if (record.empty() || record.front() == '#') {
            continue;
        }

        const std::size_t separator = record.find(':');
        if (separator == std::string::npos) {
            throw std::runtime_error("Некорректная запись в базе клиентов, строка "
                                     + std::to_string(lineNumber));
        }

        const std::string login = trim(record.substr(0, separator));
        const std::string password = record.substr(separator + 1);
        if (login.empty() || password.empty()) {
            throw std::runtime_error("Пустой идентификатор или пароль в базе, строка "
                                     + std::to_string(lineNumber));
        }
        if (loaded.find(login) != loaded.end()) {
            throw std::runtime_error("Повторяющийся идентификатор в базе: " + login);
        }
        loaded.emplace(login, password);
    }

    if (!input.eof()) {
        throw std::runtime_error("Ошибка чтения базы клиентов: " + fileName_);
    }
    if (loaded.empty()) {
        throw std::runtime_error("База клиентов не содержит ни одной записи: " + fileName_);
    }

    users_ = std::move(loaded);
}

bool UserDatabase::find(const std::string& login) const noexcept {
    return users_.find(login) != users_.end();
}

const std::string& UserDatabase::getPassword(const std::string& login) const {
    const auto user = users_.find(login);
    if (user == users_.end()) {
        throw std::out_of_range("Пользователь не найден: " + login);
    }
    return user->second;
}

}
