//
// Created by rvova on 25.04.2026.

#ifndef MINIDB_DATATYPES_H
#define MINIDB_DATATYPES_H
#include <any>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <optional>
#include <variant>


class Null {
};

class Column;

/* ========================================== ALIASES ========================================== */
using Value = std::variant<int, std::string, Null>;
using Row = std::map<Column, Value>;

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

    void update_element(ptrdiff_t slot_id, const std::vector<char> &data);

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

    void erase_element_from_page(ptrdiff_t page_id, ptrdiff_t slot_id) const;

    void update_element_into_page(ptrdiff_t page_id, ptrdiff_t slot_id, const std::vector<char> &data) const; // TODO: impl it!
};

enum class DataType { Int, String, Null };

class Column {
    ptrdiff_t _column_id = -1;
    std::string _name = "";
    DataType _type = DataType::Null;
    bool _is_nullable = false;
    bool _is_indexed = false;

    friend class Table;

public:
    Column() = default;

    bool operator<(const Column &column) const;

    explicit Column(const std::string &name, DataType type, bool is_nullable = false, bool is_indexed = false);
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

    /**
     * Полиморфная функция для единого интерфейса для каждого условия
     * @param column_values Словарь данных со строки. Ключ - колонка, значение - значение в этой колонке
     * @return True - условие выполняется, False - условие не выполняется
     */
    virtual bool evaluate(const Row& column_values) const = 0;
};

class ComparisonCondition : public Condition {
    ComparisonDataType _comparison_type;
    std::any _value;

public:
    ComparisonCondition(Column column, ComparisonDataType comparison_type, std::any value);

    bool evaluate(const Row& column_values) const override;
};

class BetweenCondition : public Condition {
    Column _column_start;
    Column _column_end;

public:
    BetweenCondition(Column column, Column column_start, Column column_end);

    bool evaluate(const Row& column_values) const override;
};

class RegexCondition : public Condition {
    std::string _regex;

public:
    RegexCondition(Column column, std::string regex);

    bool evaluate(const Row& column_values) const override;
};


/**
 * На первой страницы файла {name}.binary хранится :
 * ptrdiff_t size_name
 * std::string name
 * ptrdiff_t size_columns
 * std::vector<Column> columns
 * ptrdiff_t size_pages
 */
class Table {
    std::fstream _file;

    void move_to_position_columns();

    void move_to_position_count_page();

    void move_to_position_pages();

    ptrdiff_t get_pages_begin_offset();

    Row get_empty_values();

    Row get_completed_values(const std::vector<Column> &columns, const std::vector<Value> &values);

    std::vector<char> get_bytes_from_row(const Row& column_values);

    explicit Table(const std::filesystem::path &path, const std::string &name,
                   const std::optional<std::vector<Column> > &columns, bool need_create);

public:
    static Table create_table(const std::filesystem::path &path, const std::string &name,
                              const std::vector<Column> &columns);

    static Table load_table(const std::filesystem::path &path, const std::string &name);

    void insert_elements(const std::vector<Column> &columns, const std::vector<Value> &values);

    void update_elements(std::unique_ptr<Condition> condition, const std::vector<Column> &columns,
                         const std::vector<Value> &values);

    void delete_elements(std::unique_ptr<Condition> condition); // TODO: impl it!

    std::vector<Row> select_elements(std::unique_ptr<Condition> condition); // TODO: impl it!

    std::string get_name();

    std::vector<Column> get_columns();

    ptrdiff_t get_count_pages();

    static Row get_values_from_row(const std::vector<char> &data,
                                                       const std::vector<Column> &columns);
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

    explicit Database(const std::string &name, bool need_create);

public:
    static Database create_database(const std::string &name);

    static Database load_database(const std::string &name);

    /**
     *
     * @exception DatabaseDoesNotExistException Попытка удалить несуществующую базу данных
     */
    void drop_database();

    void create_table(const std::string &name, const std::vector<Column> &columns);

    void drop_table(const std::string &name);

    void insert_elements(const std::string &table_name, const std::vector<Column> &columns,
                         const std::vector<Value> &values);

    void update_elements(const std::string &table_name, const Condition &condition, const std::vector<Column> &columns,
                         const std::vector<Value> &values);

    void delete_elements(const std::string &table_name, const Condition &condition);

    std::vector<Value> select_elements(const std::string &table_name, const std::optional<Condition> &condition);

    std::string get_name();

    std::vector<std::string> get_tables();
};


#endif //MINIDB_DATATYPES_H
