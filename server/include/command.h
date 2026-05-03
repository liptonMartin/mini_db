//
// Created by MialistaPC on 26.04.2026.

#ifndef MINIDB_COMMAND_H
#define MINIDB_COMMAND_H

#include <nlohmann/json.hpp>

#include "storage_engine.h"

/**
 * Абстрактный класс для работы с командами.
 * Представляет единый интерфейс для работы со всеми командами
 */
class Command {
protected:
    virtual nlohmann::json get_success_message() const;

public:
    virtual nlohmann::json process_command(StorageEngine &engine) = 0;

    virtual ~Command() = default;
};


class CreateDatabaseCommand : public Command {
    std::string _database_name;

public:
    explicit CreateDatabaseCommand(const std::string &database_name);

    nlohmann::json process_command(StorageEngine &engine) override;
};


class DropDatabaseCommand : public Command {
    std::string _database_name;

public:
    explicit DropDatabaseCommand(const std::string &database_name);

    nlohmann::json process_command(StorageEngine &engine) override;
};


class UseDatabaseCommand : public Command {
    std::string _database_name;

public:
    explicit UseDatabaseCommand(const std::string &database_name);

    nlohmann::json process_command(StorageEngine &engine) override;
};


class CreateTableCommand : public Command {
    std::optional<std::string> _database_name;
    std::string _table_name;
    std::vector<Column> _columns;

public:
    explicit CreateTableCommand(const std::string &table_name, const std::vector<Column> &columns,
                                const std::string &database_name = "");


    nlohmann::json process_command(StorageEngine &engine) override;
};


class DropTableCommand : public Command {
    std::optional<std::string> _database_name;
    std::string _table_name;

public:
    explicit DropTableCommand(const std::string &table_name, const std::string &database_name = "");

    nlohmann::json process_command(StorageEngine &engine) override;
};


class InsertIntoCommand : public Command {
    std::optional<std::string> _database_name;
    std::string _table_name;

    std::vector<Column> _columns;
    std::vector<Value> _values;

public:
    explicit InsertIntoCommand(const std::string &table_name, const std::vector<Column> &columns,
                               const std::vector<Value> &values, const std::string &database_name = "");

    nlohmann::json process_command(StorageEngine &engine) override;
};


class UpdateCommand : public Command {
    std::optional<std::string> _database_name;
    std::string _table_name;

    std::unique_ptr<Condition> _condition;

    std::vector<Column> _columns;
    std::vector<Value> _values;

public:
    explicit UpdateCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                           const std::vector<Column> &columns, const std::vector<Value> &values,
                           const std::string &database_name = "");

    nlohmann::json process_command(StorageEngine &engine) override;
};


class DeleteFromCommand : public Command {
    std::optional<std::string> _database_name;
    std::string _table_name;

    std::unique_ptr<Condition> _condition;

public:
    explicit DeleteFromCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                               const std::string &database_name = "");

    nlohmann::json process_command(StorageEngine &engine) override;
};


class SelectCommand : public Command {
    std::optional<std::string> _database_name;
    std::string _table_name;

    std::optional<std::vector<Column> > _columns;
    std::optional<std::vector<std::string> > _aliases;

    std::unique_ptr<Condition> _condition;

public:
    SelectCommand(const std::string &table_name, std::unique_ptr<Condition> condition,
                  const std::string &database_name = "");

    SelectCommand(const std::string &table_name, const std::vector<Column> &columns,
                  std::unique_ptr<Condition> condition,
                  const std::string &database_name = "");

    SelectCommand(const std::string &table_name, const std::vector<Column> &columns,
                  const std::vector<std::string> &aliases, std::unique_ptr<Condition> condition,
                  const std::string &database_name = "");

    nlohmann::json process_command(StorageEngine &engine) override;
};

#endif //MINIDB_COMMAND_H
