//
// Created by rvova on 09.05.2026.
//

#ifndef MINIDB_CLIENT_SOCKET_H
#define MINIDB_CLIENT_SOCKET_H

#include <winsock2.h>
#include <windows.h>

class ClientSocket {
    SOCKET _server_socket = INVALID_SOCKET;

    void shutdown();

public:

    explicit ClientSocket(int port);
    void send_data_to_server(std::string &&data);

    std::string receive_data_from_server();

    ~ClientSocket();
};

#endif //MINIDB_CLIENT_SOCKET_H
