#include <cassert>
#include <unordered_map>

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
    const auto found = index.find(10);
    assert(found.has_value());
    assert(*found == second_record);

    const auto missing = index.find(15);
    assert(!missing.has_value());

    // test insert
    db::RecordId insert_record = {15, 4};
    index.insert(30, insert_record);
    assert(index.find(30) == insert_record);

    // test erase
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
    InMemoryIndexPageManager split_page_manager;
    db::BStarPlusIndex split_index(split_page_manager);

    constexpr std::size_t leaf_entry_size = sizeof(db::IndexKey) + sizeof(db::PageId) + sizeof(db::SlotId);
    constexpr std::size_t leaf_capacity = (db::PAGE_SIZE - db::INDEX_NODE_HEADER_SIZE) / leaf_entry_size;
    constexpr std::size_t key_count = leaf_capacity * 2 + 25;

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

    return 0;
}
