//
// Created by MialistaPC on 26.04.2026.
//

#include "../include/command.h"

// ==================== Helper functions ====================
// Хелперы для сериализации/десериализации vector<Column>
nlohmann::json columns_to_json(const std::vector<Column>& columns) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& col : columns) {
        j.push_back(col.to_json());
    }
    return j;
}

std::vector<Column> columns_from_json(const nlohmann::json& j) {
    std::vector<Column> columns;
    for (const auto& item : j) {
        columns.push_back(Column::from_json(item));
    }
    return columns;
}

// Хелперы для сериализации/десериализации vector<Value>
nlohmann::json values_to_json(const std::vector<Value>& values) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& val : values) {
        j.push_back(value_to_json(val));
    }
    return j;
}

std::vector<Value> values_from_json(const nlohmann::json& j) {
    std::vector<Value> values;
    for (const auto& item : j) {
        values.push_back(value_from_json(item));
    }
    return values;
}


// ==================== Command base ====================

nlohmann::json Command::get_success_message() const {
    nlohmann::json response;
    response["Status"] = 200;
    return response;
}

bool Command::is_database_name_set() const {
    return false;  // базовый класс не имеет database_name
}

bool Command::set_database_name(const std::string& db_name) {
    return false;  // базовый класс не поддерживает установку имени БД
}

void Command::set_token(const std::string& token) {
    _jwt_token = token;
}

std::string Command::get_token() const {
    return _jwt_token;
}

std::string Command::serialize_command() {
    nlohmann::json j;
    j["jwt_token"] = _jwt_token; // Добавляем JWT токен в сериализацию
    return j.dump();
}

// ==================== CreateDatabaseCommand ====================

CreateDatabaseCommand::CreateDatabaseCommand(const std::string &database_name)
    : _database_name(database_name) {
}

nlohmann::json CreateDatabaseCommand::process_command() {
    try {
        StorageEngine::create_database(_database_name);
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
    return get_success_message();
}

std::string CreateDatabaseCommand::serialize_command() {
    nlohmann::json j;
    j["database_name"] = _database_name;

    return j.dump();
}

CreateDatabaseCommand CreateDatabaseCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return CreateDatabaseCommand(json["database_name"]);
}

nlohmann::json CreateDatabaseCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "The database " + _database_name + " successfully created!";
    response["Status"] = 200;
    return response;
}

// ==================== DropDatabaseCommand ====================

DropDatabaseCommand::DropDatabaseCommand(const std::string &database_name)
    : _database_name(database_name) {
}

nlohmann::json DropDatabaseCommand::process_command() {
    try {
        StorageEngine::drop_database(_database_name);
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
    return get_success_message();
}

std::string DropDatabaseCommand::serialize_command() {
    nlohmann::json j;
    j["database_name"] = _database_name;

    return j.dump();
}

DropDatabaseCommand DropDatabaseCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return DropDatabaseCommand(json["database_name"]);
}

nlohmann::json DropDatabaseCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "The database " + _database_name + " successfully dropped!";
    response["Status"] = 200;
    return response;
}

// ==================== UseDatabaseCommand ====================

UseDatabaseCommand::UseDatabaseCommand(const std::string &database_name)
    : _database_name(database_name) {
}

nlohmann::json UseDatabaseCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "USE command should be handle in main_server, not storage engine!";
    response["Status"] = 500;
    return response;
}

std::string UseDatabaseCommand::serialize_command() {
    nlohmann::json j;
    j["database_name"] = _database_name;

    return j.dump();
}

UseDatabaseCommand UseDatabaseCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return UseDatabaseCommand(json["database_name"]);
}

nlohmann::json UseDatabaseCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Switched to database " + _database_name;
    response["Status"] = 200;
    return response;
}

// ==================== CreateTableCommand ====================

CreateTableCommand::CreateTableCommand(const std::string &table_name, const std::vector<Column> &columns,
                                        const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _columns(columns) {
}

bool CreateTableCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool CreateTableCommand::set_database_name(const std::string& db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json CreateTableCommand::process_command() {
    try {
        // Всегда передаем _database_name (может быть пустой строкой)
        StorageEngine::create_table(_database_name, _table_name, _columns);
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
    return get_success_message();
}

std::string CreateTableCommand::serialize_command() {
    nlohmann::json j;
    if (is_database_name_set()) {
        j["database_name"] = _database_name;
    }
    j["table_name"] = _table_name;
    j["columns"] = columns_to_json(_columns);

    return j.dump();
}

CreateTableCommand CreateTableCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    const std::vector<Column> columns = columns_from_json(json["columns"]);

    if (json.contains("database_name")) {
        return CreateTableCommand(json["table_name"], columns, json["database_name"]);
    }
    return CreateTableCommand(json["table_name"], columns);
}

nlohmann::json CreateTableCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "The table " + _table_name + " successfully created!";
    response["Status"] = 200;
    return response;
}

// ==================== DropTableCommand ====================

DropTableCommand::DropTableCommand(const std::string &table_name, const std::string &database_name)
    : _database_name(database_name), _table_name(table_name) {
}

bool DropTableCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool DropTableCommand::set_database_name(const std::string& db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json DropTableCommand::process_command() {
    try {
        // Всегда передаем _database_name (может быть пустой строкой)
        StorageEngine::drop_table(_database_name, _table_name);
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
    return get_success_message();
}

std::string DropTableCommand::serialize_command() {
    nlohmann::json j;
    if (is_database_name_set()) {
        j["database_name"] = _database_name;
    }
    j["table_name"] = _table_name;

    return j.dump();
}

DropTableCommand DropTableCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);

    if (json.contains("database_name")) {
        return DropTableCommand(json["table_name"], json["database_name"]);
    }
    return DropTableCommand(json["table_name"]);
}

nlohmann::json DropTableCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "The table " + _table_name + " successfully dropped!";
    response["Status"] = 200;
    return response;
}

// ==================== InsertIntoCommand ====================

InsertIntoCommand::InsertIntoCommand(const std::string &table_name, const std::vector<Column> &columns,
                                     const std::vector<Value> &values, const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _columns(columns), _values(values) {
}

bool InsertIntoCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool InsertIntoCommand::set_database_name(const std::string& db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json InsertIntoCommand::process_command() {
    try {
        // Всегда передаем _database_name (может быть пустой строкой)
        StorageEngine::insert_elements(_database_name, _table_name, _columns, _values);
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
    return get_success_message();
}

std::string InsertIntoCommand::serialize_command() {
    nlohmann::json j;
    if (is_database_name_set()) {
        j["database_name"] = _database_name;
    }
    j["table_name"] = _table_name;
    j["columns"] = columns_to_json(_columns);
    j["values"] = values_to_json(_values);

    return j.dump();
}

InsertIntoCommand InsertIntoCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    const std::vector<Column> columns = columns_from_json(json["columns"]);
    const std::vector<Value> values = values_from_json(json["values"]);

    if (json.contains("database_name")) {
        return InsertIntoCommand(json["table_name"], columns, values, json["database_name"]);
    }
    return InsertIntoCommand(json["table_name"], columns, values);
}

nlohmann::json InsertIntoCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Successfully inserted into " + _table_name;
    response["Status"] = 200;
    return response;
}

// ==================== UpdateCommand ====================

UpdateCommand::UpdateCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                             const std::vector<Column> &columns, const std::vector<Value> &values,
                             const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _condition(std::move(condition)),
      _columns(columns), _values(values) {
}

bool UpdateCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool UpdateCommand::set_database_name(const std::string& db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json UpdateCommand::process_command() {
    try {
        // Всегда передаем _database_name (может быть пустой строкой)
        StorageEngine::update_elements(_database_name, _table_name, std::move(_condition), _columns, _values);
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
    return get_success_message();
}

std::string UpdateCommand::serialize_command() {
    nlohmann::json j;
    if (is_database_name_set()) {
        j["database_name"] = _database_name;
    }
    j["table_name"] = _table_name;
    j["condition"] = _condition->to_json();
    j["columns"] = columns_to_json(_columns);
    j["values"] = values_to_json(_values);

    return j.dump();
}

UpdateCommand UpdateCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    std::vector<Column> columns = columns_from_json(json["columns"]);
    std::vector<Value> values = values_from_json(json["values"]);
    auto condition = Condition::from_json(json["condition"]);

    if (json.contains("database_name")) {
        return UpdateCommand(json["table_name"], std::move(condition), columns, values, json["database_name"]);
    }
    return UpdateCommand(json["table_name"], std::move(condition), columns, values);
}

nlohmann::json UpdateCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Successfully updated " + _table_name;
    response["Status"] = 200;
    return response;
}

// ==================== DeleteFromCommand ====================

DeleteFromCommand::DeleteFromCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                                     const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _condition(std::move(condition)) {
}

bool DeleteFromCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool DeleteFromCommand::set_database_name(const std::string& db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json DeleteFromCommand::process_command() {
    try {
        // Всегда передаем _database_name (может быть пустой строкой)
        StorageEngine::delete_elements(_database_name, _table_name, std::move(_condition));
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
    return get_success_message();
}

std::string DeleteFromCommand::serialize_command() {
    nlohmann::json j;
    if (is_database_name_set()) {
        j["database_name"] = _database_name;
    }
    j["table_name"] = _table_name;
    j["condition"] = _condition->to_json();

    return j.dump();
}

DeleteFromCommand DeleteFromCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    auto condition = Condition::from_json(json["condition"]);

    if (json.contains("database_name")) {
        return DeleteFromCommand(json["table_name"], std::move(condition), json["database_name"]);
    }
    return DeleteFromCommand(json["table_name"], std::move(condition));
}

nlohmann::json DeleteFromCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Successfully deleted from " + _table_name;
    response["Status"] = 200;
    return response;
}

// ==================== SelectCommand ====================

SelectCommand::SelectCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                             const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _condition(std::move(condition)) {
}

SelectCommand::SelectCommand(const std::string &table_name, const std::vector<Column> &columns,
                             std::unique_ptr<Condition> condition, const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _columns(columns), _condition(std::move(condition)) {
}

SelectCommand::SelectCommand(const std::string &table_name, const std::vector<Column> &columns,
                             const std::vector<std::string> &aliases, std::unique_ptr<Condition> condition,
                             const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _columns(columns), _aliases(aliases),
      _condition(std::move(condition)) {
}

bool SelectCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool SelectCommand::set_database_name(const std::string& db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json SelectCommand::process_command() {
    try {
        // TODO:
        // if (_columns.has_value() && _aliases.has_value()) {
        //     // Всегда передаем _database_name (может быть пустой строкой)
        //     return StorageEngine::select_elements(_database_name, _table_name, _columns.value(), _aliases.value(), *_condition);
        // } else if (_columns.has_value()) {
        //     return StorageEngine::select_elements(_database_name, _table_name, _columns.value(), *_condition);
        // } else {
        //     auto rows = StorageEngine::select_elements(_database_name, _table_name, std::move(_condition));
        // }
        auto rows = StorageEngine::select_elements(_database_name, _table_name, std::move(_condition));
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }

    return get_success_message(); // TODO: edit!
}

std::string SelectCommand::serialize_command() {
    nlohmann::json j;
    if (is_database_name_set()) {
        j["database_name"] = _database_name;
    }
    j["table_name"] = _table_name;
    if (_condition) {
        j["condition"] = _condition->to_json();
    }

    if (_columns.has_value()) {
        j["columns"] = columns_to_json(_columns.value());
    }
    if (_aliases.has_value()) {
        j["aliases"] = _aliases.value();
    }

    return j.dump();
}

SelectCommand SelectCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    auto condition = Condition::from_json(json["condition"]);
    std::string database_name = json.contains("database_name") ? json["database_name"].get<std::string>() : "";

    if (json.contains("columns") && json.contains("aliases")) {
        std::vector<Column> columns = columns_from_json(json["columns"]);
        std::vector<std::string> aliases = json["aliases"].get<std::vector<std::string>>();
        return SelectCommand(json["table_name"], columns, aliases, std::move(condition), database_name);
    } else if (json.contains("columns")) {
        std::vector<Column> columns = columns_from_json(json["columns"]);
        return SelectCommand(json["table_name"], columns, std::move(condition), database_name);
    } else {
        return SelectCommand(json["table_name"], std::move(condition), database_name);
    }
}

nlohmann::json SelectCommand::get_success_message() const {
    nlohmann::json response;
    response["Status"] = 200;
    return response;
}