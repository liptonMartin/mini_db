#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <initializer_list>
#include <not_implemented.h>

#ifndef SYS_PROG_BS_PLUS_TREE_H
#define SYS_PROG_BS_PLUS_TREE_H

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BSP_tree final : private compare {
public:
    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:
    static constexpr size_t minimum_keys_in_root = 1;
    static constexpr size_t maximum_keys_in_root = 4 * t - 1;

    static constexpr size_t minimum_keys_in_node = 2 * t - 1;
    static constexpr size_t maximum_keys_in_node = 3 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey &lhs, const tkey &rhs) const;

    inline bool compare_pairs(const tree_data_type &lhs, const tree_data_type &rhs) const;

    bool less(tkey a, tkey b) { return compare::operator()(a, b); }
    bool greater(tkey a, tkey b) { return compare::operator()(b, a); }
    bool equal(tkey a, tkey b) { return !less(a, b) && !greater(a, b); }
    bool not_equal(tkey a, tkey b) { return !equal(a, b); }
    bool less_or_equal(tkey a, tkey b) { return less(a, b) || equal(a, b); }
    bool greater_or_equal(tkey a, tkey b) { return greater(a, b) || equal(a, b); }

    // endregion comparators declaration

    struct bsptree_node_base {
        bool _is_terminated;

        bsptree_node_base() noexcept;

        virtual size_t keys_size() = 0;

        virtual boost::container::static_vector<tkey, maximum_keys_in_root + 1> keys() = 0;

        virtual ~bsptree_node_base() = default;
    };

    struct bsptree_node_term : public bsptree_node_base {
        bsptree_node_term *_next;
        boost::container::static_vector<tree_data_type, maximum_keys_in_root + 1> _data;

        bsptree_node_term() noexcept;

        size_t keys_size() override { return _data.size(); }

        boost::container::static_vector<tkey, maximum_keys_in_root + 1> keys() override {
            auto keys = boost::container::static_vector<tkey, maximum_keys_in_root + 1>{};
            for (auto &elem: _data) {
                keys.push_back(elem.first);
            }
            return keys;
        };
    };

    struct bsptree_node_middle : public bsptree_node_base {
        boost::container::static_vector<tkey, maximum_keys_in_root + 1> _keys;
        boost::container::static_vector<bsptree_node_base *, maximum_keys_in_root + 2> _pointers;

        bsptree_node_middle() noexcept;

        size_t keys_size() override { return _keys.size(); }
        boost::container::static_vector<tkey, maximum_keys_in_root + 1> keys() override { return _keys; };
    };

    pp_allocator<value_type> _allocator;
    bsptree_node_base *_root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

public:
    // region constructors declaration

    explicit BSP_tree(const compare &cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit BSP_tree(pp_allocator<value_type> alloc, const compare &comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BSP_tree(iterator begin, iterator end, const compare &cmp = compare(),
                      pp_allocator<value_type> = pp_allocator<value_type>());

    BSP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp = compare(),
             pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    BSP_tree(const BSP_tree &other);

    BSP_tree(BSP_tree &&other) noexcept;

    BSP_tree &operator=(const BSP_tree &other);

    BSP_tree &operator=(BSP_tree &&other) noexcept;

    ~BSP_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class bsptree_iterator;
    class bsptree_const_iterator;

    class bsptree_iterator final {
        bsptree_node_term *_node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type &;
        using pointer = value_type *;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bsptree_iterator;

        friend class BSP_tree;
        friend class bsptree_const_iterator;

        reference operator*() const noexcept;

        pointer operator->() const noexcept;

        self &operator++();

        self operator++(int);

        bool operator==(const self &other) const noexcept;

        bool operator!=(const self &other) const noexcept;

        size_t current_node_keys_count() const noexcept;

        size_t index() const noexcept;

        explicit bsptree_iterator(bsptree_node_term *node = nullptr, size_t index = 0);
    };

    class bsptree_const_iterator final {
        const bsptree_node_term *_node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = const value_type &;
        using pointer = const value_type *;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bsptree_const_iterator;

        friend class BSP_tree;
        friend class bsptree_iterator;

        bsptree_const_iterator(const bsptree_iterator &it) noexcept;

        reference operator*() const noexcept;

        pointer operator->() const noexcept;

        self &operator++();

        self operator++(int);

        bool operator==(const self &other) const noexcept;

        bool operator!=(const self &other) const noexcept;

        size_t current_node_keys_count() const noexcept;

        size_t index() const noexcept;

        explicit bsptree_const_iterator(const bsptree_node_term *node = nullptr, size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue &at(const tkey &);

    const tvalue &at(const tkey &) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue &operator[](const tkey &key);

    tvalue &operator[](tkey &&key);

    // endregion element access declaration
    // region iterator begins declaration

    bsptree_iterator begin();

    bsptree_iterator end();

    bsptree_const_iterator begin() const;

    bsptree_const_iterator end() const;

    bsptree_const_iterator cbegin() const;

    bsptree_const_iterator cend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;

    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    bsptree_iterator find(const tkey &key);

    bsptree_const_iterator find(const tkey &key) const;

    bsptree_iterator lower_bound(const tkey &key);

    bsptree_const_iterator lower_bound(const tkey &key) const;

    bsptree_iterator upper_bound(const tkey &key);

    bsptree_const_iterator upper_bound(const tkey &key) const;

    bool contains(const tkey &key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<bsptree_iterator, bool> insert(const tree_data_type &data);

    std::pair<bsptree_iterator, bool> insert(tree_data_type &&data);

    template<typename... Args>
    std::pair<bsptree_iterator, bool> emplace(Args &&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    bsptree_iterator insert_or_assign(const tree_data_type &data);

    bsptree_iterator insert_or_assign(tree_data_type &&data);

    template<typename... Args>
    bsptree_iterator emplace_or_assign(Args &&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    bsptree_iterator erase(bsptree_iterator pos);

    bsptree_iterator erase(bsptree_const_iterator pos);

    bsptree_iterator erase(bsptree_iterator beg, bsptree_iterator en);

    bsptree_iterator erase(bsptree_const_iterator beg, bsptree_const_iterator en);


    bsptree_iterator erase(const tkey &key);

    // endregion modifiers declaration

    // region debug functions

    void print_tree();

    void print_node(bsptree_node_base *node, int depth = 0);

    // endregion

private:
    // region helper functions

    class path_position {
        std::vector<std::pair<bsptree_node_base *, size_t> > _path;
        size_t _index;

    public:
        path_position(
            std::vector<std::pair<bsptree_node_base *, size_t> > path = std::vector<std::pair<bsptree_node_base *,
                size_t> >{}, size_t index = 0);

        std::vector<std::pair<bsptree_node_base *, size_t> > &get_path();

        bsptree_node_base *get_last_node();

        size_t get_index();

        void set_index(size_t index);
    };

    friend class path_position;

    path_position search_terminate_node(tkey key);

    // region helper functions for emplace declaration

    void rebalancing_after_insert(std::vector<std::pair<bsptree_node_base *, size_t> > &path);

    void handle_rebalancing_root();

    void give_right_brother_middle(bsptree_node_middle *node, bsptree_node_middle *right, bsptree_node_middle *parent,
                                   size_t parent_index);

    void give_right_brother_term(bsptree_node_term *node, bsptree_node_term *right, bsptree_node_middle *parent,
                                 size_t parent_index);

    void give_left_brother_middle(bsptree_node_middle *node, bsptree_node_middle *left, bsptree_node_middle *parent,
                                  size_t parent_index);

    void give_left_brother_term(bsptree_node_term *node, bsptree_node_term *left, bsptree_node_middle *parent,
                                size_t parent_index);

    void split_middle(bsptree_node_middle *left, bsptree_node_middle *right, bsptree_node_middle *parent,
                      size_t parent_index);

    void split_term(bsptree_node_term *left, bsptree_node_term *right, bsptree_node_middle *parent,
                    size_t parent_index);

    bool is_right_brother_exist(bsptree_node_middle *parent, size_t parent_index);

    bool is_left_brother_exist(size_t parent_index);

    bool is_node_has_maximum_elements(bsptree_node_base *node);

    // end region helper functions for emplace declaration

    // region helper functions for erase declaration

    void rebalancing_after_erase(std::vector<std::pair<bsptree_node_base *, size_t> > &path, tkey &deleted_key);

    void handle_rebalancing_from_empty_root();

    void merge_with_root();

    void update_references_in_parent(std::vector<std::pair<bsptree_node_base *, size_t> > &path, tkey &deleted_key,
                                     tkey new_key);

    void borrow_from_right_middle(bsptree_node_middle *node, bsptree_node_middle *right,
                                  bsptree_node_middle *parent, size_t parent_index);

    void borrow_from_right_term(bsptree_node_term *node, bsptree_node_term *right,
                                bsptree_node_middle *parent, size_t parent_index);

    void borrow_from_right_right_middle(bsptree_node_middle *node, bsptree_node_middle *right,
                                        bsptree_node_middle *right_right,
                                        bsptree_node_middle *parent, size_t parent_index);

    void borrow_from_right_right_term(bsptree_node_term *node, bsptree_node_term *right, bsptree_node_term *right_right,
                                      bsptree_node_middle *parent, size_t parent_index);


    void borrow_from_left_middle(bsptree_node_middle *node, bsptree_node_middle *left,
                                 bsptree_node_middle *parent, size_t parent_index);

    void borrow_from_left_term(bsptree_node_term *node, bsptree_node_term *left,
                               bsptree_node_middle *parent, size_t parent_index);

    void borrow_from_left_left_middle(bsptree_node_middle *node, bsptree_node_middle *left,
                                      bsptree_node_middle *left_left,
                                      bsptree_node_middle *parent, size_t parent_index);

    void borrow_from_left_left_term(bsptree_node_term *node, bsptree_node_term *left, bsptree_node_term *left_left,
                                    bsptree_node_middle *parent, size_t parent_index);

    void merge_middle(bsptree_node_middle *left, bsptree_node_middle *node, bsptree_node_middle *right,
                      bsptree_node_middle *parent, size_t parent_index);

    void merge_term(bsptree_node_term *left, bsptree_node_term *node, bsptree_node_term *right,
                    bsptree_node_middle *parent, size_t parent_index);

    bool is_node_has_more_minimum_keys(bsptree_node_base *node);

    // endregion helper functions for erase declaration

    bsptree_node_term *first_terminate_node();


    // endregion
};


// region debug impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::print_tree() {
    if (_root) print_node(_root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::print_node(bsptree_node_base *node, int depth) {
    std::cout << std::string(depth, ' ');

    auto keys = node->keys();
    std::cout << '[';
    for (int i = 0; i < node->keys_size(); ++i) {
        std::cout << (keys[i]);
        if (i != node->keys_size() - 1) std::cout << "|";
    }
    std::cout << "]\n" << std::flush;

    if (auto node_middle = dynamic_cast<bsptree_node_middle *>(node)) {
        std::cout << "(" << keys[0] << ')';
        print_node(node_middle->_pointers[0], depth + 1);

        for (int i = 0; i < node->keys_size(); ++i) {
            if (node_middle->_pointers[i + 1]) {
                std::cout << '(' << keys[i] << ')';
                print_node(node_middle->_pointers[i + 1], depth + 1);
            }
        }
    }
}

// endregion debug impl


// region comparators impl

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type>
    compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
    std::size_t t = 5, typename U>
BSP_tree(iterator begin, iterator end, const compare &cmp = compare(),
         pp_allocator<U> = pp_allocator<U>()) -> BSP_tree<typename std::iterator_traits<
    iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BSP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp = compare(),
         pp_allocator<U> = pp_allocator<U>()) -> BSP_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::compare_pairs(const BSP_tree::tree_data_type &lhs,
                                                       const BSP_tree::tree_data_type &rhs) const {
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const {
    return compare::operator()(lhs, rhs);
}

// endregion comparators impl

// region bsptree_node_base implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_base::bsptree_node_base() noexcept : _is_terminated(false) {
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_term::bsptree_node_term() noexcept {
    this->_is_terminated = true;
    _next = nullptr;
    _data = boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1>{};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_middle::bsptree_node_middle() noexcept {
    this->_is_terminated = false;
    _keys = boost::container::static_vector<tkey, maximum_keys_in_node + 1>{};
    _pointers = boost::container::static_vector<bsptree_node_base *, maximum_keys_in_node + 2>{};
}

// endregion bsptree_node_base implementation

// region BSP_tree constructor implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BSP_tree<tkey, tvalue, compare, t>::value_type> BSP_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept {
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(const compare &cmp, pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(pp_allocator<value_type> alloc, const compare &cmp)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(iterator begin, iterator end, const compare &cmp,
                                             pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
    while (begin != end) {
        insert(*begin);
        ++begin;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp,
                                             pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
    for (auto item: data) insert_or_assign(item);
}

// endregion BSP_tree constructor implementations

// region BSP_tree copy and move constructors

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(const BSP_tree &other)
    : compare(other), _allocator(other._allocator), _root(nullptr), _size(0) {
    for (auto it = other->begin(); it != other->end(); ++it) {
        insert(it);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(BSP_tree &&other) noexcept
    : compare(std::move(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size) {
    other._root = nullptr;
    other._size = 0;
}

// endregion BSP_tree copy and move constructors

// region BSP_tree copy and move assignment operators

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t> &BSP_tree<tkey, tvalue, compare, t>::operator=(const BSP_tree &other) {
    if (this != &other) {
        auto tmp = BSP_tree(other);
        clear();
        *this = tmp;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t> &BSP_tree<tkey, tvalue, compare, t>::operator=(BSP_tree &&other) noexcept {
    if (this != &other) {
        clear();

        static_cast<compare &>(*this) = std::move(other);
        _allocator = std::move(other._allocator);
        _size = other._size;
        _root = other._root;

        other._root = nullptr;
        other._size = 0;
    }
    return *this;
}

// endregion BSP_tree copy and move assignment operators

// region BSP_tree desctructor

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::~BSP_tree() noexcept {
    clear();
}

// endregion BSP_tree desctructor

// region BSP_tree iterators implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::bsptree_iterator(bsptree_node_term *node, size_t index) {
    _node = node;
    _index = index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::reference BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator*() const noexcept {
    return reinterpret_cast<reference>(_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::pointer BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator->() const noexcept {
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator &BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator++() {
    if (_node == nullptr) return *this; /* уже в конце */

    /* если мы в конце текущего узла */
    if (_index + 1 >= current_node_keys_count()) {
        _index = 0;
        _node = _node->_next;
    }
    /* сдвигаемся в текущем узле на следующий элемент */
    else {
        ++_index;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator++(int) {
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator==(const self &other) const noexcept {
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator!=(const self &other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::current_node_keys_count() const noexcept {
    return _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::index() const noexcept {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::bsptree_const_iterator(const bsptree_node_term *node,
    size_t index) {
    _node = node;
    _index = index;
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::bsptree_const_iterator(const bsptree_iterator &it) noexcept {
    _node = it._node;
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::reference BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator*() const noexcept {
    return reinterpret_cast<reference>(_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::pointer BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator->() const noexcept {
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator &BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator++() {
    if (_node == nullptr) return *this; /* уже в конце */

    /* если мы в конце текущего узла */
    if (_index + 1 >= current_node_keys_count()) {
        _index = 0;
        _node = _node->_next;
    }
    /* сдвигаемся в текущем узле на следующий элемент */
    else {
        ++_index;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator++(int) {
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator==(const self &other) const noexcept {
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator!=(const self &other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::current_node_keys_count() const noexcept {
    return _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::index() const noexcept {
    return _index;
}

// endregion BSP_tree iterators implementations

// region BSP_tree element access implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BSP_tree<tkey, tvalue, compare, t>::at(const tkey &key) {
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("This key doesn't exist!");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue &BSP_tree<tkey, tvalue, compare, t>::at(const tkey &key) const {
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("This key doesn't exist!");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BSP_tree<tkey, tvalue, compare, t>::operator[](const tkey &key) {
    auto iterator = find(key);
    if (iterator == end()) {
        /* добавляем элемент по умолчанию */
        iterator = insert(key, tvalue{});
    }

    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BSP_tree<tkey, tvalue, compare, t>::operator[](tkey &&key) {
    return operator[](std::move(key));
}

// endregion BSP_tree element access implementations

// region BSP_tree iterator begins implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::begin() {
    auto node = first_terminate_node();
    return bsptree_iterator(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::end() {
    return bsptree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::begin() const {
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::end() const {
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::cbegin() const {
    auto *node = (const_cast<BSP_tree *>(this))->first_terminate_node();
    return bsptree_const_iterator(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::cend() const {
    return bsptree_const_iterator();
}

// endregion BSP_tree iterator begins implementations

// region BSP_tree lookup implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::size() const noexcept {
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::empty() const noexcept {
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::find(const tkey &key) {
    if (_root == nullptr) return end();

    auto *node = _root;
    while (node) {
        auto keys = node->keys();
        /* бинарный поиск по ключам */
        size_t l = 0;
        size_t r = keys.size();
        while (l + 1 < r) {
            size_t m = (l + r) / 2;

            if (less_or_equal(keys[m], key)) {
                /* keys[m] <= key */
                l = m;
            } else {
                r = m;
            }
        }

        /* такой ключ существует в листе */
        if (node->_is_terminated && equal(keys[l], key))
            return bsptree_iterator(
                dynamic_cast<bsptree_node_term *>(node), l);

        /* мы уже в листовых узлах или нужно перейти в самого левого ребенка, значит l увеличивать не нужно */
        if (!(node->_is_terminated || less(key, keys[l]))) ++l;

        auto middle_node = dynamic_cast<bsptree_node_middle *>(node);
        if (middle_node != nullptr) node = middle_node->_pointers[l];
        else node = nullptr;
    }

    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::find(
    const tkey &key) const {
    return bsptree_const_iterator(const_cast<BSP_tree *>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::lower_bound(const tkey &key) {
    if (_root == nullptr) return end();

    auto *node = _root;
    auto *last_node = node;
    size_t index = 0;
    while (node != nullptr) {
        auto keys = node->keys();
        /* бинарный поиск по ключам */
        size_t l = 0;
        size_t r = keys.size();
        while (l + 1 < r) {
            size_t m = (l + r) / 2;

            if (less_or_equal(keys[m], key)) {
                /* keys[m] <= key */
                l = m;
            } else {
                r = m;
            }
        }

        /* такой ключ существует в листе */
        if (node->_is_terminated && equal(keys[l], key))
            return bsptree_iterator(
                dynamic_cast<bsptree_node_term *>(node), l);

        /* уже дошли до листового узла или нужно перейти в самого левого ребенка, значит l увеличивать не нужно */
        if (!(node->_is_terminated || less(key, keys[l]))) ++l;

        auto middle_node = dynamic_cast<bsptree_node_middle *>(node);
        if (middle_node != nullptr) {
            node = middle_node->_pointers[l];
            last_node = node;
        } else node = nullptr;
        index = l;
    }

    auto iterator = bsptree_iterator(dynamic_cast<bsptree_node_term *>(last_node), index);
    /* в случае (index == 0 && key < (*iterator).first) мы уже находимся на элементе, который больше нашего */
    if (iterator->first == key || (index == 0 && key < iterator->first)) return iterator;
    return ++iterator; /* следующий элемент */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::lower_bound(
    const tkey &key) const {
    return bstree_const_iterator(const_cast<BSP_tree *>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::upper_bound(const tkey &key) {
    auto iterator = lower_bound(key);

    if (iterator == end()) return iterator;
    if (iterator->first == key) return ++iterator;
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::upper_bound(
    const tkey &key) const {
    return bstree_const_iterator(const_cast<BSP_tree *>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::contains(const tkey &key) const {
    return find(key) != end();
}

// endregion BSP_tree lookup implementations

// region BSP_tree modifiers implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::clear() noexcept {
    while (!empty()) erase(begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare,
    t>::insert(const tree_data_type &data) {
    return emplace(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare,
    t>::insert(tree_data_type &&data) {
    return emplace(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare,
    t>::emplace(Args &&... args) {
    tree_data_type data(std::forward<Args>(args)...);

    if (contains(data.first)) return {end(), false};

    auto path_to_root = search_terminate_node(data.first);

    auto *last_node = path_to_root.get_last_node();
    auto index = path_to_root.get_index();
    auto &path = path_to_root.get_path();

    if (last_node == nullptr) {
        /* дерево пустое */
        auto new_root = get_allocator().template new_object<bsptree_node_term>();
        new_root->_data.push_back(data);
        _root = new_root;
        path.emplace_back(new_root, 0);
    } else {
        auto terminate_node = dynamic_cast<bsptree_node_term *>(last_node);
        if (terminate_node == nullptr) {
            const std::string message = std::format("The node(first key = {}) isn't a leaf!", data.first);
            throw std::logic_error(message);
        }
        terminate_node->_data.insert(terminate_node->_data.begin() + index, data);
    }

    rebalancing_after_insert(path);
    ++_size;
    return {find(data.first), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(
    const tree_data_type &data) {
    emplace_or_assign(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(
    tree_data_type &&data) {
    emplace_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::emplace_or_assign(Args &&... args) {
    tree_data_type data(std::forward<Args>(args)...);

    bsptree_iterator found_key = find(data.first);
    if (found_key != end()) {
        /* такой ключ уже существует */
        auto *node = found_key._node;
        auto index = found_key._index;
        auto node_term = dynamic_cast<bsptree_node_term *>(node);
        if (!node_term) throw std::logic_error("Node isn't a leaf!");

        node_term->_data[index].second = data.second;
        return found_key;
    }

    return insert(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_iterator pos) {
    return erase(bsptree_const_iterator(pos));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_const_iterator pos) {
    if (pos == end()) return end();

    auto key = pos->first;
    auto path_to_root = search_terminate_node(key);

    auto *last_node = path_to_root.get_last_node();
    auto index = path_to_root.get_index();
    auto &path = path_to_root.get_path();

    auto last_node_term = dynamic_cast<bsptree_node_term *>(last_node);
    if (last_node_term == nullptr) {
        std::string message = std::format("Attempt to erase node(first key = {}) is not a leaf!", last_node->keys()[0]);
        throw std::logic_error(message);
    }

    auto deleted_element = last_node_term->_data[index];
    last_node_term->_data.erase(last_node_term->_data.begin() + index);
    rebalancing_after_erase(path, deleted_element.first);
    --_size;
    return upper_bound(key); /* следующий элемент после удаленного */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_iterator beg, bsptree_iterator en) {
    auto iterator = beg;
    while (iterator != en) {
        erase(iterator);
        ++iterator;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_const_iterator beg, bsptree_const_iterator en) {
    auto iterator = beg;
    while (iterator != en) {
        erase(iterator);
        ++iterator;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::erase(const tkey &key) {
    return erase(find(key));
}

// endregion BSP_tree modifiers implementations

// region helper functions

// region helper functions for emplace impl

/**
 * Обертка над lower_bound, которая возвращает путь до узла.
 * В случае, если дерево пустое, то оно не создает новый узел
 * @param key Ключ, по которому производится поиск
 * @return Путь от корня до узла
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::path_position BSP_tree<tkey, tvalue, compare, t>::search_terminate_node(tkey key) {
    if (_root == nullptr) return path_position();

    auto *node = _root;
    size_t index = 0;
    std::vector<std::pair<bsptree_node_base *, size_t> > path{};
    path.emplace_back(_root, 0);
    tkey less_or_equal_key = node->keys()[index]; /* будем хранить последний ключ, который мы посетили */

    while (node) {
        auto keys = node->keys();
        /* бинарный поиск по ключам */
        size_t l = 0;
        size_t r = keys.size();
        while (l + 1 < r) {
            size_t m = (l + r) / 2;

            if (less_or_equal(keys[m], key)) {
                /* keys[m] <= key */
                l = m;
            } else {
                r = m;
            }
        }

        less_or_equal_key = keys[l];

        /* нужно перейти в самого левого ребенка или дошли до конца, значит l увеличивать не нужно */
        if (!(node->_is_terminated || less(key, keys[l]))) ++l;

        auto middle_node = dynamic_cast<bsptree_node_middle *>(node);
        if (middle_node != nullptr) {
            node = middle_node->_pointers[l];
            path.emplace_back(middle_node->_pointers[l], l);
        } else node = nullptr;
        index = l;
    }

    /* в случае (index == 0 && key < less_or_equal_key) мы уже находимся на элементе, который больше нашего */
    if (less_or_equal_key == key || (index == 0 && less(key, less_or_equal_key))) return path_position(path, index);
    /* в случае, если мы находимся на последнем элементе, то как раз ++index будет указывать на последний (еще несуществующий) элемент*/
    return path_position(path, ++index);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::rebalancing_after_insert(
    std::vector<std::pair<bsptree_node_base *, size_t> > &path) {
    auto [node, parent_index] = path.back();
    path.pop_back();

    if (node->keys_size() <= maximum_keys_in_node) return; /* балансировка точно не нужна */

    if (node == _root && node->keys_size() <= maximum_keys_in_root) return; /* балансировка точно не нужна */
    if (node == _root) {
        /* балансировка нужна корню */
        handle_rebalancing_root();
        return;
    }

    auto [parent, parent_parent_index] = path.back();
    auto parent_middle = dynamic_cast<bsptree_node_middle *>(parent);
    if (!parent_middle) throw std::logic_error("Parent isn't a middle node!");

    auto node_term = dynamic_cast<bsptree_node_term *>(node);
    auto node_middle = dynamic_cast<bsptree_node_middle *>(node);

    /* пробуем отдать правому брату */
    if (is_right_brother_exist(parent_middle, parent_index)) {
        auto right_brother = parent_middle->_pointers[parent_index + 1];
        if (!is_node_has_maximum_elements(right_brother)) {
            /* значит можно отдать правому брату */
            if (auto right_middle = dynamic_cast<bsptree_node_middle *>(right_brother)) {
                give_right_brother_middle(node_middle, right_middle, parent_middle, parent_index);
            } else if (auto right_term = dynamic_cast<bsptree_node_term *>(right_brother)) {
                give_right_brother_term(node_term, right_term, parent_middle, parent_index);
            } else {
                throw std::logic_error("Right brother has unknown type!");
            }
            return; /* балансировка закончилась */
        }
    }

    /* отдать правому брату не получилось, поэтому пытаемся отдать левому брату */
    if (is_left_brother_exist(parent_index)) {
        auto left_brother = parent_middle->_pointers[parent_index - 1];
        if (!is_node_has_maximum_elements(left_brother)) {
            /* значит можно отдать левому брату */
            if (auto left_middle = dynamic_cast<bsptree_node_middle *>(left_brother)) {
                give_left_brother_middle(node_middle, left_middle, parent_middle, parent_index);
            } else if (auto left_term = dynamic_cast<bsptree_node_term *>(left_brother)) {
                give_left_brother_term(node_term, left_term, parent_middle, parent_index);
            } else {
                throw std::logic_error("Left brother has unknown type!");
            }

            return; /* балансировка закончилась */
        }
    }

    /* отдать братьям не получилось, пытаемся разделить два соседних узла в три */
    /* пытаемся разделиться с правым братом */
    if (is_right_brother_exist(parent_middle, parent_index)) {
        auto right_brother = parent_middle->_pointers[parent_index + 1];
        if (auto right_middle = dynamic_cast<bsptree_node_middle *>(right_brother)) {
            split_middle(node_middle, right_middle, parent_middle, parent_index);
        } else if (auto right_term = dynamic_cast<bsptree_node_term *>(right_brother)) {
            split_term(node_term, right_term, parent_middle, parent_index);
        } else {
            throw std::logic_error("Right brother has unknown type!");
        }
        rebalancing_after_insert(path);
        return;
    }

    /* пытаемся разделиться с левым братом */
    if (is_left_brother_exist(parent_index)) {
        auto left_brother = parent_middle->_pointers[parent_index - 1];
        if (auto left_middle = dynamic_cast<bsptree_node_middle *>(left_brother)) {
            split_middle(left_middle, node_middle, parent_middle, parent_index - 1);
        } else if (auto left_term = dynamic_cast<bsptree_node_term *>(left_brother)) {
            split_term(left_term, node_term, parent_middle, parent_index - 1);
        } else {
            throw std::logic_error("Left brother has unknown type!");
        }
        rebalancing_after_insert(path);
        return;
    }
    throw std::logic_error("The node hasn't brothers and it isn't a root!");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::handle_rebalancing_root() {
    /* новый корень будет обязательно middle элементом */
    auto new_root = get_allocator().template new_object<bsptree_node_middle>();

    auto index_middle_element = _root->keys_size() / 2;
    auto middle_element = _root->keys()[index_middle_element];

    /* заполняем новый корень */
    new_root->_keys.push_back(middle_element);

    if (auto old_root_term = dynamic_cast<bsptree_node_term *>(_root)) {
        auto second_node = get_allocator().template new_object<bsptree_node_term>();

        /* обновляем указатель на next */
        old_root_term->_next = second_node;

        /* добавляем со средним элементом */
        second_node->_data.insert(second_node->_data.begin(), old_root_term->_data.begin() + index_middle_element,
                                  old_root_term->_data.end());
        old_root_term->_data.erase(old_root_term->_data.begin() + index_middle_element, old_root_term->_data.end());

        new_root->_pointers.push_back(_root);
        new_root->_pointers.push_back(dynamic_cast<bsptree_node_base *>(second_node));
    } else if (auto old_root_middle = dynamic_cast<bsptree_node_middle *>(_root)) {
        auto second_node = get_allocator().template new_object<bsptree_node_middle>();


        second_node->_keys.insert(second_node->_keys.begin(), old_root_middle->_keys.begin() + index_middle_element + 1,
                                  old_root_middle->_keys.end());
        /* удаляем еще и средний элемент */
        old_root_middle->_keys.erase(old_root_middle->_keys.begin() + index_middle_element,
                                     old_root_middle->_keys.end());


        second_node->_pointers.insert(second_node->_pointers.begin(),
                                      old_root_middle->_pointers.begin() + index_middle_element + 1,
                                      old_root_middle->_pointers.end());
        old_root_middle->_pointers.erase(old_root_middle->_pointers.begin() + index_middle_element + 1,
                                         old_root_middle->_pointers.end());

        new_root->_pointers.push_back(_root);
        new_root->_pointers.push_back(dynamic_cast<bsptree_node_base *>(second_node));
    } else throw std::logic_error("Root has unknown type!");

    _root = dynamic_cast<bsptree_node_base *>(new_root);
}


/**
 * Функция забирает последний элемент из node, отдать правому брату, при этому мы находимся во внутренних узлах
 * @param node Узел, откуда надо забрать последний элемент и отдать правому брату
 * @param right Правый брат
 * @param parent Родитель
 * @param parent_index Индекс сына в родителе, откуда мы попали в node
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::give_right_brother_middle(bsptree_node_middle *node,
                                                                   bsptree_node_middle *right,
                                                                   bsptree_node_middle *parent,
                                                                   size_t parent_index) {
    auto last_element_node = node->_keys.back();
    node->_keys.pop_back();

    auto element_from_parent = parent->_keys[parent_index];
    parent->_keys[parent_index] = last_element_node;

    right->_keys.insert(right->_keys.begin(), element_from_parent);

    auto last_child_node = node->_pointers.back();
    node->_pointers.pop_back();

    right->_pointers.insert(right->_pointers.begin(), last_child_node);
}

/**
 * Функция забирает последний элемент из node, отдать правому брату, при этому мы находимся в листах
 * Ключевая особенность в том, что запоминать элемент с родителя не нужно, ведь он совпадает с первым элементом
 * в правом брате! Поэтому просто обновляем его
 * @param node Узел, откуда надо забрать последний элемент и отдать правому брату
 * @param right Правый брат
 * @param parent Родитель
 * @param parent_index Индекс сына в родителе, откуда мы попали в node
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::give_right_brother_term(bsptree_node_term *node, bsptree_node_term *right,
                                                                 bsptree_node_middle *parent,
                                                                 size_t parent_index) {
    auto last_element_node = node->_data.back();
    node->_data.pop_back();

    parent->_keys[parent_index] = last_element_node.first;

    right->_data.insert(right->_data.begin(), last_element_node);
}

/**
 * Функция забирает первый элемент из node, отдает левому брату в самый последний элемент,
 * при этом мы находимся во внутренних узлах
 * @param node Узел, откуда надо забрать первый элемент и отдать левому брату
 * @param left Левый брат
 * @param parent Родитель
 * @param parent_index Индекс сына в родителе, откуда мы попали в node
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::give_left_brother_middle(bsptree_node_middle *node, bsptree_node_middle *left,
                                                                  bsptree_node_middle *parent,
                                                                  size_t parent_index) {
    auto first_element_node = node->_keys.front();
    node->_keys.erase(node->_keys.begin());

    auto element_from_parent = parent->_keys[parent_index - 1];
    parent->_keys[parent_index - 1] = first_element_node;

    left->_keys.push_back(element_from_parent);

    auto first_child_node = node->_pointers.front();
    node->_pointers.erase(node->_pointers.begin());

    left->_pointers.push_back(first_child_node);
}

/**
 * Функция забирает первый элемент из node, отдает левому брату в самый последний элемент,
 * при этом мы находимся во листах. Ключевая особенность в том, что мы добавляем в родителя именно второй элемент
 * из node, потому что в родителе не хранится сам элемент, а только фиктивный ключ!
 * @param node Узел, откуда надо забрать первый элемент и отдать левому брату
 * @param left Левый брат
 * @param parent Родитель
 * @param parent_index Индекс сына в родителе, откуда мы попали в node
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::give_left_brother_term(bsptree_node_term *node, bsptree_node_term *left,
                                                                bsptree_node_middle *parent,
                                                                size_t parent_index) {
    auto first_element_node = node->_data.front();
    node->_data.erase(node->_data.begin());

    auto second_element_node = node->_data.front();

    parent->_keys[parent_index - 1] = second_element_node.first; /* Обновляем именно на второй элемент! */

    left->_data.push_back(first_element_node);
}

/**
 * Разделяет два узла left и right на три узла left, middle, right
 *
 * @param left Узел, который находится слева
 * @param right Узел, который находится справа
 * @param parent Узел-родитель
 * @param parent_index Индекс сына в родителе, который указывает на left
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::split_middle(bsptree_node_middle *left, bsptree_node_middle *right,
                                                      bsptree_node_middle *parent,
                                                      size_t parent_index) {
    auto parent_split_element = parent->_keys[parent_index];
    parent->_keys.erase(parent->_keys.begin() + parent_index);

    constexpr auto index_split_element_left = 2 * t; /* элемент разделитель с первого узла */
    auto first_element_to_add_parent = left->_keys[index_split_element_left];

    /*
     * в случае, если переполнен левый узел, то тогда правый разделитель имеет индекс t - 1
     * в случае, если переполнен правый узел, то тогда правый разделитель имеет индекс t
     */
    auto index_split_element_right = t; /* элемент разделитель со второго узла */
    if (left->_keys.size() >= maximum_keys_in_node + 1) index_split_element_right = t - 1;

    auto second_element_to_add_parent = right->_keys[index_split_element_right];

    /* удаляем после всего, чтобы не нарушилась проверка на индекс правого элемента */
    left->_keys.erase(left->_keys.begin() + index_split_element_left);
    right->_keys.erase(right->_keys.begin() + index_split_element_right);

    auto middle_node = get_allocator().template new_object<bsptree_node_middle>();

    /* заполняем средний узел */
    int i = index_split_element_left;
    auto left_keys_size = left->keys_size();
    for (int _ = index_split_element_left; _ < left_keys_size; ++_) {
        /* не изменяется переменная i, потому что мы делаем erase и все элементы сдвигаются */
        auto element = left->_keys[i];
        left->_keys.erase(left->_keys.begin() + i);
        middle_node->_keys.push_back(element);

        auto child = left->_pointers.back();
        middle_node->_pointers.insert(middle_node->_pointers.begin(), child);
        left->_pointers.pop_back();
    }
    /* добавляем последнего ребенка узла в средний узел, если он есть */
    if (!left->_pointers.empty()) {
        auto child = left->_pointers.back();
        middle_node->_pointers.insert(middle_node->_pointers.begin(), child);
        left->_pointers.pop_back();
    }

    /* добавляем родительский узел */
    middle_node->_keys.push_back(parent_split_element);

    /* добавляем первого ребенка брата в средний узел, если он есть */
    if (!right->_pointers.empty()) {
        auto child = right->_pointers.front();
        middle_node->_pointers.push_back(child);
        right->_pointers.erase(right->_pointers.begin());
    }
    i = 0;
    for (int _ = 0; _ < index_split_element_right; ++_) {
        /* аналогично как и выше, переменная i не изменяется, потому что мы выполняем erase и элементы сдвигаются */
        auto element = right->_keys[i];
        right->_keys.erase(right->_keys.begin() + i);
        middle_node->_keys.push_back(element);

        if (!right->_pointers.empty()) {
            auto child = right->_pointers.front();
            middle_node->_pointers.push_back(child);
            right->_pointers.erase(right->_pointers.begin());
        }
    }

    /* заполняем родителя */
    parent->_keys.insert(parent->_keys.begin() + parent_index, first_element_to_add_parent);
    parent->_keys.insert(parent->_keys.begin() + parent_index + 1, second_element_to_add_parent);

    /* распределяем детей */
    parent->_pointers.insert(parent->_pointers.begin() + parent_index + 1, middle_node);
}

/**
 * Разъединяет два узла в три, причем мы находимся в листах.
 * Логика разделения: всего у нас 6t - 1 элементов (3t|3t - 1 или 3t-1|3t).
 * Значит делаем так:  * 2t|2t|2t-1 - в сумме 6t - 1 элемент.
 * Тогда если переполнен левый узел, то индексы:
 * left_index = 2t, right_index = t
 * Тогда если переполнен правый узел, то индексы:
 * left_index = 2t, right_index = t + 1
 *
 * @param left Левый узел
 * @param right Правый узел
 * @param parent Родитель
 * @param parent_index Индекс сына в родителя, который указывает на left
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::split_term(bsptree_node_term *left, bsptree_node_term *right,
                                                    bsptree_node_middle *parent,
                                                    size_t parent_index) {
    parent->_keys.erase(parent->_keys.begin() + parent_index);

    constexpr auto index_split_element_left = 2 * t; /* элемент разделитель с первого узла */

    auto index_split_element_right = t; /* элемент разделитель со второго узла */
    if (right->_data.size() >= maximum_keys_in_node + 1) index_split_element_right = t + 1;

    auto middle_node = get_allocator().template new_object<bsptree_node_term>();
    /* изменяем указатель на next */
    middle_node->_next = right;
    left->_next = middle_node;

    /* заполняем средний узел */
    size_t i = index_split_element_left;
    const size_t left_size = left->_data.size();
    for (int _ = index_split_element_left; _ < left_size; ++_) {
        /* не изменяется переменная i, потому что мы делаем erase и все элементы сдвигаются */
        auto element = left->_data[i];
        left->_data.erase(left->_data.begin() + i);
        middle_node->_data.push_back(element);
    }

    i = 0;
    for (int _ = 0; _ < index_split_element_right; ++_) {
        /* аналогично как и выше, переменная i не изменяется, потому что мы выполняем erase и элементы сдвигаются */
        auto element = right->_data[i];
        right->_data.erase(right->_data.begin() + i);
        middle_node->_data.push_back(element);
    }

    /* заполняем родителя */
    auto first_element_to_add_parent = middle_node->_data.front().first; /* ключ */
    auto second_element_to_add_parent = right->_data.front().first; /*  ключ */

    parent->_keys.insert(parent->_keys.begin() + parent_index, first_element_to_add_parent);
    parent->_keys.insert(parent->_keys.begin() + parent_index + 1, second_element_to_add_parent);

    /* добавляем нового ребенка */
    parent->_pointers.insert(parent->_pointers.begin() + parent_index + 1, middle_node);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::is_right_brother_exist(bsptree_node_middle *parent, size_t parent_index) {
    return parent_index + 1 < parent->_pointers.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::is_left_brother_exist(size_t parent_index) {
    return parent_index != 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::is_node_has_maximum_elements(bsptree_node_base *node) {
    return node->keys_size() >= maximum_keys_in_node;
}

// endregion helper functions for emplace impl

// region helper functions for erase impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::rebalancing_after_erase(
    std::vector<std::pair<bsptree_node_base *, size_t> > &path, tkey &deleted_key) {
    auto [node, parent_index] = path.back();
    path.pop_back();

    if (path.empty()) {
        if (node == _root) {
            if (node->keys_size() == 0) {
                handle_rebalancing_from_empty_root();
            }
            return;
        }
        throw std::logic_error("Rebalancing from node without parent and it isn't root!");
    }

    if (node->keys_size() >= minimum_keys_in_node) {
        if (node->_is_terminated) {
            auto new_key = node->keys()[0];
            update_references_in_parent(path, deleted_key, new_key);
        }
        return;
    }

    auto [parent, parent_parent_index] = path.back();
    auto parent_middle = dynamic_cast<bsptree_node_middle *>(parent);
    if (!parent_middle) throw std::logic_error("Parent node is not middle node!");

    auto node_term = dynamic_cast<bsptree_node_term *>(node);
    auto node_middle = dynamic_cast<bsptree_node_middle *>(node);

    /* пробуем занять у правого брата */
    if (is_right_brother_exist(parent_middle, parent_index)) {
        auto right_brother = parent_middle->_pointers[parent_index + 1];
        if (is_node_has_more_minimum_keys(right_brother)) {
            /* можно занять у правого брата */
            if (auto right_middle = dynamic_cast<bsptree_node_middle *>(right_brother)) {
                borrow_from_right_middle(node_middle, right_middle, parent_middle, parent_index);
                return;
            }
            if (auto right_term = dynamic_cast<bsptree_node_term *>(right_brother)) {
                borrow_from_right_term(node_term, right_term, parent_middle, parent_index);
                auto new_key = node_term->_data[0].first;
                update_references_in_parent(path, deleted_key, new_key);
                return;
            }
            throw std::logic_error("Right brother has unknown type!");
        }

        /* пробуем занять у правого двоюродного брата */
        if (is_right_brother_exist(parent_middle, parent_index + 1)) {
            auto right_right_brother = parent_middle->_pointers[parent_index + 1 + 1];
            if (is_node_has_more_minimum_keys(right_right_brother)) {
                /* можно занять у правого двоюродного брата */
                if (auto right_right_middle = dynamic_cast<bsptree_node_middle *>(right_right_brother), right_middle =
                                    dynamic_cast<bsptree_node_middle *>(right_brother);
                    right_right_middle && right_middle) {
                    borrow_from_right_right_middle(node_middle, right_middle, right_right_middle, parent_middle,
                                                   parent_index);
                    return;
                }
                if (auto right_right_term = dynamic_cast<bsptree_node_term *>(right_right_brother), right_term =
                                    dynamic_cast<bsptree_node_term *>(right_brother); right_right_term && right_term) {
                    borrow_from_right_right_term(node_term, right_term, right_right_term, parent_middle, parent_index);
                    auto new_key = node_term->_data[0].first;
                    update_references_in_parent(path, deleted_key, new_key);
                    return;
                }
                throw std::logic_error("Right brother and right right brother have different types!");
            }
        }
    }

    /* пробуем занять у левого брата */
    if (is_left_brother_exist(parent_index)) {
        auto left_brother = parent_middle->_pointers[parent_index - 1];
        if (is_node_has_more_minimum_keys(left_brother)) {
            /* можно занять у левого брата */
            if (auto left_middle = dynamic_cast<bsptree_node_middle *>(left_brother)) {
                borrow_from_left_middle(node_middle, left_middle, parent_middle, parent_index - 1);
                return;
            }
            if (auto left_term = dynamic_cast<bsptree_node_term *>(left_brother)) {
                borrow_from_left_term(node_term, left_term, parent_middle, parent_index - 1);
                auto new_key = node_term->_data[0].first;
                update_references_in_parent(path, deleted_key, new_key);
                return;
            }
            throw std::logic_error("Right brother has unknown type!");
        }

        /* пробуем занять у левого двоюродного брата */
        if (is_left_brother_exist(parent_index - 1)) {
            auto left_left_brother = parent_middle->_pointers[parent_index - 1 - 1];
            if (is_node_has_more_minimum_keys(left_left_brother)) {
                /* можно занять у левого двоюродного брата */
                if (auto left_left_middle = dynamic_cast<bsptree_node_middle *>(left_left_brother), left_middle =
                                    dynamic_cast<bsptree_node_middle *>(left_brother);
                    left_left_middle && left_middle) {
                    borrow_from_left_left_middle(node_middle, left_middle, left_left_middle, parent_middle,
                                                 parent_index - 1);
                    return;
                }
                if (auto left_left_term = dynamic_cast<bsptree_node_term *>(left_left_brother), left_term =
                                    dynamic_cast<bsptree_node_term *>(left_brother); left_left_term && left_term) {
                    borrow_from_left_left_term(node_term, left_term, left_left_term, parent_middle, parent_index - 1);
                    auto new_key = node_term->_data[0].first;
                    update_references_in_parent(path, deleted_key, new_key);
                    return;
                }
                throw std::logic_error("Right brother and right right brother have different types!");
            }
        }
    }

    /*
     * Ни у кого занять не получилось, поэтому пытаемся выполнить merge
     * у merge бывает три кейса:
     * 1. merge (node, right, right_right)
     * 2. merge (left, node, right)
     * 3. merge (left_left, left, node)
     */

    /* case 1 */
    if (is_right_brother_exist(parent_middle, parent_index)) {
        auto right_brother = parent_middle->_pointers[parent_index + 1];
        if (is_right_brother_exist(parent_middle, parent_index + 1)) {
            auto right_right_brother = parent_middle->_pointers[parent_index + 1 + 1];
            if (auto right_middle = dynamic_cast<bsptree_node_middle *>(right_brother), right_right_middle =
                                dynamic_cast<bsptree_node_middle *>(right_right_brother);
                right_middle && right_right_middle) {
                merge_middle(node_middle, right_middle, right_right_middle, parent_middle, parent_index + 1);
                rebalancing_after_erase(path, deleted_key);
                return;
            }
            if (auto right_term = dynamic_cast<bsptree_node_term *>(right_brother), right_right_term = dynamic_cast<
                            bsptree_node_term *>(right_right_brother); right_term && right_right_term) {
                merge_term(node_term, right_term, right_right_term, parent_middle, parent_index + 1);
                auto new_key = node_term->_data[0].first;
                update_references_in_parent(path, deleted_key, new_key);
                rebalancing_after_erase(path, deleted_key);
                return;
            }
            throw std::logic_error("Right brother and right right brother have different types!");
        }
    }


    /* case 2 */
    if (is_right_brother_exist(parent_middle, parent_index) && is_left_brother_exist(parent_index)) {
        auto right_brother = parent_middle->_pointers[parent_index + 1];
        auto left_brother = parent_middle->_pointers[parent_index - 1];

        if (auto right_middle = dynamic_cast<bsptree_node_middle *>(right_brother), left_middle = dynamic_cast<
                        bsptree_node_middle *>(left_brother); right_middle && left_middle) {
            merge_middle(left_middle, node_middle, right_middle, parent_middle, parent_index);
            rebalancing_after_erase(path, deleted_key);
            return;
        }
        if (auto right_term = dynamic_cast<bsptree_node_term *>(right_brother), left_term = dynamic_cast<
                        bsptree_node_term *>(left_brother); right_term && left_term) {
            merge_term(left_term, node_term, right_term, parent_middle, parent_index);
            auto new_key = left_term->_data[0].first;
            update_references_in_parent(path, deleted_key, new_key);
            rebalancing_after_erase(path, deleted_key);
            return;
        }
        throw std::logic_error("Left brother and right brother have different types!");
    }

    /* case 3 */
    if (is_left_brother_exist(parent_index)) {
        auto left_brother = parent_middle->_pointers[parent_index - 1];
        if (is_left_brother_exist(parent_index - 1)) {
            auto left_left_brother = parent_middle->_pointers[parent_index - 1 - 1];

            if (auto left_middle = dynamic_cast<bsptree_node_middle *>(left_brother), left_left_middle = dynamic_cast<
                            bsptree_node_middle *>(left_left_brother); left_middle && left_left_middle) {
                merge_middle(left_left_middle, left_middle, node_middle, parent_middle, parent_index - 1);
                rebalancing_after_erase(path, deleted_key);
                return;
            }
            if (auto left_term = dynamic_cast<bsptree_node_term *>(left_brother), left_left_term = dynamic_cast<
                            bsptree_node_term *>(left_left_brother); left_term && left_left_term) {
                merge_term(left_left_term, left_term, node_term, parent_middle, parent_index - 1);
                auto new_key = left_left_term->_data[0].first;
                update_references_in_parent(path, deleted_key, new_key);
                rebalancing_after_erase(path, deleted_key);
                return;
            }
            throw std::logic_error("Left brother and left left brother have different types!");
        }
    }

    merge_with_root();
}

/**
 * Мы дошли до того момента, что элементов в root не осталось,
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::handle_rebalancing_from_empty_root() {
    if (auto root_middle = dynamic_cast<bsptree_node_middle *>(_root)) {
        if (root_middle->_pointers.size() > 1)
            throw std::logic_error(
                "The root has 0 elements, but it has more than 1 child");
        auto child = root_middle->_pointers[0];

        get_allocator().template delete_object<bsptree_node_middle>(root_middle);
        _root = child; /* удалили корень, теперь корень новый */
    } else if (auto root_term = dynamic_cast<bsptree_node_term *>(_root)) {
        get_allocator().template delete_object<bsptree_node_term>(root_term);
        _root = nullptr;
    } else throw std::logic_error("The root has unknown type");
}

/**
 * Функцию следует вызывать только в случае, если остался один корень, два сына,
 * в каждом из сыновей было по минимум элементов и из одного из узлов мы удалили элемент
 *
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::merge_with_root() {
    auto root_middle = dynamic_cast<bsptree_node_middle *>(_root);
    if (!root_middle) throw std::logic_error("Root node is not middle node!");

    auto left = root_middle->_pointers.front();
    auto right = root_middle->_pointers.back();

    root_middle->_pointers.erase(root_middle->_pointers.begin(), root_middle->_pointers.end());

    if (auto left_middle = dynamic_cast<bsptree_node_middle *>(left), right_middle = dynamic_cast<bsptree_node_middle *>
                        (right); left_middle && right_middle) {
        /* переносим элементы с левого узла */
        while (!left_middle->_keys.empty()) {
            auto element = left_middle->_keys.back();
            left_middle->_keys.pop_back();
            root_middle->_keys.insert(root_middle->_keys.begin(), element);

            auto child = left_middle->_pointers.back();
            left_middle->_pointers.pop_back();
            root_middle->_pointers.insert(root_middle->_pointers.begin(), child);
        }

        /* переносим последнего ребенка */
        auto child = left_middle->_pointers.back();
        left_middle->_pointers.pop_back();
        root_middle->_pointers.insert(root_middle->_pointers.begin(), child);

        /* переносим элементы из правого узла */
        while (!right_middle->_keys.empty()) {
            auto element = right_middle->_keys.front();
            right_middle->_keys.erase(right_middle->_keys.begin());
            root_middle->_keys.push_back(element);

            child = right_middle->_pointers.front();
            right_middle->_pointers.erase(right_middle->_pointers.begin());
            root_middle->_pointers.push_back(child);
        }

        /* не забываем про последнего ребенка */

        child = right_middle->_pointers.front();
        right_middle->_pointers.erase(right_middle->_pointers.begin());
        root_middle->_pointers.push_back(child);
        return;
    }

    if (auto left_term = dynamic_cast<bsptree_node_term *>(left), right_term = dynamic_cast<bsptree_node_term *>(right);
        left_term && right_term) {
        /* после того как мы смержим, останется всего лишь один узел - корень, который должен стать bsptree_node_term */
        auto new_root = get_allocator().template new_object<bsptree_node_term>();
        new_root->_next = nullptr;

        /* переносим элементы с левого узла */
        while (!left_term->_data.empty()) {
            auto element = left_term->_data.back();
            left_term->_data.pop_back();
            new_root->_data.insert(new_root->_data.begin(), element);
        }

        /* переносим элементы с правого узла */
        while (!right_term->_data.empty()) {
            auto element = right_term->_data.front();
            right_term->_data.erase(right_term->_data.begin());
            new_root->_data.push_back(element);
        }

        get_allocator().template delete_object<bsptree_node_middle>(root_middle);
        _root = new_root;

        return;
    }

    throw std::logic_error("Childs of root have different types!");
}


/**
 * Обновить все ссылки на удаленный узел
 * @param path Вектор, в котором уже удален текущий узел
 * @param deleted_key Старый ключ, который надо заменить
 * @param new_key Новый ключ, на который нужно заменить
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::update_references_in_parent(std::vector<std::pair<bsptree_node_base *,
                                                                         size_t> > &path, tkey &deleted_key,
                                                                     tkey new_key) {
    for (auto &item: path) {
        auto [node, _] = item;
        auto node_middle = dynamic_cast<bsptree_node_middle *>(node);
        if (!node_middle) throw std::logic_error("Node is not middle node!");

        for (auto &key: node_middle->_keys) {
            if (key == deleted_key) key = new_key;
        }
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_right_middle(bsptree_node_middle *node, bsptree_node_middle *right,
                                                                  bsptree_node_middle *parent, size_t parent_index) {
    /* забираем элемент из родителя */
    auto element = parent->_keys[parent_index];
    parent->_keys.erase(parent->_keys.begin() + parent_index);
    node->_keys.push_back(element);

    /* удаляем элемент из правого брата и добавляем в родителя */
    element = right->_keys.front();
    right->_keys.erase(right->_keys.begin());
    parent->_keys.insert(parent->_keys.begin() + parent_index, element);

    /* не забываем перенести ребенка */
    auto child = right->_pointers.front();
    right->_pointers.erase(right->_pointers.begin());
    node->_pointers.push_back(child);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_right_term(bsptree_node_term *node, bsptree_node_term *right,
                                                                bsptree_node_middle *parent, size_t parent_index) {
    auto element = right->_data.front();
    right->_data.erase(right->_data.begin());
    auto second_key = right->_data.front().first;

    /* в родителе именно обновляем элемент */
    parent->_keys[parent_index] = second_key;

    /* добавляем в наш узел */
    node->_data.push_back(element);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_right_right_middle(bsptree_node_middle *node,
                                                                        bsptree_node_middle *right,
                                                                        bsptree_node_middle *right_right,
                                                                        bsptree_node_middle *parent,
                                                                        size_t parent_index) {
    borrow_from_right_middle(right, right_right, parent, parent_index + 1);
    borrow_from_right_middle(node, right, parent, parent_index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_right_right_term(bsptree_node_term *node, bsptree_node_term *right,
                                                                      bsptree_node_term *right_right,
                                                                      bsptree_node_middle *parent,
                                                                      size_t parent_index) {
    borrow_from_right_term(right, right_right, parent, parent_index + 1);
    borrow_from_right_term(node, right, parent, parent_index);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_left_middle(bsptree_node_middle *node, bsptree_node_middle *left,
                                                                 bsptree_node_middle *parent, size_t parent_index) {
    /* забираем элемент из родителя */
    auto element = parent->_keys[parent_index];
    parent->_keys.erase(parent->_keys.begin() + parent_index);
    node->_keys.insert(node->_keys.begin(), element);

    /* удаляем элемент из левого брата и добавляем в родителя */
    element = left->_keys.back();
    left->_keys.pop_back();
    parent->_keys.insert(parent->_keys.begin() + parent_index, element);

    /* не забываем перенести ребенка */
    auto child = left->_pointers.back();
    left->_pointers.pop_back();
    node->_pointers.insert(node->_pointers.begin(), child);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_left_term(bsptree_node_term *node, bsptree_node_term *left,
                                                               bsptree_node_middle *parent, size_t parent_index) {
    auto element = left->_data.back();
    left->_data.pop_back();

    /* обновляем элемент в родителе */
    parent->_keys[parent_index] = element.first;

    node->_data.insert(node->_data.begin(), element);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_left_left_middle(bsptree_node_middle *node,
                                                                      bsptree_node_middle *left,
                                                                      bsptree_node_middle *left_left,
                                                                      bsptree_node_middle *parent,
                                                                      size_t parent_index) {
    borrow_from_left_middle(left, left_left, parent, parent_index - 1);
    borrow_from_left_middle(node, left, parent, parent_index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::borrow_from_left_left_term(bsptree_node_term *node, bsptree_node_term *left,
                                                                    bsptree_node_term *left_left,
                                                                    bsptree_node_middle *parent, size_t parent_index) {
    borrow_from_left_term(left, left_left, parent, parent_index - 1);
    borrow_from_left_term(node, left, parent, parent_index);
}

/**
 * Мержит три узла в два узла. Узлы являются middle узлами
 * Экспериментально доказано, что если:
 * 1. переполнен самый левый узел, то индекс элемента в среднем узле равен t
 * 2. переполнен или средний, правый узел, то индекс элемента в среднем узле равен t - 1
 *
 * @param left Левый брат node
 * @param node Узел, который находится посередине
 * @param right Правый брат node
 * @param parent Родитель
 * @param parent_index Индекс родителя, который находится над node
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::merge_middle(bsptree_node_middle *left, bsptree_node_middle *node,
                                                      bsptree_node_middle *right,
                                                      bsptree_node_middle *parent, size_t parent_index) {
    auto index_split_element = t - 1;
    if (left->keys_size() < minimum_keys_in_node) index_split_element = t;

    auto split_element = node->_keys[index_split_element];
    node->_keys.erase(node->_keys.begin() + index_split_element);

    /* элементы-разделители из родителя */
    auto first_element_from_parent = parent->_keys[parent_index - 1];
    auto second_element_from_parent = parent->_keys[parent_index];

    parent->_keys.erase(parent->_keys.begin() + parent_index - 1);
    parent->_keys.erase(parent->_keys.begin() + parent_index - 1);

    /* заполняем левый узел */
    /* сначала добавляем элемент из родителя */
    left->_keys.push_back(first_element_from_parent);
    /* переносим элементы из среднего узла в левый */
    for (int _ = 0; _ < index_split_element; ++_) {
        auto element_from_node = node->_keys.front();
        left->_keys.push_back(element_from_node);
        node->_keys.erase(node->_keys.begin());

        auto child_from_node = node->_pointers.front();
        left->_pointers.push_back(child_from_node);
        node->_pointers.erase(node->_pointers.begin());
    }

    if (!node->_pointers.empty()) {
        auto child_from_node = node->_pointers.front();
        left->_pointers.push_back(child_from_node);
        node->_pointers.erase(node->_pointers.begin());
    }

    /* заполняем правый узел */
    /* сначала добавляем элемент из родителя */
    right->_keys.insert(right->_keys.begin(), second_element_from_parent);
    /* переносим оставшиеся элемент из среднего узла в правый */
    size_t node_keys_size = node->_keys.size();
    for (int _ = 0; _ < node_keys_size; ++_) {
        auto element_from_node = node->_keys.back();
        right->_keys.insert(right->_keys.begin(), element_from_node);
        node->_keys.pop_back();

        auto child_from_node = node->_pointers.back();
        right->_pointers.insert(right->_pointers.begin(), child_from_node);
        node->_pointers.pop_back();
    }
    if (!node->_pointers.empty()) {
        auto child_from_node = node->_pointers.back();
        right->_pointers.insert(right->_pointers.begin(), child_from_node);
        node->_pointers.pop_back();
    }

    get_allocator().template delete_object<bsptree_node_middle>(node);

    parent->_keys.insert(parent->_keys.begin() + parent_index - 1, split_element); /* добавляем в родителя */
    parent->_pointers.erase(parent->_pointers.begin() + parent_index); /* удаляем ребенка, который указывал на node */
}


/**
 * Мержит три узла в два узла. Узлы являются листьями
 * Экспериментально доказано, что если:
 * 1. переполнен самый левый узел, то индекс элемента в среднем узле равен t
 * 2. переполнен или средний, правый узел, то индекс элемента в среднем узле равен t - 1
 * @param left Левый брат node
 * @param node Узел, который находится посередине
 * @param right Правый брат node
 * @param parent Родитель
 * @param parent_index Индекс родителя, который находится над node
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::merge_term(bsptree_node_term *left, bsptree_node_term *node,
                                                    bsptree_node_term *right,
                                                    bsptree_node_middle *parent, size_t parent_index) {
    auto index_split_element = t - 1;
    if (left->keys_size() < minimum_keys_in_node) index_split_element = t;

    auto split_element = node->_data[index_split_element];

    /* изменяем родителя */
    parent->_keys.erase(parent->_keys.begin() + parent_index - 1);
    parent->_keys.erase(parent->_keys.begin() + parent_index - 1);

    parent->_keys.insert(parent->_keys.begin() + parent_index - 1, split_element.first);

    /* обновляем указатель на next */
    left->_next = right;

    /* заполняем левый узел */
    for (auto _ = 0; _ < index_split_element; ++_) {
        auto element = node->_data.front();
        node->_data.erase(node->_data.begin());
        left->_data.push_back(element);
    }

    /* заполняем правый узел */
    auto node_keys_size = node->keys_size();
    for (auto _ = 0; _ < node_keys_size; ++_) {
        auto element = node->_data.back();
        node->_data.pop_back();
        right->_data.insert(right->_data.begin(), element);
    }

    /* удаляем средний элемент */
    get_allocator().template delete_object<bsptree_node_term>(node);

    /* удаляем ребенка, который был под node */
    parent->_pointers.erase(parent->_pointers.begin() + parent_index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::is_node_has_more_minimum_keys(bsptree_node_base *node) {
    return node->keys_size() > minimum_keys_in_node;
}

// endregion helper functions for erase impl


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_node_term *BSP_tree<tkey, tvalue, compare,
    t>::first_terminate_node() {
    bsptree_node_base *node = _root;
    while (auto node_middle = dynamic_cast<bsptree_node_middle *>(node)) {
        node = node_middle->_pointers[0];
    }
    return dynamic_cast<bsptree_node_term *>(node);
}

// endregion

// region path_position impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::path_position::path_position(
    std::vector<std::pair<bsptree_node_base *, size_t> > path,
    size_t index)
    : _path(path), _index(index) {
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::vector<std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_node_base *, size_t> > &BSP_tree<tkey, tvalue
    ,
    compare,
    t>::path_position::get_path() {
    return _path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_base *BSP_tree<tkey, tvalue, compare,
    t>::path_position::get_last_node() {
    if (_path.empty()) return nullptr;
    auto [node, _] = _path.back();
    return node;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::path_position::get_index() {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::path_position::set_index(size_t index) {
    _index = index;
}

// endregion


#endif
