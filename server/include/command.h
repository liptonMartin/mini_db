//
// Created by MialistaPC on 26.04.2026.
//

#ifndef MINIDB_COMMAND_H
#define MINIDB_COMMAND_H

#include <nlohmann/json.hpp>

#include "storage_engine.h"

enum class CommandType {
    CreateDatabase,
    DropDatabase,
    UseDatabase,
    CreateTable,
    DropTable,
    InsertInto,
    Update,
    DeleteFrom,
    Select,
    Heartbeat,
    CreateUser,
    Auth,
    AlterUserAddToGroup,
    AlterUserRemoveFromGroup,
    AlterUserAddPermission,
    AlterGroupAddPermission,
    AlterGroupDeletePermission,
    AlterDatabaseAddGroup,
    AlterDatabaseRemoveGroup,
};

/* helper функции для правильной сериализации данных */
nlohmann::json columns_to_json(const std::vector<Column> &columns);

std::vector<Column> columns_from_json(const nlohmann::json &j);

nlohmann::json values_to_json(const std::vector<Value> &values);

std::vector<Value> values_from_json(const nlohmann::json &j);


/**
 * Абстрактный класс для работы с командами.
 * Представляет единый интерфейс для работы со всеми командами
 */
class Command {
protected:
    virtual nlohmann::json get_success_message() const;

    /**
     * Проверяет, установлено ли имя базы данных
     * @return True если имя базы данных не пустое
     */
    virtual bool is_database_name_set() const;

    /**
     * Устанавливает имя базы данных (только если оно еще не установлено)
     * @param db_name Имя базы данных
     * @return True если имя было установлено, false если уже было установлено
     */
    virtual bool set_database_name(const std::string &db_name);

public:
    virtual nlohmann::json process_command() = 0;

    virtual std::string serialize_command();

    virtual void set_token(const std::string& token);
    std::string get_token() const;

    virtual ~Command() = default;

protected:
    std::string _jwt_token;
};


class CreateDatabaseCommand : public Command {
    std::string _database_name;
    std::vector<std::string> _group_names;

protected:
    nlohmann::json get_success_message() const override;

public:
    explicit CreateDatabaseCommand(const std::string &database_name, const std::vector<std::string> &group_names = {});

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static CreateDatabaseCommand parse_from_bytes(const std::string &bytes);
};


class DropDatabaseCommand : public Command {
    std::string _database_name;

protected:
    nlohmann::json get_success_message() const override;

public:
    explicit DropDatabaseCommand(const std::string &database_name);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static DropDatabaseCommand parse_from_bytes(const std::string &bytes);
};


class UseDatabaseCommand : public Command {
    std::string _database_name;

protected:
    nlohmann::json get_success_message() const override;

public:
    explicit UseDatabaseCommand(const std::string &database_name);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static UseDatabaseCommand parse_from_bytes(const std::string &bytes);
};


class CreateTableCommand : public Command {
    std::string _database_name;
    std::string _table_name;
    std::vector<Column> _columns;

protected:
    nlohmann::json get_success_message() const override;

    bool is_database_name_set() const override;

    bool set_database_name(const std::string &db_name) override;

public:
    explicit CreateTableCommand(const std::string &table_name, const std::vector<Column> &columns,
                                const std::string &database_name = "");


    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static CreateTableCommand parse_from_bytes(const std::string &bytes);
};


class DropTableCommand : public Command {
    std::string _database_name;
    std::string _table_name;

protected:
    nlohmann::json get_success_message() const override;

    bool is_database_name_set() const override;

    bool set_database_name(const std::string &db_name) override;

public:
    explicit DropTableCommand(const std::string &table_name, const std::string &database_name = "");

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static DropTableCommand parse_from_bytes(const std::string &bytes);
};


class InsertIntoCommand : public Command {
    std::string _database_name;
    std::string _table_name;

    std::vector<std::string> _column_names;
    std::vector<Value> _values;

protected:
    nlohmann::json get_success_message() const override;

    bool is_database_name_set() const override;

    bool set_database_name(const std::string &db_name) override;

public:
    explicit InsertIntoCommand(const std::string &table_name, const std::vector<std::string> &column_names,
                               const std::vector<Value> &values, const std::string &database_name = "");

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static InsertIntoCommand parse_from_bytes(const std::string &bytes);
};


class UpdateCommand : public Command {
    std::string _database_name;
    std::string _table_name;

    std::unique_ptr<Condition> _condition;

    std::vector<std::string> _column_names;
    std::vector<Value> _values;

protected:
    nlohmann::json get_success_message() const override;

    bool is_database_name_set() const override;

    bool set_database_name(const std::string &db_name) override;

public:
    explicit UpdateCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                           const std::vector<std::string> &column_names, const std::vector<Value> &values,
                           const std::string &database_name = "");

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static UpdateCommand parse_from_bytes(const std::string &bytes);
};


class DeleteFromCommand : public Command {
    std::string _database_name;
    std::string _table_name;

    std::unique_ptr<Condition> _condition;

protected:
    nlohmann::json get_success_message() const override;

    bool is_database_name_set() const override;

    bool set_database_name(const std::string &db_name) override;

public:
    explicit DeleteFromCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                               const std::string &database_name = "");

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static DeleteFromCommand parse_from_bytes(const std::string &bytes);
};


enum class AggregateFunction { Sum, Count, Avg };

struct SelectTarget {
    std::string column_name;                     // имя колонки (пусто для COUNT(*))
    Alias alias;                                 // опциональный алиас
    std::optional<AggregateFunction> agg_func;   // nullopt для обычных колонок
};

class SelectCommand : public Command {
    std::string _database_name;
    std::string _table_name;

    std::vector<SelectTarget> _select_targets;

    std::unique_ptr<Condition> _condition;

protected:
    nlohmann::json get_success_message() const override;

    bool is_database_name_set() const override;

    bool set_database_name(const std::string &db_name) override;

public:
    SelectCommand(const std::string &table_name, std::unique_ptr<Condition> condition = nullptr,
                  const std::vector<SelectTarget> &select_targets = {},
                  const std::string &database_name = "");

    void add_column(const std::string &column_name, const Alias &alias = std::nullopt);

    void add_aggregate(AggregateFunction func, const std::string &column_name);

    const std::vector<SelectTarget> &get_select_targets() const;

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static SelectCommand parse_from_bytes(const std::string &bytes);
};

// ==================== CreateUserCommand ====================

class CreateUserCommand : public Command {
    std::string _username;
    std::string _password;

protected:
    nlohmann::json get_success_message() const override;

public:
    CreateUserCommand(const std::string &username, const std::string &password);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static CreateUserCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AuthCommand ====================

class AuthCommand : public Command {
    std::string _username;
    std::string _password;

protected:
    nlohmann::json get_success_message() const override;

public:
    AuthCommand(const std::string &username, const std::string &password);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AuthCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AlterUserAddToGroupCommand ====================

class AlterUserAddToGroupCommand : public Command {
    std::string _username;
    std::string _database_name;
    std::string _group_name;

protected:
    nlohmann::json get_success_message() const override;

public:
    AlterUserAddToGroupCommand(const std::string &username, const std::string &database_name,
                               const std::string &group_name);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AlterUserAddToGroupCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AlterUserRemoveFromGroupCommand ====================

class AlterUserRemoveFromGroupCommand : public Command {
    std::string _username;
    std::string _database_name;
    std::string _group_name;

protected:
    nlohmann::json get_success_message() const override;

public:
    AlterUserRemoveFromGroupCommand(const std::string &username, const std::string &database_name,
                                    const std::string &group_name);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AlterUserRemoveFromGroupCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AlterUserAddPermissionCommand ====================

class AlterUserAddPermissionCommand : public Command {
    std::string _username;
    std::string _database_name;
    std::vector<std::string> _permissions;

protected:
    nlohmann::json get_success_message() const override;

public:
    AlterUserAddPermissionCommand(const std::string &username, const std::string &database_name,
                                  const std::vector<std::string> &permissions);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AlterUserAddPermissionCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AlterGroupAddPermissionCommand ====================

class AlterGroupAddPermissionCommand : public Command {
    std::string _group_name;
    std::vector<std::string> _permissions;

protected:
    nlohmann::json get_success_message() const override;

public:
    AlterGroupAddPermissionCommand(const std::string &group_name,
                                   const std::vector<std::string> &permissions);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AlterGroupAddPermissionCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AlterGroupDeletePermissionCommand ====================

class AlterGroupDeletePermissionCommand : public Command {
    std::string _group_name;
    std::vector<std::string> _permissions;

protected:
    nlohmann::json get_success_message() const override;

public:
    AlterGroupDeletePermissionCommand(const std::string &group_name,
                                      const std::vector<std::string> &permissions);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AlterGroupDeletePermissionCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AlterDatabaseAddGroupCommand ====================

class AlterDatabaseAddGroupCommand : public Command {
    std::string _database_name;
    std::string _group_name;

protected:
    nlohmann::json get_success_message() const override;

public:
    AlterDatabaseAddGroupCommand(const std::string &database_name, const std::string &group_name);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AlterDatabaseAddGroupCommand parse_from_bytes(const std::string &bytes);
};

// ==================== AlterDatabaseRemoveGroupCommand ====================

class AlterDatabaseRemoveGroupCommand : public Command {
    std::string _database_name;
    std::string _group_name;

protected:
    nlohmann::json get_success_message() const override;

public:
    AlterDatabaseRemoveGroupCommand(const std::string &database_name, const std::string &group_name);

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static AlterDatabaseRemoveGroupCommand parse_from_bytes(const std::string &bytes);
};

#endif //MINIDB_COMMAND_H
