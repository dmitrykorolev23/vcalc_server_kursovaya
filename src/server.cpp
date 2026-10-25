#include "vcalc/server.h"

#include "vcalc/protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>

namespace vcalc {
namespace {

[[noreturn]] void throwSystemError(const char* operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}

}

Server::Server(std::uint16_t port) : port_(port) {}

Server::~Server() {
    stop();
}

void Server::start() {
    if (listenSocket_ != -1) {
        throw std::logic_error("Сервер уже запущен");
    }

    listenSocket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ == -1) {
        throwSystemError("socket");
    }

    try {
        const int reuseAddress = 1;
        if (setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR,
                       &reuseAddress, sizeof(reuseAddress)) != 0) {
            throwSystemError("setsockopt(SO_REUSEADDR)");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port_);

        if (::bind(listenSocket_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
            throwSystemError("bind");
        }
        if (::listen(listenSocket_, SOMAXCONN) != 0) {
            throwSystemError("listen");
        }
    } catch (...) {
        stop();
        throw;
    }
}

int Server::acceptConnection() const {
    if (listenSocket_ == -1) {
        throw std::logic_error("Сервер не запущен");
    }

    const int clientSocket = ::accept(listenSocket_, nullptr, nullptr);
    if (clientSocket == -1) {
        throwSystemError("accept");
    }

    try {
        protocol::setSocketTimeouts(clientSocket, 180);
    } catch (...) {
        ::close(clientSocket);
        throw;
    }
    return clientSocket;
}

void Server::stop() noexcept {
    if (listenSocket_ != -1) {
        ::shutdown(listenSocket_, SHUT_RDWR);
        ::close(listenSocket_);
        listenSocket_ = -1;
    }
}

}
