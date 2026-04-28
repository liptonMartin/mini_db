//
// Created by rvova on 28.04.2026.
//

#include <format>
#include <filesystem>


#include "datatypes.h"
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

    _file.read(reinterpret_cast<char *>(&count_columns), sizeof(count_columns));
    for (auto i = 0; i < count_columns; ++i) {
        _file.read(reinterpret_cast<char *>(&len_column_name), sizeof(len_column_name));
        column_name.resize(len_column_name);
        _file.read(&column_name[0], len_column_name);
        _file.read(reinterpret_cast<char *>(&column_type), sizeof(column_type));
        columns.emplace_back(column_name, column_type);
    }

    return columns;
}