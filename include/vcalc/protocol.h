#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace vcalc::protocol {

constexpr std::size_t kMaxTextMessageLength = 1024;

void sendAll(int socket, const void* data, std::size_t size);

void receiveAll(int socket, void* data, std::size_t size);

void sendText(int socket, const std::string& text);

std::string receiveText(int socket, std::size_t maxLength = kMaxTextMessageLength);

void sendUint32(int socket, std::uint32_t value);

std::uint32_t receiveUint32(int socket);

void sendInt16(int socket, std::int16_t value);

void setSocketTimeouts(int socket, int timeoutSeconds);

}
