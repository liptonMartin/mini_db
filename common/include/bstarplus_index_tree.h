#ifndef MINIDB_BSTARPLUS_INDEX_H
#define MINIDB_BSTARPLUS_INDEX_H

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace db {

using PageId = std::uint64_t;
using SlotId = std::uint16_t;
using IndexKey = std::int64_t;

inline constexpr PageId INVALID_PAGE_ID = UINT64_MAX;

struct RecordId {
    PageId page_id = INVALID_PAGE_ID;
    SlotId slot_id = 0;

    friend bool operator==(const RecordId&, const RecordId&) = default;
};

struct IndexNode {
    bool is_leaf = true;
    PageId page_id = INVALID_PAGE_ID;
    PageId parent_page_id = INVALID_PAGE_ID;
    PageId next_leaf_page_id = INVALID_PAGE_ID;
    std::vector<IndexKey> keys;
    std::vector<PageId> children;
    std::vector<RecordId> records;

    friend bool operator==(const IndexNode&, const IndexNode&) = default;
};

std::vector<char> serialize_node(const IndexNode& node);

IndexNode deserialize_node(const std::vector<char>& data);

struct BStarPlusIndexMetadata {
    PageId root_page_id = INVALID_PAGE_ID;
    PageId first_leaf_page_id = INVALID_PAGE_ID;
    std::uint32_t height = 0;
    std::uint64_t size = 0;

    bool empty() const noexcept;
};

class IndexPageManager {
public:
    virtual ~IndexPageManager() = default;

    virtual PageId allocate_page() = 0;
    virtual std::vector<char> read_page(PageId page_id) = 0;
    virtual void write_page(PageId page_id, const std::vector<char>& data) = 0;
    virtual void flush() = 0;
};

class BStarPlusIndex {
public:
    explicit BStarPlusIndex(
        IndexPageManager& page_manager,
        BStarPlusIndexMetadata metadata = {}
    );

    const BStarPlusIndexMetadata& metadata() const noexcept;

    std::optional<RecordId> find(IndexKey key);
    void insert(IndexKey key, RecordId record);
    bool erase(IndexKey key);

private:
    [[noreturn]] static void not_implemented();

    IndexPageManager& _page_manager;
    BStarPlusIndexMetadata _metadata;
};

} // namespace db

#endif // MINIDB_BSTARPLUS_INDEX_H
