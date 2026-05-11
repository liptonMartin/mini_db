//
// Created by rvova on 09.05.2026.
//

#include <windows.h>

#include <constants.h>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "command.h"

class StorageNode {
    boost::asio::io_context _io_context;
    boost::asio::ip::tcp::socket _server_socket;

    std::shared_ptr<spdlog::logger> _logger;

    void start_listen_server() {
        async_read_command_type();
    }

    void async_read_command_type() {
        auto command_type = std::make_shared<CommandType>();
        boost::asio::async_read(
            _server_socket,
            boost::asio::buffer(command_type.get(), sizeof(*command_type)),
            [this, command_type](const boost::system::error_code &ec, std::size_t length) {
                if (ec) {
                    if (ec == boost::asio::error::eof) {
                        _logger->info("Connection closed by peer");
                        return;
                    }
                    _logger->error("Read error {}", ec.message());
                    return;
                }

                async_read_header(*command_type);
            }
        );
    }

    void async_read_header(CommandType command_type) {
        const auto length_data = std::make_shared<uint32_t>();
        boost::asio::async_read(
            _server_socket,
            boost::asio::buffer(length_data.get(), sizeof(*length_data)),
            [this, command_type, length_data](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    if (ec == boost::asio::error::eof) {
                        _logger->info("Connection closed by peer");
                        return;
                    }
                    _logger->error("Read error {}", ec.message());
                    return;
                }

                async_read_request(command_type, *length_data);
            }
        );
    }

    void async_read_request(CommandType command_type, const uint32_t length_data) {
        auto buffer = std::make_shared<std::string>();
        buffer->resize(length_data);

        boost::asio::async_read(
            _server_socket,
            boost::asio::buffer(buffer->data(), length_data),
            [this, command_type, buffer](const boost::system::error_code &ec, std::size_t) {
                if (ec) {
                    if (ec == boost::asio::error::eof) {
                        _logger->info("Connection closed by peer");
                        return;
                    }
                    _logger->error("Read error {}", ec.message());
                    return;
                }

                process_task(command_type, buffer);
            }
        );
    }

    void process_task(const CommandType& command_type, const std::shared_ptr<std::string> &data) {
        _logger->info("Start process task!");
        nlohmann::json response;

        std::unique_ptr<Command> command;
        switch (command_type) {
            case CommandType::CreateDatabase:
                command = std::make_unique<CreateDatabaseCommand>(CreateDatabaseCommand::parse_from_bytes(*data));
                break;
            case CommandType::DropDatabase:
                command = std::make_unique<DropDatabaseCommand>(DropDatabaseCommand::parse_from_bytes(*data));
                break;
            case CommandType::UseDatabase:
                command = nullptr;
                break;
            case CommandType::CreateTable:
                command = std::make_unique<CreateTableCommand>(CreateTableCommand::parse_from_bytes(*data));
                break;
            case CommandType::DropTable:
                command = std::make_unique<DropTableCommand>(DropTableCommand::parse_from_bytes(*data));
                break;
            case CommandType::InsertInto:
                command = std::make_unique<InsertIntoCommand>(InsertIntoCommand::parse_from_bytes(*data));
                break;
            case CommandType::Update:
                command = std::make_unique<UpdateCommand>(UpdateCommand::parse_from_bytes(*data));
                break;
            case CommandType::DeleteFrom:
                command = std::make_unique<DeleteFromCommand>(DeleteFromCommand::parse_from_bytes(*data));
                break;
            case CommandType::Select:
                command = std::make_unique<SelectCommand>(SelectCommand::parse_from_bytes(*data));
                break;
        }

        if (command) {
            response = command->process_command();
        } else {
            response["Status"] = 500;
            response["Message"] = "USE command should be handle on ClientSession, not Storage engine!";
        }
        _logger->info("Task successfully processed");
        async_send_response_to_server(response);
    }

    void async_send_response_to_server(const nlohmann::json& response) {
        /* Отправляем длину ответа */
        const auto raw_response = response.dump();
        const uint32_t length_data = raw_response.size();

        const auto buffer = std::make_shared<std::vector<uint8_t> >();
        buffer->resize(sizeof(length_data) + length_data);
        memcpy(buffer->data(), &length_data, sizeof(length_data));
        memcpy(buffer->data() + sizeof(length_data), raw_response.data(), raw_response.size());

        boost::asio::async_write(
            _server_socket,
            boost::asio::buffer(buffer->data(), sizeof(length_data) + length_data),
            // ReSharper disable once CppLambdaCaptureNeverUsed
            [this, buffer](const boost::system::error_code &ec, std::size_t length) {
                if (ec) {
                    _logger->error("Error while writing to server {}", ec.message());
                }

                start_listen_server();
            }
        );
    }

public:
    explicit StorageNode() : _server_socket(_io_context) {
        const auto pid = GetCurrentProcessId();
        const auto logger_name = "storage node (" + std::to_string(pid) + ")";

        _logger = spdlog::stdout_color_mt(logger_name);

        _logger->info("Starting storage node");

        const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), entrypoint::SERVER_PORT);

        _server_socket.connect(endpoint);
        _logger->info("Connect to server!");

        start_listen_server();

        _io_context.run();
    }

    ~StorageNode() {
        _io_context.stop();

        boost::system::error_code ec;
        _server_socket.close(ec);
        if (ec) {
            _logger->error("Error while closing socket: {}", ec.message());
        }
    }
};

int main() {
    StorageNode storage_node{};
}
