#include "vcalc/client_session.h"
#include "vcalc/config.h"
#include "vcalc/logger.h"
#include "vcalc/server.h"
#include "vcalc/user_database.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

namespace {

volatile std::sig_atomic_t stopRequested = 0;

extern "C" void requestStop(int) noexcept {
    stopRequested = 1;
}

void installSignalHandlers() {
    struct sigaction stopAction {};
    stopAction.sa_handler = requestStop;
    sigemptyset(&stopAction.sa_mask);
    // Сигнал остановки должен прерывать ожидание на сокете.
    stopAction.sa_flags = 0;

    if (sigaction(SIGINT, &stopAction, nullptr) != 0 ||
        sigaction(SIGTERM, &stopAction, nullptr) != 0) {
        throw std::system_error(errno, std::generic_category(), "sigaction");
    }

    // Дополнительная защита, если клиент закрыл соединение.
    struct sigaction pipeAction {};
    pipeAction.sa_handler = SIG_IGN;
    sigemptyset(&pipeAction.sa_mask);
    if (sigaction(SIGPIPE, &pipeAction, nullptr) != 0) {
        throw std::system_error(errno, std::generic_category(), "sigaction(SIGPIPE)");
    }
}

std::string peerAddress(int socket) {
    sockaddr_in address{};
    socklen_t length = sizeof(address);
    if (getpeername(socket, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        return "unknown";
    }

    char text[INET_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET, &address.sin_addr, text, sizeof(text)) == nullptr) {
        return "unknown";
    }
    return std::string(text) + ':' + std::to_string(ntohs(address.sin_port));
}

}

int main(int argc, char* argv[]) {
    vcalc::Config config;
    try {
        if (!config.parseArgs(argc, argv)) {
            config.printHelp(std::cout);
            return 0;
        }
    } catch (const std::exception& error) {
        std::cerr << "Ошибка параметров: " << error.what() << '\n';
        config.printHelp(std::cerr);
        return 2;
    }

    // Главный цикл явно принимает клиентов и проверяет условие остановки.
    std::unique_ptr<vcalc::Logger> logger;
    try {
        logger = std::make_unique<vcalc::Logger>(config.logFileName());
        vcalc::UserDatabase database(config.databaseFileName());
        database.load();

        vcalc::Server server(config.port());
        installSignalHandlers();
        server.start();

        while (!stopRequested) {
            try {
                const int clientSocket = server.acceptConnection();
                if (stopRequested) {
                    ::close(clientSocket);
                    break;
                }
                vcalc::ClientSession session(clientSocket, database, *logger, peerAddress(clientSocket));
                session.handle(); // Клиенты обслуживаются по одному.
            } catch (const std::system_error& error) {
                if (stopRequested && error.code().value() == EINTR) {
                    break;
                }
                logger->logError("Ошибка приёма TCP-подключения", false,
                                 std::string("reason=") + error.what());
            } catch (const std::exception& error) {
                logger->logError("Некритическая ошибка главного цикла сервера", false,
                                 std::string("reason=") + error.what());
            }
        }

        server.stop();
        return 0;
    } catch (const std::exception& error) {
        // Ошибки записываются в журнал, если он доступен.
        if (logger) {
            try {
                logger->logError("Критическая ошибка сервера", true,
                                 std::string("reason=") + error.what());
            } catch (...) {
                // Если журнал недоступен, сообщение выводится в stderr.
            }
        }
        std::cerr << "Критическая ошибка сервера: " << error.what() << '\n';
        return 1;
    }
}
