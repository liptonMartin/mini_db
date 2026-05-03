//
// Created by MialistaPC on 26.04.2026.
//

#include "../include/command.h"

nlohmann::json Command::get_success_message() const {
    return nlohmann::json::parse("{\"Response\": \"Success!\"}");
}

CreateDatabaseCommand::CreateDatabaseCommand(const std::string &database_name)
    : _database_name(database_name) {
}

nlohmann::json CreateDatabaseCommand::process_command(StorageEngine &engine) {
    try {
        engine.create_database(_database_name);
    } catch (const std::exception &e) {
        auto message = "{\"Response\": \"" + std::string(e.what()) + "\"}";
        return nlohmann::json::parse(message);
    }
    return get_success_message();
}

DropDatabaseCommand::DropDatabaseCommand(const std::string &database_name)
    : _database_name(database_name) {
}

nlohmann::json DropDatabaseCommand::process_command(StorageEngine &engine) {
    try {
        engine.drop_database(_database_name);
    } catch (const std::exception &e) {
        auto message = "{\"Response\": \"" + std::string(e.what()) + "\"}";
        return nlohmann::json::parse(message);
    }
    return get_success_message();
}
