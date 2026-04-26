//
// Created by rvova on 25.04.2026.

#include <fstream>

#include "datatypes.h"

#include <filesystem>
#include <format>

#include "constants.h"
#include "exceptions.h"


Column::Column(const std::string &name, const DataType type) : _name(name), _type(type) {
}

/**
 *
 * @param path Путь, где хранится бд: root/databases/{database_name}/
 * @param name Имя таблицы
 * @param columns Столбцы
 */
Table::Table(const std::filesystem::path &path, const std::string &name, const std::vector<Column> &columns) {
    auto table_path = path / (name + ".binary");

    const auto file_exist = std::filesystem::exists(table_path);
    if (file_exist) {
        _file.open(table_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!_file.is_open())
            throw InvalidOpenFileException(std::format("Can't open file {}", table_path.string()));
    } else {
        /* таблицы еще не существовало */
        _file.open(table_path, std::ios::out); /* создали файл */
        _file.close();

        _file.open(table_path, std::ios::in | std::ios::out | std::ios::binary); /* открыли уже существующий файл */
        if (!_file.is_open())
            throw InvalidOpenFileException(std::format("Can't open file {}", table_path.string()));

        /* записали название таблицы */
        const auto len_name = std::ssize(name);
        _file.write(reinterpret_cast<const char *>(&len_name), sizeof(len_name));
        _file.write(name.c_str(), len_name);

        /* записываем столбцы */
        const auto count_columns = std::ssize(columns);
        _file.write(reinterpret_cast<const char *>(&count_columns), sizeof(count_columns));

        for (const auto &column: columns) {
            const auto len_column_name = std::ssize(column._name);
            _file.write(reinterpret_cast<const char *>(&len_column_name), sizeof(len_column_name));
            _file.write(column._name.c_str(), len_column_name);
            _file.write(reinterpret_cast<const char *>(&column._type), sizeof(column._type));
        }

        /* записываем информацию о страницах */
        // TODO: сделать страницы
    }
}

void Table::move_to_position_columns() {
    get_name(); /* сдвинет указатель на начало памяти, где хранится столбцы */
}

std::string Table::get_name() {
    _file.seekg(0, std::ios::beg); /* переместить в начало */
    ptrdiff_t len_name;
    std::string name;
    _file.read(reinterpret_cast<char *>(&len_name), sizeof(len_name));
    name.resize(len_name);
    _file.read(&name[0], len_name);
    return name;
}

std::vector<Column> Table::get_columns() {
    move_to_position_columns();

    ptrdiff_t count_columns;
    ptrdiff_t len_column_name;
    std::string column_name;
    DataType column_type;
    std::vector<Column> columns;

    _file.read(reinterpret_cast<char*>(&count_columns), sizeof(count_columns));
    for (auto i = 0; i < count_columns; ++i) {
        _file.read(reinterpret_cast<char*>(&len_column_name), sizeof(len_column_name));
        column_name.resize(len_column_name);
        _file.read(&column_name[0], len_column_name);
        _file.read(reinterpret_cast<char*>(&column_type), sizeof(column_type));
        columns.emplace_back(column_name, column_type);
    }

    return columns;
}


Database::Database(const std::string &name) {
    /* путь до схемы: root/databases/{database_name}/{database_name}.schema */
    auto path_to_file = make_path_to_file(name);
    if (!std::filesystem::exists(path_to_file)) {
        std::filesystem::create_directories(path_to_file);
    }
    auto schema_path = path_to_file / (name + ".schema");

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

std::filesystem::path Database::make_path_to_file(const std::string &name) {
    const std::filesystem::path exe_path = std::filesystem::current_path();
    // Поднимаемся на 2 уровня выше: из build/bin/ -> в корень проекта
    const std::filesystem::path project_root = exe_path.parent_path().parent_path().parent_path(); // от build/
    std::filesystem::path db_dir = project_root / "databases" / name;
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

std::string Database::get_name() {
    _file.seekg(0, std::ios::beg); /* переместить в начало */
    ptrdiff_t len_name;
    std::string name;
    _file.read(reinterpret_cast<char *>(&len_name), sizeof(len_name));
    name.resize(len_name);
    _file.read(&name[0], len_name);
    return name;
}

void Database::insert_table(const std::string &name, const std::vector<Column> &columns) {
    auto path = make_path_to_file(get_name());
    Table(path, name, columns);

    move_to_position_tables_begin();
    const auto pos = _file.tellg();

    ptrdiff_t count_tables;
    _file.read(reinterpret_cast<char*>(&count_tables), sizeof(count_tables));

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

std::vector<std::string> Database::get_tables() {
    move_to_position_tables_begin();

    ptrdiff_t count_tables;
    ptrdiff_t len_table_name;
    std::string table_name;
    std::vector<std::string> tables;

    _file.read(reinterpret_cast<char *>(&count_tables), sizeof(count_tables));
    for (auto i = 0; i < count_tables; ++i) {
        _file.read(reinterpret_cast<char*>(&len_table_name), sizeof(len_table_name));
        table_name.resize(len_table_name);
        _file.read(&table_name[0], len_table_name);
        tables.push_back(table_name);
    }

    return tables;
}
