//
// Created by rvova on 25.04.2026.
//

#ifndef MINI_DB_SERVER_H
#define MINI_DB_SERVER_H

#include <nlohmann/json.hpp>

class Server {
public:
    nlohmann::json ping();

private:

};


#endif //MINI_DB_SERVER_H
