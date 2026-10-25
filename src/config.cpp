#include "vcalc/config.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <ostream>
#include <stdexcept>
#include <string>

namespace vcalc {
namespace {

std::uint16_t parsePort(const std::string& text) {
    if (text.empty()) {
        throw std::runtime_error("Пустое значение параметра порта");
    }

    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0' || value < 1 || value > 65535) {
        throw std::runtime_error("Порт должен быть целым числом от 1 до 65535");
    }
    return static_cast<std::uint16_t>(value);
}

const char* nextValue(int argc, char* argv[], int& index, const std::string& option) {
    if (index + 1 >= argc) {
        throw std::runtime_error("Для параметра " + option + " не задано значение");
    }
    ++index;
    return argv[index];
}

}

bool Config::parseArgs(int argc, char* argv[]) {
    if (argc == 1) {
        return false; // При запуске без параметров выводится справка.
    }

    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        if (option == "-h" || option == "--help") {
            return false;
        }
        if (option == "-d" || option == "--database") {
            databaseFileName_ = nextValue(argc, argv, index, option);
            continue;
        }
        if (option == "-l" || option == "--log") {
            logFileName_ = nextValue(argc, argv, index, option);
            continue;
        }
        if (option == "-p" || option == "--port") {
            port_ = parsePort(nextValue(argc, argv, index, option));
            continue;
        }
        throw std::runtime_error("Неизвестный параметр командной строки: " + option
                                 + ". Используйте -h для справки.");
    }

    return true;
}

void Config::printHelp(std::ostream& out) const {
    out << "Программная реализация сетевого сервера vcalc\n\n"
        << "Использование:\n"
        << "  vcalc_server [-d ФАЙЛ_БАЗЫ] [-l ФАЙЛ_ЖУРНАЛА] [-p ПОРТ]\n"
        << "  vcalc_server -h\n\n"
        << "Параметры:\n"
        << "  -d, --database ФАЙЛ  База клиентов LOGIN:PASSWORD"
        << " (по умолчанию /etc/vcalc.conf)\n"
        << "  -l, --log ФАЙЛ       Журнал ошибок"
        << " (по умолчанию /var/log/vcalc.log)\n"
        << "  -p, --port ПОРТ      TCP-порт от 1 до 65535"
        << " (по умолчанию 33333)\n"
        << "  -h, --help           Вывести эту справку\n\n"
        << "При запуске без параметров выводится справка; интерактивный ввод не используется.\n";
}

}
