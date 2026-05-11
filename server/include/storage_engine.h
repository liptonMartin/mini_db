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
                                const std::vector<Column> &columns,
                                const std::vector<Value> &values);

    static void update_elements(const std::string &db_name, const std::string &table_name,
                                std::unique_ptr<Condition> condition, const std::vector<Column> &columns,
                                const std::vector<Value> &values);

    static void delete_elements(const std::string &db_name, const std::string &table_name,
                                std::unique_ptr<Condition> condition);

    static std::vector<Row> select_elements(const std::string &db_name, const std::string &table_name,
                                            std::unique_ptr<Condition> condition);
};


#endif //MINI_DB_STORAGE_H
