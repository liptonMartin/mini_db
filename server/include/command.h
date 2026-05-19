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

    virtual ~Command() = default;
};


class CreateDatabaseCommand : public Command {
    std::string _database_name;

protected:
    nlohmann::json get_success_message() const override;

public:
    explicit CreateDatabaseCommand(const std::string &database_name);

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


class SelectCommand : public Command {
    std::string _database_name;
    std::string _table_name;

    std::optional<std::unordered_map<std::string, Alias> > _columns_with_aliases;

    std::unique_ptr<Condition> _condition;

protected:
    nlohmann::json get_success_message() const override;

    bool is_database_name_set() const override;

    bool set_database_name(const std::string &db_name) override;

public:
    SelectCommand(const std::string &table_name, std::unique_ptr<Condition> condition = nullptr,
                  const std::optional<std::unordered_map<std::string, Alias> > &columns_with_aliases = std::nullopt,
                  const std::string &database_name = "");

    nlohmann::json process_command() override;

    std::string serialize_command() override;

    static SelectCommand parse_from_bytes(const std::string &bytes);
};

#endif //MINIDB_COMMAND_H
