//
// Created by rvova on 26.04.2026.
//

#include <iostream>

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
    page.insert_element(data);
    page.insert_element(data);

    std::vector<Slot> slots = page.get_slots();


    return 0;
}