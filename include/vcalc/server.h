#pragma once

#include <cstdint>

namespace vcalc {

class Server {
public:
    explicit Server(std::uint16_t port);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    
    void start();

    
    int acceptConnection() const;

    
    void stop() noexcept;

    std::uint16_t port() const noexcept { return port_; }

private:
    std::uint16_t port_;
    int listenSocket_{-1};
};

}
