#include <cassert>
#include <unordered_map>

#include "bstarplus_index_tree.h"

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

    return 0;
}
