//
// Created by rvova on 25.04.2026.
//

#ifndef MINIDB_DATATYPES_H
#define MINIDB_DATATYPES_H
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>


class PageHeader {
    size_t _page_id;
    size_t _next_page_id;
    size_t _free_size;

public:
    explicit PageHeader(size_t page_id);

    size_t get_page_id();

    size_t get_free_size();

    void set_free_size(size_t free_size);
};

class Slot {
    size_t _offset;
    size_t _size;

public:
    Slot(size_t offset, size_t size);

    size_t get_size();

    size_t get_offset();
};


class Page {
    std::vector<char> _data;

public:
    explicit Page(size_t page_id);

    PageHeader get_header();

    std::vector<Slot> get_slots();

    std::vector<char> get_data();

    void insert_element(std::vector<char> &data);

    void erase_element(size_t slot_id);
};

enum class DataType { Int, String };

class Column {
    std::string _name;
    DataType _type;

    friend class Table;

public:
    explicit Column(const std::string& name, const DataType type);
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
