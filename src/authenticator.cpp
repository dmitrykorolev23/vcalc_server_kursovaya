#include "vcalc/authenticator.h"

#include <cryptopp/hex.h>
#include <cryptopp/misc.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace vcalc {
namespace {

bool isUpperHex(const std::string& value) noexcept {
    if (value.size() != 64) {
        return false;
    }
    for (const char character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (!((byte >= '0' && byte <= '9') ||
              (byte >= 'A' && byte <= 'F'))) {
            return false;
        }
    }
    return true;
}

}

std::uint64_t Authenticator::generateSalt() const {
    // Получение случайных байтов для 64-битной соли.
    CryptoPP::AutoSeededRandomPool randomGenerator;
    std::array<CryptoPP::byte, sizeof(std::uint64_t)> bytes{};
    randomGenerator.GenerateBlock(bytes.data(), bytes.size());

    std::uint64_t salt = 0;
    for (const CryptoPP::byte byte : bytes) {
        salt = (salt << 8U) | static_cast<std::uint64_t>(byte);
    }
    return salt;
}

std::string Authenticator::saltToHex(std::uint64_t salt) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << salt;
    return out.str();
}

std::string Authenticator::computeHash(const std::string& salt16,
                                       const std::string& password) {
    const std::string source = salt16 + password;

    CryptoPP::SHA256 sha256;
    std::array<CryptoPP::byte, CryptoPP::SHA256::DIGESTSIZE> digest{};
    sha256.CalculateDigest(
        digest.data(),
        reinterpret_cast<const CryptoPP::byte*>(source.data()),
        source.size());

    // Хеш передаётся в виде 64 заглавных шестнадцатеричных символов.
    std::string encodedDigest;
    CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(encodedDigest), true);
    encoder.Put(digest.data(), digest.size());
    encoder.MessageEnd();
    return encodedDigest;
}

bool Authenticator::verify(const std::string& received,
                           const std::string& expected) noexcept {
    if (!isUpperHex(received) || !isUpperHex(expected)) {
        return false;
    }
    return CryptoPP::VerifyBufsEqual(
        reinterpret_cast<const CryptoPP::byte*>(received.data()),
        reinterpret_cast<const CryptoPP::byte*>(expected.data()),
        expected.size());
}

}
