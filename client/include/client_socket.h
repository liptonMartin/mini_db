//
// Created by rvova on 11.05.2026.
//

#ifndef MINIDB_CLIENT_SOCKET_H
#define MINIDB_CLIENT_SOCKET_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <spdlog/logger.h>
#include <nlohmann/json.hpp>

using AsioSocket = boost::asio::ip::tcp::socket;

using AsioSSLSocket = boost::asio::ssl::stream<AsioSocket>;
using AsioSSLSocketPtr = std::shared_ptr<AsioSSLSocket>;

class ClientSocket {
    boost::asio::io_context _io_context;
    boost::asio::ssl::context _ssl_context;
    AsioSSLSocketPtr _ssl_socket;

    std::shared_ptr<spdlog::logger> _logger;

    bool handshake_with_server() const;

    uint32_t read_header() const;

    std::string read_body(const uint32_t &length_data) const;

    void shutdown() const;

    static nlohmann::json get_bad_request_response();

    static nlohmann::json get_internal_server_error();

public:
    explicit ClientSocket();

    /**
     * Запускает event loop прям в этом же потоке
     * @return True - успешно запустился, false - проблемы при подключении к серверу
     */
    bool run(int server_port);

    void stop() const;

    nlohmann::json read_response() const;

    bool send_request(const std::string& request) const;

    ~ClientSocket();
};


#endif //MINIDB_CLIENT_SOCKET_H
