//
// Created by rvova on 09.05.2026.
//


#include <iostream>
#include <boost/process/v1/child.hpp>

#include "entrypoint.h"
#include "exceptions.h"

Entrypoint::Entrypoint(const int count_storage_nodes)
    : _work_guard(boost::asio::make_work_guard(_io_context)), _acceptor(_io_context) {
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

    _worker_thread = std::thread([this, count_storage_nodes] {
        for (int i = 0; i < count_storage_nodes; i++) {
            add_storage_node();
        }

        _io_context.run();
    });
}

void Entrypoint::add_storage_node() {
    /* создаем новый процесс, который теперь пытается подключиться к нашему listen_socket */
    try {
        const auto child_process_ptr = std::make_shared<BoostProcess>(
            boost::process::v1::child("./main_storage_node.exe"));

        _logger->info("Starting new process with PID {}", child_process_ptr->id());

        start_accept(child_process_ptr);
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

    /* удаляем из maps */
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

    async_send_next_task(socket);
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

    boost::system::error_code ec;
    _acceptor.close(ec);
    if (ec) {
        _logger->error("Error while closing acceptor: {}", ec.message());
    }
}

Entrypoint::~Entrypoint() {
    shutdown();
}
