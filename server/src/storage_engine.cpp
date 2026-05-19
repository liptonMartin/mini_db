//
// Created by rvova on 25.04.2026.
//
#include "storage_engine.h"

void StorageEngine::create_table(const std::string &db_name, const std::string &table_name,
                                 const std::vector<Column> &columns) {
    auto db = Database::load_database(db_name);
    db.create_table(table_name, columns);
}

void StorageEngine::drop_table(const std::string &db_name, const std::string &table_name) {
    auto db = Database::load_database(db_name);
    db.drop_table(table_name);
}

void StorageEngine::create_database(const std::string &db_name) {
    Database::create_database(db_name);
}

void StorageEngine::drop_database(const std::string &db_name) {
    auto db = Database::load_database(db_name);
    db.drop_database();
}

void StorageEngine::insert_elements(const std::string &db_name, const std::string &table_name,
                                    const std::vector<std::string> &column_names,
                                    const std::vector<Value> &values) {
    auto db = Database::load_database(db_name);
    // TODO: impl it!
    std::vector<Column> columns{};
    db.insert_elements(table_name, column_names, values);
}

void StorageEngine::update_elements(const std::string &db_name, const std::string &table_name,
                                    std::unique_ptr<Condition> condition, const std::vector<std::string> &column_names,
                                    const std::vector<Value> &values) {
    auto db = Database::load_database(db_name);
    // TODO: impl it!
    std::vector<Column> columns{};
    db.update_elements(table_name, std::move(condition), column_names, values);
}

void StorageEngine::delete_elements(const std::string &db_name, const std::string &table_name,
                                    std::unique_ptr<Condition> condition) {
    auto db = Database::load_database(db_name);
    db.delete_elements(table_name, std::move(condition));
}

nlohmann::json StorageEngine::select_elements(
    const std::string &db_name,
    const std::string &table_name,
    const std::optional<std::unordered_map<std::string, Alias> > &columns_with_aliases,
    std::unique_ptr<Condition> condition) {
    auto db = Database::load_database(db_name);

    const auto select = db.select_elements(table_name, columns_with_aliases, std::move(condition));
    nlohmann::json result;

    /* заполняем названия столбцов */
    if (columns_with_aliases.has_value()) {
        for (const auto &[column_name, alias]: columns_with_aliases.value()) {
            if (alias.has_value()) {
                result[alias.value()] = nlohmann::json::array();
            } else {
                result[column_name] = nlohmann::json::array();
            }
        }
    } else {
        const auto column_names = db.get_table_column_names(table_name);

        for (const auto &column_name: column_names) {
            result[column_name] = nlohmann::json::array();
        }
    }

    /* заполняем сами столбцы */
    for (const auto &row: select) {
        for (const auto &[alias, value]: row) {
            if (std::holds_alternative<int>(value)) {
                const int int_value = std::get<int>(value);
                result[alias].push_back(int_value);
            } else if (std::holds_alternative<std::string>(value)) {
                const std::string string_value = std::get<std::string>(value);
                result[alias] = string_value;
            } else {
                result[alias] = "null";
            }
        }
    }

    return result;
}
