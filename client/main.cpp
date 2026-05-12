//
// Created by rvova on 08.05.2026.
//

#include <iostream>

#include "client_socket.h"

void help() {
    std::cout << "Usage: ./client PORT\n";
}

void print_message_input_command() {
    std::cout << "Please input your command (Ctrl+C for exit):\n";
}

int main(const int argc, char *argv[]) {
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

    ClientSocket client_socket{};
    const auto is_running = client_socket.run(port);
    if (!is_running) {
        std::cout << "Error while run client socket! Please check logs\n";
    }

    print_message_input_command();
    std::string data;
    while (std::getline(std::cin, data)) {
        client_socket.send_request(std::move(data));
        const auto response = client_socket.read_response();
        std::cout << response << "\n";
        print_message_input_command();
    }

    client_socket.stop();
    return 0;
}
