//
// Created by rvova on 28.04.2026.
//

#include <iostream>
#include <nlohmann/json.hpp>
#include <boost/uuid.hpp>

#include "entrypoint.h"
#include "exceptions.h"
#include "server_socket.h"

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
    /* создаем балансировщик, в своем потоке будет все выполнять */
    Entrypoint entrypoint{};

    /* создаем генератор uuid */
    boost::uuids::random_generator generator{};

    /* в этом же потоке выполняет все операции */
    ServerSocket server_socket{entrypoint, generator, port};

    return 0;
}
