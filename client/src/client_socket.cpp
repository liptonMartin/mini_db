//
// Created by rvova on 11.05.2026.
//

#include "client_socket.h"

#include <spdlog/sinks/stdout_color_sinks.h>

ClientSocket::ClientSocket() : _ssl_context(boost::asio::ssl::context::tlsv12) {
    _logger = spdlog::stdout_color_mt("client");

    /* настройка ssl */
    // TODO: по хорошему надо как переменные окружения пробрасывать путь до сертов
    _ssl_context.load_verify_file("../../certs/server.crt");

    /* режим: проверять удаленный узел */
    _ssl_context.set_verify_mode(boost::asio::ssl::context::verify_peer);
}

bool ClientSocket::run(const int server_port) {
    _logger->info("Client is running...");

    /* создаю ssl стрим */
    const auto ssl_socket = std::make_shared<AsioSSLSocket>(_io_context, _ssl_context);
    _ssl_socket = ssl_socket;

    const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), server_port);

    try {
        ssl_socket->lowest_layer().connect(endpoint);
    } catch (const boost::system::error_code &error) {
        _logger->error("Error connecting to server: {}", error.message());
        return false;
    }

    _logger->info("Client successfully connected to server");
    return handshake_with_server();
}

void ClientSocket::stop() const {
    _logger->info("Try to stop client");
    shutdown();
}

nlohmann::json ClientSocket:: read_response() const {
    _logger->info("Try to read request");
    uint32_t length_data = 0;
    try {
        length_data = read_header();
        if (length_data == 0) {
            return get_bad_request_response();
        }
    } catch (const boost::system::system_error &) {
        return get_internal_server_error();
    }

    try {
        const auto body = read_body(length_data);
        auto response = nlohmann::json::parse(body);
        return response;
    } catch (const boost::system::system_error &) {
        return get_internal_server_error();
    }
    return get_internal_server_error();
}

bool ClientSocket::send_request(const std::string &request) const {
    _logger->info("Try to send request");

    std::vector<uint8_t> buffer;
    const uint32_t length_data = request.size();
    buffer.resize(length_data + sizeof(length_data));

    memcpy(buffer.data(), &length_data, sizeof(length_data));
    memcpy(buffer.data() + sizeof(length_data), request.data(), length_data);

    boost::system::error_code error;
    boost::asio::write(*_ssl_socket, boost::asio::buffer(buffer.data(), length_data + sizeof(length_data)), error);
    if (error) {
        _logger->error("Error writing to socket: {}", error.message());
        return false;
    }
    _logger->info("Successfully written request to socket");
    return true;
}

bool ClientSocket::handshake_with_server() const {
    _logger->info("Try to handshake with server");
    try {
        _ssl_socket->handshake(boost::asio::ssl::stream_base::client);
    } catch (const boost::system::system_error &e) {
        _logger->error("Error handshake with server: {}", e.what());
        return false;
    }
    _logger->info("Client successfully handshake with server");
    return true;
}

uint32_t ClientSocket::read_header() const {
    _logger->info("Try to read header");
    uint32_t length_data = 0;

    boost::system::error_code error;
    boost::asio::read(*_ssl_socket, boost::asio::buffer(&length_data, sizeof(length_data)), error);
    if (error) {
        _logger->error("Error reading header!");
        throw error;
    }

    _logger->info("Successfully read header from client");
    return length_data;
}

std::string ClientSocket::read_body(const uint32_t &length_data) const {
    _logger->info("Try to read body");

    std::string body;
    body.resize(length_data);
    boost::system::error_code error;

    boost::asio::read(*_ssl_socket, boost::asio::buffer(body.data(), length_data), error);
    if (error) {
        _logger->error("Error reading body!");
        throw error;
    }

    _logger->info("Successfully read body from client");
    return body;
}

void ClientSocket::shutdown() const {
    boost::system::error_code error;

    _ssl_socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    if (error) {
        _logger->error("Error shutting down socket!");
    }

    _ssl_socket->lowest_layer().close(error);
    if (error) {
        _logger->error("Error closing socket: {}", error.message());
    }
}

ClientSocket::~ClientSocket() {
    shutdown();
}

nlohmann::json ClientSocket::get_bad_request_response() {
    nlohmann::json json;
    json["Status"] = 400;
    json["Message"] = "Bad request";
    return json;
}

nlohmann::json ClientSocket::get_internal_server_error() {
    nlohmann::json json;
    json["Status"] = 500;
    json["Message"] = "Internal error";
    return json;
}
