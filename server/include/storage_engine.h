//
// Created by rvova on 25.04.2026.
//

#ifndef MINI_DB_STORAGE_H
#define MINI_DB_STORAGE_H
#include <optional>
#include <variant>
#include <vector>

#include "datatypes.h"

class StorageEngine {
public:
    StorageEngine() = default;

    static void create_database(const std::string &db_name);

    static void drop_database(const std::string &db_name);

    static void create_table(const std::string &db_name, const std::string &table_name,
                             const std::vector<Column> &columns);

    static void drop_table(const std::string &db_name, const std::string &table_name);

    static void insert_elements(const std::string &db_name, const std::string &table_name,
                                const std::vector<std::string> &column_names,
                                const std::vector<Value> &values);

    static void update_elements(const std::string &db_name, const std::string &table_name,
                                std::unique_ptr<Condition> condition, const std::vector<std::string> &column_names,
                                const std::vector<Value> &values);

    static void delete_elements(const std::string &db_name, const std::string &table_name,
                                std::unique_ptr<Condition> condition);

    static nlohmann::json select_elements(
        const std::string &db_name, const std::string &table_name,
        const std::optional<std::unordered_map<std::string, Alias> > &columns_with_aliases,
        std::unique_ptr<Condition> condition);
};


#endif //MINI_DB_STORAGE_H
