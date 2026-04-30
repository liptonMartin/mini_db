#include "bstarplus_index_tree.h"

#include <cstring>
#include <limits>
#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include <utility>

#include "constants.h"

namespace db {
    
    namespace {

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

        std::size_t child_index_for_key(const std::vector<IndexKey>& keys, const IndexKey key) {
            return static_cast<std::size_t>(std::upper_bound(keys.begin(), keys.end(), key) - keys.begin());
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

    IndexNode BStarPlusIndex::read_node(const PageId page_id) {
        return deserialize_node(_page_manager.read_page(page_id));
    }

    void BStarPlusIndex::write_node(const IndexNode& node) {
        _page_manager.write_page(node.page_id, serialize_node(node));
    }

    bool BStarPlusIndex::node_fits_page(const IndexNode& node) {
        try {
            serialize_node(node);
            return true;
        } catch (const std::length_error&) {
            return false;
        }
    }

    void BStarPlusIndex::update_children_parent(const IndexNode& node) {
        if (node.is_leaf) {
            return;
        }

        for (const auto child_page_id : node.children) {
            auto child = read_node(child_page_id);
            if (child.parent_page_id != node.page_id) {
                child.parent_page_id = node.page_id;
                write_node(child);
            }
        }
    }

    BStarPlusIndex::LeafSearchResult BStarPlusIndex::find_leaf(const IndexKey key) {
        PageId current_page_id = _metadata.root_page_id;
        std::vector<PageId> path;

        while (current_page_id != INVALID_PAGE_ID) {
            path.push_back(current_page_id);

            auto page = _page_manager.read_page(current_page_id);
            auto node = deserialize_node(page);

            if (node.is_leaf) {
                return LeafSearchResult{std::move(node), std::move(path)};
            }

            const auto child_index = child_index_for_key(node.keys, key);
            if (child_index >= node.children.size()) {
                throw std::runtime_error("Index internal node child pointer is missing");
            }

            current_page_id = node.children[child_index];
        }

        throw std::runtime_error("Index leaf page was not found");
    }

    void BStarPlusIndex::insert_into_parent(IndexNode& left, const IndexKey separator, IndexNode& right,
                                            const std::vector<PageId>& left_path) {
        if (left_path.empty() || left_path.back() != left.page_id) {
            throw std::logic_error("Invalid path to left index node");
        }

        if (left_path.size() == 1) {
            const PageId new_root_page_id = _page_manager.allocate_page();

            IndexNode new_root;
            new_root.is_leaf = false;
            new_root.page_id = new_root_page_id;
            new_root.parent_page_id = INVALID_PAGE_ID;
            new_root.keys = {separator};
            new_root.children = {left.page_id, right.page_id};

            left.parent_page_id = new_root_page_id;
            right.parent_page_id = new_root_page_id;

            write_node(left);
            write_node(right);
            write_node(new_root);

            _metadata.root_page_id = new_root_page_id;
            ++_metadata.height;
            return;
        }

        const auto parent_page_id = left_path[left_path.size() - 2];
        auto parent = read_node(parent_page_id);
        if (parent.is_leaf) {
            throw std::logic_error("Leaf node cannot be a parent");
        }

        auto child_it = std::find(parent.children.begin(), parent.children.end(), left.page_id);
        if (child_it == parent.children.end()) {
            throw std::logic_error("Parent does not reference split child");
        }

        const auto child_index = static_cast<std::size_t>(child_it - parent.children.begin());
        parent.keys.insert(parent.keys.begin() + static_cast<std::ptrdiff_t>(child_index), separator);
        parent.children.insert(parent.children.begin() + static_cast<std::ptrdiff_t>(child_index + 1), right.page_id);

        left.parent_page_id = parent.page_id;
        right.parent_page_id = parent.page_id;
        write_node(left);
        write_node(right);

        if (node_fits_page(parent)) {
            write_node(parent);
            return;
        }

        auto parent_path = left_path;
        parent_path.pop_back();
        split_internal(parent, parent_path);
    }

    void BStarPlusIndex::split_leaf(IndexNode& leaf, const std::vector<PageId>& path) {
        if (!leaf.is_leaf) {
            throw std::logic_error("Only leaf node can be split by split_leaf");
        }

        if (path.empty() || path.back() != leaf.page_id) {
            throw std::logic_error("Invalid path to leaf index node");
        }

        if (path.size() > 1) {
            const auto parent_page_id = path[path.size() - 2];
            auto parent = read_node(parent_page_id);
            if (parent.is_leaf) {
                throw std::logic_error("Leaf node cannot be a parent");
            }

            const auto child_it = std::find(parent.children.begin(), parent.children.end(), leaf.page_id);
            if (child_it == parent.children.end()) {
                throw std::logic_error("Parent does not reference split leaf");
            }

            const auto child_index = static_cast<std::size_t>(child_it - parent.children.begin());
            if (try_give_to_right_leaf(leaf, parent, child_index)) {
                return;
            }
            if (try_give_to_left_leaf(leaf, parent, child_index)) {
                return;
            }

            auto parent_path = path;
            parent_path.pop_back();

            if (child_index + 1 < parent.children.size()) {
                auto right = read_node(parent.children[child_index + 1]);
                if (!right.is_leaf) {
                    throw std::logic_error("Leaf sibling has unexpected node type");
                }
                split_leaf_pair(leaf, right, parent, child_index, parent_path);
                return;
            }

            if (child_index > 0) {
                auto left = read_node(parent.children[child_index - 1]);
                if (!left.is_leaf) {
                    throw std::logic_error("Leaf sibling has unexpected node type");
                }
                split_leaf_pair(left, leaf, parent, child_index - 1, parent_path);
                return;
            }
        }

        const auto right_page_id = _page_manager.allocate_page();
        IndexNode right;
        right.is_leaf = true;
        right.page_id = right_page_id;
        right.parent_page_id = leaf.parent_page_id;
        right.next_leaf_page_id = leaf.next_leaf_page_id;

        const auto split_index = leaf.keys.size() / 2;

        right.keys.assign(leaf.keys.begin() + static_cast<std::ptrdiff_t>(split_index), leaf.keys.end());
        right.records.assign(leaf.records.begin() + static_cast<std::ptrdiff_t>(split_index), leaf.records.end());

        leaf.keys.erase(leaf.keys.begin() + static_cast<std::ptrdiff_t>(split_index), leaf.keys.end());
        leaf.records.erase(leaf.records.begin() + static_cast<std::ptrdiff_t>(split_index), leaf.records.end());
        leaf.next_leaf_page_id = right.page_id;

        if (right.keys.empty()) {
            throw std::logic_error("Leaf split produced empty right node");
        }

        const auto separator = right.keys.front();
        insert_into_parent(leaf, separator, right, path);
    }

    bool BStarPlusIndex::try_give_to_right_leaf(IndexNode& leaf, IndexNode& parent, const std::size_t child_index) {
        if (child_index + 1 >= parent.children.size()) {
            return false;
        }

        auto right = read_node(parent.children[child_index + 1]);
        if (!right.is_leaf || leaf.keys.empty()) {
            return false;
        }

        auto candidate_leaf = leaf;
        auto candidate_right = right;

        const auto key = candidate_leaf.keys.back();
        const auto record = candidate_leaf.records.back();
        candidate_leaf.keys.pop_back();
        candidate_leaf.records.pop_back();
        candidate_right.keys.insert(candidate_right.keys.begin(), key);
        candidate_right.records.insert(candidate_right.records.begin(), record);

        if (candidate_leaf.keys.empty()) {
            return false;
        }

        if (!node_fits_page(candidate_right) || !node_fits_page(candidate_leaf)) {
            return false;
        }

        leaf = std::move(candidate_leaf);
        right = std::move(candidate_right);
        parent.keys[child_index] = right.keys.front();

        write_node(leaf);
        write_node(right);
        write_node(parent);
        return true;
    }

    bool BStarPlusIndex::try_give_to_left_leaf(IndexNode& leaf, IndexNode& parent, const std::size_t child_index) {
        if (child_index == 0 || leaf.keys.empty()) {
            return false;
        }

        auto left = read_node(parent.children[child_index - 1]);
        if (!left.is_leaf) {
            return false;
        }

        auto candidate_left = left;
        auto candidate_leaf = leaf;

        const auto key = candidate_leaf.keys.front();
        const auto record = candidate_leaf.records.front();
        candidate_leaf.keys.erase(candidate_leaf.keys.begin());
        candidate_leaf.records.erase(candidate_leaf.records.begin());
        candidate_left.keys.push_back(key);
        candidate_left.records.push_back(record);

        if (candidate_leaf.keys.empty()) {
            return false;
        }

        if (!node_fits_page(candidate_left) || !node_fits_page(candidate_leaf)) {
            return false;
        }

        left = std::move(candidate_left);
        leaf = std::move(candidate_leaf);
        parent.keys[child_index - 1] = leaf.keys.front();

        write_node(left);
        write_node(leaf);
        write_node(parent);
        return true;
    }

    void BStarPlusIndex::split_leaf_pair(IndexNode& left, IndexNode& right, IndexNode& parent,
                                         const std::size_t parent_index, const std::vector<PageId>& parent_path) {
        std::vector<IndexKey> keys;
        std::vector<RecordId> records;
        keys.reserve(left.keys.size() + right.keys.size());
        records.reserve(left.records.size() + right.records.size());

        keys.insert(keys.end(), left.keys.begin(), left.keys.end());
        keys.insert(keys.end(), right.keys.begin(), right.keys.end());
        records.insert(records.end(), left.records.begin(), left.records.end());
        records.insert(records.end(), right.records.begin(), right.records.end());

        std::vector<std::pair<IndexKey, RecordId>> items;
        items.reserve(keys.size());
        for (std::size_t i = 0; i < keys.size(); ++i) {
            items.emplace_back(keys[i], records[i]);
        }
        std::sort(items.begin(), items.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });

        const auto middle_page_id = _page_manager.allocate_page();
        IndexNode middle;
        middle.is_leaf = true;
        middle.page_id = middle_page_id;
        middle.parent_page_id = parent.page_id;
        middle.next_leaf_page_id = right.page_id;

        const auto first_count = items.size() / 3;
        const auto second_count = items.size() / 3;

        left.keys.clear();
        left.records.clear();
        middle.keys.clear();
        middle.records.clear();
        right.keys.clear();
        right.records.clear();

        for (std::size_t i = 0; i < items.size(); ++i) {
            auto* target = &right;
            if (i < first_count) {
                target = &left;
            } else if (i < first_count + second_count) {
                target = &middle;
            }

            target->keys.push_back(items[i].first);
            target->records.push_back(items[i].second);
        }

        if (left.keys.empty() || middle.keys.empty() || right.keys.empty()) {
            throw std::logic_error("Leaf pair split produced an empty node");
        }

        left.next_leaf_page_id = middle.page_id;
        middle.next_leaf_page_id = right.page_id;
        right.parent_page_id = parent.page_id;

        parent.keys[parent_index] = middle.keys.front();
        parent.keys.insert(parent.keys.begin() + static_cast<std::ptrdiff_t>(parent_index + 1), right.keys.front());
        parent.children.insert(parent.children.begin() + static_cast<std::ptrdiff_t>(parent_index + 1), middle.page_id);

        write_node(left);
        write_node(middle);
        write_node(right);

        if (node_fits_page(parent)) {
            write_node(parent);
            return;
        }

        split_internal(parent, parent_path);
    }

    void BStarPlusIndex::split_internal(IndexNode& node, const std::vector<PageId>& path) {
        if (node.is_leaf) {
            throw std::logic_error("Leaf node cannot be split by split_internal");
        }

        if (node.keys.empty()) {
            throw std::logic_error("Cannot split empty internal node");
        }

        const auto split_index = node.keys.size() / 2;
        const auto separator = node.keys[split_index];

        const auto right_page_id = _page_manager.allocate_page();
        IndexNode right;
        right.is_leaf = false;
        right.page_id = right_page_id;
        right.parent_page_id = node.parent_page_id;
        right.keys.assign(node.keys.begin() + static_cast<std::ptrdiff_t>(split_index + 1), node.keys.end());
        right.children.assign(node.children.begin() + static_cast<std::ptrdiff_t>(split_index + 1), node.children.end());

        node.keys.erase(node.keys.begin() + static_cast<std::ptrdiff_t>(split_index), node.keys.end());
        node.children.erase(node.children.begin() + static_cast<std::ptrdiff_t>(split_index + 1), node.children.end());

        update_children_parent(right);
        insert_into_parent(node, separator, right, path);
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

        auto leaf_search = find_leaf(key);
        auto& leaf = leaf_search.leaf;

        auto it = std::lower_bound(leaf.keys.begin(), leaf.keys.end(), key);
        const auto index = static_cast<std::size_t>(it - leaf.keys.begin());

        if (it != leaf.keys.end() && *it == key) {
            throw std::invalid_argument("Duplicate index key");
        }

        leaf.keys.insert(leaf.keys.begin() + index, key);
        leaf.records.insert(leaf.records.begin() + index, record);

        if (node_fits_page(leaf)) {
            write_node(leaf);
        } else {
            split_leaf(leaf, leaf_search.path);
        }

        ++_metadata.size;
    }

    bool BStarPlusIndex::erase(IndexKey key) {
        if (_metadata.empty()) {
            return false;
        }

        auto leaf_search = find_leaf(key);
        auto& leaf = leaf_search.leaf;

        auto it = std::lower_bound(leaf.keys.begin(), leaf.keys.end(), key);
        if (it == leaf.keys.end() || *it != key) {
            return false;
        }

        const auto erased_index = static_cast<std::size_t>(it - leaf.keys.begin());
        leaf.keys.erase(it);
        leaf.records.erase(leaf.records.begin() + static_cast<std::ptrdiff_t>(erased_index));

        if (_metadata.size > 0) {
            --_metadata.size;
        }

        write_node(leaf);

        if (_metadata.size == 0) {
            _metadata.root_page_id = INVALID_PAGE_ID;
            _metadata.first_leaf_page_id = INVALID_PAGE_ID;
            _metadata.height = 0;
            return true;
        }

        // TODO: rebalance underfilled leaves with borrow/merge.
        if (erased_index == 0 && !leaf.keys.empty() && leaf_search.path.size() > 1) {
            const auto parent_page_id = leaf_search.path[leaf_search.path.size() - 2];
            auto parent = read_node(parent_page_id);
            if (parent.is_leaf) {
                throw std::logic_error("Leaf node cannot be a parent");
            }

            const auto child_it = std::find(parent.children.begin(), parent.children.end(), leaf.page_id);
            if (child_it == parent.children.end()) {
                throw std::logic_error("Parent does not reference erased leaf");
            }

            const auto child_index = static_cast<std::size_t>(child_it - parent.children.begin());
            if (child_index > 0) {
                parent.keys[child_index - 1] = leaf.keys.front();
                write_node(parent);
            }
        }

        return true;
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

            if (node.is_leaf) {
                auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);
                if (it != node.keys.end() && *it == key) {
                    auto index = static_cast<std::size_t>(it - node.keys.begin());
                    return node.records[index];
                }

                return std::nullopt;
            }

            const auto child_index = child_index_for_key(node.keys, key);
            if (child_index >= node.children.size()) {
                throw std::runtime_error("Index internal node child pointer is missing");
            }

            current_page_id = node.children[child_index];
        }

        return std::nullopt;
    }

} // namespace db
