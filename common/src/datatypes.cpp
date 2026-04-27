//
// Created by rvova on 25.04.2026.

#include <fstream>

#include "datatypes.h"

#include <algorithm>
#include <filesystem>
#include <format>

#include "constants.h"
#include "exceptions.h"

Slot::Slot(const ptrdiff_t slot_id, const ptrdiff_t offset, const ptrdiff_t size) : _slot_id(slot_id), _offset(offset),
    _size(size), _is_occupied(true) {
}

ptrdiff_t Slot::get_offset() {
    return _offset;
}

ptrdiff_t Slot::get_size() {
    return _size;
}

ptrdiff_t Slot::get_id() {
    return _slot_id;
}

bool Slot::is_occupied() {
    return _is_occupied;
}

void Slot::clear() {
    _slot_id = -1;
    _offset = -1;
    _size = -1;
    _is_occupied = false;
}

void Slot::make_busy(const ptrdiff_t slot_id, const ptrdiff_t offset, const ptrdiff_t size) {
    _slot_id = slot_id;
    _offset = offset;
    _size = size;
    _is_occupied = true;
}


Page::Page(std::vector<char> &&data) : _data(std::move(data)) {
}

/**
 * Функция, которая инициализирует страницу
 */
Page Page::create_page() {
    std::vector<char> data;
    data.resize(db::PAGE_SIZE);

    PageHeader header{};
    header._free_size = db::PAGE_SIZE - sizeof(header);
    header._count_slots = 0;

    std::copy_n(reinterpret_cast<const char *>(&header), sizeof(header), data.begin());

    return Page(std::move(data));
}

PageHeader Page::get_header() {
    PageHeader header{};

    std::copy_n(_data.begin(), sizeof(header), reinterpret_cast<char *>(&header));
    return header;
}

ptrdiff_t Page::get_free_size() {
    return get_header()._free_size;
}

ptrdiff_t Page::get_count_slots() {
    return get_header()._count_slots;
}

std::vector<char> Page::get_page_data() {
    return _data;
}

std::vector<Slot> Page::get_slots() {
    std::vector<Slot> slots;
    ptrdiff_t count_slots;

    std::copy_n(_data.begin() + sizeof(PageHeader), sizeof(count_slots), reinterpret_cast<char *>(&count_slots));

    for (auto i = 0; i < count_slots; ++i) {
        Slot slot;

        std::copy_n(_data.begin() + sizeof(PageHeader) + sizeof(count_slots) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));

        slots.push_back(slot);
    }

    return slots;
}

Slot Page::get_last_slot() {
    auto count_slots = get_count_slots();

    if (count_slots == 0) throw SlotNotFoundException("Page doesn't have any slots");

    /* считываем последний слот */
    Slot slot{};
    std::copy_n(_data.begin() + sizeof(PageHeader) + (count_slots - 1) * sizeof(Slot), sizeof(Slot),
                reinterpret_cast<char *>(&slot));
    return slot;
}

Slot Page::get_slot(ptrdiff_t slot_id) {
    auto count_slots = get_count_slots();
    Slot slot{};

    if (slot_id >= count_slots)
        throw SlotNotFoundException(std::format("Page doesn't have slot id {}", slot_id));

    /* проходимся по всем slots на странице и ищем slot_id */
    for (auto i = 0; i < count_slots; ++i) {
        /* считываем слот */
        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));
        if (slot.get_id() == slot_id) return slot;
    }

    throw SlotNotFoundException(std::format("Page doesn't have slot id {}", slot_id));
}


void Page::add_new_slot(const Slot &slot) {
    auto count_slots = get_count_slots();

    /* записываем новый слот */
    std::copy_n(reinterpret_cast<const char *>(&slot), sizeof(Slot),
                _data.begin() + sizeof(PageHeader) + sizeof(count_slots) + count_slots * sizeof(Slot));

    /* увеличиваем количество слотов */
    ++count_slots;

    /* обновляем данные */
    update_count_slots(count_slots);
}

void Page::update_free_size(const ptrdiff_t size) {
    auto header = get_header();

    header._free_size = size;
    std::copy_n(reinterpret_cast<const char *>(&header), sizeof(header), _data.begin());
}

void Page::update_count_slots(const ptrdiff_t count_slots) {
    auto header = get_header();

    header._count_slots = count_slots;
    std::copy_n(reinterpret_cast<const char *>(&header), sizeof(header), _data.begin());
}

ptrdiff_t Page::get_id_free_block() {
    const auto count_slots = get_count_slots();
    Slot slot{};

    /* проходимся по всем slots на странице и ищем свободный блок */
    for (auto i = 0; i < count_slots; ++i) {
        /* считываем слот */
        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));
        if (!slot.is_occupied()) return i;
    }

    throw SlotNotFoundException("Page doesn't have holes!");
}

/**
 * Вставка в страницу данных.
 * Внимание: в странице должно быть место для вставки, иначе выбрасывается исключение
 * @param data Значение, которое надо вставить
 * @exception InvalidWriteToPageException Не хватило места для вставки
 */
void Page::insert_element(const std::vector<char> &data) {
    auto len_data = std::ssize(data);
    auto old_free_size = get_free_size();
    auto old_count_slots = get_count_slots();

    ptrdiff_t id_free_block;
    try {
        id_free_block = get_id_free_block();
        if (len_data > old_free_size)
            throw InvalidWriteToPageException("Page doesn't have space for insert element!");
        update_free_size(old_free_size - len_data);

        auto slot = get_slot(id_free_block);
        // TODO: slot.make_busy();

    } catch (SlotNotFoundException) {
        id_free_block = old_count_slots; /* индексация с нуля */
        if (len_data + sizeof(Slot) > old_free_size)
            throw InvalidWriteToPageException("Page doesn't have space for insert element!");

        /* будем создавать новый слот*/
        ++old_count_slots;
        update_count_slots(old_count_slots);
        update_free_size(old_free_size - len_data - sizeof(Slot));

        Slot last_slot;
        try {
            last_slot = get_last_slot();
        } catch (SlotNotFoundException) {
            last_slot = Slot(-1, db::PAGE_SIZE, 0);
        }

        const ptrdiff_t new_offset = last_slot.get_offset() - len_data;
        const Slot slot(last_slot.get_id() + 1, new_offset, len_data);
        add_new_slot(slot);

        /* записываем новые данные по новому смещению */
        std::copy_n(data.begin(), len_data, _data.begin() + new_offset);
    }
}

void Page::erase_element(ptrdiff_t slot_id) {
    auto count_occupied_slots = get_count_occupied_slots();
    if (slot_id >= count_occupied_slots || slot_id < 0)
        throw SlotNotFoundException(std::format("Page doesn't have slot id {}", slot_id));

    auto deleted_slot = get_slot(slot_id);
    auto offset_deleted_slot = deleted_slot.get_offset();
    auto size_deleted_slot = deleted_slot.get_size();

    auto last_slot = get_last_slot();
    auto offset_last_slot = last_slot.get_offset();

    /* часть страницы, которую надо перенести */
    std::vector<char> data_to_move(_data.begin() + offset_last_slot, _data.begin() + offset_deleted_slot);

    /* сдвигаем ровно на size_deleted_slot */
    std::copy_n(data_to_move.begin(), data_to_move.size(), _data.begin() + offset_last_slot + size_deleted_slot);

    /* сдвигаем slots в начале страницы */


    // TODO: update_header () !!! update_size() !!!
}


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
