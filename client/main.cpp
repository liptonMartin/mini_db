//
// Created by rvova on 08.05.2026.
//

#include <winsock2.h>
#include <iostream>
#include <ws2tcpip.h>
#include <nlohmann/json.hpp>

#include "exceptions.h"


class ClientSocket {
    SOCKET _server_socket = INVALID_SOCKET;

    void shutdown() {
        if (_server_socket != INVALID_SOCKET) {
            closesocket(_server_socket);
            _server_socket = INVALID_SOCKET;
        }

        WSACleanup();
        std::cout << "The client socket was shutdown!";
    }

public:
    explicit ClientSocket(const int port) {
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

    void send_data_to_server(std::string &&data) {
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

    std::string receive_data_from_server() {
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

    ~ClientSocket() {
        shutdown();
    }
};


void help() {
    std::cout << "Usage: ./client PORT\n";
}

void print_message_input_command() {
    std::cout << "Please input your command (Ctrl+C for exit):\n";
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        help();
        return -1;
    }

    int port;
    try {
        port = std::stoi(argv[1]);
    } catch (std::exception &) {
        std::cout << "Invalid format for PORT\n";
        help();
        return -1;
    }

    ClientSocket client_socket(port);

    print_message_input_command();
    std::string data;
    while (std::getline(std::cin, data)) {
        client_socket.send_data_to_server(std::move(data));
        std::string response;
        try {
            response = client_socket.receive_data_from_server();
        } catch (InternalServerErrorException &e) {
            std::cout << e.what() << "\n";
            return 1;
        }

        auto json_response = nlohmann::json::parse(response);
        std::cout << json_response << "\n";
        print_message_input_command();
    }

    return 0;
}
