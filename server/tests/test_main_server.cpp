//
// Created by rvova on 25.04.2026.
//

#include <iostream>

#include "server.h"

int main() {
    auto server = Server();
    auto ping = server.ping();

    std::cout << ping << std::endl;
    return 0;
}
