#include "datatypes.h"

#include <format>
#include <stdexcept>

#include "constants.h"

PageManager::PageManager(std::fstream &file, const ptrdiff_t pages_begin_offset)
    : _file(file), _pages_begin_offset(pages_begin_offset) {
    if (_pages_begin_offset < 0) {
        throw std::invalid_argument("Pages begin offset cannot be negative");
    }
}

ptrdiff_t PageManager::page_offset(const ptrdiff_t page_id) const {
    if (page_id < 0) {
        throw std::invalid_argument("Page id cannot be negative");
    }

    return _pages_begin_offset + page_id * static_cast<ptrdiff_t>(db::PAGE_SIZE);
}

ptrdiff_t PageManager::allocate_page() {
    if (!_file.is_open()) {
        throw std::runtime_error("PageManager file is not open");
    }

    _file.clear();
    _file.seekp(0, std::ios::end);
    const auto end_position = _file.tellp();
    if (end_position == std::streampos(-1)) {
        throw std::runtime_error("Cannot determine file size");
    }

    const auto end_offset = static_cast<ptrdiff_t>(end_position);
    if (end_offset < _pages_begin_offset) {
        throw std::runtime_error("File ends before pages begin offset");
    }

    const auto pages_bytes = end_offset - _pages_begin_offset;
    if (pages_bytes % static_cast<ptrdiff_t>(db::PAGE_SIZE) != 0) {
        throw std::runtime_error("Page area size is not aligned to PAGE_SIZE");
    }

    const auto page_id = pages_bytes / static_cast<ptrdiff_t>(db::PAGE_SIZE);
    auto page = Page::create_page();
    const auto page_data = page.get_page_data();

    _file.seekp(page_offset(page_id), std::ios::beg);
    _file.write(page_data.data(), static_cast<ptrdiff_t>(page_data.size()));

    if (!_file) {
        throw std::runtime_error(std::format("Cannot write page {}", page_id));
    }

    return page_id;
}

ptrdiff_t PageManager::search_free_page(const ptrdiff_t need_size) {
    if (need_size < 0) {
        throw std::invalid_argument("Need size cannot be negative");
    }

    _file.clear();
    _file.seekg(0, std::ios::end);
    const auto end_position = _file.tellg();
    if (end_position == std::streampos(-1)) {
        throw std::runtime_error("Cannot determine file size");
    }

    const auto end_offset = static_cast<ptrdiff_t>(end_position);
    if (end_offset < _pages_begin_offset) {
        return -1;
    }

    const auto pages_bytes = end_offset - _pages_begin_offset;
    if (pages_bytes % static_cast<ptrdiff_t>(db::PAGE_SIZE) != 0) {
        throw std::runtime_error("Page area size is not aligned to PAGE_SIZE");
    }

    const auto pages_count = pages_bytes / static_cast<ptrdiff_t>(db::PAGE_SIZE);
    for (ptrdiff_t page_id = 0; page_id < pages_count; ++page_id) {
        auto page = read_page(page_id);
        if (page.get_free_size() >= need_size + sizeof(Slot)) {
            return page_id;
        }
    }

    return -1;
}

Page PageManager::read_page(const ptrdiff_t page_id) const {
    if (!_file.is_open()) {
        throw std::runtime_error("PageManager file is not open");
    }

    std::vector<char> data(db::PAGE_SIZE);

    _file.clear();
    _file.seekg(page_offset(page_id), std::ios::beg);
    _file.read(data.data(), static_cast<std::streamsize>(data.size()));

    if (_file.gcount() != static_cast<std::streamsize>(db::PAGE_SIZE)) {
        throw std::runtime_error(std::format("Cannot read page {}", page_id));
    }

    return Page(std::move(data));
}

ptrdiff_t PageManager::insert_element_into_page(const ptrdiff_t page_id, const std::vector<char> &data) const {
    if (!_file.is_open()) {
        throw std::runtime_error("PageManager file is not open");
    }

    auto page = read_page(page_id);
    const auto slot_id = page.insert_element(data);
    const auto page_data = page.get_page_data();

    _file.clear();
    _file.seekp(page_offset(page_id), std::ios::beg);
    _file.write(page_data.data(), static_cast<std::streamsize>(page_data.size()));

    if (!_file) {
        throw std::runtime_error(std::format("Cannot write page {}", page_id));
    }
    return slot_id;
}

void PageManager::erase_element_from_page(ptrdiff_t page_id, ptrdiff_t slot_id) const {
    if (!_file.is_open()) {
        throw std::runtime_error("PageManager file is not open");
    }

    auto page = read_page(page_id);
    page.erase_element(slot_id);

    const auto page_data = page.get_page_data();

    _file.clear();
    _file.seekp(page_offset(page_id), std::ios::beg);
    _file.write(page_data.data(), static_cast<std::streamsize>(page_data.size()));

    if (!_file) {
        throw std::runtime_error(std::format("Cannot write page {}", page_id));
    }
}

void PageManager::update_element_into_page(ptrdiff_t page_id, ptrdiff_t slot_id, const std::vector<char> &data) const {
    if (!_file.is_open()) {
        throw std::runtime_error("PageManager file is not open");
    }

    auto page = read_page(page_id);
    page.update_element(slot_id, data);

    const auto page_data = page.get_page_data();

    _file.clear();
    _file.seekp(page_offset(page_id), std::ios::beg);
    _file.write(page_data.data(), static_cast<std::streamsize>(page_data.size()));

    if (!_file) {
        throw std::runtime_error(std::format("Cannot write page {}", page_id));
    }
}
