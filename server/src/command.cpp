//
// Created by MialistaPC on 26.04.2026.
//

#include "../include/command.h"

// ==================== Helper functions ====================
// Хелперы для сериализации/десериализации vector<Column>
nlohmann::json columns_to_json(const std::vector<Column> &columns) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto &col: columns) {
        j.push_back(col.to_json());
    }
    return j;
}

std::vector<Column> columns_from_json(const nlohmann::json &j) {
    std::vector<Column> columns;
    for (const auto &item: j) {
        columns.push_back(Column::from_json(item));
    }
    return columns;
}

// Хелперы для сериализации/десериализации vector<Value>
nlohmann::json values_to_json(const std::vector<Value> &values) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto &val: values) {
        j.push_back(value_to_json(val));
    }
    return j;
}

std::vector<Value> values_from_json(const nlohmann::json &j) {
    std::vector<Value> values;
    for (const auto &item: j) {
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
    return false; // базовый класс не имеет database_name
}

bool Command::set_database_name(const std::string &db_name) {
    return false; // базовый класс не поддерживает установку имени БД
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

CreateDatabaseCommand::CreateDatabaseCommand(const std::string &database_name,
                                             const std::vector<std::string> &group_names)
    : _database_name(database_name), _group_names(group_names) {
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
    if (!_group_names.empty()) {
        j["group_names"] = _group_names;
    }

    return j.dump();
}

CreateDatabaseCommand CreateDatabaseCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    if (json.contains("group_names")) {
        return CreateDatabaseCommand(json["database_name"], json["group_names"].get<std::vector<std::string>>());
    }
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

bool CreateTableCommand::set_database_name(const std::string &db_name) {
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

bool DropTableCommand::set_database_name(const std::string &db_name) {
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

InsertIntoCommand::InsertIntoCommand(const std::string &table_name, const std::vector<std::string> &column_names,
                                     const std::vector<Value> &values, const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _column_names(column_names), _values(values) {
}

bool InsertIntoCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool InsertIntoCommand::set_database_name(const std::string &db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json InsertIntoCommand::process_command() {
    try {
        // Всегда передаем _database_name (может быть пустой строкой)
        StorageEngine::insert_elements(_database_name, _table_name, _column_names, _values);
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
    j["columns"] = _column_names;
    j["values"] = values_to_json(_values);

    return j.dump();
}

InsertIntoCommand InsertIntoCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    const std::vector<std::string> columns = json["columns"];
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
                             const std::vector<std::string> &column_names, const std::vector<Value> &values,
                             const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _condition(std::move(condition)),
      _column_names(column_names), _values(values) {
}

bool UpdateCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool UpdateCommand::set_database_name(const std::string &db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json UpdateCommand::process_command() {
    try {
        // Всегда передаем _database_name (может быть пустой строкой)
        StorageEngine::update_elements(_database_name, _table_name, std::move(_condition), _column_names, _values);
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
    if (_condition) {
        j["condition"] = _condition->to_json();
    }
    j["columns"] = _column_names;
    j["values"] = values_to_json(_values);

    return j.dump();
}

UpdateCommand UpdateCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    std::vector<std::string> columns = json["columns"];
    std::vector<Value> values = values_from_json(json["values"]);

    std::unique_ptr<Condition> condition = nullptr;
    if (json.contains("condition")) {
        condition = Condition::from_json(json["condition"]);
    }

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

bool DeleteFromCommand::set_database_name(const std::string &db_name) {
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
    if (_condition) {
        j["condition"] = _condition->to_json();
    }

    return j.dump();
}

DeleteFromCommand DeleteFromCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);

    std::unique_ptr<Condition> condition = nullptr;
    if (json.contains("condition")) {
        condition = Condition::from_json(json["condition"]);
    }

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

static std::string agg_func_to_string(AggregateFunction func) {
    switch (func) {
        case AggregateFunction::Sum: return "sum";
        case AggregateFunction::Count: return "count";
        case AggregateFunction::Avg: return "avg";
    }
    return "";
}

static AggregateFunction string_to_agg_func(const std::string &s) {
    if (s == "sum") return AggregateFunction::Sum;
    if (s == "count") return AggregateFunction::Count;
    if (s == "avg") return AggregateFunction::Avg;
    throw std::runtime_error("Unknown aggregate function: " + s);
}

static nlohmann::json select_target_to_json(const SelectTarget &t) {
    nlohmann::json j;
    if (t.agg_func.has_value()) {
        j["type"] = "aggregate";
        j["func"] = agg_func_to_string(t.agg_func.value());
        j["column_name"] = t.column_name;
    } else {
        j["type"] = "column";
        j["column_name"] = t.column_name;
        if (t.alias.has_value()) {
            j["alias"] = t.alias.value();
        }
    }
    return j;
}

static SelectTarget select_target_from_json(const nlohmann::json &j) {
    SelectTarget t;
    if (j.at("type") == "aggregate") {
        t.agg_func = string_to_agg_func(j.at("func"));
        t.column_name = j.value("column_name", "");
    } else {
        t.column_name = j.at("column_name");
        if (j.contains("alias")) {
            t.alias = j["alias"].get<std::string>();
        }
    }
    return t;
}

SelectCommand::SelectCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                             const std::vector<SelectTarget> &select_targets,
                             const std::string &database_name)
    : _database_name(database_name), _table_name(table_name), _select_targets(select_targets),
      _condition(std::move(condition)) {
}

void SelectCommand::add_column(const std::string &column_name, const Alias &alias) {
    _select_targets.push_back({column_name, alias, std::nullopt});
}

void SelectCommand::add_aggregate(AggregateFunction func, const std::string &column_name) {
    _select_targets.push_back({column_name, std::nullopt, func});
}

const std::vector<SelectTarget> &SelectCommand::get_select_targets() const {
    return _select_targets;
}

bool SelectCommand::is_database_name_set() const {
    return !_database_name.empty();
}

bool SelectCommand::set_database_name(const std::string &db_name) {
    if (_database_name.empty() && !db_name.empty()) {
        _database_name = db_name;
        return true;
    }
    return false;
}

nlohmann::json SelectCommand::process_command() {
    try {
        /* собираем map колонок для storage engine (только обычные колонки, без агрегатов) */
        std::optional<std::unordered_map<std::string, Alias>> columns_map = std::nullopt;
        for (const auto &t : _select_targets) {
            if (!t.agg_func.has_value()) {
                if (!columns_map.has_value()) columns_map = std::unordered_map<std::string, Alias>();
                (*columns_map)[t.column_name] = t.alias;
            }
        }

        return StorageEngine::select_elements(_database_name, _table_name, columns_map,
                                              std::move(_condition));
    } catch (const std::exception &e) {
        nlohmann::json response;
        response["Message"] = e.what();
        response["Status"] = 500;
        return response;
    }
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
    j["select_targets"] = nlohmann::json::array();
    for (const auto &t : _select_targets) {
        j["select_targets"].push_back(select_target_to_json(t));
    }
    return j.dump();
}

SelectCommand SelectCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    std::unique_ptr<Condition> condition = nullptr;

    if (json.contains("condition")) {
        condition = Condition::from_json(json["condition"]);
    }

    std::string database_name = json.contains("database_name") ? json["database_name"].get<std::string>() : "";

    std::vector<SelectTarget> targets;
    if (json.contains("select_targets")) {
        for (const auto &item : json["select_targets"]) {
            targets.push_back(select_target_from_json(item));
        }
    }

    return SelectCommand(json["table_name"], std::move(condition), targets, database_name);
}

nlohmann::json SelectCommand::get_success_message() const {
    // TODO: edit!
    nlohmann::json response;
    response["Status"] = 200;
    return response;
}

// ==================== CreateUserCommand ====================

CreateUserCommand::CreateUserCommand(const std::string &username, const std::string &password)
    : _username(username), _password(password) {
}

nlohmann::json CreateUserCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "CreateUserCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string CreateUserCommand::serialize_command() {
    nlohmann::json j;
    j["username"] = _username;
    j["password"] = _password;
    return j.dump();
}

CreateUserCommand CreateUserCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return CreateUserCommand(json["username"], json["password"]);
}

nlohmann::json CreateUserCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "User " + _username + " created";
    response["Status"] = 200;
    return response;
}

// ==================== AuthCommand ====================

AuthCommand::AuthCommand(const std::string &username, const std::string &password)
    : _username(username), _password(password) {
}

nlohmann::json AuthCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AuthCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AuthCommand::serialize_command() {
    nlohmann::json j;
    j["username"] = _username;
    j["password"] = _password;
    return j.dump();
}

AuthCommand AuthCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AuthCommand(json["username"], json["password"]);
}

nlohmann::json AuthCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Auth successful";
    response["Status"] = 200;
    return response;
}

// ==================== AlterUserAddToGroupCommand ====================

AlterUserAddToGroupCommand::AlterUserAddToGroupCommand(const std::string &username, const std::string &database_name,
                                                       const std::string &group_name)
    : _username(username), _database_name(database_name), _group_name(group_name) {
}

nlohmann::json AlterUserAddToGroupCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AlterUserAddToGroupCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AlterUserAddToGroupCommand::serialize_command() {
    nlohmann::json j;
    j["username"] = _username;
    j["database_name"] = _database_name;
    j["group_name"] = _group_name;
    return j.dump();
}

AlterUserAddToGroupCommand AlterUserAddToGroupCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AlterUserAddToGroupCommand(json["username"], json["database_name"], json["group_name"]);
}

nlohmann::json AlterUserAddToGroupCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "User " + _username + " added to group " + _group_name;
    response["Status"] = 200;
    return response;
}

// ==================== AlterUserRemoveFromGroupCommand ====================

AlterUserRemoveFromGroupCommand::AlterUserRemoveFromGroupCommand(const std::string &username,
                                                                 const std::string &database_name,
                                                                 const std::string &group_name)
    : _username(username), _database_name(database_name), _group_name(group_name) {
}

nlohmann::json AlterUserRemoveFromGroupCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AlterUserRemoveFromGroupCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AlterUserRemoveFromGroupCommand::serialize_command() {
    nlohmann::json j;
    j["username"] = _username;
    j["database_name"] = _database_name;
    j["group_name"] = _group_name;
    return j.dump();
}

AlterUserRemoveFromGroupCommand AlterUserRemoveFromGroupCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AlterUserRemoveFromGroupCommand(json["username"], json["database_name"], json["group_name"]);
}

nlohmann::json AlterUserRemoveFromGroupCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "User " + _username + " removed from group " + _group_name;
    response["Status"] = 200;
    return response;
}

// ==================== AlterUserAddPermissionCommand ====================

AlterUserAddPermissionCommand::AlterUserAddPermissionCommand(const std::string &username,
                                                             const std::string &database_name,
                                                             const std::vector<std::string> &permissions)
    : _username(username), _database_name(database_name), _permissions(permissions) {
}

nlohmann::json AlterUserAddPermissionCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AlterUserAddPermissionCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AlterUserAddPermissionCommand::serialize_command() {
    nlohmann::json j;
    j["username"] = _username;
    j["database_name"] = _database_name;
    j["permissions"] = _permissions;
    return j.dump();
}

AlterUserAddPermissionCommand AlterUserAddPermissionCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AlterUserAddPermissionCommand(json["username"], json["database_name"],
                                         json["permissions"].get<std::vector<std::string>>());
}

nlohmann::json AlterUserAddPermissionCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Permissions added to user " + _username;
    response["Status"] = 200;
    return response;
}

// ==================== AlterGroupAddPermissionCommand ====================

AlterGroupAddPermissionCommand::AlterGroupAddPermissionCommand(const std::string &group_name,
                                                               const std::vector<std::string> &permissions)
    : _group_name(group_name), _permissions(permissions) {
}

nlohmann::json AlterGroupAddPermissionCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AlterGroupAddPermissionCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AlterGroupAddPermissionCommand::serialize_command() {
    nlohmann::json j;
    j["group_name"] = _group_name;
    j["permissions"] = _permissions;
    return j.dump();
}

AlterGroupAddPermissionCommand AlterGroupAddPermissionCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AlterGroupAddPermissionCommand(json["group_name"],
                                          json["permissions"].get<std::vector<std::string>>());
}

nlohmann::json AlterGroupAddPermissionCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Permissions added to group " + _group_name;
    response["Status"] = 200;
    return response;
}

// ==================== AlterGroupDeletePermissionCommand ====================

AlterGroupDeletePermissionCommand::AlterGroupDeletePermissionCommand(const std::string &group_name,
                                                                     const std::vector<std::string> &permissions)
    : _group_name(group_name), _permissions(permissions) {
}

nlohmann::json AlterGroupDeletePermissionCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AlterGroupDeletePermissionCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AlterGroupDeletePermissionCommand::serialize_command() {
    nlohmann::json j;
    j["group_name"] = _group_name;
    j["permissions"] = _permissions;
    return j.dump();
}

AlterGroupDeletePermissionCommand AlterGroupDeletePermissionCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AlterGroupDeletePermissionCommand(json["group_name"],
                                             json["permissions"].get<std::vector<std::string>>());
}

nlohmann::json AlterGroupDeletePermissionCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Permissions deleted from group " + _group_name;
    response["Status"] = 200;
    return response;
}

// ==================== AlterDatabaseAddGroupCommand ====================

AlterDatabaseAddGroupCommand::AlterDatabaseAddGroupCommand(const std::string &database_name,
                                                           const std::string &group_name)
    : _database_name(database_name), _group_name(group_name) {
}

nlohmann::json AlterDatabaseAddGroupCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AlterDatabaseAddGroupCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AlterDatabaseAddGroupCommand::serialize_command() {
    nlohmann::json j;
    j["database_name"] = _database_name;
    j["group_name"] = _group_name;
    return j.dump();
}

AlterDatabaseAddGroupCommand AlterDatabaseAddGroupCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AlterDatabaseAddGroupCommand(json["database_name"], json["group_name"]);
}

nlohmann::json AlterDatabaseAddGroupCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Group " + _group_name + " added to database " + _database_name;
    response["Status"] = 200;
    return response;
}

// ==================== AlterDatabaseRemoveGroupCommand ====================

AlterDatabaseRemoveGroupCommand::AlterDatabaseRemoveGroupCommand(const std::string &database_name,
                                                                 const std::string &group_name)
    : _database_name(database_name), _group_name(group_name) {
}

nlohmann::json AlterDatabaseRemoveGroupCommand::process_command() {
    nlohmann::json response;
    response["Message"] = "AlterDatabaseRemoveGroupCommand not implemented yet";
    response["Status"] = 501;
    return response;
}

std::string AlterDatabaseRemoveGroupCommand::serialize_command() {
    nlohmann::json j;
    j["database_name"] = _database_name;
    j["group_name"] = _group_name;
    return j.dump();
}

AlterDatabaseRemoveGroupCommand AlterDatabaseRemoveGroupCommand::parse_from_bytes(const std::string &bytes) {
    auto json = nlohmann::json::parse(bytes);
    return AlterDatabaseRemoveGroupCommand(json["database_name"], json["group_name"]);
}

nlohmann::json AlterDatabaseRemoveGroupCommand::get_success_message() const {
    nlohmann::json response;
    response["Message"] = "Group " + _group_name + " removed from database " + _database_name;
    response["Status"] = 200;
    return response;
}
