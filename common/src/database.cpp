//
// Created by rvova on 28.04.2026.
//

#include <format>
#include <filesystem>

#include "datatypes.h"
#include "exceptions.h"


Database::Database(const std::string &name, const bool need_to_create) {
    /* путь до схемы: root/databases/{database_name}/{database_name}.schema */
    auto path_to_file = make_path_to_file(name);
    if (!std::filesystem::exists(path_to_file) && need_to_create) {
        std::filesystem::create_directories(path_to_file);
    } else if (std::filesystem::exists(path_to_file) && need_to_create) {
        throw DatabaseHasAlreadyExistsException(name);
    }

    const auto schema_path = path_to_file / (name + ".schema");

    const auto file_exist = std::filesystem::exists(schema_path);
    if (file_exist) {
        _file.open(schema_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!_file.is_open())
            throw InvalidOpenFileException(std::format("Can't open file {}", schema_path.string()));
    } else {
        /* файла еще не существовало, значит только создали базу данных */
        _file.open(schema_path, std::ios::out); /* создали файл */
        _file.close();

        _file.open(schema_path, std::ios::in | std::ios::out | std::ios::binary); /* открыли уже существующий файл */
        if (!_file.is_open())
            throw InvalidOpenFileException(std::format("Can't open file {}", schema_path.string()));

        const auto len_name = std::ssize(name);
        _file.write(reinterpret_cast<const char *>(&len_name), sizeof(len_name));
        _file.write(name.c_str(), len_name);

        /* количество столбцов */
        const ptrdiff_t count_tables = 0;
        _file.write(reinterpret_cast<const char *>(&count_tables), sizeof(count_tables));
    }
}

Database Database::create_database(const std::string &name) {
    return Database(name, true);
}

Database Database::load_database(const std::string &name) {
    return Database(name, false);
}

void Database::drop_database() {
    auto db_name = get_name();
    auto path = make_path_to_file(db_name);

    if (!std::filesystem::exists(path)) {
        throw DatabaseDoesNotExistException(db_name);
    }

    _file.close();
    std::filesystem::remove_all(path); /* удаляем все файлы (а значит и таблицы) */
}

void Database::create_table(const std::string &name, const std::vector<Column> &columns) {
    const auto path = make_path_to_file(get_name());
    Table::create_table(path, name, columns);

    move_to_position_tables_begin();
    const auto pos = _file.tellg();

    ptrdiff_t count_tables;
    _file.read(reinterpret_cast<char *>(&count_tables), sizeof(count_tables));

    /* записываем название таблицы */
    move_to_position_tables_end();
    _file.seekp(_file.tellg()); /* синхронизировали указатели */

    auto len_table_name = std::ssize(name);
    _file.write(reinterpret_cast<const char *>(&len_table_name), sizeof(len_table_name));
    _file.write(name.c_str(), len_table_name);

    ++count_tables; /* добавили одну таблицу */
    _file.seekp(pos); /* передвинули указатель на чтение, в место, где хранится количество таблиц */
    _file.write(reinterpret_cast<const char *>(&count_tables), sizeof(count_tables));
}

void Database::drop_table(const std::string &name) {
    const auto db_name = get_name();
    const auto path = make_path_to_file(db_name);

    if (!std::filesystem::exists(path)) {
        throw DatabaseDoesNotExistException(db_name);
    }

    const auto table_path = path / (db_name + ".binary");
    _file.close();
    std::filesystem::remove(table_path);
}

void Database::insert_elements(const std::string &table_name, const std::vector<Column> &columns,
                               const std::vector<Value> &values) {
    const auto db_name = get_name();
    const auto path = make_path_to_file(db_name);
    auto table = Table::load_table(path, table_name);

    table.insert_elements(columns, values);
}

void Database::update_elements(const std::string &table_name, std::unique_ptr<Condition> condition,
                               const std::vector<Column> &columns, const std::vector<Value> &values) {
    const auto db_name = get_name();
    const auto path = make_path_to_file(db_name);
    auto table = Table::load_table(path, table_name);

    table.update_elements(std::move(condition), columns, values);
}

void Database::delete_elements(const std::string &table_name, std::unique_ptr<Condition> condition) {
    const auto db_name = get_name();
    const auto path = make_path_to_file(db_name);
    auto table = Table::load_table(path, table_name);

    table.delete_elements(std::move(condition));
}

std::vector<Row> Database::select_elements(const std::string &table_name, std::unique_ptr<Condition> condition) {
    const auto db_name = get_name();
    const auto path = make_path_to_file(db_name);
    auto table = Table::load_table(path, table_name);

    return table.select_elements(std::move(condition));
}

std::string Database::get_name() {
    _file.seekg(0, std::ios::beg); /* переместить в начало */
    ptrdiff_t len_name;
    std::string name;
    _file.read(reinterpret_cast<char *>(&len_name), sizeof(len_name));
    name.resize(len_name);
    _file.read(&name[0], len_name);
    return name;
}

std::vector<std::string> Database::get_tables() {
    move_to_position_tables_begin();

    ptrdiff_t count_tables;
    ptrdiff_t len_table_name;
    std::string table_name;
    std::vector<std::string> tables;

    _file.read(reinterpret_cast<char *>(&count_tables), sizeof(count_tables));
    for (auto i = 0; i < count_tables; ++i) {
        _file.read(reinterpret_cast<char *>(&len_table_name), sizeof(len_table_name));
        table_name.resize(len_table_name);
        _file.read(&table_name[0], len_table_name);
        tables.push_back(table_name);
    }

    return tables;
}

std::filesystem::path Database::make_path_to_file(const std::string &db_name) {
    const std::filesystem::path exe_path = std::filesystem::current_path();
    // Поднимаемся на 2 уровня выше: из build/bin/ -> в корень проекта
    const std::filesystem::path project_root = exe_path.parent_path().parent_path(); // от build/
    std::filesystem::path db_dir = project_root / "databases" / db_name;
    return db_dir;
}

/**
 * Переносит указатель на начало памяти, где начинаются tables
 */
void Database::move_to_position_tables_begin() {
    get_name(); /* сдвинет указатель как раз куда надо */
}

/**
 * Переносит указатель на начало памяти, где заканчиваются tables
 */
void Database::move_to_position_tables_end() {
    get_tables(); /* сдвинет указатель как раз куда надо */
}
