#include "vcalc/client_session.h"

#include "vcalc/protocol.h"

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace vcalc {
namespace {

bool isValidLogin(const std::string& login) {
    if (login.empty() || login.size() > 128) {
        return false;
    }
    for (const char character : login) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::iscntrl(byte) != 0 || character == ':') {
            return false;
        }
    }
    return true;
}

std::string sessionParameters(const std::string& peer, const std::string& login) {
    std::ostringstream out;
    out << "peer=" << peer;
    if (!login.empty()) {
        out << "; login=" << login;
    }
    return out.str();
}

}

ClientSession::ClientSession(int clientSocket, const UserDatabase& database,
                             const Logger& logger, std::string peerAddress)
    : clientSocket_(clientSocket), database_(database), logger_(logger),
      peerAddress_(std::move(peerAddress)) {
    if (clientSocket_ < 0) {
        throw std::invalid_argument("Некорректный сокет клиентской сессии");
    }
}

ClientSession::~ClientSession() {
    close();
}

void ClientSession::handle() {
    try {
        if (!identify()) {
            return;
        }
        if (!authenticate()) {
            return;
        }
        processVectors();
    } catch (const std::exception& error) {
        // В двоичной фазе нельзя отправлять текстовый ответ ERR.
        try {
            logger_.logError("Ошибка обслуживания клиентской сессии", false,
                             sessionParameters(peerAddress_, login_) + "; reason=" + error.what());
        } catch (...) {
            // Ошибка одной сессии не должна завершать сервер.
        }
    }
    close();
}

bool ClientSession::identify() {
    // Клиент передаёт логин без символа перевода строки.
    login_ = protocol::receiveText(clientSocket_);
    if (!isValidLogin(login_) || !database_.find(login_)) {
        sendErrorAndClose("Ошибка идентификации клиента");
        return false;
    }
    return true;
}

bool ClientSession::authenticate() {
    const std::string salt16 = Authenticator::saltToHex(authenticator_.generateSalt());
    // Соль содержит 16 заглавных шестнадцатеричных символов без перевода строки.
    protocol::sendText(clientSocket_, salt16);

    const std::string receivedHash = protocol::receiveText(clientSocket_);
    const std::string expectedHash = Authenticator::computeHash(
        salt16, database_.getPassword(login_));

    if (!Authenticator::verify(receivedHash, expectedHash)) {
        sendErrorAndClose("Ошибка аутентификации клиента");
        return false;
    }

    authenticated_ = true;
    protocol::sendText(clientSocket_, "OK");
    return true;
}

void ClientSession::processVectors() {
    if (!authenticated_) {
        throw std::logic_error("Попытка обработки векторов до аутентификации");
    }

    const std::uint32_t vectorCount = protocol::receiveUint32(clientSocket_);
    for (std::uint32_t index = 0; index < vectorCount; ++index) {
        const std::uint32_t vectorSize = protocol::receiveUint32(clientSocket_);
        const std::int16_t result = receiveAndCalculateVector(vectorSize);
        sendSingleResult(result);
    }
}

void ClientSession::close() noexcept {
    if (clientSocket_ != -1) {
        ::shutdown(clientSocket_, SHUT_RDWR);
        ::close(clientSocket_);
        clientSocket_ = -1;
    }
}

void ClientSession::sendErrorAndClose(const std::string& reason) {
    try {
        protocol::sendText(clientSocket_, "ERR");
    } catch (...) {
        // Клиент мог уже закрыть соединение.
    }
    try {
        logger_.logError(reason, false, sessionParameters(peerAddress_, login_));
    } catch (...) {
        // Сокет нужно закрыть даже при ошибке записи в журнал.
    }
    close();
}

std::int16_t ClientSession::receiveAndCalculateVector(std::uint32_t size) {
    // Вектор читается частями, чтобы не выделять память размером S.
    constexpr std::size_t kChunkLength = 4096;
    std::array<std::int16_t, kChunkLength> values{};
    std::int64_t total = 0;
    std::uint32_t remaining = size;

    while (remaining > 0) {
        const std::size_t currentLength = remaining < kChunkLength
            ? static_cast<std::size_t>(remaining)
            : kChunkLength;

        // Значения int16_t передаются непрерывным двоичным блоком.
        protocol::receiveAll(clientSocket_, values.data(),
                             currentLength * sizeof(values[0]));
        total += VectorCalculator::sumToWide(values.data(), currentLength);
        remaining -= static_cast<std::uint32_t>(currentLength);
    }

    return VectorCalculator::saturate(total);
}

void ClientSession::sendSingleResult(std::int16_t result) {
    // После каждого вектора клиент получает один результат int16_t.
    protocol::sendInt16(clientSocket_, result);
}

}
