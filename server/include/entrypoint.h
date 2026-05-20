//
// Created by rvova on 09.05.2026.
//

#ifndef MINIDB_ENTRYPOINT_H
#define MINIDB_ENTRYPOINT_H


#include <queue>
#include <boost/process/v1/child.hpp>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/uuid.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "command.h"
#include "constants.h"

using AsioSocket = boost::asio::ip::tcp::socket;
using AsioSocketPtr = std::shared_ptr<AsioSocket>;

using BoostProcess = boost::process::v1::child;
using BoostProcessPtr = std::shared_ptr<BoostProcess>;

using Result = nlohmann::json;

using CommandPtr = std::unique_ptr<Command>;


struct Task {
    boost::uuids::uuid task_uuid;
    CommandType command_type;
    CommandPtr command;
};

class Entrypoint {
    std::mutex _task_results_mutex;

    boost::asio::io_context _io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _work_guard;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::map<AsioSocketPtr, bool> _storage_nodes_busy; /* сокет:индикатор_занятости */
    std::map<AsioSocketPtr, BoostProcessPtr> _storage_nodes_process; /* сокет:pid_процесса */

    /* heartbeat */
    boost::asio::ip::tcp::acceptor _heartbeat_acceptor;
    std::map<AsioSocketPtr, AsioSocketPtr> _storage_heartbeat_sockets; /* main_socket -> heartbeat_socket */
    std::map<BoostProcessPtr, AsioSocketPtr> _pending_heartbeat_sockets; /* process -> heartbeat_socket (до main-connection) */
    boost::asio::deadline_timer _heartbeat_timer;

    std::thread _worker_thread;

    std::queue<Task> _task_queue;
    std::unordered_map<boost::uuids::uuid, Result> _task_results;

    std::shared_ptr<spdlog::logger> _logger;


    void add_storage_node();

    void remove_storage_node(const AsioSocketPtr &socket);

    void start_accept(const BoostProcessPtr &child_process_ptr);

    void handle_new_connection(const AsioSocketPtr &socket, const BoostProcessPtr &child_process_ptr);

    void async_read_response_header(const AsioSocketPtr &socket, boost::uuids::uuid task_id);

    void async_read_response_body(const AsioSocketPtr &socket, uint32_t length_data, boost::uuids::uuid task_id);

    void async_send_next_task(const AsioSocketPtr &socket);

    void async_send_task(const AsioSocketPtr &socket, Task &&task);

    /* heartbeat */
    void start_heartbeat_accept(const BoostProcessPtr &child_process_ptr);

    void handle_heartbeat_accept(const AsioSocketPtr &heartbeat_socket, const BoostProcessPtr &child_process_ptr);

    void start_heartbeat_timer();

    void check_heartbeats();

    void async_send_heartbeat_ping(const AsioSocketPtr &socket, const AsioSocketPtr &heartbeat_socket);

    void async_read_heartbeat_response(const AsioSocketPtr &socket, const AsioSocketPtr &heartbeat_socket);

    void restart_storage_node(const AsioSocketPtr &socket);

    void shutdown();

public:
    explicit Entrypoint(int count_storage_nodes = entrypoint::MIN_COUNT_STORAGE_NODES);

    void post_task(Task &&task);

    nlohmann::json get_result_by_id(const boost::uuids::uuid& task_id);

    ~Entrypoint();
};


#endif //MINIDB_ENTRYPOINT_H
