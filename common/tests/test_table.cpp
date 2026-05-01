//
// Created by rvova on 01.05.2026.
//
#include <gtest/gtest.h>

#include "datatypes.h"


class TableFixture : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::remove("test_table.binary");
        table = Table::create_table(std::filesystem::current_path(), "test_table", columns);
    }

    Table table;
    Column column_id = Column("id", DataType::Int, false, false);
    Column column_name = Column("name", DataType::String, false, false);
    Column column_count_cats = Column("count_cats", DataType::Int, true, false);
    std::vector<Column> columns{column_id, column_name, column_count_cats};
    std::vector<Column> columns_without_count_cats = {column_id, column_name};
};


Row create_row_from_values(const std::vector<Column> &columns, const std::vector<Value> &values) {
    Row row;
    if (columns.size() != values.size())
        throw std::runtime_error("Please fill values!");
    for (auto i = 0; i < columns.size(); ++i)
        row[columns[i]] = values[i];

    return row;
}


TEST_F(TableFixture, TestInsertElements) {
    std::vector<Value> values{1, "vova", 1};

    table.insert_elements(columns, values);
}

TEST_F(TableFixture, TestSelectElemets) {
    std::vector<Value> values{1, "vova", 1};

    table.insert_elements(columns, values);

    auto result = table.select_elements(nullptr);

    ASSERT_EQ(result.size(), 1);

    std::vector<Row> expected_result{create_row_from_values(columns, values)};

    ASSERT_EQ(expected_result, result);
}

TEST_F(TableFixture, TestInsertNullElements) {
    std::vector<Value> values_1{1, "vova", 1};
    std::vector<Value> values_2{2, "mr_palkin", Null{}};
    std::vector<Value> values_3{3, "artem"};
    std::vector<Value> values_4{4, "matvey", 0};


    table.insert_elements(columns, values_1);
    table.insert_elements(columns, values_2);
    table.insert_elements(columns_without_count_cats, values_3);
    table.insert_elements(columns, values_4);

    auto result = table.select_elements(nullptr);

    ASSERT_EQ(result.size(), 4);

    values_3.emplace_back(Null{}); /* заполнили недостающий элемент */
    std::vector<Row> expected_result{
        create_row_from_values(columns, values_1),
        create_row_from_values(columns, values_2),
        create_row_from_values(columns, values_3),
        create_row_from_values(columns, values_4),
    };

    ASSERT_EQ(result[0], expected_result[0]);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
