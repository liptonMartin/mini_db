//
// Created by rvova on 28.04.2026.
//
#include <algorithm>
#include <format>


#include "datatypes.h"
#include "exceptions.h"
#include "constants.h"


Page::Page(std::vector<char> &&data) : _data(std::move(data)) {
}

/**
 * Функция, которая инициализирует страницу
 */
Page Page::create_page() {
    std::vector<char> data;
    data.resize(db::PAGE_SIZE);

    PageHeader header{};
    header._free_size = db::PAGE_SIZE - sizeof(header);
    header._count_slots = 0;

    std::copy_n(reinterpret_cast<const char *>(&header), sizeof(header), data.begin());

    return Page(std::move(data));
}

PageHeader Page::get_header() {
    PageHeader header{};

    std::copy_n(_data.begin(), sizeof(header), reinterpret_cast<char *>(&header));
    return header;
}

ptrdiff_t Page::get_free_size() {
    return get_header()._free_size;
}

ptrdiff_t Page::get_count_slots() {
    return get_header()._count_slots;
}

std::vector<char> Page::get_page_data() {
    return _data;
}

std::vector<Slot> Page::get_slots() {
    std::vector<Slot> slots;
    const auto count_slots = get_count_slots();

    for (auto i = 0; i < count_slots; ++i) {
        Slot slot;

        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));

        slots.push_back(slot);
    }

    return slots;
}

std::vector<Slot> Page::get_occupied_slots() {
    std::vector<Slot> slots;
    const auto count_slots = get_count_slots();

    for (auto i = 0; i < count_slots; ++i) {
        Slot slot;

        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));

        if (slot.is_occupied()) slots.push_back(slot);
    }

    return slots;
}

std::vector<Slot> Page::get_free_slots() {
    std::vector<Slot> slots;
    const auto count_slots = get_count_slots();

    for (auto i = 0; i < count_slots; ++i) {
        Slot slot;

        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));

        if (!slot.is_occupied()) slots.push_back(slot);
    }

    return slots;
}

Slot Page::get_last_occupied_slot() {
    auto count_slots = get_count_slots();

    if (count_slots == 0) throw SlotNotFoundException("Page doesn't have any slots");

    /* считываем последний слот */
    Slot slot{};
    /* проходимся по всем slots на странице и ищем slot_id */
    for (auto i = count_slots - 1; i >= 0; --i) {
        /* считываем слот */
        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));
        if (slot.is_occupied()) return slot;
    }

    throw SlotNotFoundException("Page doesn't have occupied slots!");
}

Slot Page::get_slot(ptrdiff_t slot_id) {
    const auto count_slots = get_count_slots();
    Slot slot{};

    if (slot_id >= count_slots)
        throw SlotNotFoundException(std::format("Page doesn't have slot id {}", slot_id));

    std::copy_n(_data.begin() + sizeof(PageHeader) + slot_id * sizeof(Slot), sizeof(Slot),
                reinterpret_cast<char *>(&slot));
    return slot;
}


void Page::add_new_slot(const Slot &slot) {
    auto count_slots = get_count_slots();

    /* записываем новый слот */
    std::copy_n(reinterpret_cast<const char *>(&slot), sizeof(Slot),
                _data.begin() + sizeof(PageHeader) + count_slots * sizeof(Slot));

    /* увеличиваем количество слотов */
    ++count_slots;

    /* обновляем данные */
    update_count_slots(count_slots);
}

void Page::update_free_size(const ptrdiff_t size) {
    auto header = get_header();

    header._free_size = size;
    std::copy_n(reinterpret_cast<const char *>(&header), sizeof(header), _data.begin());
}

void Page::update_count_slots(const ptrdiff_t count_slots) {
    auto header = get_header();

    header._count_slots = count_slots;
    std::copy_n(reinterpret_cast<const char *>(&header), sizeof(header), _data.begin());
}

void Page::update_offsets_in_slots(const ptrdiff_t slot_id, const ptrdiff_t shift_offset) {
    const auto count_slots = get_count_slots();

    for (auto i = slot_id; i < count_slots; ++i) {
        Slot slot;

        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));

        auto old_offset = slot.get_offset();
        slot.set_offset(old_offset + shift_offset);

        /* записываем новые данные */
        std::copy_n(reinterpret_cast<const char *>(&slot), sizeof(Slot),
                    _data.begin() + sizeof(PageHeader) + i * sizeof(Slot));
    }
}

ptrdiff_t Page::get_id_first_free_block() {
    const auto count_slots = get_count_slots();
    Slot slot{};

    /* проходимся по всем slots на странице и ищем свободный блок */
    for (auto i = 0; i < count_slots; ++i) {
        /* считываем слот */
        std::copy_n(_data.begin() + sizeof(PageHeader) + i * sizeof(Slot), sizeof(Slot),
                    reinterpret_cast<char *>(&slot));
        if (!slot.is_occupied()) return i;
    }

    throw SlotNotFoundException("Page doesn't have holes!");
}

void Page::make_busy_slot(Slot &slot, const ptrdiff_t offset, const ptrdiff_t size) {
    slot.make_busy(offset, size);

    /* обновляем слот */
    std::copy_n(reinterpret_cast<const char *>(&slot), sizeof(Slot),
                _data.begin() + sizeof(PageHeader) + slot.get_id() * sizeof(Slot));
}


void Page::release_slot(Slot &slot) {
    slot.release();

    /* обновляем слот */
    std::copy_n(reinterpret_cast<const char *>(&slot), sizeof(Slot),
                _data.begin() + sizeof(PageHeader) + slot.get_id() * sizeof(Slot));
}


/**
 * Вставка в страницу данных.
 * Внимание: в странице должно быть место для вставки, иначе выбрасывается исключение
 * @param data Значение, которое надо вставить
 * @exception InvalidWriteToPageException Не хватило места для вставки
 */
ptrdiff_t Page::insert_element(const std::vector<char> &data) {
    const auto len_data = std::ssize(data);
    const auto old_free_size = get_free_size();
    auto old_count_slots = get_count_slots();

    std::optional<ptrdiff_t> id_free_block;
    try {
        id_free_block = get_id_first_free_block();
    } catch (SlotNotFoundException &) {
    }

    const auto required_size = len_data + (id_free_block.has_value() ? 0 : static_cast<ptrdiff_t>(sizeof(Slot)));
    if (required_size > old_free_size)
        throw InvalidWriteToPageException("Page doesn't have space for insert element!");

    Slot last_slot;
    try {
        last_slot = get_last_occupied_slot(); /* последний занятый блок */
    } catch (SlotNotFoundException &e) {
        last_slot = Slot(-1, db::PAGE_SIZE, 0);
    }

    const ptrdiff_t new_offset = last_slot.get_offset() - len_data;

    /* записываем новые данные по новому смещению */
    std::copy_n(data.begin(), len_data, _data.begin() + new_offset);

    if (id_free_block.has_value()) {
        update_free_size(old_free_size - len_data);

        auto slot = get_slot(*id_free_block);

        make_busy_slot(slot, new_offset, len_data);
        return *id_free_block;
    }

    const auto new_slot_id = old_count_slots; /* индексация с нуля */

    /* будем создавать новый слот */
    update_free_size(old_free_size - len_data - static_cast<ptrdiff_t>(sizeof(Slot)));

    const Slot slot(new_slot_id, new_offset, len_data);
    add_new_slot(slot); /* самостоятельно увеличивает count_slots */
    return new_slot_id;
}

void Page::erase_element(ptrdiff_t slot_id) {
    auto count_slots = get_count_slots();
    if (slot_id >= count_slots || slot_id < 0)
        throw SlotNotFoundException(std::format("Page doesn't have slot id {}", slot_id));

    auto deleted_slot = get_slot(slot_id);
    if (!deleted_slot.is_occupied())
        throw InvalidSlotEraseException(std::format("The slot id {} has been already released", slot_id));
    auto offset_deleted_slot = deleted_slot.get_offset();
    auto size_deleted_slot = deleted_slot.get_size();

    auto last_occupied_slot = get_last_occupied_slot();
    auto offset_last_slot = last_occupied_slot.get_offset();

    /* часть страницы, которую надо перенести */
    std::vector<char> data_to_move(_data.begin() + offset_last_slot, _data.begin() + offset_deleted_slot);

    /* сдвигаем ровно на size_deleted_slot */
    std::copy_n(data_to_move.begin(), data_to_move.size(), _data.begin() + offset_last_slot + size_deleted_slot);

    /* меняем offset для измененных слотов */
    update_offsets_in_slots(slot_id, size_deleted_slot);

    auto old_free_size = get_free_size();
    if (slot_id == count_slots - 1) {
        /* удаляемый элемент был последним, значит можно полностью освободить данные на уровне Slot */
        --count_slots; /* Удаляем слот, а не освобождаем его! */
        update_count_slots(count_slots);
        update_free_size(old_free_size + size_deleted_slot + sizeof(Slot));
    } else {
        /* Освобождаем слот, но не удаляем его, поэтому count_slots на странице не меняется! */
        release_slot(deleted_slot);
        update_free_size(old_free_size + size_deleted_slot);
    }
}

void Page::update_element(ptrdiff_t slot_id, const std::vector<char> &data) {
    const auto slot = get_slot(slot_id);
    const auto slot_offset = slot.get_offset();

    std::copy_n(data.begin(), data.size(), _data.begin() + slot_offset);
}

std::vector<char> Page::get_slot_data(const Slot &slot) {
    const auto size = slot.get_size();
    const auto offset = slot.get_offset();

    std::vector<char> slot_data;
    slot_data.resize(size);

    std::copy_n(_data.begin() + offset, size, slot_data.begin());
    return slot_data;
}

std::vector<std::vector<char> > Page::get_slots_data() {
    std::vector<std::vector<char> > slots_data;
    const auto count_slots = get_count_slots();
    for (int i = 0; i < count_slots; ++i) {
        auto slot = get_slot(i);

        if (!slot.is_occupied()) continue;

        auto slot_data = get_slot_data(slot);
        slots_data.push_back(slot_data);
    }
    return slots_data;
}

std::vector<char> Page::get_slot_data_by_id(const ptrdiff_t slot_id) {
    const auto slot = get_slot(slot_id);
    return get_slot_data(slot);
}

