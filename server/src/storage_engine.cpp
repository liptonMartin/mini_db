//
// Created by rvova on 25.04.2026.
//
#include "storage_engine.h"
#include "exceptions.h"

std::string StorageEngine::ping() {
    return "pong";
}

void StorageEngine::create_table(const std::optional<std::string> &db_name, const std::string &table_name,
                                 const std::vector<Column> &columns) {
    if (db_name) {
        auto db = Database::load_database(db_name.value());
        db.create_table(table_name, columns);
    } else if (_db) {
        _db->create_table(table_name, columns);
    } else throw DatabaseIsNotChosenException();
}

void StorageEngine::drop_table(const std::optional<std::string> &db_name, const std::string &table_name) {
    if (db_name) {
        auto db = Database::load_database(db_name.value());
        db.drop_table(table_name);
    } else if (_db) {
        _db->drop_table(table_name);
    } else throw DatabaseIsNotChosenException();
}

void StorageEngine::use_database(const std::string &db_name) {
    _db = Database::load_database(db_name);
}

void StorageEngine::create_database(const std::string &db_name) {
    Database::create_database(db_name);
}

void StorageEngine::drop_database(const std::string &db_name) {
    auto db = Database::load_database(db_name);
    db.drop_database();
}

void StorageEngine::insert_elements(const std::optional<std::string> &db_name, const std::string &table_name,
                                    const std::vector<Column> &columns,
                                    const std::vector<Value> &values) {
    if (db_name) {
        auto db = Database::load_database(db_name.value());
        db.insert_elements(table_name, columns, values);
    } else if (_db) {
        _db->insert_elements(table_name, columns, values);
    } else throw DatabaseIsNotChosenException();
}

void StorageEngine::update_elements(const std::optional<std::string> &db_name, const std::string &table_name,
                                    std::unique_ptr<Condition> condition, const std::vector<Column> &columns,
                                    const std::vector<Value> &values) {
    if (db_name) {
        auto db = Database::load_database(db_name.value());
        db.update_elements(table_name, std::move(condition), columns, values);
    } else if (_db) {
        _db->update_elements(table_name, std::move(condition), columns, values);
    } else throw DatabaseIsNotChosenException();
}

void StorageEngine::delete_elements(const std::optional<std::string> &db_name, const std::string &table_name,
                                    std::unique_ptr<Condition> condition) {
    if (db_name) {
        auto db = Database::load_database(db_name.value());
        db.delete_elements(table_name, std::move(condition));
    } else if (_db) {
        _db->delete_elements(table_name, std::move(condition));
    } else throw DatabaseIsNotChosenException();
}

std::vector<Row> StorageEngine::select_elements(const std::optional<std::string> &db_name,
                                                  const std::string &table_name,
                                                  std::unique_ptr<Condition> condition) {
    if (db_name) {
        auto db = Database::load_database(db_name.value());
        return db.select_elements(table_name, std::move(condition));
    }
    if (_db) {
        return _db->select_elements(table_name, std::move(condition));
    }
    throw DatabaseIsNotChosenException();
}
