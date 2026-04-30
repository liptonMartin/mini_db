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
    std::optional<Database> _db;

public:
    std::string ping();

    void create_database(const std::string &db_name);

    void drop_database(const std::string &db_name);

    void use_database(const std::string &db_name);

    void create_table(const std::optional<std::string> &db_name, const std::string &table_name,
                      const std::vector<Column> &columns);

    void drop_table(const std::optional<std::string> &db_name, const std::string &table_name);

    void insert_elements(const std::optional<std::string> &db_name, const std::string &table_name,
                         const std::vector<Column> &columns,
                         const std::vector<Value > &values);

    void update_elements(const std::optional<std::string> &db_name, const std::string &table_name,
                         const Condition &condition, const std::vector<Column> &columns,
                         const std::vector<Value> &values);

    void delete_elements(const std::optional<std::string> &db_name, const std::string &table_name,
                         const Condition &condition);

    std::vector<Value> select_elements(const std::optional<std::string> &db_name, const std::string &table_name,
                                        const std::optional<Condition> &condition);
};


#endif //MINI_DB_STORAGE_H
