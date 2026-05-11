#include <cassert>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "bstarplus_index_tree.h"
#include "constants.h"

class InMemoryIndexPageManager final : public db::IndexPageManager {
    std::unordered_map<db::PageId, std::vector<char>> _pages;
    db::PageId _next_page_id = 1;

public:
    db::PageId allocate_page() override {
        return _next_page_id++;
    }

    std::vector<char> read_page(const db::PageId page_id) override {
        return _pages.at(page_id);
    }

    void write_page(const db::PageId page_id, const std::vector<char>& data) override {
        _pages[page_id] = data;
    }

    void flush() override {
    }
};

constexpr std::size_t test_leaf_entry_size = sizeof(db::IndexKey) + sizeof(db::PageId) + sizeof(db::SlotId);
constexpr std::size_t test_leaf_capacity = (db::PAGE_SIZE - db::INDEX_NODE_HEADER_SIZE) / test_leaf_entry_size;
constexpr std::size_t test_min_leaf_keys = (2 * test_leaf_capacity + 2) / 3;
constexpr std::size_t test_internal_entry_size = sizeof(db::IndexKey) + sizeof(db::PageId);
constexpr std::size_t test_internal_capacity =
    (db::PAGE_SIZE - db::INDEX_NODE_HEADER_SIZE - sizeof(db::PageId)) / test_internal_entry_size;

db::RecordId record_for_key(const db::IndexKey key) {
    return {
        static_cast<db::PageId>(5000 + key),
        static_cast<db::SlotId>(key)
    };
}

std::vector<db::IndexKey> make_keys(const db::IndexKey first, const std::size_t count) {
    std::vector<db::IndexKey> keys;
    keys.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        keys.push_back(first + static_cast<db::IndexKey>(i));
    }
    return keys;
}

std::vector<db::RecordId> make_records(const std::vector<db::IndexKey>& keys) {
    std::vector<db::RecordId> records;
    records.reserve(keys.size());
    for (const auto key : keys) {
        records.push_back(record_for_key(key));
    }
    return records;
}

db::IndexNode make_leaf(
    const db::PageId page_id,
    const db::PageId parent_page_id,
    const db::PageId next_leaf_page_id,
    const std::vector<db::IndexKey>& keys
) {
    db::IndexNode node;
    node.is_leaf = true;
    node.page_id = page_id;
    node.parent_page_id = parent_page_id;
    node.next_leaf_page_id = next_leaf_page_id;
    node.keys = keys;
    node.records = make_records(keys);
    return node;
}

db::IndexNode make_internal(
    const db::PageId page_id,
    const db::PageId parent_page_id,
    const std::vector<db::IndexKey>& keys,
    const std::vector<db::PageId>& children
) {
    db::IndexNode node;
    node.is_leaf = false;
    node.page_id = page_id;
    node.parent_page_id = parent_page_id;
    node.keys = keys;
    node.children = children;
    return node;
}

void assert_found(db::BStarPlusIndex& index, const db::IndexKey key) {
    const auto found = index.find(key);
    assert(found.has_value());
    assert(*found == record_for_key(key));
}

int main() {
    InMemoryIndexPageManager page_manager;

    const db::PageId root_page_id = page_manager.allocate_page();
    const db::RecordId first_record{10, 1};
    const db::RecordId second_record{11, 2};
    const db::RecordId third_record{12, 3};

    db::IndexNode root;
    root.is_leaf = true;
    root.page_id = root_page_id;
    root.keys = {5, 10, 20};
    root.records = {first_record, second_record, third_record};

    page_manager.write_page(root_page_id, db::serialize_node(root));

    db::BStarPlusIndex index(
        page_manager,
        db::BStarPlusIndexMetadata{
            .root_page_id = root_page_id,
            .first_leaf_page_id = root_page_id,
            .height = 1,
            .size = root.keys.size()
        }
    );

    // test find
    std::cout << "test find: ";
    const auto found = index.find(10);
    assert(found.has_value());
    assert(*found == second_record);

    const auto missing = index.find(15);
    assert(!missing.has_value());

    // test insert
    std::cout << "Good!\ntest insert: ";
    db::RecordId insert_record = {15, 4};
    index.insert(30, insert_record);
    assert(index.find(30) == insert_record);

    // test erase
    std::cout << "Good!\ntest erase: ";
    assert(index.erase(10));
    assert(!index.find(10).has_value());
    assert(index.find(5) == first_record);
    assert(index.find(20) == third_record);
    assert(index.find(30) == insert_record);
    assert(index.metadata().size == 3);

    assert(!index.erase(15));
    assert(index.metadata().size == 3);

    assert(index.erase(5));
    assert(index.erase(20));
    assert(index.erase(30));
    assert(index.metadata().empty());
    assert(index.metadata().size == 0);
    assert(!index.find(5).has_value());
    assert(!index.erase(30));

    const db::PageId internal_root_page_id = page_manager.allocate_page();
    const db::PageId left_leaf_page_id = page_manager.allocate_page();
    const db::PageId right_leaf_page_id = page_manager.allocate_page();

    db::IndexNode left_leaf;
    left_leaf.is_leaf = true;
    left_leaf.page_id = left_leaf_page_id;
    left_leaf.parent_page_id = internal_root_page_id;
    left_leaf.next_leaf_page_id = right_leaf_page_id;
    left_leaf.keys = {10, 20};
    left_leaf.records = {db::RecordId{100, 1}, db::RecordId{100, 2}};

    db::IndexNode right_leaf;
    right_leaf.is_leaf = true;
    right_leaf.page_id = right_leaf_page_id;
    right_leaf.parent_page_id = internal_root_page_id;
    right_leaf.keys = {60, 80};
    right_leaf.records = {db::RecordId{200, 1}, db::RecordId{200, 2}};

    db::IndexNode internal_root;
    internal_root.is_leaf = false;
    internal_root.page_id = internal_root_page_id;
    internal_root.keys = {50};
    internal_root.children = {left_leaf_page_id, right_leaf_page_id};

    page_manager.write_page(left_leaf_page_id, db::serialize_node(left_leaf));
    page_manager.write_page(right_leaf_page_id, db::serialize_node(right_leaf));
    page_manager.write_page(internal_root_page_id, db::serialize_node(internal_root));

    db::BStarPlusIndex height_two_index(
        page_manager,
        db::BStarPlusIndexMetadata{
            .root_page_id = internal_root_page_id,
            .first_leaf_page_id = left_leaf_page_id,
            .height = 2,
            .size = left_leaf.keys.size() + right_leaf.keys.size()
        }
    );

    const db::RecordId height_two_insert_record{300, 7};
    height_two_index.insert(70, height_two_insert_record);
    assert(height_two_index.find(70) == height_two_insert_record);

    const auto updated_right_leaf = db::deserialize_node(page_manager.read_page(right_leaf_page_id));
    assert(updated_right_leaf.keys.size() == 3);
    assert(updated_right_leaf.keys[0] == 60);
    assert(updated_right_leaf.keys[1] == 70);
    assert(updated_right_leaf.keys[2] == 80);
    assert(updated_right_leaf.records[1] == height_two_insert_record);

    const auto unchanged_left_leaf = db::deserialize_node(page_manager.read_page(left_leaf_page_id));
    assert(unchanged_left_leaf.keys.size() == 2);
    assert(unchanged_left_leaf.keys[0] == 10);
    assert(unchanged_left_leaf.keys[1] == 20);

    assert(height_two_index.erase(70));
    assert(!height_two_index.find(70).has_value());
    assert((height_two_index.find(60) == db::RecordId{200, 1}));
    assert((height_two_index.find(80) == db::RecordId{200, 2}));
    assert((height_two_index.find(10) == db::RecordId{100, 1}));
    assert(!height_two_index.erase(70));

    // test split
    std::cout << "Good!\ntest split: ";
    InMemoryIndexPageManager split_page_manager;
    db::BStarPlusIndex split_index(split_page_manager);

    constexpr std::size_t key_count = test_leaf_capacity * 2 + 25;

    for (db::IndexKey key = 0; key < key_count; ++key) {
        split_index.insert(key, db::RecordId{static_cast<db::PageId>(1000 + key), static_cast<db::SlotId>(key)});
    }

    assert(split_index.metadata().height == 2);
    assert(split_index.metadata().size == key_count);

    for (db::IndexKey key = 0; key < key_count; ++key) {
        const auto found_after_split = split_index.find(key);
        assert(found_after_split.has_value());
        assert(found_after_split->page_id == static_cast<db::PageId>(1000 + key));
        assert(found_after_split->slot_id == static_cast<db::SlotId>(key));
    }

    assert(split_index.erase(7));
    assert(!split_index.find(7).has_value());
    assert(split_index.find(8).has_value());
    assert(split_index.metadata().size == key_count - 1);

    // test internal B* redistribution/split under insert pressure
    std::cout << "Good!\ntest internal B* redistribution/split under insert pressure: ";
    InMemoryIndexPageManager internal_split_page_manager;
    db::BStarPlusIndex internal_split_index(internal_split_page_manager);
    constexpr std::size_t internal_split_key_count = test_min_leaf_keys * (test_internal_capacity + 8);

    for (db::IndexKey key = 0; key < static_cast<db::IndexKey>(internal_split_key_count); ++key) {
        internal_split_index.insert(key, record_for_key(key));
    }

    assert(internal_split_index.metadata().height >= 3);
    assert(internal_split_index.metadata().size == internal_split_key_count);
    for (db::IndexKey key = 0; key < static_cast<db::IndexKey>(internal_split_key_count); key += 257) {
        assert_found(internal_split_index, key);
    }

    // test borrow
    std::cout << "Good!\ntest borrow: ";
    InMemoryIndexPageManager borrow_page_manager;
    const auto borrow_root_page_id = borrow_page_manager.allocate_page();
    const auto borrow_left_page_id = borrow_page_manager.allocate_page();
    const auto borrow_right_page_id = borrow_page_manager.allocate_page();
    const auto borrow_left_keys = make_keys(0, test_min_leaf_keys);
    const auto borrow_right_keys = make_keys(
        static_cast<db::IndexKey>(test_min_leaf_keys),
        test_min_leaf_keys + 1
    );

    auto borrow_left = make_leaf(borrow_left_page_id, borrow_root_page_id, borrow_right_page_id, borrow_left_keys);
    auto borrow_right = make_leaf(borrow_right_page_id, borrow_root_page_id, db::INVALID_PAGE_ID, borrow_right_keys);
    auto borrow_root = make_internal(
        borrow_root_page_id,
        db::INVALID_PAGE_ID,
        {borrow_right_keys.front()},
        {borrow_left_page_id, borrow_right_page_id}
    );

    borrow_page_manager.write_page(borrow_left_page_id, db::serialize_node(borrow_left));
    borrow_page_manager.write_page(borrow_right_page_id, db::serialize_node(borrow_right));
    borrow_page_manager.write_page(borrow_root_page_id, db::serialize_node(borrow_root));

    db::BStarPlusIndex borrow_index(
        borrow_page_manager,
        db::BStarPlusIndexMetadata{
            .root_page_id = borrow_root_page_id,
            .first_leaf_page_id = borrow_left_page_id,
            .height = 2,
            .size = borrow_left_keys.size() + borrow_right_keys.size()
        }
    );

    assert(borrow_index.erase(0));
    assert(!borrow_index.find(0).has_value());
    assert(borrow_index.metadata().height == 2);
    for (db::IndexKey key = 1; key < static_cast<db::IndexKey>(test_min_leaf_keys * 2 + 1); ++key) {
        assert_found(borrow_index, key);
    }

    const auto borrow_root_after = db::deserialize_node(borrow_page_manager.read_page(borrow_root_page_id));
    const auto borrow_left_after = db::deserialize_node(borrow_page_manager.read_page(borrow_left_page_id));
    const auto borrow_right_after = db::deserialize_node(borrow_page_manager.read_page(borrow_right_page_id));
    assert(borrow_left_after.keys.size() == test_min_leaf_keys);
    assert(borrow_right_after.keys.size() == test_min_leaf_keys);
    assert(borrow_root_after.keys.front() == borrow_right_after.keys.front());

    // test merge
    std::cout << "Good!\ntest merge: ";
    InMemoryIndexPageManager merge_page_manager;
    const auto merge_root_page_id = merge_page_manager.allocate_page();
    const auto merge_left_page_id = merge_page_manager.allocate_page();
    const auto merge_right_page_id = merge_page_manager.allocate_page();
    const auto merge_left_keys = make_keys(0, 2);
    const auto merge_right_keys = make_keys(
        static_cast<db::IndexKey>(merge_left_keys.size()),
        2
    );

    auto merge_left = make_leaf(merge_left_page_id, merge_root_page_id, merge_right_page_id, merge_left_keys);
    auto merge_right = make_leaf(merge_right_page_id, merge_root_page_id, db::INVALID_PAGE_ID, merge_right_keys);
    auto merge_root = make_internal(
        merge_root_page_id,
        db::INVALID_PAGE_ID,
        {merge_right_keys.front()},
        {merge_left_page_id, merge_right_page_id}
    );

    merge_page_manager.write_page(merge_left_page_id, db::serialize_node(merge_left));
    merge_page_manager.write_page(merge_right_page_id, db::serialize_node(merge_right));
    merge_page_manager.write_page(merge_root_page_id, db::serialize_node(merge_root));

    db::BStarPlusIndex merge_index(
        merge_page_manager,
        db::BStarPlusIndexMetadata{
            .root_page_id = merge_root_page_id,
            .first_leaf_page_id = merge_left_page_id,
            .height = 2,
            .size = merge_left_keys.size() + merge_right_keys.size()
        }
    );

    assert(merge_index.erase(0));
    assert(!merge_index.find(0).has_value());
    assert(merge_index.metadata().height == 1);
    assert(merge_index.metadata().root_page_id == merge_left_page_id);
    assert(merge_index.metadata().first_leaf_page_id == merge_left_page_id);
    for (db::IndexKey key = 1; key < static_cast<db::IndexKey>(merge_left_keys.size() + merge_right_keys.size()); ++key) {
        assert_found(merge_index, key);
    }

    // test recursive merge
    std::cout << "Good!\ntest recursive merge: ";
    InMemoryIndexPageManager recursive_page_manager;
    const auto recursive_root_page_id = recursive_page_manager.allocate_page();
    const auto recursive_left_internal_page_id = recursive_page_manager.allocate_page();
    const auto recursive_right_internal_page_id = recursive_page_manager.allocate_page();
    const auto leaf_a_page_id = recursive_page_manager.allocate_page();
    const auto leaf_b_page_id = recursive_page_manager.allocate_page();
    const auto leaf_c_page_id = recursive_page_manager.allocate_page();
    const auto leaf_d_page_id = recursive_page_manager.allocate_page();

    constexpr std::size_t recursive_leaf_key_count = 2;
    const auto leaf_a_keys = make_keys(0, recursive_leaf_key_count);
    const auto leaf_b_keys = make_keys(static_cast<db::IndexKey>(recursive_leaf_key_count), recursive_leaf_key_count);
    const auto leaf_c_keys = make_keys(static_cast<db::IndexKey>(recursive_leaf_key_count * 2), recursive_leaf_key_count);
    const auto leaf_d_keys = make_keys(static_cast<db::IndexKey>(recursive_leaf_key_count * 3), recursive_leaf_key_count);

    recursive_page_manager.write_page(
        leaf_a_page_id,
        db::serialize_node(make_leaf(leaf_a_page_id, recursive_left_internal_page_id, leaf_b_page_id, leaf_a_keys))
    );
    recursive_page_manager.write_page(
        leaf_b_page_id,
        db::serialize_node(make_leaf(leaf_b_page_id, recursive_left_internal_page_id, leaf_c_page_id, leaf_b_keys))
    );
    recursive_page_manager.write_page(
        leaf_c_page_id,
        db::serialize_node(make_leaf(leaf_c_page_id, recursive_right_internal_page_id, leaf_d_page_id, leaf_c_keys))
    );
    recursive_page_manager.write_page(
        leaf_d_page_id,
        db::serialize_node(make_leaf(leaf_d_page_id, recursive_right_internal_page_id, db::INVALID_PAGE_ID, leaf_d_keys))
    );

    recursive_page_manager.write_page(
        recursive_left_internal_page_id,
        db::serialize_node(make_internal(
            recursive_left_internal_page_id,
            recursive_root_page_id,
            {leaf_b_keys.front()},
            {leaf_a_page_id, leaf_b_page_id}
        ))
    );
    recursive_page_manager.write_page(
        recursive_right_internal_page_id,
        db::serialize_node(make_internal(
            recursive_right_internal_page_id,
            recursive_root_page_id,
            {leaf_d_keys.front()},
            {leaf_c_page_id, leaf_d_page_id}
        ))
    );
    recursive_page_manager.write_page(
        recursive_root_page_id,
        db::serialize_node(make_internal(
            recursive_root_page_id,
            db::INVALID_PAGE_ID,
            {leaf_c_keys.front()},
            {recursive_left_internal_page_id, recursive_right_internal_page_id}
        ))
    );

    db::BStarPlusIndex recursive_index(
        recursive_page_manager,
        db::BStarPlusIndexMetadata{
            .root_page_id = recursive_root_page_id,
            .first_leaf_page_id = leaf_a_page_id,
            .height = 3,
            .size = leaf_a_keys.size() + leaf_b_keys.size() + leaf_c_keys.size() + leaf_d_keys.size()
        }
    );

    assert(recursive_index.erase(0));
    assert(!recursive_index.find(0).has_value());
    assert(recursive_index.metadata().height == 2);
    assert(recursive_index.metadata().root_page_id == recursive_left_internal_page_id);
    assert(recursive_index.metadata().first_leaf_page_id == leaf_a_page_id);
    for (db::IndexKey key = 1; key < static_cast<db::IndexKey>(recursive_leaf_key_count * 4); ++key) {
        assert_found(recursive_index, key);
    }

    // test mixed (insert + erase)
    std::cout << "Good!\ntest mixed (insert + erase): ";
    InMemoryIndexPageManager series_page_manager;
    db::BStarPlusIndex series_index(series_page_manager);
    constexpr std::size_t series_key_count = test_leaf_capacity * 3 + 17;

    for (db::IndexKey key = 0; key < static_cast<db::IndexKey>(series_key_count); ++key) {
        series_index.insert(key, record_for_key(key));
    }

    for (db::IndexKey key = 0; key < static_cast<db::IndexKey>(series_key_count); key += 2) {
        assert(series_index.erase(key));
    }

    for (db::IndexKey key = 0; key < static_cast<db::IndexKey>(series_key_count); ++key) {
        const auto found_after_erase = series_index.find(key);
        if (key % 2 == 0) {
            assert(!found_after_erase.has_value());
        } else {
            assert(found_after_erase.has_value());
            assert(*found_after_erase == record_for_key(key));
        }
    }
    std::cout << "Good!";
    
    return 0;
}
