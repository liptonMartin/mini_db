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

    constexpr std::uint32_t INDEX_NODE_MAGIC = 0x31495042;
    constexpr std::uint16_t INDEX_NODE_VERSION = 1;
    constexpr std::uint8_t INTERNAL_NODE_TYPE = 0;
    constexpr std::uint8_t LEAF_NODE_TYPE = 1;
    constexpr std::size_t INDEX_NODE_HEADER_SIZE =
        sizeof(std::uint32_t) + // magic
        sizeof(std::uint16_t) + // version
        sizeof(std::uint8_t)  + // node_type
        sizeof(std::uint8_t)  + // reserved
        sizeof(PageId)        + // page_id
        sizeof(PageId)        + // parent_page_id
        sizeof(PageId)        + // next_leaf_page_id
        sizeof(std::uint32_t) + // key_count
        sizeof(std::uint32_t);  // payload_count

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
        struct LeafSearchResult {
            IndexNode leaf;
            std::vector<PageId> path;
        };

        LeafSearchResult find_leaf(IndexKey key);
        IndexNode read_node(PageId page_id);
        void write_node(const IndexNode& node);
        bool node_fits_page(const IndexNode& node);
        void insert_into_parent(IndexNode& left, IndexKey separator, IndexNode& right, const std::vector<PageId>& left_path);
        void split_leaf(IndexNode& leaf, const std::vector<PageId>& path);
        bool try_give_to_right_leaf(IndexNode& leaf, IndexNode& parent, std::size_t child_index);
        bool try_give_to_left_leaf(IndexNode& leaf, IndexNode& parent, std::size_t child_index);
        void split_leaf_pair(IndexNode& left, IndexNode& right, IndexNode& parent, std::size_t parent_index,
                             const std::vector<PageId>& parent_path);
        void split_internal(IndexNode& node, const std::vector<PageId>& path);
        void update_children_parent(const IndexNode& node);

        [[noreturn]] static void not_implemented();

        IndexPageManager& _page_manager;
        BStarPlusIndexMetadata _metadata;
    };

} // namespace db

#endif // MINIDB_BSTARPLUS_INDEX_H
