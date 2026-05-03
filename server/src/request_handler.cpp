//
// Created by rvova on 01.05.2026.
//

#include "request_handler.h"

RequestHandler::RequestHandler(StorageEngine &&engine) {
    _engine = std::move(engine);
}

nlohmann::json RequestHandler::process_request(const std::vector<std::unique_ptr<Command> > &commands) {
    nlohmann::json response;

    for (const auto &command: commands) {
        auto command_response = command->process_command(_engine);
        response.push_back(command_response);
    }

    return response;
}
