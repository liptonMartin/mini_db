//
// Created by rvova on 26.04.2026.
//

#include <iostream>
#include <cassert>

#include "datatypes.h"

int main() {
    // Database database("vova");
    //
    // std::vector<Column> columns;
    // columns.emplace_back("column_1", DataType::Int);
    // database.insert_table("table_2", columns);
    //
    // auto tables = database.get_tables();
    // for (const auto& item : tables) {
    //     std::cout << item << ' ';
    // }

    auto page = Page::create_page();
    std::vector<char> data;
    data.push_back('a');
    const auto first_slot_id = page.insert_element(data);
    const auto second_slot_id = page.insert_element(data);
    const auto third_slot_id = page.insert_element(data);
    assert(first_slot_id == 0);
    assert(second_slot_id == 1);
    assert(third_slot_id == 2);

    std::vector<Slot> occupied_slots = page.get_occupied_slots();
    std::vector<Slot> free_slots = page.get_free_slots();
    std::vector<Slot> slots = page.get_slots();

    page.erase_element(1);
    occupied_slots = page.get_occupied_slots();
    free_slots = page.get_free_slots();
    slots = page.get_slots();

    std::vector<char> big_data{'b', 'b', 'b'};
    const auto reused_slot_id = page.insert_element(big_data);
    assert(reused_slot_id == second_slot_id);
    occupied_slots = page.get_occupied_slots();
    free_slots = page.get_free_slots();
    slots = page.get_slots();

    auto slots_data = page.get_slots_data();
    for (auto &slot: slots_data) {
        for (auto& item: slot) {
            std::cout << item << ' ';
        }
        std::cout << '\n';
    }

    return 0;
}
