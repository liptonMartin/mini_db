//
// Created by rvova on 25.04.2026.
//
#include "server.h"


nlohmann::json Server::ping() {
    auto message = R"({"Pong": 123})";
    return nlohmann::json::parse(message);
}
