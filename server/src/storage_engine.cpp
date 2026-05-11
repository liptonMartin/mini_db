//
// Created by rvova on 25.04.2026.
//
#include "storage_engine.h"
#include "exceptions.h"

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
                                    const std::vector<Column> &columns,
                                    const std::vector<Value> &values) {
    auto db = Database::load_database(db_name);
    db.insert_elements(table_name, columns, values);
}

void StorageEngine::update_elements(const std::string &db_name, const std::string &table_name,
                                    std::unique_ptr<Condition> condition, const std::vector<Column> &columns,
                                    const std::vector<Value> &values) {
    auto db = Database::load_database(db_name);
    db.update_elements(table_name, std::move(condition), columns, values);
}

void StorageEngine::delete_elements(const std::string &db_name, const std::string &table_name,
                                    std::unique_ptr<Condition> condition) {
    auto db = Database::load_database(db_name);
    db.delete_elements(table_name, std::move(condition));
}

std::vector<Row> StorageEngine::select_elements(const std::string &db_name,
                                                const std::string &table_name,
                                                std::unique_ptr<Condition> condition) {
    auto db = Database::load_database(db_name);
    return db.select_elements(table_name, std::move(condition));
}
