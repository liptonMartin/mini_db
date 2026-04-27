//
// Created by rvova on 25.04.2026.

#ifndef MINIDB_DATATYPES_H
#define MINIDB_DATATYPES_H
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>


/**
 * _free_size - количество в байтах свободного места на странице
 * _count_slots - количество всех слотов (и свободных, и занятых)
 */
class PageHeader {
    ptrdiff_t _free_size;
    ptrdiff_t _count_slots;

    friend class Page;
};

class Slot {
    ptrdiff_t _slot_id;
    ptrdiff_t _offset;
    ptrdiff_t _size;
    bool _is_occupied;

public:
    Slot() = default;

    Slot(ptrdiff_t slot_id, ptrdiff_t offset, ptrdiff_t size);

    ptrdiff_t get_id();

    ptrdiff_t get_size();

    ptrdiff_t get_offset();

    bool is_occupied();

    void clear();

    void make_busy(ptrdiff_t slot_id, ptrdiff_t offset, ptrdiff_t size);
};


/**
 * Страница содержит:
 * PageHeader(свободный размер этой страницы)
 * ptrdiff_t count slots
 * std::vector<Slot> slots (слоты) (растет вниз)
 * ... (свободное место)
 * slots (растут вверх)
 *
 * Это сделано для того, чтобы можно было быстро обращаться к slot_id,
 * быстро добавлять и удалять новые элементы
 */
class Page {
    std::vector<char> _data;

    void add_new_slot(const Slot &slot);

    void update_free_size(ptrdiff_t size);

    void update_count_slots(ptrdiff_t count_slots);

    /**
     *
     * @return ID первого свободного блока
     * @exception SlotNotFoundException Свободных блоков нет, нужно создавать новый
     */
    ptrdiff_t get_id_free_block();


public:
    explicit Page(std::vector<char> &&data);

    static Page create_page();

    PageHeader get_header();

    ptrdiff_t get_free_size();

    ptrdiff_t get_count_slots();

    std::vector<Slot> get_slots();

    /**
     * @return Последний слот на странице
     * @exception SlotNotFoundException Если элементов нет
     */
    Slot get_last_slot();

    /**
     * @param slot_id Индекс слота
     * @return Возвращает слот по индексу slot_id
     * @exception SlotNotFoundException Если такого индекса не существует
     */
    Slot get_slot(ptrdiff_t slot_id);

    std::vector<char> get_page_data();

    void insert_element(const std::vector<char> &data);

    void erase_element(ptrdiff_t slot_id);
};

enum class DataType { Int, String };

class Column {
    std::string _name;
    DataType _type;

    friend class Table;

public:
    explicit Column(const std::string &name, const DataType type);
};

/**
 * На первой страницы файла {name}.binary хранится :
 * ptrdiff_t size_name
 * std::string name
 * ptrdiff_t size_columns
 * std::vector<Column> columns
 * ptrdiff_t size_pages
 * std::vector<Page> pages
 */
class Table {
    std::fstream _file;

    void move_to_position_columns();

public:
    explicit Table(const std::filesystem::path &path, const std::string &name, const std::vector<Column> &columns);

    std::string get_name();

    std::vector<Column> get_columns();

    std::vector<Page> get_pages();
};

/**
 * в файле {name}.schema хранится:
 * ptrdiff_t size_name
 * std::string name
 * ptrdiff_t size_tables
 * std::vector<std::string> tables (названия таблиц)
 */
class Database {
    std::fstream _file;

    static std::filesystem::path make_path_to_file(const std::string &name);

    void move_to_position_tables_begin();

    void move_to_position_tables_end();

public:
    explicit Database(const std::string &name);

    std::string get_name();

    std::vector<std::string> get_tables();

    void insert_table(const std::string &name, const std::vector<Column> &columns);

    void drop_table(const std::string &name);
};


#endif //MINIDB_DATATYPES_H
