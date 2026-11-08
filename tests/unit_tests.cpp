#include <UnitTest++/UnitTest++.h>

#include "vcalc/authenticator.h"
#include "vcalc/config.h"
#include "vcalc/logger.h"
#include "vcalc/server.h"
#include "vcalc/user_database.h"
#include "vcalc/vector_calculator.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string temporaryPath(const std::string& name) {
    return "/tmp/vcalc_" + name + "_" + std::to_string(getpid());
}

std::uint16_t findFreePort() {
    const int socketDescriptor = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1) {
        throw std::runtime_error("Не удалось создать временный сокет");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (::bind(socketDescriptor, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        ::close(socketDescriptor);
        throw std::runtime_error("Не удалось выбрать свободный порт");
    }

    socklen_t addressLength = sizeof(address);
    if (::getsockname(socketDescriptor, reinterpret_cast<sockaddr*>(&address), &addressLength) != 0) {
        ::close(socketDescriptor);
        throw std::runtime_error("Не удалось получить номер свободного порта");
    }

    ::close(socketDescriptor);
    return ntohs(address.sin_port);
}

struct CorrectDatabase {
    std::string path{temporaryPath("correct_database") + ".conf"};

    CorrectDatabase() {
        std::ofstream output(path);
        output << "# Тестовая база\n"
               << "alice:secret\n"
               << "bob:password\n";
    }

    ~CorrectDatabase() {
        std::remove(path.c_str());
    }
};

} 

SUITE(AuthenticatorTest) {

TEST(SaltZero) {
    CHECK_EQUAL(std::string("0000000000000000"), vcalc::Authenticator::saltToHex(0));
}

TEST(SaltWithLeadingZeros) {
    CHECK_EQUAL(std::string("0000000000001A2B"), vcalc::Authenticator::saltToHex(0x1A2BULL));
}

TEST(MaximumSalt) {
    CHECK_EQUAL(std::string("FFFFFFFFFFFFFFFF"),
                vcalc::Authenticator::saltToHex(0xFFFFFFFFFFFFFFFFULL));
}

TEST(HashForFixedSaltAndPassword) {
    const std::string hash = vcalc::Authenticator::computeHash("0000000000000000", "password");
    CHECK_EQUAL(std::string("94E8C5EFE6766B876EB886D770F28BB6D5637DF79506988A61098B5F7A334907"),
                hash);
}

TEST(CorrectHash) {
    const std::string hash = vcalc::Authenticator::computeHash("0000000000000000", "password");
    CHECK(vcalc::Authenticator::verify(hash, hash));
}

TEST(ChangedHash) {
    const std::string correct =
        "94E8C5EFE6766B876EB886D770F28BB6D5637DF79506988A61098B5F7A334907";
    const std::string changed =
        "94E8C5EFE6766B876EB886D770F28BB6D5637DF79506988A61098B5F7A334906";
    CHECK(!vcalc::Authenticator::verify(changed, correct));
}

TEST(HashInLowercase) {
    const std::string correct =
        "94E8C5EFE6766B876EB886D770F28BB6D5637DF79506988A61098B5F7A334907";
    CHECK(!vcalc::Authenticator::verify("94e8c5efe6766b876eb886d770f28bb6d5637df79506988a61098b5f7a334907",
                                        correct));
}

TEST(GeneratedSaltsAreDifferent) {
    const auto firstSalt = vcalc::Authenticator{}.generateSalt();
    const auto secondSalt = vcalc::Authenticator{}.generateSalt();
    CHECK(firstSalt != secondSalt);
}

}

SUITE(VectorCalculatorTest) {

TEST(OrdinarySum) {
    const std::array<std::int16_t, 3> values{100, 200, -50};
    CHECK_EQUAL(static_cast<std::int16_t>(250),
                vcalc::VectorCalculator::sum(values.data(), values.size()));
}

TEST(WideSum) {
    const std::array<std::int16_t, 2> values{32767, 32767};
    CHECK_EQUAL(static_cast<std::int64_t>(65534),
                vcalc::VectorCalculator::sumToWide(values.data(), values.size()));
}

TEST(OverflowUp) {
    const std::array<std::int16_t, 2> values{32767, 1};
    CHECK_EQUAL(static_cast<std::int16_t>(32767),
                vcalc::VectorCalculator::sum(values.data(), values.size()));
}

TEST(OverflowDown) {
    const std::array<std::int16_t, 2> values{-32768, -1};
    CHECK_EQUAL(static_cast<std::int16_t>(-32768),
                vcalc::VectorCalculator::sum(values.data(), values.size()));
}

TEST(UpperSaturationBorder) {
    CHECK_EQUAL(static_cast<std::int16_t>(32767),
                vcalc::VectorCalculator::saturate(32767));
}

TEST(LowerSaturationBorder) {
    CHECK_EQUAL(static_cast<std::int16_t>(-32768),
                vcalc::VectorCalculator::saturate(-32768));
}

}

SUITE(UserDatabaseTest) {

TEST_FIXTURE(CorrectDatabase, LoadCorrectDatabase) {
    vcalc::UserDatabase database(path);
    database.load();
    CHECK_EQUAL(static_cast<std::size_t>(2), database.size());
}

TEST_FIXTURE(CorrectDatabase, FindExistingUser) {
    vcalc::UserDatabase database(path);
    database.load();
    CHECK(database.find("alice"));
}

TEST_FIXTURE(CorrectDatabase, FindUnknownUser) {
    vcalc::UserDatabase database(path);
    database.load();
    CHECK(!database.find("carol"));
}

TEST_FIXTURE(CorrectDatabase, GetPassword) {
    vcalc::UserDatabase database(path);
    database.load();
    CHECK_EQUAL(std::string("secret"), database.getPassword("alice"));
}

TEST_FIXTURE(CorrectDatabase, GetUnknownUserPassword) {
    vcalc::UserDatabase database(path);
    database.load();
    CHECK_THROW(database.getPassword("carol"), std::out_of_range);
}

TEST(MissingDatabaseFile) {
    vcalc::UserDatabase database(temporaryPath("missing_database") + ".conf");
    CHECK_THROW(database.load(), std::runtime_error);
}

TEST(InvalidDatabaseRecord) {
    const std::string path = temporaryPath("invalid_database") + ".conf";
    {
        std::ofstream output(path);
        output << "wrong record\n";
    }

    vcalc::UserDatabase database(path);
    CHECK_THROW(database.load(), std::runtime_error);
    std::remove(path.c_str());
}

TEST(DuplicateLogin) {
    const std::string path = temporaryPath("duplicate_database") + ".conf";
    {
        std::ofstream output(path);
        output << "alice:secret\n"
               << "alice:another_password\n";
    }

    vcalc::UserDatabase database(path);
    CHECK_THROW(database.load(), std::runtime_error);
    std::remove(path.c_str());
}

}

SUITE(ConfigTest) {

TEST(CorrectParameters) {
    char program[] = "vcalc_server";
    char database[] = "-d";
    char databasePath[] = "users.conf";
    char log[] = "-l";
    char logPath[] = "server.log";
    char port[] = "-p";
    char portValue[] = "34567";
    char* argv[] = {program, database, databasePath, log, logPath, port, portValue, nullptr};

    vcalc::Config config;
    CHECK(config.parseArgs(7, argv));
    CHECK_EQUAL(std::string("users.conf"), config.databaseFileName());
    CHECK_EQUAL(std::string("server.log"), config.logFileName());
    CHECK_EQUAL(static_cast<std::uint16_t>(34567), config.port());
}

TEST(NoParameters) {
    char program[] = "vcalc_server";
    char* argv[] = {program, nullptr};
    vcalc::Config config;
    CHECK(!config.parseArgs(1, argv));
}

TEST(HelpParameter) {
    char program[] = "vcalc_server";
    char help[] = "-h";
    char* argv[] = {program, help, nullptr};
    vcalc::Config config;
    CHECK(!config.parseArgs(2, argv));
}

TEST(UnknownParameter) {
    char program[] = "vcalc_server";
    char wrong[] = "--wrong";
    char* argv[] = {program, wrong, nullptr};
    vcalc::Config config;
    CHECK_THROW(config.parseArgs(2, argv), std::runtime_error);
}

TEST(ZeroPort) {
    char program[] = "vcalc_server";
    char port[] = "-p";
    char portValue[] = "0";
    char* argv[] = {program, port, portValue, nullptr};
    vcalc::Config config;
    CHECK_THROW(config.parseArgs(3, argv), std::runtime_error);
}

TEST(TooLargePort) {
    char program[] = "vcalc_server";
    char port[] = "-p";
    char portValue[] = "70000";
    char* argv[] = {program, port, portValue, nullptr};
    vcalc::Config config;
    CHECK_THROW(config.parseArgs(3, argv), std::runtime_error);
}

TEST(HelpText) {
    vcalc::Config config;
    std::ostringstream output;
    config.printHelp(output);
    CHECK(output.str().find("Использование") != std::string::npos);
}

}

SUITE(LoggerTest) {

TEST(TimestampFormat) {
    const std::string value = vcalc::Logger::timestamp();
    CHECK_EQUAL(static_cast<std::size_t>(19), value.size());
    CHECK_EQUAL('-', value[4]);
    CHECK_EQUAL('-', value[7]);
    CHECK_EQUAL(' ', value[10]);
    CHECK_EQUAL(':', value[13]);
    CHECK_EQUAL(':', value[16]);
}

TEST(NonCriticalError) {
    const std::string path = temporaryPath("noncritical_log") + ".log";
    vcalc::Logger logger(path);
    logger.logError("Тестовая ошибка", false, "port=33333");

    std::ifstream input(path);
    std::string line;
    std::getline(input, line);
    CHECK(line.find("NONCRITICAL") != std::string::npos);
    CHECK(line.find("Тестовая ошибка") != std::string::npos);
    CHECK(line.find("port=33333") != std::string::npos);
    std::remove(path.c_str());
}

TEST(CriticalError) {
    const std::string path = temporaryPath("critical_log") + ".log";
    vcalc::Logger logger(path);
    logger.logError("Критическая ошибка", true);

    std::ifstream input(path);
    std::string line;
    std::getline(input, line);
    CHECK(line.find("CRITICAL") != std::string::npos);
    std::remove(path.c_str());
}

TEST(EmptyLogFileName) {
    CHECK_THROW(vcalc::Logger logger(""), std::runtime_error);
}

}

SUITE(ServerTest) {

TEST(StartServer) {
    const std::uint16_t port = findFreePort();
    vcalc::Server server(port);
    server.start();
    CHECK_EQUAL(port, server.port());
    server.stop();
}

TEST(SecondStart) {
    vcalc::Server server(findFreePort());
    server.start();
    CHECK_THROW(server.start(), std::logic_error);
    server.stop();
}

TEST(StopServer) {
    vcalc::Server server(findFreePort());
    server.start();
    server.stop();
    CHECK(true);
}

TEST(SecondStop) {
    vcalc::Server server(findFreePort());
    server.start();
    server.stop();
    server.stop();
    CHECK(true);
}

}

int main() {
    return UnitTest::RunAllTests();
}
