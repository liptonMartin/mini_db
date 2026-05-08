//
// Created by rvova on 28.04.2026.
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#include "exceptions.h"

class ServerSocket {
    SOCKET _listen_socket = INVALID_SOCKET;
    SOCKET _client_socket = INVALID_SOCKET;

    void shutdown() {
        if (_listen_socket != INVALID_SOCKET) {
            closesocket(_listen_socket);
            _listen_socket = INVALID_SOCKET;
        }
        if (_client_socket != INVALID_SOCKET) {
            closesocket(_client_socket);
            _client_socket = INVALID_SOCKET;
        }
        WSACleanup();
        std::cout << "The server shutdown";
    }

public:
    explicit ServerSocket(const int port) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            /* инициализация WSADATA версии 2.2 */
            std::cout << "WSAStartup failed!\n";
            throw FailedStartSocketException();
        }
        /* создаем сокет */
        _listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_listen_socket == INVALID_SOCKET) {
            std::cout << "Socket creation failed!\n";
            WSACleanup();
            throw FailedStartSocketException();
        }

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        InetPton(AF_INET, "127.0.0.1", &server_address.sin_addr);
        server_address.sin_port = htons(port);

        if (bind(_listen_socket, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address)) ==
            SOCKET_ERROR) {
            std::cout << "Bind failed!\n";
            closesocket(_listen_socket);
            WSACleanup();
            throw FailedStartSocketException();
        }
    }

    void start_listen_socket() {
        if (listen(_listen_socket, 1) == SOCKET_ERROR) {
            std::cout << "Listen failed!\n";
            shutdown();
            throw FailedStartSocketException();
        }
        std::cout << "Start listening socket!\n";
        _client_socket = accept(_listen_socket, nullptr, nullptr);
        if (_client_socket == INVALID_SOCKET) {
            std::cout << "Accept failed!\n";
            shutdown();
            throw FailedStartSocketException();
        }
        std::cout << "Server accepted!\n";
    }

    /**
     *
     * @param request Строка, в которую нужно положить данные, отправленные пользователем
     * @return Количество считанных байт
     */
    int get_client_request(std::string &request) {
        /* считываем длину сообщения */
        size_t length_data;
        if (recv(_client_socket, reinterpret_cast<char *>(&length_data), sizeof(size_t), 0) == SOCKET_ERROR) {
            std::cout << "Receive data from client failed!\n";
            shutdown();
            throw FailedReceiveDataException();
        }

        /* считываем само сообщение */
        request.resize(length_data);
        const int count_bytes = recv(_client_socket, request.data(), static_cast<int>(request.size()), 0);
        if (count_bytes == SOCKET_ERROR) {
            std::cout << "Receive data from client failed!\n";
            shutdown();
            throw FailedReceiveDataException();
        }
        return count_bytes;
    }

    void send_response_to_client(const std::string &response) {
        /* сначала отправляем длину ответа */
        const size_t length_data = response.size();
        if (send(_client_socket, reinterpret_cast<const char *>(&length_data), sizeof(length_data) , 0) == SOCKET_ERROR) {
            std::cout << "Send data to client failed!\n";
            shutdown();
            throw FailedSendDataException();
        }

        /* затем отправляем сами данные */
        if (send(_client_socket, response.data(), static_cast<int>(length_data), 0) == SOCKET_ERROR) {
            std::cout << "Send data to client failed!\n";
            shutdown();
            throw FailedSendDataException();
        }
    }

    ~ServerSocket() {
        shutdown();
    }
};

void help() {
    std::cout << "Usage: ./main_server PORT\n";
}

int main(int argc, char **argv) {
    if (argc != 2) {
        help();
        return -1;
    }

    int port;
    try {
        port = std::stoi(argv[1]);
    } catch (std::exception &) {
        std::cout << "Invalid format port!\n";
        help();
        return -1;
    }
    ServerSocket server_socket(port);
    server_socket.start_listen_socket();

    std::string request;
    while (server_socket.get_client_request(request) != 0) {
        // TODO: handle client request
        server_socket.send_response_to_client(request);
    }

    return 0;
}
