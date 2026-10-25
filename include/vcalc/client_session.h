#pragma once

#include "vcalc/authenticator.h"
#include "vcalc/logger.h"
#include "vcalc/user_database.h"
#include "vcalc/vector_calculator.h"

#include <string>

namespace vcalc {

class ClientSession {
public:
    ClientSession(int clientSocket, const UserDatabase& database,
                  const Logger& logger, std::string peerAddress);
    ~ClientSession();

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    
    void handle();

    
    bool identify();

    
    bool authenticate();

    
    void processVectors();

    
    void close() noexcept;

private:
    void sendErrorAndClose(const std::string& reason);
    std::int16_t receiveAndCalculateVector(std::uint32_t size);
    void sendSingleResult(std::int16_t result);

    int clientSocket_{-1};
    const UserDatabase& database_;
    const Logger& logger_;
    Authenticator authenticator_;
    std::string peerAddress_;
    std::string login_;
    bool authenticated_{false};
};

}
