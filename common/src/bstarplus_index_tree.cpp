#include "bstarplus_index_tree.h"

#include <cstring>
#include <limits>
#include <type_traits>
#include <algorithm>

#include "constants.h"

namespace db {

namespace {

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
        sizeof(std::uint32_t);  // payload_count;

    template<typename T>
    void write_scalar(std::vector<char>& page, std::size_t& offset, const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);

        if (offset + sizeof(T) > page.size()) {
            throw std::length_error("Index node does not fit into page");
        }

        std::memcpy(page.data() + offset, &value, sizeof(T));
        offset += sizeof(T);
    }

    template<typename T>
    T read_scalar(const std::vector<char>& page, std::size_t& offset) {
        static_assert(std::is_trivially_copyable_v<T>);

        if (offset + sizeof(T) > page.size()) {
            throw std::runtime_error("Index node page is truncated");
        }

        T value{};
        std::memcpy(&value, page.data() + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    void validate_node_shape(const IndexNode& node) {
        if (node.keys.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::length_error("Index node has too many keys");
        }

        if (node.is_leaf) {
            if (!node.children.empty()) {
                throw std::invalid_argument("Leaf index node cannot have child pages");
            }
            if (node.records.size() != node.keys.size()) {
                throw std::invalid_argument("Leaf index node must have one record per key");
            }
            return;
        }

        if (!node.records.empty()) {
            throw std::invalid_argument("Internal index node cannot have records");
        }
        if (node.children.size() != node.keys.size() + 1) {
            throw std::invalid_argument("Internal index node must have keys.size() + 1 children");
        }
    }

    std::size_t serialized_node_size(const IndexNode& node) {
        const auto payload_count = node.is_leaf ? node.records.size() : node.children.size();

        if (payload_count > std::numeric_limits<std::uint32_t>::max()) {
            throw std::length_error("Index node payload is too large");
        }

        auto size = INDEX_NODE_HEADER_SIZE + node.keys.size() * sizeof(IndexKey);
        if (node.is_leaf) {
            size += node.records.size() * (sizeof(PageId) + sizeof(SlotId));
        } else {
            size += node.children.size() * sizeof(PageId);
        }

        return size;
    }

    } // namespace

    std::vector<char> serialize_node(const IndexNode& node) {
        validate_node_shape(node);

        if (serialized_node_size(node) > db::PAGE_SIZE) {
            throw std::length_error("Index node does not fit into page");
        }

        std::vector<char> page(db::PAGE_SIZE, 0);
        std::size_t offset = 0;

        const auto node_type = node.is_leaf ? LEAF_NODE_TYPE : INTERNAL_NODE_TYPE;
        const auto reserved = std::uint8_t{0};
        const auto key_count = static_cast<std::uint32_t>(node.keys.size());
        const auto payload_count = static_cast<std::uint32_t>(node.is_leaf ? node.records.size() : node.children.size());

        write_scalar(page, offset, INDEX_NODE_MAGIC);
        write_scalar(page, offset, INDEX_NODE_VERSION);
        write_scalar(page, offset, node_type);
        write_scalar(page, offset, reserved);
        write_scalar(page, offset, node.page_id);
        write_scalar(page, offset, node.parent_page_id);
        write_scalar(page, offset, node.next_leaf_page_id);
        write_scalar(page, offset, key_count);
        write_scalar(page, offset, payload_count);

        for (const auto key : node.keys) {
            write_scalar(page, offset, key);
        }

        if (node.is_leaf) {
            for (const auto& record : node.records) {
                write_scalar(page, offset, record.page_id);
                write_scalar(page, offset, record.slot_id);
            }
        } else {
            for (const auto child : node.children) {
                write_scalar(page, offset, child);
            }
        }

        return page;
    }

    IndexNode deserialize_node(const std::vector<char>& data) {
        if (data.size() != db::PAGE_SIZE) {
            throw std::invalid_argument("Index node page must have PAGE_SIZE bytes");
        }

        std::size_t offset = 0;

        const auto magic = read_scalar<std::uint32_t>(data, offset);
        if (magic != INDEX_NODE_MAGIC) {
            throw std::runtime_error("Invalid index node page magic");
        }

        const auto version = read_scalar<std::uint16_t>(data, offset);
        if (version != INDEX_NODE_VERSION) {
            throw std::runtime_error("Unsupported index node page version");
        }

        const auto node_type = read_scalar<std::uint8_t>(data, offset);
        read_scalar<std::uint8_t>(data, offset);

        if (node_type != INTERNAL_NODE_TYPE && node_type != LEAF_NODE_TYPE) {
            throw std::runtime_error("Invalid index node type");
        }

        IndexNode node;
        node.is_leaf = node_type == LEAF_NODE_TYPE;
        node.page_id = read_scalar<PageId>(data, offset);
        node.parent_page_id = read_scalar<PageId>(data, offset);
        node.next_leaf_page_id = read_scalar<PageId>(data, offset);

        const auto key_count = read_scalar<std::uint32_t>(data, offset);
        const auto payload_count = read_scalar<std::uint32_t>(data, offset);

        const auto payload_item_size = node.is_leaf ? sizeof(PageId) + sizeof(SlotId) : sizeof(PageId);
        const auto required_size = INDEX_NODE_HEADER_SIZE
            + static_cast<std::size_t>(key_count) * sizeof(IndexKey)
            + static_cast<std::size_t>(payload_count) * payload_item_size;
        if (required_size > data.size()) {
            throw std::runtime_error("Index node payload exceeds page size");
        }

        node.keys.reserve(key_count);
        for (std::uint32_t i = 0; i < key_count; ++i) {
            node.keys.push_back(read_scalar<IndexKey>(data, offset));
        }

        if (node.is_leaf) {
            if (payload_count != key_count) {
                throw std::runtime_error("Leaf index node payload count does not match key count");
            }

            node.records.reserve(payload_count);
            for (std::uint32_t i = 0; i < payload_count; ++i) {
                RecordId record;
                record.page_id = read_scalar<PageId>(data, offset);
                record.slot_id = read_scalar<SlotId>(data, offset);
                node.records.push_back(record);
            }
        } else {
            if (payload_count != key_count + 1) {
                throw std::runtime_error("Internal index node payload count does not match child count");
            }

            node.children.reserve(payload_count);
            for (std::uint32_t i = 0; i < payload_count; ++i) {
                node.children.push_back(read_scalar<PageId>(data, offset));
            }
        }

        validate_node_shape(node);
        return node;
    }

    bool BStarPlusIndexMetadata::empty() const noexcept {
        return root_page_id == INVALID_PAGE_ID || height == 0;
    }

    BStarPlusIndex::BStarPlusIndex(IndexPageManager &page_manager,
                                BStarPlusIndexMetadata metadata)
        : _page_manager(page_manager), _metadata(metadata) {
    }

    const BStarPlusIndexMetadata &BStarPlusIndex::metadata() const noexcept {
        return _metadata;
    }

    void BStarPlusIndex::insert(IndexKey key, RecordId record) {
        if (_metadata.empty()) {
            const PageId root_page_id = _page_manager.allocate_page();

            IndexNode root;
            root.is_leaf = true;
            root.page_id = root_page_id;
            root.parent_page_id = INVALID_PAGE_ID;
            root.next_leaf_page_id = INVALID_PAGE_ID;
            root.keys.push_back(key);
            root.records.push_back(record);

            _page_manager.write_page(root_page_id, serialize_node(root));

            _metadata.root_page_id = root_page_id;
            _metadata.first_leaf_page_id = root_page_id;
            _metadata.height = 1;
            _metadata.size = 1;

            return;
        }

        auto page = _page_manager.read_page(_metadata.root_page_id);
        auto root = deserialize_node(page);

        if (!root.is_leaf) {
            throw std::logic_error("Insert into non-leaf root is not implemented yet");
        }

        auto it = std::lower_bound(root.keys.begin(), root.keys.end(), key);
        const auto index = static_cast<std::size_t>(it - root.keys.begin());

        if (it != root.keys.end() && *it == key) {
            throw std::invalid_argument("Duplicate index key");
        }

        root.keys.insert(root.keys.begin() + index, key);
        root.records.insert(root.records.begin() + index, record);

        _page_manager.write_page(root.page_id, serialize_node(root));

        ++_metadata.size;
    }

    bool BStarPlusIndex::erase(IndexKey) {
        if (_metadata.empty()) {
            return false;
        }

        not_implemented();
    }

    void BStarPlusIndex::not_implemented() {
        throw std::logic_error("BStarPlusIndex page-based logic is not implemented yet");
    }

    std::optional<RecordId> BStarPlusIndex::find(IndexKey key) {
        if (_metadata.empty()) {
            return std::nullopt;
        }

        PageId current_page_id = _metadata.root_page_id;

        while (current_page_id != INVALID_PAGE_ID) {
            auto page = _page_manager.read_page(current_page_id);
            auto node = deserialize_node(page);

            auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);

            if (node.is_leaf) {
                if (it != node.keys.end() && *it == key) {
                    auto index = static_cast<std::size_t>(it - node.keys.begin());
                    return node.records[index];
                }

                return std::nullopt;
            }

            auto child_index = static_cast<std::size_t>(it - node.keys.begin());

            if (it != node.keys.end() && key >= *it) {
                ++child_index;
            }

            current_page_id = node.children[child_index];
        }

        return std::nullopt;
    }

} // namespace db

