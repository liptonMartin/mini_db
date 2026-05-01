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

    auto completed_values = get_completed_values(columns, values);
    auto buffer = get_bytes_from_row(completed_values);

    const auto buffer_size = buffer.size();
    buffer.resize(buffer_size);
    if (buffer_size + sizeof(PageHeader) + sizeof(Slot) > db::PAGE_SIZE)
        throw FailedInsertElementsToTableException("The data takes up too much memory!");

    PageManager page_manager(_file, get_pages_begin_offset());
    auto page_id = page_manager.search_free_page(static_cast<ptrdiff_t>(buffer_size));
    if (page_id == -1) {
        page_id = page_manager.allocate_page();
    }
    // ReSharper disable once CppExpressionWithoutSideEffects
    page_manager.insert_element_into_page(page_id, buffer);
}

void Table::update_elements(std::unique_ptr<Condition> condition, const std::vector<Column> &columns,
                            const std::vector<Value> &values) {
    if (!condition)
        throw FailedUpdateElementsToTableException("The update elements requires a condition!");

    if (columns.size() != values.size())
        throw FailedUpdateElementsToTableException("Count columns not equal count values");

    const auto completed_values = get_completed_values(columns, values);

    auto page_manager = PageManager(_file, get_pages_begin_offset());
    const auto count_pages = get_count_pages();
    for (auto page_id = 0; page_id < count_pages; ++page_id) {
        auto page = page_manager.read_page(page_id);
        for (auto slot_id = 0; slot_id < page.get_count_slots(); ++slot_id) {
            const auto slot = page.get_slot(slot_id);
            if (!slot.is_occupied()) continue;

            const auto &slot_data = page.get_slot_data_by_id(slot_id);
            const auto &row_data = get_values_from_row(slot_data, columns);

            if (condition->evaluate(row_data)) {
                /* обновляем данные */
                const auto &bytes = get_bytes_from_row(row_data);
                page_manager.update_element_into_page(page_id, slot_id, bytes); /* перезапишет файл */
            }
        }
    }
}

void Table::delete_elements(std::unique_ptr<Condition> condition) {
    if (!condition)
        throw FailedDeleteElementsToTableException("The delete elements requires a condition!");

    auto page_manager = PageManager(_file, get_pages_begin_offset());
    const auto count_pages = get_count_pages();
    const auto &columns = get_columns();
    for (auto page_id = 0; page_id < count_pages; ++page_id) {
        auto page = page_manager.read_page(page_id);
        for (auto slot_id = 0; slot_id < page.get_count_slots(); ++slot_id) {
            const auto slot = page.get_slot(slot_id);
            if (!slot.is_occupied()) continue;

            const auto &slot_data = page.get_slot_data_by_id(slot_id);
            const auto &row_data = get_values_from_row(slot_data, columns);

            if (condition->evaluate(row_data)) {
                /* удаляем данные */
                page_manager.erase_element_from_page(page_id, slot_id);
            }
        }
    }
}

std::vector<Row> Table::select_elements(std::unique_ptr<Condition> condition) {
    auto page_manager = PageManager(_file, get_pages_begin_offset());
    const auto &columns = get_columns();
    const auto count_pages = get_count_pages();
    std::vector<Row> rows{};
    for (auto page_id = 0; page_id < count_pages; ++page_id) {
        auto page = page_manager.read_page(page_id);
        for (const auto &slot_data: page.get_slots_data()) {
            const auto &row_data = get_values_from_row(slot_data, columns);
            if ((condition && condition->evaluate(row_data)) || !condition) {
                /* если условия нет, то добавляем все элементы */
                rows.push_back(row_data);
            }
        }
    }

    return rows;
}

Row Table::get_completed_values(const std::vector<Column> &columns,
                                const std::vector<Value> &values) {
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
        }
    }

    return column_values;
}

std::vector<char> Table::get_bytes_from_row(const Row &column_values) {
    std::vector<char> buffer{};
    ptrdiff_t offset = 0;
    for (const auto &[column, value]: column_values) {
        /* если значение может быть null, то перед каждым элементом будем держать bool значение,
         * является ли дальнейший тип null
         */
        if (column._is_nullable) {
            const auto is_null = std::holds_alternative<Null>(value);
            buffer.resize(offset + sizeof(is_null));
            std::copy_n(reinterpret_cast<const char *>(&is_null), sizeof(is_null), buffer.begin() + offset);
            offset += sizeof(is_null);
        }

        if (std::holds_alternative<int>(value)) {
            const auto &int_value = std::get<int>(value);
            buffer.resize(offset + sizeof(int_value));
            std::copy_n(reinterpret_cast<const char *>(&int_value), sizeof(int_value), buffer.begin() + offset);
            offset += sizeof(int_value);
        } else if (std::holds_alternative<std::string>(value)) {
            const auto &string_value = std::get<std::string>(value);
            const auto &string_length = string_value.length();

            buffer.resize(offset + sizeof(string_length) + string_length);
            /* записываем длину строки */
            std::copy_n(reinterpret_cast<const char *>(&string_length), sizeof(string_length), buffer.begin() + offset);
            offset += sizeof(string_length);

            /* записываем саму строку */
            std::copy_n(string_value.c_str(), string_length, buffer.begin() + offset);
            offset += static_cast<ptrdiff_t>(string_length);
        }
    }
    return buffer;
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

Row Table::get_values_from_row(const std::vector<char> &data, const std::vector<Column> &columns) {
    Row result;
    ptrdiff_t offset = 0;
    for (const auto& column: columns) {
        Value value;
        /* если колонка может быть null, то перед каждым значением лежит bool значение, может ли быть null */
        bool is_null = false;
        if (column._is_nullable) {
            std::copy_n(data.begin() + offset, sizeof(is_null), reinterpret_cast<char *>(&is_null));
            offset += sizeof(is_null);
        }

        if (is_null) value = Null{};

        else {
            switch (column._type) {
                case DataType::Int:
                    /* считываем один элемент */
                    std::copy_n(data.begin() + offset, sizeof(int), reinterpret_cast<char *>(&value));
                    offset += sizeof(int);
                    break;
                case DataType::String:
                    /* считываем сначала длину строки */
                    size_t length_string;
                    std::copy_n(data.begin() + offset, sizeof(size_t), reinterpret_cast<char *>(&length_string));
                    offset += sizeof(size_t);

                    /* считываем строку */
                    std::copy_n(data.begin() + offset, length_string, reinterpret_cast<char *>(&value));
                    offset += static_cast<ptrdiff_t>(length_string);
                    break;
            }
        }
        result[column] = value;
    }
    return result;
}

Row Table::get_empty_values() {
    Row result;
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
