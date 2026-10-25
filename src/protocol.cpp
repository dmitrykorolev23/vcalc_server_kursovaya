#include "vcalc/protocol.h"

#include <sys/socket.h>
#include <sys/time.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace vcalc::protocol {
namespace {

[[noreturn]] void throwSystemError(const char* operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}

}

void sendAll(int socket, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::size_t sent = 0;
    while (sent < size) {
#ifdef MSG_NOSIGNAL
        constexpr int sendFlags = MSG_NOSIGNAL;
#else
        constexpr int sendFlags = 0;
#endif
        const ssize_t result = ::send(socket, bytes + sent, size - sent, sendFlags);
        if (result > 0) {
            sent += static_cast<std::size_t>(result);
            continue;
        }
        if (result == 0) {
            throw std::runtime_error("Соединение закрыто при отправке данных");
        }
        throwSystemError("send");
    }
}

void receiveAll(int socket, void* data, std::size_t size) {
    auto* bytes = static_cast<unsigned char*>(data);
    std::size_t received = 0;
    while (received < size) {
        const ssize_t result = ::recv(socket, bytes + received, size - received, 0);
        if (result > 0) {
            received += static_cast<std::size_t>(result);
            continue;
        }
        if (result == 0) {
            throw std::runtime_error("Клиент закрыл соединение до завершения приёма данных");
        }
        throwSystemError("recv");
    }
}

void sendText(int socket, const std::string& text) {
    if (text.empty()) {
        throw std::invalid_argument("Пустое текстовое сообщение нельзя передать по протоколу");
    }
    sendAll(socket, text.data(), text.size());
}

std::string receiveText(int socket, std::size_t maxLength) {
    std::string buffer(maxLength, '\0');
    const ssize_t result = ::recv(socket, buffer.data(), buffer.size(), 0);
    if (result > 0) {
        buffer.resize(static_cast<std::size_t>(result));
        return buffer;
    }
    if (result == 0) {
        throw std::runtime_error("Клиент закрыл соединение при приёме текстового сообщения");
    }
    throwSystemError("recv");
}

void sendUint32(int socket, std::uint32_t value) {
    sendAll(socket, &value, sizeof(value));
}

std::uint32_t receiveUint32(int socket) {
    std::uint32_t value{};
    receiveAll(socket, &value, sizeof(value));
    return value;
}

void sendInt16(int socket, std::int16_t value) {
    sendAll(socket, &value, sizeof(value));
}

void setSocketTimeouts(int socket, int timeoutSeconds) {
    const timeval timeout{timeoutSeconds, 0};
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0 ||
        setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
        throwSystemError("setsockopt(SO_RCVTIMEO/SO_SNDTIMEO)");
    }

    // В macOS используется SO_NOSIGPIPE, в Linux — MSG_NOSIGNAL.
#ifdef SO_NOSIGPIPE
    const int enabled = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0) {
        throwSystemError("setsockopt(SO_NOSIGPIPE)");
    }
#endif
}

}
