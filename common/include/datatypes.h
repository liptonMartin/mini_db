//
// Created by rvova on 25.04.2026.

#ifndef MINIDB_DATATYPES_H
#define MINIDB_DATATYPES_H
#include <any>
#include <cstddef>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <variant>

using Values = std::variant<int, std::string>;

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

    ptrdiff_t get_id() const;

    ptrdiff_t get_size() const;

    ptrdiff_t get_offset() const;

    void set_offset(ptrdiff_t offset);

    bool is_occupied() const;

    void release();

    void make_busy(ptrdiff_t offset, ptrdiff_t size);
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
     * @param slot_id ID слота, начиная с которого, нужно обновить offsets
     * @param shift_offset На сколько изменились offset (размер удаляемого элемента)
     */
    void update_offsets_in_slots(ptrdiff_t slot_id, ptrdiff_t shift_offset);

    /**
     *
     * @return ID первого свободного блока
     * @exception SlotNotFoundException Свободных блоков нет, нужно создавать новый
     */
    ptrdiff_t get_id_first_free_block();

    void make_busy_slot(Slot &slot, ptrdiff_t offset, ptrdiff_t size);

    void release_slot(Slot &slot);

    std::vector<char> get_slot_data(const Slot &slot);

public:
    explicit Page(std::vector<char> &&data);

    static Page create_page();

    PageHeader get_header();

    ptrdiff_t get_free_size();

    ptrdiff_t get_count_slots();

    std::vector<Slot> get_slots();

    std::vector<Slot> get_occupied_slots();

    std::vector<Slot> get_free_slots();

    /**
     * @return Последний слот на странице
     * @exception SlotNotFoundException Если элементов нет
     */
    Slot get_last_occupied_slot();

    /**
     * @param slot_id Индекс слота
     * @return Возвращает слот по индексу slot_id
     * @exception SlotNotFoundException Если такого индекса не существует
     */
    Slot get_slot(ptrdiff_t slot_id);

    std::vector<char> get_page_data();

    ptrdiff_t insert_element(const std::vector<char> &data);

    void erase_element(ptrdiff_t slot_id);

    /**
     * @return Вектор из самих данных
     */
    std::vector<std::vector<char> > get_slots_data();

    std::vector<char> get_slot_data_by_id(ptrdiff_t slot_id);
};

class PageManager {
    std::fstream &_file;
    ptrdiff_t _pages_begin_offset;

    ptrdiff_t page_offset(ptrdiff_t page_id) const;

public:
    PageManager(std::fstream &file, ptrdiff_t pages_begin_offset);

    ptrdiff_t allocate_page();

    ptrdiff_t search_free_page(ptrdiff_t need_size);

    Page read_page(ptrdiff_t page_id) const;

    /**
     * Обертка над Page::insert_element, записывает полезные данные (data) на страницу,
     * автоматически изменяет PageHeader на странице
     * Внимание, на странице должно быть свободное место для вставки!
     * @param page_id ID страницы
     * @param data Полезная информация, которую нужно добавить.
     * @return ID slot, в который вставлен новый элемент
     */
    ptrdiff_t insert_element_into_page(ptrdiff_t page_id, const std::vector<char> &data) const;

    void erase_element_from_page(ptrdiff_t page_id, ptrdiff_t slot_id);
};

enum class DataType { Int, String };

class Column {
    std::string _name = "";
    DataType _type;

    friend class Table;

public:
    Column() = default;

    explicit Column(const std::string &name, const DataType type);
};


enum class ComparisonDataType {
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
};


class Condition {
    Column _column{};

public:
    virtual ~Condition() = default;

    virtual bool evaluate() const = 0;
};

class ComparisonCondition : public Condition {
    ComparisonDataType _comparison_type;
    std::any _value;

public:
    ComparisonCondition(Column column, ComparisonDataType comparison_type, std::any value);

    bool evaluate() const override;
};

class BetweenCondition : public Condition {
    Column _column_start;
    Column _column_end;

public:
    BetweenCondition(Column column, Column column_start, Column column_end);

    bool evaluate() const override;
};

class RegexCondition : public Condition {
    std::string _regex;

public:
    RegexCondition(Column column, std::string regex);

    bool evaluate() const override;
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

    void insert_elements(); // TODO: edit declaration

    void update_elements(); // TODO: edit declaration

    void delete_elements(); // TODO: edit declaration

    std::vector<Values> select_elements(); // TODO: edit declaration
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

    static std::filesystem::path make_path_to_file(const std::string &db_name);

    void move_to_position_tables_begin();

    void move_to_position_tables_end();

    explicit Database(const std::string &name);

public:
    static Database create_database(const std::string &name);
    static Database load_database(const std::string &name);

    /**
     *
     * @param db_name Имя базы данных
     * @exception DatabaseDoesNotExistException Попытка удалить несуществующей базы данных
     */
    void drop_database(const std::string& db_name);

    std::string get_name();

    std::vector<std::string> get_tables();

    void create_table(const std::string &name, const std::vector<Column> &columns);

    void drop_table(const std::string &name); // TODO: impl it!

    void insert_elements(const std::string &table_name, const std::vector<Column> &columns,
                         const std::vector<std::variant<int, std::string> > &values); // TODO: impl it!

    void update_elements(const std::string &table_name, const Condition &condition, const std::vector<Column> &columns,
                         const std::vector<Values> &values); // TODO: impl it!

    void delete_elements(const std::string &table_name, const Condition &condition); // TODO: impl it!

    std::vector<Values> select_elements(const std::string &table_name, const std::optional<Condition> &condition);
};


#endif //MINIDB_DATATYPES_H
