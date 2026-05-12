//
// Created by rvova on 11.05.2026.
//

#include "server_socket.h"

ServerSocket::ServerSocket(Entrypoint &entrypoint, BoostGenerator &generator, const int port)
    : _generator(generator),
      _entrypoint(entrypoint),
      _ssl_context(boost::asio::ssl::context::tlsv12),
      _acceptor(_io_context) {
    _logger = spdlog::stdout_color_mt("server");
    _logger->info("Starting server...");

    /* настройка ssl */
    _ssl_context.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::single_dh_use

    );
    _ssl_context.use_certificate_chain_file("../../server.crt");
    _ssl_context.use_private_key_file("../../server.key", boost::asio::ssl::context::pem);

    /* loopback = localhost */
    _logger->info("Starting acceptor...");
    const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), port);
    _acceptor.open(endpoint.protocol());
    _acceptor.bind(endpoint);
    _acceptor.listen();

    _logger->info("Acceptor successfully started");

    async_start_accept();

    _io_context.run();
}


void ServerSocket::async_start_accept() {
    _logger->info("Starting accept new connection...");
    auto client_socket = std::make_shared<AsioSocket>(_io_context);

    _acceptor.async_accept(
        *client_socket,
        [this, client_socket](const boost::system::error_code &ec) {
            if (ec) {
                _logger->error("Error accepting: {}", ec.message());
            } else {
                _logger->info("New connection found, try to handshake");

                const auto ssl_socket = std::make_shared<AsioSSLSocket>(std::move(*client_socket), _ssl_context);

                async_handshake(ssl_socket);
            }
            /* продолжать слушать других клиентов */
            async_start_accept();
        }
    );
}

void ServerSocket::async_handshake(const AsioSSLSocketPtr &ssl_client_socket) {
    ssl_client_socket->async_handshake(
        boost::asio::ssl::stream_base::server,
        [this, ssl_client_socket](const boost::system::error_code &ec) {
            if (ec) {
                _logger->error(
                    "Error handshake with client {}:{}. Details: {}",
                    ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
                    ssl_client_socket->lowest_layer().remote_endpoint().port(),
                    ec.message()
                );
                return;
            }
            _logger->info(
                "Successful tls handshake with client: {}:{}",
                ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
                ssl_client_socket->lowest_layer().remote_endpoint().port()
            );

            /* получилось соединиться, создаем обертку для сессии с клиентом */
            const auto session = ClientSession(ssl_client_socket);

            /* начинаем слушать его сообщения */
            async_read_header(session);
        }
    );
}

void ServerSocket::async_read_header(const ClientSession &session) {
    _logger->info(
        "Starting listing header from client: {}:{}",
        session.ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
        session.ssl_client_socket->lowest_layer().remote_endpoint().port()
    );

    auto length_data = std::make_shared<uint32_t>();
    boost::asio::async_read(
        *session.ssl_client_socket,
        boost::asio::buffer(length_data.get(), sizeof(*length_data)),
        [this, session, length_data](const boost::system::error_code &ec, std::size_t length) {
            if (ec) {
                if (ec == boost::asio::error::eof) {
                    _logger->info("Client disconnected");
                    return;
                }
                _logger->error("Error reading length data: {}", ec.message());
                /* пробуем еще раз */
                async_read_header(session);
                return;
            }
            _logger->info(
                "Successfully read the header from the client: {}:{}",
                session.ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
                session.ssl_client_socket->lowest_layer().remote_endpoint().port()
            );

            /* успешно прочитали, можно читать далее */
            async_read_request(session, length_data);
        }
    );
}

void ServerSocket::async_read_request(const ClientSession &session, const std::shared_ptr<uint32_t> &length_data) {
    _logger->info(
        "Starting read request from client: {}:{}",
        session.ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
        session.ssl_client_socket->lowest_layer().remote_endpoint().port()
    );

    if (*length_data == 0) {
        const nlohmann::json response = get_message_bad_request();
        async_send_response_client(session, response);
        return;
    }

    const auto buffer = std::make_shared<std::string>();
    buffer->resize(*length_data);

    boost::asio::async_read(
        *session.ssl_client_socket,
        boost::asio::buffer(buffer->data(), buffer->size()),
        [this, session, buffer](const boost::system::error_code &ec, std::size_t) {
            if (ec) {
                if (ec == boost::asio::error::eof) {
                    _logger->info("Client disconnected");
                    return;
                }
                _logger->error("Error reading length data: {}", ec.message());
                /* пробуем еще раз */
                async_read_header(session);
                return;
            }

            _logger->info(
                "Successfully read the request from the client: {}:{}",
                session.ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
                session.ssl_client_socket->lowest_layer().remote_endpoint().port()
            );

            /* успешно прочитали, обрабатываем запрос */
            async_handle_request(session, buffer);
        }
    );
}

void ServerSocket::async_handle_request(const ClientSession &session, const std::shared_ptr<std::string> &request) {
    /* создаем задачу, отдаем пользователю ее uuid, отдаем задачу в entrypoint */

    _logger->info(
        "Starting handle request {} from client: {}:{}",
        *request,
        session.ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
        session.ssl_client_socket->lowest_layer().remote_endpoint().port()
    );

    // TODO: decompose to lexer and parser!

    // TODO: handle client permission to this command!

    // TODO: handle USE, GET uuid command

    CreateDatabaseCommand command{*request};

    const auto task_id = _generator();

    // TODO: edit create task after impl parser
    Task task{
        task_id,
        CommandType::CreateDatabase,
        std::make_unique<CreateDatabaseCommand>(command)
    };

    async_send_task_id_client(session, task_id);

    _entrypoint.post_task(std::move(task));
}

void ServerSocket::async_send_task_id_client(const ClientSession &session, const boost::uuids::uuid &task_id) {
    /* преобразуем в json и делегируем другому методу */

    nlohmann::json response;
    response["Status"] = 200;
    response["Message"] = "Task successfully created";
    response["uuid"] = boost::uuids::to_string(task_id);

    async_send_response_client(session, response);
}

void ServerSocket::async_send_response_client(const ClientSession &session, const nlohmann::json &response) {
    /* сначала отправляем длину ответа, затем отправляем сам json */

    _logger->info(
        "Starting writing the response {} to client: {}:{}",
        nlohmann::to_string(response),
        session.ssl_client_socket->lowest_layer().remote_endpoint().address().to_string(),
        session.ssl_client_socket->lowest_layer().remote_endpoint().port()
    );

    const auto raw_response = response.dump();
    const uint32_t length_data = raw_response.size();

    const auto buffer = std::make_shared<std::vector<uint8_t> >();
    buffer->resize(sizeof(length_data) + length_data);
    memcpy(buffer->data(), &length_data, sizeof(length_data));
    memcpy(buffer->data() + sizeof(length_data), raw_response.data(), raw_response.size());

    boost::asio::async_write(
        *session.ssl_client_socket,
        boost::asio::buffer(buffer->data(), length_data + sizeof(length_data)),
        // ReSharper disable once CppLambdaCaptureNeverUsed
        [this, session, buffer](const boost::system::error_code &ec, std::size_t) {
            if (ec) {
                _logger->error("Error writing response: {}", ec.message());
                /* Даже если ошибка, то начинаем снова слушать? */
            }
            async_read_header(session);
        }
    );
}

void ServerSocket::shutdown() {
    _io_context.stop();

    boost::system::error_code ec;
    _acceptor.close(ec);
    if (ec) {
        _logger->error("Error while closing acceptor: {}", ec.message());
    }
}

ServerSocket::~ServerSocket() {
    shutdown();
}

nlohmann::json ServerSocket::get_message_bad_request() {
    nlohmann::json j;
    j["Status"] = 400;
    j["Message"] = "Bad request";

    return j;
}
