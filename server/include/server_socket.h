//
// Created by rvova on 11.05.2026.

#ifndef MINIDB_SERVER_SOCKET_H
#define MINIDB_SERVER_SOCKET_H

#include "entrypoint.h"

using boost_generator = boost::uuids::random_generator;

/**
 * Обертка над клиентским соединением.
 * Содержит его сокет, его активную базу данных (что выбрана в команде USE),
 * в будущем будет поддерживать распределение прав (какие команды может выполнять)
 */
struct ClientSession {
    asio_socket_ptr client_socket;
    std::string db_name = "";
};

class ServerSocket {
    boost::uuids::random_generator& _generator;
    Entrypoint& _entrypoint;

    boost::asio::io_context _io_context;
    boost::asio::ip::tcp::acceptor _acceptor;

    std::shared_ptr<spdlog::logger> _logger;

    void async_start_accept();

    void async_read_header(const ClientSession &session);

    void async_read_request(const ClientSession &session, const std::shared_ptr<uint32_t> &length_data);

    void async_handle_request(const ClientSession &session, const std::shared_ptr<std::string> &request);

    void async_send_task_id_client(const ClientSession &session, const boost::uuids::uuid& task_id);

    void async_send_response_client(const ClientSession &session, const nlohmann::json &response);

    void shutdown();

    static nlohmann::json get_message_bad_request();

public:
    explicit ServerSocket(Entrypoint& entrypoint, boost_generator& generator, int port);

    ~ServerSocket();
};

#endif //MINIDB_SERVER_SOCKET_H
