//
// Created by rvova on 09.05.2026.
//


#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/process/v1/child.hpp>

#include "entrypoint.h"
#include "exceptions.h"

Entrypoint::Entrypoint(const int count_storage_nodes)
    : _work_guard(boost::asio::make_work_guard(_io_context)), _acceptor(_io_context),
      _heartbeat_acceptor(_io_context), _heartbeat_timer(_io_context) {
    if (
        count_storage_nodes < entrypoint::MIN_COUNT_STORAGE_NODES
        || count_storage_nodes > entrypoint::MAX_COUNT_STORAGE_NODES) {
        throw FailedStartEntrypointException();
    }

    _logger = spdlog::stdout_color_mt("entrypoint");
    _logger->info("Starting entrypoint");

    /* loopback = localhost */
    const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), entrypoint::SERVER_PORT);
    _acceptor.open(endpoint.protocol());
    _acceptor.bind(endpoint);
    _acceptor.listen();

    _logger->info("Starting acceptor");

    /* heartbeat acceptor */
    const boost::asio::ip::tcp::endpoint heartbeat_endpoint(boost::asio::ip::address_v4::loopback(), entrypoint::HEARTBEAT_PORT);
    _heartbeat_acceptor.open(heartbeat_endpoint.protocol());
    _heartbeat_acceptor.bind(heartbeat_endpoint);
    _heartbeat_acceptor.listen();

    _logger->info("Starting heartbeat acceptor");

    _worker_thread = std::thread([this, count_storage_nodes] {
        for (int i = 0; i < count_storage_nodes; i++) {
            add_storage_node();
        }

        start_heartbeat_timer();
        _io_context.run();
    });
}

void Entrypoint::add_storage_node() {
    /* создаем новый процесс, который теперь пытается подключиться к нашему listen_socket */
    try {
        const auto child_process_ptr = std::make_shared<BoostProcess>(
            boost::process::v1::child("./main_storage_node.exe"));

        _logger->info("Starting new process with PID {}", child_process_ptr->id());

        _pending_processes.push(child_process_ptr);
        start_accept(child_process_ptr);
        start_heartbeat_accept(child_process_ptr);
    } catch (const std::system_error &e) {
        std::cout << e.what() << "\n";
    }
}

void Entrypoint::remove_storage_node(const AsioSocketPtr &socket) {
    if (_storage_nodes_busy[socket] == true) {
        _logger->error("try to remove busy storage node!");
        return;
    }

    const auto child_process = _storage_nodes_process[socket];

    child_process->terminate();
    child_process->wait();

    /* удаляем heartbeat сокет */
    const auto heartbeat_it = _storage_heartbeat_sockets.find(socket);
    if (heartbeat_it != _storage_heartbeat_sockets.end()) {
        boost::system::error_code ec;
        heartbeat_it->second->close(ec);
        _storage_heartbeat_sockets.erase(heartbeat_it);
    }

    /* удаляем из maps */
    _pending_heartbeat_sockets.erase(child_process);
    _storage_nodes_busy.erase(socket);
    _storage_nodes_process.erase(socket);
}


void Entrypoint::start_accept(const BoostProcessPtr &child_process_ptr) {
    /* создаю новый сокет для будущего storage node */
    auto client_socket = std::make_shared<AsioSocket>(_io_context);

    _acceptor.async_accept(
        *client_socket,
        [this, client_socket, child_process_ptr](const boost::system::error_code &error) {
            if (error) {
                _logger->error("Error accepting: {}", error.message());
                return;
            }

            handle_new_connection(client_socket, child_process_ptr);
        }
    );
}

void Entrypoint::handle_new_connection(const AsioSocketPtr &socket, const BoostProcessPtr &child_process_ptr) {
    /* добавляю в список сокетов */
    _storage_nodes_busy[socket] = false;
    _storage_nodes_process[socket] = child_process_ptr;

    /* если heartbeat уже пришёл — связываем */
    auto it = _pending_heartbeat_sockets.find(child_process_ptr);
    if (it != _pending_heartbeat_sockets.end()) {
        _storage_heartbeat_sockets[socket] = it->second;
        _pending_heartbeat_sockets.erase(it);
    }

    async_send_next_task(socket);
}

/* ==================== Heartbeat ==================== */

void Entrypoint::start_heartbeat_accept(const BoostProcessPtr &child_process_ptr) {
    auto heartbeat_socket = std::make_shared<AsioSocket>(_io_context);

    _heartbeat_acceptor.async_accept(
        *heartbeat_socket,
        [this, heartbeat_socket, child_process_ptr](const boost::system::error_code &error) {
            if (error) {
                _logger->error("Heartbeat accept error: {}", error.message());
                return;
            }

            handle_heartbeat_accept(heartbeat_socket, child_process_ptr);
        }
    );
}

void Entrypoint::handle_heartbeat_accept(const AsioSocketPtr &heartbeat_socket,
                                          const BoostProcessPtr &child_process_ptr) {
    _logger->info("Heartbeat connection accepted");

    /* сохраняем — может main ещё не пришёл */
    _pending_heartbeat_sockets[child_process_ptr] = heartbeat_socket;

    /* если main уже пришёл — связываем */
    for (const auto &[main_socket, process]: _storage_nodes_process) {
        if (process == child_process_ptr) {
            _storage_heartbeat_sockets[main_socket] = heartbeat_socket;
            _pending_heartbeat_sockets.erase(child_process_ptr);
            break;
        }
    }
}

void Entrypoint::start_heartbeat_timer() {
    _heartbeat_timer.expires_from_now(boost::posix_time::seconds(60));
    _heartbeat_timer.async_wait([this](const boost::system::error_code &ec) {
        if (ec) {
            if (ec != boost::asio::error::operation_aborted) {
                _logger->error("Heartbeat timer error: {}", ec.message());
            }
            return;
        }

        check_heartbeats();
        start_heartbeat_timer();
    });
}

void Entrypoint::check_heartbeats() {
    _logger->info("Checking heartbeats for {} storage nodes", _storage_heartbeat_sockets.size());
    for (const auto &[socket, heartbeat_socket]: _storage_heartbeat_sockets) {
        if (!_storage_nodes_busy[socket]) {
            async_send_heartbeat_ping(socket, heartbeat_socket);
        }
    }
}

void Entrypoint::async_send_heartbeat_ping(const AsioSocketPtr &socket, const AsioSocketPtr &heartbeat_socket) {
    CommandType heartbeat_type = CommandType::Heartbeat;

    auto buffer = std::make_shared<std::vector<uint8_t>>();
    buffer->resize(sizeof(heartbeat_type));
    memcpy(buffer->data(), &heartbeat_type, sizeof(heartbeat_type));

    boost::asio::async_write(
        *heartbeat_socket,
        boost::asio::buffer(buffer->data(), buffer->size()),
        [this, socket, heartbeat_socket](const boost::system::error_code &ec, size_t) {
            if (ec) {
                _logger->error("Heartbeat ping write error: {}", ec.message());
                restart_storage_node(socket);
                return;
            }

            async_read_heartbeat_response(socket, heartbeat_socket);
        }
    );
}

void Entrypoint::async_read_heartbeat_response(const AsioSocketPtr &socket, const AsioSocketPtr &heartbeat_socket) {
    auto length_data = std::make_shared<uint32_t>();

    boost::asio::async_read(
        *heartbeat_socket,
        boost::asio::buffer(length_data.get(), sizeof(*length_data)),
        [this, socket, heartbeat_socket, length_data](const boost::system::error_code &ec, std::size_t) {
            if (ec) {
                _logger->error("Heartbeat response read error: {}", ec.message());
                restart_storage_node(socket);
                return;
            }

            auto buffer = std::make_shared<std::vector<uint8_t>>();
            buffer->resize(*length_data);

            boost::asio::async_read(
                *heartbeat_socket,
                boost::asio::buffer(buffer->data(), *length_data),
                [this, socket, buffer](const boost::system::error_code &ec, std::size_t) {
                    if (ec) {
                        _logger->error("Heartbeat response body read error: {}", ec.message());
                        restart_storage_node(socket);
                        return;
                    }

                    try {
                        auto response = nlohmann::json::parse(*buffer);
                        if (response.contains("Message") && response["Message"] == "alive") {
                            _logger->info("Heartbeat OK for storage node");
                        } else {
                            _logger->warn("Unexpected heartbeat response: {}", response.dump());
                        }
                    } catch (...) {
                        _logger->error("Failed to parse heartbeat response");
                    }
                }
            );
        }
    );
}

void Entrypoint::restart_storage_node(const AsioSocketPtr &socket) {
    _logger->info("Restarting storage node");

    const auto child_process = _storage_nodes_process[socket];
    if (child_process) {
        child_process->terminate();
        child_process->wait();
    }

    const auto heartbeat_it = _storage_heartbeat_sockets.find(socket);
    if (heartbeat_it != _storage_heartbeat_sockets.end()) {
        boost::system::error_code ec;
        heartbeat_it->second->close(ec);
        _storage_heartbeat_sockets.erase(heartbeat_it);
    }

    _pending_heartbeat_sockets.erase(child_process);
    _storage_nodes_busy.erase(socket);
    _storage_nodes_process.erase(socket);

    add_storage_node();
}


void Entrypoint::async_read_response_header(const AsioSocketPtr &socket, boost::uuids::uuid task_id) {
    auto length_data = std::make_shared<uint32_t>();

    boost::asio::async_read(
        *socket,
        boost::asio::buffer(length_data.get(), sizeof(*length_data)),
        [this, socket, length_data, task_id](const boost::system::error_code &ec, std::size_t length) {
            if (ec) {
                _logger->error("Error reading length data: {}", ec.message());
                return;
            }

            async_read_response_body(socket, *length_data, task_id);
        }
    );
}


void Entrypoint::async_read_response_body(const AsioSocketPtr &socket, const uint32_t length_data,
                                          boost::uuids::uuid task_id) {
    auto buffer = std::make_shared<std::vector<uint8_t> >();
    buffer->resize(length_data);

    boost::asio::async_read(
        *socket,
        boost::asio::buffer(buffer->data(), length_data),
        [this, socket, buffer, task_id](const boost::system::error_code &ec, std::size_t length) {
            if (ec) {
                _logger->error("Error reading response body: {}", ec.message());
                return;
            }
            /* десериализуем данные */
            const Result response = nlohmann::json::parse(*buffer);

            /* добавляем в выполненную задачу */
            _task_results_mutex.lock();
            _task_results[task_id] = response;
            _task_results_mutex.unlock();

            /* добавляем новую задачу освободившемуся узлу */
            async_send_next_task(socket);
        }
    );
}

void Entrypoint::async_send_task(const AsioSocketPtr &socket, Task &&task) {
    if (_storage_nodes_busy[socket] == true) {
        _logger->error("Try send new task busy storage node!");
    }

    _storage_nodes_busy[socket] = true;
    auto [task_id, command_type, abstract_command] = std::move(task);

    /* сначала отправляем тип команды */
    /* сначала отправляем длину команды */
    /* затем отправляем саму команду */

    auto buffer = std::make_shared<std::vector<uint8_t> >();
    const auto command = abstract_command->serialize_command();
    const uint32_t length_data = command.size();
    buffer->resize(sizeof(command_type) + sizeof(length_data) + length_data);

    memcpy(buffer->data(), &command_type, sizeof(command_type));
    memcpy(buffer->data() + sizeof(command_type), &length_data, sizeof(length_data));
    memcpy(buffer->data() + sizeof(command_type) + sizeof(length_data), command.data(), length_data);

    boost::asio::async_write(
        *socket,
        boost::asio::buffer(buffer->data(), buffer->size()),
        // ReSharper disable once CppLambdaCaptureNeverUsed
        [this, socket, task_id, buffer](const boost::system::error_code &ec, size_t) {
            if (ec) {
                _logger->error("Error writing request: {}", ec.message());
                async_send_next_task(socket);
                return;
            }

            async_read_response_header(socket, task_id);
        }
    );
}

void Entrypoint::async_send_next_task(const AsioSocketPtr &socket) {
    if (_task_queue.empty()) {
        _storage_nodes_busy[socket] = false; /* storage node свободен */
        return;
    }
    auto task = std::move(_task_queue.front());
    _task_queue.pop();
    async_send_task(socket, std::move(task));
}

void Entrypoint::post_task(Task &&task) {
    nlohmann::json j;
    j["Status"] = 200;
    j["Message"] = "Not ready";
    _task_results[task.task_uuid] = j;

    boost::asio::dispatch(_io_context, [this, task = std::move(task)] mutable {
        for (const auto &[socket, is_busy]: _storage_nodes_busy) {
            if (!is_busy) {
                async_send_task(socket, std::move(task));
                return;
            }
        }
        _task_queue.push(std::move(task));
    });
}

nlohmann::json Entrypoint::get_result_by_id(const boost::uuids::uuid &task_id) {
    std::lock_guard lock(_task_results_mutex);
    if (!_task_results.contains(task_id)) {
        nlohmann::json response;
        response["Status"] = 404;
        response["Message"] = "Task not found";
        return response;
    }
    return _task_results[task_id];
}

void Entrypoint::shutdown() {
    _work_guard.reset();

    _io_context.stop();

    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }

    for (auto &[socket, process_ptr]: _storage_nodes_process) {
        if (process_ptr && process_ptr->running()) {
            process_ptr->terminate();
            process_ptr->wait();
        }
    }

    for (auto &[main_socket, heartbeat_socket]: _storage_heartbeat_sockets) {
        boost::system::error_code ec;
        heartbeat_socket->close(ec);
    }
    _storage_heartbeat_sockets.clear();

    boost::system::error_code ec;
    _heartbeat_acceptor.close(ec);
    if (ec) {
        _logger->error("Error while closing heartbeat acceptor: {}", ec.message());
    }

    _acceptor.close(ec);
    if (ec) {
        _logger->error("Error while closing acceptor: {}", ec.message());
    }
}

Entrypoint::~Entrypoint() {
    shutdown();
}
