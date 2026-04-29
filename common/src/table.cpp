//
// Created by rvova on 28.04.2026.
//

#include <algorithm>
#include <format>
#include <filesystem>


#include "constants.h"
#include "datatypes.h"
#include "exceptions.h"


Column::Column(const std::string &name, const DataType type, const bool is_nullable, const bool is_indexed)
    : _name(name), _type(type), _is_nullable(is_nullable), _is_indexed(is_indexed) {
}

bool Column::operator<(const Column &column) const {
    return _name < column._name;
}

/**
 *
 * @param path Путь, где хранится бд: root/databases/{database_name}/
 * @param name Имя таблицы
 * @param columns Столбцы
 */
Table::Table(const std::filesystem::path &path, const std::string &name,
             const std::optional<std::vector<Column> > &columns, bool need_create) {
    auto table_path = path / (name + ".binary");

    const auto file_exist = std::filesystem::exists(table_path);
    if (file_exist && need_create)
        throw TableHasAlreadyExistsException(name);
    if (!file_exist && !need_create)
        throw TableDoesNotExistException(name);
    if (!need_create && columns)
        throw FailedCreateTableException("Unexpected argument columns for create existed table!");
    if (need_create && !columns)
        throw FailedCreateTableException("Missing necessary argument columns for create table!");
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
        // TODO: нужно заполнить column_id!
        const auto count_columns = std::ssize(columns.value());
        _file.write(reinterpret_cast<const char *>(&count_columns), sizeof(count_columns));

        for (const auto &column: columns.value()) {
            const auto len_column_name = std::ssize(column._name);
            _file.write(reinterpret_cast<const char *>(&len_column_name), sizeof(len_column_name));
            _file.write(column._name.c_str(), len_column_name);
            _file.write(reinterpret_cast<const char *>(&column._type), sizeof(column._type));
        }

        /* записываем информацию о количестве страниц */
        constexpr ptrdiff_t count_pages = 0;
        _file.write(reinterpret_cast<const char *>(&count_pages), sizeof(count_pages));
    }
}

Table Table::create_table(const std::filesystem::path &path, const std::string &name,
                          const std::vector<Column> &columns) {
    return Table(path, name, columns, true);
}

Table Table::load_table(const std::filesystem::path &path, const std::string &name) {
    return Table(path, name, std::nullopt, false);
}

void Table::insert_elements(const std::vector<Column> &columns, const std::vector<Value> &values) {
    if (columns.size() != values.size())
        throw FailedInsertElementsToTableException("Count columns not equal count values");

    auto column_values = get_empty_values();
    for (auto i = 0; i < columns.size(); ++i) {
        const auto column = columns[i];
        const auto value = values[i];
        column_values[column] = value;
    }
    for (const auto &item: column_values) {
        // TODO: добавить валидацию, что если поле index, то оно уникальное. Наверное надо дергать методы b-дерева
        const auto &column = item.first;
        const auto &value = item.second;
        if (!column._is_nullable && std::holds_alternative<Null>(value))
            throw FailedInsertElementsToTableException("Column " + column._name + " doesn't support nullable value!");

        switch (column._type) {
            case DataType::Int: {
                if (std::holds_alternative<std::string>(value))
                    throw FailedInsertElementsToTableException(
                        "Column" + column._name + "has integer type, but " + std::get<std::string>(value) +
                        "has string type");
                break;
            }
            case DataType::String: {
                if (std::holds_alternative<int>(value))
                    throw FailedInsertElementsToTableException(
                        "Column" + column._name + "has string type, but " + std::to_string(std::get<int>(value)) +
                        "has integer type");
                break;
            }
            case DataType::Null:
                /* Null не может быть */
                throw FailedInsertElementsToTableException("The database has invalid table" + get_name() + "!");
        }
    }
    std::vector<char> buffer{};
    for (const auto &[column, value]: column_values) {
        switch (column._type) {
            case DataType::Int: {
                /* просто записываем */
                const auto& int_value = std::get<int>(value);
                std::copy_n(reinterpret_cast<const char*>(&int_value), sizeof(int_value), buffer.end());
                break;
            }
            case DataType::String: {
                const auto& string_value = std::get<std::string>(value);
                const auto& string_length = string_value.length();

                /* сначала записываем длину строки */
                std::copy_n(reinterpret_cast<const char*>(&string_length), sizeof(string_length), buffer.end());
                std::copy_n(string_value.c_str(), sizeof(string_value), buffer.end());
                break;
            }
            case DataType::Null:
                /* вообще не будем ничего записывать */
                break;
        }
    }

    const auto buffer_size = buffer.size();
    buffer.resize(buffer_size);
    if (buffer_size + sizeof(PageHeader) > db::PAGE_SIZE)
        throw FailedInsertElementsToTableException("The data takes up too much memory!");

    PageManager page_manager(_file, get_pages_begin_offset());
    const auto page_id = page_manager.search_free_page(buffer_size);
    page_manager.write_page(page_id, buffer);
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

ptrdiff_t Table::get_count_pages() {
    move_to_position_count_page();

    ptrdiff_t count_pages;
    _file.read(reinterpret_cast<char *>(&count_pages), sizeof(count_pages));
    return count_pages;
}

std::map<Column, Value> Table::get_empty_values() {
    std::map<Column, Value> result;
    for (auto column: get_columns()) {
        result[column] = Null{};
    }
    return result;
}


ptrdiff_t Table::get_pages_begin_offset() {
    move_to_position_pages();
    const auto position_start_pages = _file.tellg();
    _file.seekg(0, std::ios::beg);
    const auto position_start_file = _file.tellg();
    return position_start_pages - position_start_file;
}

void Table::move_to_position_pages() {
    get_count_pages(); /* сдвинет указатель на начало памяти, где начинаются сами страницы */
}

void Table::move_to_position_count_page() {
    get_columns(); /* сдвинет указатель на начало памяти, где хранится количество страниц в файле */
}

void Table::move_to_position_columns() {
    get_name(); /* сдвинет указатель на начало памяти, где хранится столбцы */
}
