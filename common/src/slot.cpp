//
// Created by rvova on 28.04.2026.
//

#include "datatypes.h"


Slot::Slot(const ptrdiff_t slot_id, const ptrdiff_t offset, const ptrdiff_t size) : _slot_id(slot_id), _offset(offset),
    _size(size), _is_occupied(true) {
}

ptrdiff_t Slot::get_offset() const {
    return _offset;
}

void Slot::set_offset(const ptrdiff_t offset) {
    _offset = offset;
}

ptrdiff_t Slot::get_size() const {
    return _size;
}

ptrdiff_t Slot::get_id() const {
    return _slot_id;
}

bool Slot::is_occupied() const {
    return _is_occupied;
}

void Slot::release() {
    _offset = -1;
    _size = -1;
    _is_occupied = false;
}

void Slot::make_busy(const ptrdiff_t offset, const ptrdiff_t size) {
    _offset = offset;
    _size = size;
    _is_occupied = true;
}