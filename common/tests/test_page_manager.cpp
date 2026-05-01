//
// Created by rvova on 30.04.2026.
//
#include <gtest/gtest.h>

#include "constants.h"
#include "datatypes.h"


class FileTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file.open(test_file_name, std::ios::trunc | std::ios::out | std::ios::in);
    }
    void TearDown() override {
        test_file.close();
        std::filesystem::remove(test_file_name);
    }

    std::string test_file_name = "test_page_manager.txt";
    std::fstream test_file;
};


TEST_F(FileTest, TestAllocateFirstPage) {
    PageManager page_manager(test_file, 0);
    const auto page_id = page_manager.allocate_page();
    ASSERT_EQ(page_id, 0);
}

TEST_F(FileTest, TestReadFile) {
    PageManager page_manager(test_file, 0);
    const auto page_id = page_manager.allocate_page();

    auto page = page_manager.read_page(page_id);

    auto expected_page = Page::create_page();
    ASSERT_EQ(page.get_page_data(), expected_page.get_page_data());
}

TEST_F(FileTest, TestAllocateSeveralPages) {
    PageManager page_manager(test_file, 0);

    const auto page_id_1 = page_manager.allocate_page();
    const auto page_id_2 = page_manager.allocate_page();
    const auto page_id_3 = page_manager.allocate_page();
    const auto page_id_4 = page_manager.allocate_page();
    const auto page_id_5 = page_manager.allocate_page();

    ASSERT_EQ(page_id_1, 0);
    ASSERT_EQ(page_id_2, 1);
    ASSERT_EQ(page_id_3, 2);
    ASSERT_EQ(page_id_4, 3);
    ASSERT_EQ(page_id_5, 4);
}

TEST_F(FileTest, TestInsertIntoPage) {
    PageManager page_manager(test_file, 0);

    const auto page_id = page_manager.allocate_page();

    const auto data = std::vector<char>{'v', 'o', 'v', 'a'};
    const auto slot_id = page_manager.insert_element_into_page(page_id, data);

    auto page = page_manager.read_page(page_id);

    ASSERT_EQ(page.get_slot_data_by_id(slot_id), data);
}

TEST_F(FileTest, TestEraseFromPage) {
    PageManager page_manager(test_file, 0);

    const auto page_id = page_manager.allocate_page();

    const auto data = std::vector<char>{'v', 'o', 'v', 'a'};
    const auto slot_id = page_manager.insert_element_into_page(page_id, data);

    page_manager.erase_element_from_page(page_id, slot_id);

    auto page = page_manager.read_page(page_id);

    constexpr auto expected_size_page = db::PAGE_SIZE - sizeof(PageHeader);

    ASSERT_EQ(page.get_count_slots(), 0);
    ASSERT_EQ(page.get_free_size(), expected_size_page);
}

TEST_F(FileTest, E2ETestPageManager) {
    PageManager page_manager(test_file, 0);

    const auto page_id_1 = page_manager.allocate_page();
    const auto page_id_2 = page_manager.allocate_page();
    const auto page_id_3 = page_manager.allocate_page();

    const auto data_vova = std::vector<char>{'v', 'o', 'v', 'a'};
    const auto data_mr_palkin = std::vector<char>{'m', 'r', '_', 'p', 'a', 'l', 'k', 'i', 'n'};
    const auto data_artem = std::vector<char>{'a', 'e', 't', 'e', 'm'};
    const auto data_matvey = std::vector<char>{'m', 'a', 't', 'e', 'y'};

    const auto very_big_data = std::vector<char>(db::PAGE_SIZE - sizeof(PageHeader) - sizeof(Slot) - 1);

    auto free_page_id = page_manager.search_free_page(data_vova.size());
    ASSERT_EQ(free_page_id, 0);
    auto slot_id = page_manager.insert_element_into_page(free_page_id, data_vova);
    ASSERT_EQ(slot_id, 0);

    free_page_id = page_manager.search_free_page(data_mr_palkin.size());
    ASSERT_EQ(free_page_id, 0);
    slot_id = page_manager.insert_element_into_page(free_page_id, data_mr_palkin);
    ASSERT_EQ(slot_id, 1);

    free_page_id = page_manager.search_free_page(data_artem.size());
    ASSERT_EQ(free_page_id, 0);
    slot_id = page_manager.insert_element_into_page(free_page_id, data_artem);
    ASSERT_EQ(slot_id, 2);

    /* удаляем один элемент, чтобы проверить, что все сдвигается */
    page_manager.erase_element_from_page(0, 0);

    free_page_id = page_manager.search_free_page(data_matvey.size());
    ASSERT_EQ(free_page_id, 0);
    slot_id = page_manager.insert_element_into_page(free_page_id, data_matvey);
    ASSERT_EQ(slot_id, 0); /* вставился в первый элемент */

    free_page_id = page_manager.search_free_page(very_big_data.size());
    ASSERT_EQ(free_page_id, 1); /* на нулевой странице места нет */
    slot_id = page_manager.insert_element_into_page(free_page_id, very_big_data);
    ASSERT_EQ(slot_id, 0);

    auto first_page = page_manager.read_page(0);
    std::vector<std::vector<char>> data_first_page = first_page.get_slots_data();

    ASSERT_EQ(data_first_page[0], data_matvey);
    ASSERT_EQ(data_first_page[1], data_mr_palkin);
    ASSERT_EQ(data_first_page[2], data_artem);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}