//
// Created by rvova on 09.05.2026.
//

#include <ws2tcpip.h>
#include <winsock2.h>
#include <iostream>
#include <cstdint>

#include "client_socket.h"
#include "exceptions.h"

ClientSocket::ClientSocket(const int port) {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cout << "WSAStartup failed\n";
        throw FailedStartSocketException();
    }

    _server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_server_socket == INVALID_SOCKET) {
        std::cout << "Could not create socket\n";
        WSACleanup();
        throw FailedStartSocketException();
    }

    sockaddr_in client_address{};
    client_address.sin_family = AF_INET;
    InetPton(AF_INET, "127.0.0.1", &client_address.sin_addr);
    client_address.sin_port = htons(port);

    if (connect(_server_socket, reinterpret_cast<sockaddr *>(&client_address), sizeof(client_address))) {
        std::cout << "Could not connect to server\n";
        shutdown();
        throw FailedStartSocketException();
    }

    std::cout << "Connected to server\n";
}

void ClientSocket::send_data_to_server(std::string &&data) {
    if (_server_socket == INVALID_SOCKET) {
        std::cout << "Could not connect to server\n";
        shutdown();
        throw FailedSendDataException();
    }
    /* сначала отправляем длину сообщения */
    const uint32_t length_data = data.length();
    const auto res = send(_server_socket, reinterpret_cast<const char *>(&length_data), sizeof(length_data), 0);
    if (res == SOCKET_ERROR) {
        std::cout << "Failed to send data to server\n";
        shutdown();
        throw FailedSendDataException();
    }

    /* затем отправляем сами данные */
    if (send(_server_socket, data.c_str(), static_cast<int>(data.length()), 0) == SOCKET_ERROR) {
        std::cout << "Failed to send data to server\n";
        shutdown();
        throw FailedSendDataException();
    }
    std::cout << "Data was sent to server successfully\n";
}

std::string ClientSocket::receive_data_from_server() {
    if (_server_socket == INVALID_SOCKET) {
        std::cout << "Could not connect to server\n";
        shutdown();
        throw FailedReceiveDataException();
    }

    /* сначала считываем длину сообщения */
    uint32_t length_data;
    int count_bytes = recv(_server_socket, reinterpret_cast<char *>(&length_data), sizeof(length_data), 0);
    if (count_bytes == SOCKET_ERROR) {
        std::cout << "Failed to receive data from server\n";
        shutdown();
        throw FailedReceiveDataException();
    }
    if (count_bytes == 0) {
        std::cout << "Server has internal error!\n";
        shutdown();
        throw InternalServerErrorException();
    }
    if (length_data == 0) {
        std::cout << "Got empty message\n";
        return "";
    }

    std::string data;
    data.resize(length_data);
    count_bytes = recv(_server_socket, data.data(), static_cast<int>(data.length()), 0);
    if (count_bytes == SOCKET_ERROR) {
        std::cout << "Failed to receive data from server\n";
        shutdown();
        throw FailedReceiveDataException();
    }
    if (count_bytes == 0) {
        std::cout << "Server has internal error!\n";
        shutdown();
        throw InternalServerErrorException();
    }
    return data;
}

void ClientSocket::shutdown() {
    if (_server_socket != INVALID_SOCKET) {
        closesocket(_server_socket);
        _server_socket = INVALID_SOCKET;
    }

    WSACleanup();
    std::cout << "The client socket was shutdown!";
}

ClientSocket::~ClientSocket() {
    shutdown();
}
