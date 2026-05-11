//
// Created by rvova on 09.05.2026.
//

#ifndef MINIDB_ENTRYPOINT_H
#define MINIDB_ENTRYPOINT_H


#include <queue>
#include <boost/process/v1/child.hpp>
#include <boost/asio.hpp>
#include <boost/uuid.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "command.h"
#include "constants.h"

using asio_socket = boost::asio::ip::tcp::socket;
using asio_socket_ptr = std::shared_ptr<asio_socket>;

using boost_process = boost::process::v1::child;
using boost_process_ptr = std::shared_ptr<boost_process>;

using result = nlohmann::json;

using command_ptr = std::unique_ptr<Command>;


struct Task {
    boost::uuids::uuid task_uuid;
    CommandType command_type;
    command_ptr command;
};

class Entrypoint {
    std::mutex _task_results_mutex;

    boost::asio::io_context _io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _work_guard;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::map<asio_socket_ptr, bool> _storage_nodes_busy; /* сокет:индикатор_занятости */
    std::map<asio_socket_ptr, boost_process_ptr> _storage_nodes_process; /* сокет:pid_процесса */

    std::thread _worker_thread;

    std::queue<Task> _task_queue;
    std::unordered_map<boost::uuids::uuid, result> _task_results;

    std::shared_ptr<spdlog::logger> _logger;


    void add_storage_node();

    void remove_storage_node(const asio_socket_ptr &socket);

    void start_accept(const boost_process_ptr &child_process_ptr);

    void handle_new_connection(const asio_socket_ptr &socket, const boost_process_ptr &child_process_ptr);

    void async_read_response_header(const asio_socket_ptr &socket, boost::uuids::uuid task_id);

    void async_read_response_body(const asio_socket_ptr &socket, uint32_t length_data, boost::uuids::uuid task_id);

    void async_send_front_task(const asio_socket_ptr &socket);

    void async_send_task(const asio_socket_ptr &socket, Task &&task);

    void shutdown();

public:
    explicit Entrypoint(int count_storage_nodes = entrypoint::MIN_COUNT_STORAGE_NODES);

    void post_task(Task &&task);

    nlohmann::json get_result_by_id(const boost::uuids::uuid& task_id);

    ~Entrypoint();
};


#endif //MINIDB_ENTRYPOINT_H
