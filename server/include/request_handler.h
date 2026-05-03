//
// Created by rvova on 01.05.2026.
//

#ifndef MINIDB_REQUEST_HANDLER_H
#define MINIDB_REQUEST_HANDLER_H

#include <nlohmann/json.hpp>

#include "command.h"
#include "storage_engine.h"

class RequestHandler {
    StorageEngine _engine;
public:
    RequestHandler(StorageEngine&& engine);

    nlohmann::json process_request(const std::vector<std::unique_ptr<Command> >& commands);
};


#endif //MINIDB_REQUEST_HANDLER_H
