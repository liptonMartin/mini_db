
//
// Created by rvova on 01.05.2026.
//
#include <gtest/gtest.h>
#include <unordered_map>

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
};


Row create_row_from_values(const std::vector<Column> &columns, const std::vector<Value> &values) {
    Row row;
    if (columns.size() != values.size())
        throw std::runtime_error("Please fill values!");
    for (size_t i = 0; i < columns.size(); ++i) {
        row[columns[i].to_json()["name"]] = values[i];
    }
    return row;
}


TEST_F(TableFixture, TestInsertElements) {
    const std::vector<Value> values{1, "vova", 1};
    const std::vector<std::string> column_names{"id", "name", "count_cats"};

    table.insert_elements(column_names, values);
}

TEST_F(TableFixture, TestSelectAllElements) {
    std::vector<Value> values{1, "vova", 1};
    std::vector<std::string> column_names{"id", "name", "count_cats"};

    table.insert_elements(column_names, values);

    // Выбираем все колонки без алиасов
    auto result = table.select_elements(std::nullopt, nullptr);

    ASSERT_EQ(result.size(), 1);

    std::vector expected_result{create_row_from_values(columns, values)};

    ASSERT_EQ(expected_result, result);
}

TEST_F(TableFixture, TestInsertNullElements) {
    const std::vector<Value> values_1{1, "vova", 1};
    const std::vector<Value> values_2{2, "mr_palkin", Null{}};
    std::vector<Value> values_3{3, "artem"};
    const std::vector<Value> values_4{4, "matvey", 0};

    const std::vector<std::string> column_names{"id", "name", "count_cats"};
    const std::vector<std::string> column_names_without_cats{"id", "name"};

    table.insert_elements(column_names, values_1);
    table.insert_elements(column_names, values_2);
    table.insert_elements(column_names_without_cats, values_3);
    table.insert_elements(column_names, values_4);

    auto result = table.select_elements(std::nullopt, nullptr);

    ASSERT_EQ(result.size(), 4);

    values_3.emplace_back(Null{}); /* заполнили недостающий элемент */
    const std::vector<Row> expected_result{
        create_row_from_values(columns, values_1),
        create_row_from_values(columns, values_2),
        create_row_from_values(columns, values_3),
        create_row_from_values(columns, values_4),
    };

    ASSERT_EQ(result[0], expected_result[0]);
    ASSERT_EQ(result[1], expected_result[1]);
    ASSERT_EQ(result[2], expected_result[2]);
    ASSERT_EQ(result[3], expected_result[3]);
}

TEST_F(TableFixture, TestSelectWithConditionEqual) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 2);
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(std::get<int>(result[0]["id"]), 2);
    ASSERT_EQ(std::get<std::string>(result[0]["name"]), "mr_palkin");
}

TEST_F(TableFixture, TestSelectWithConditionNotEqual) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::NotEqual, 2);
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(std::get<int>(result[0]["id"]), 1);
    ASSERT_EQ(std::get<int>(result[1]["id"]), 3);
}

TEST_F(TableFixture, TestSelectWithConditionGreater) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});
    table.insert_elements(column_names, {4, "matvey", 4});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Greater, 2);
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(std::get<int>(result[0]["id"]), 3);
    ASSERT_EQ(std::get<int>(result[1]["id"]), 4);
}

TEST_F(TableFixture, TestSelectWithConditionGreaterEqual) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::GreaterEqual, 2);
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(std::get<int>(result[0]["id"]), 2);
    ASSERT_EQ(std::get<int>(result[1]["id"]), 3);
}

TEST_F(TableFixture, TestSelectWithConditionLess) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    auto condition = std::make_unique<ComparisonCondition>("count_cats", ComparisonDataType::Less, 3);
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 2);
}

TEST_F(TableFixture, TestSelectWithConditionLessEqual) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    auto condition = std::make_unique<ComparisonCondition>("count_cats", ComparisonDataType::LessEqual, 2);
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 2);
}

TEST_F(TableFixture, TestSelectWithBetweenCondition) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});
    table.insert_elements(column_names, {4, "matvey", 4});
    table.insert_elements(column_names, {5, "ivan", 5});

    auto condition = std::make_unique<BetweenCondition>("id", 2, 4);
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(std::get<int>(result[0]["id"]), 2);
    ASSERT_EQ(std::get<int>(result[1]["id"]), 3);
    ASSERT_EQ(std::get<int>(result[2]["id"]), 4);
}

TEST_F(TableFixture, TestSelectSpecificColumns) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});

    // Выбираем только id и name без алиасов
    std::unordered_map<std::string, Alias> columns_map{
        {"id", std::nullopt},
        {"name", std::nullopt}
    };
    auto result = table.select_elements(columns_map, nullptr);

    ASSERT_EQ(result.size(), 2);
    ASSERT_TRUE(result[0].contains("id"));
    ASSERT_TRUE(result[0].contains("name"));
    ASSERT_FALSE(result[0].contains("count_cats"));
}

TEST_F(TableFixture, TestSelectWithAliasesForAllColumns) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});

    // Задаем алиасы для всех колонок
    std::unordered_map<std::string, Alias> columns_map{
        {"id", "user_id"},
        {"name", "user_name"},
        {"count_cats", "cats"}
    };
    auto result = table.select_elements(columns_map, nullptr);

    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].contains("user_id"));
    ASSERT_TRUE(result[0].contains("user_name"));
    ASSERT_TRUE(result[0].contains("cats"));
    ASSERT_EQ(std::get<int>(result[0]["user_id"]), 1);
    ASSERT_EQ(std::get<std::string>(result[0]["user_name"]), "vova");
}

TEST_F(TableFixture, TestSelectWithPartialAliases) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "artem", 2});

    // Алиас только для id, остальные без алиасов
    std::unordered_map<std::string, Alias> columns_map{
        {"id", "user_id"},
        {"name", std::nullopt},
        {"count_cats", std::nullopt}
    };
    auto result = table.select_elements(columns_map, nullptr);

    ASSERT_EQ(result.size(), 2);
    ASSERT_TRUE(result[0].contains("user_id"));
    ASSERT_TRUE(result[0].contains("name"));
    ASSERT_TRUE(result[0].contains("count_cats"));
    ASSERT_FALSE(result[0].contains("id"));
}

TEST_F(TableFixture, TestSelectWithAliasesAndCondition) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    std::unordered_map<std::string, Alias> columns_map{
        {"id", "user_id"},
        {"name", "user_name"}
    };
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Greater, 1);
    auto result = table.select_elements(columns_map, std::move(condition));

    ASSERT_EQ(result.size(), 2);
    ASSERT_TRUE(result[0].contains("user_id"));
    ASSERT_TRUE(result[0].contains("user_name"));
    ASSERT_EQ(std::get<int>(result[0]["user_id"]), 2);
}

TEST_F(TableFixture, TestUpdateElements) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 2);
    std::vector<std::string> update_columns{"name", "count_cats"};
    std::vector<Value> update_values{"updated_name", 10};
    
    table.update_elements(std::move(condition), update_columns, update_values);

    auto result = table.select_elements(std::nullopt, nullptr);
    
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(std::get<std::string>(result[1]["name"]), "updated_name");
    ASSERT_EQ(std::get<int>(result[1]["count_cats"]), 10);
}

TEST_F(TableFixture, TestUpdateMultipleElements) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});
    table.insert_elements(column_names, {4, "matvey", 4});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Greater, 2);
    std::vector<std::string> update_columns{"count_cats"};
    std::vector<Value> update_values{100};
    
    table.update_elements(std::move(condition), update_columns, update_values);

    auto result = table.select_elements(std::nullopt, nullptr);
    
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(std::get<int>(result[2]["count_cats"]), 100);
    ASSERT_EQ(std::get<int>(result[3]["count_cats"]), 100);
}

TEST_F(TableFixture, TestUpdateWithNull) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 1);
    std::vector<std::string> update_columns{"count_cats"};
    std::vector<Value> update_values{Null{}};
    
    table.update_elements(std::move(condition), update_columns, update_values);

    auto result = table.select_elements(std::nullopt, nullptr);
    
    ASSERT_TRUE(std::holds_alternative<Null>(result[0]["count_cats"]));
}

TEST_F(TableFixture, TestDeleteElements) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 2);
    table.delete_elements(std::move(condition));

    auto result = table.select_elements(std::nullopt, nullptr);
    
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(std::get<int>(result[0]["id"]), 1);
    ASSERT_EQ(std::get<int>(result[1]["id"]), 3);
}

TEST_F(TableFixture, TestDeleteMultipleElements) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});
    table.insert_elements(column_names, {4, "matvey", 4});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Greater, 2);
    table.delete_elements(std::move(condition));

    auto result = table.select_elements(std::nullopt, nullptr);
    
    ASSERT_EQ(result.size(), 2);
}

TEST_F(TableFixture, TestDeleteAllElements) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});

    // Удаляем все элементы
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Greater, 0);
    table.delete_elements(std::move(condition));

    const auto result = table.select_elements(std::nullopt, nullptr);
    
    ASSERT_EQ(result.size(), 0);
}

TEST_F(TableFixture, TestGetTableName) {
    std::string name = table.get_name();
    ASSERT_EQ(name, "test_table");
}

TEST_F(TableFixture, TestGetColumns) {
    auto cols = table.get_columns();
    ASSERT_EQ(cols.size(), 3);
}

TEST_F(TableFixture, TestGetCountPages) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    
    ptrdiff_t count = table.get_count_pages();
    ASSERT_GT(count, 0);
}

TEST_F(TableFixture, TestMultipleInserts) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    for (int i = 1; i <= 100; ++i) {
        table.insert_elements(column_names, {i, "user_" + std::to_string(i), i % 10});
    }

    auto result = table.select_elements(std::nullopt, nullptr);
    ASSERT_EQ(result.size(), 100);
}

TEST_F(TableFixture, TestRegexCondition) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    table.insert_elements(column_names, {3, "artem", 3});
    table.insert_elements(column_names, {4, "vladimir", 4});

    auto condition = std::make_unique<RegexCondition>("name", "v.*");
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_GE(result.size(), 1);
}

TEST_F(TableFixture, TestLoadExistingTable) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});

    // Загружаем существующую таблицу
    Table loaded_table = Table::load_table(std::filesystem::current_path(), "test_table");
    
    auto result = loaded_table.select_elements(std::nullopt, nullptr);
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(std::get<int>(result[0]["id"]), 1);
    ASSERT_EQ(std::get<int>(result[1]["id"]), 2);
}

TEST_F(TableFixture, TestInsertAfterDelete) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});
    
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 1);
    table.delete_elements(std::move(condition));
    
    table.insert_elements(column_names, {3, "artem", 3});
    
    auto result = table.select_elements(std::nullopt, nullptr);
    ASSERT_EQ(result.size(), 2);
}

TEST_F(TableFixture, TestComplexConditionWithStrings) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "alice", 1});
    table.insert_elements(column_names, {2, "bob", 2});
    table.insert_elements(column_names, {3, "charlie", 3});

    auto condition = std::make_unique<ComparisonCondition>("name", ComparisonDataType::Equal, "bob");
    auto result = table.select_elements(std::nullopt, std::move(condition));

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(std::get<std::string>(result[0]["name"]), "bob");
}

TEST_F(TableFixture, TestSelectEmptyTable) {
    auto result = table.select_elements(std::nullopt, nullptr);
    ASSERT_EQ(result.size(), 0);
}

TEST_F(TableFixture, TestUpdateNonExistentCondition) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 999);
    std::vector<std::string> update_columns{"name"};
    std::vector<Value> update_values{"should_not_update"};
    
    table.update_elements(std::move(condition), update_columns, update_values);

    auto result = table.select_elements(std::nullopt, nullptr);
    
    // Ничего не должно измениться
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(std::get<std::string>(result[0]["name"]), "vova");
    ASSERT_EQ(std::get<std::string>(result[1]["name"]), "mr_palkin");
}

TEST_F(TableFixture, TestDeleteNonExistentCondition) {
    std::vector<std::string> column_names{"id", "name", "count_cats"};
    
    table.insert_elements(column_names, {1, "vova", 1});
    table.insert_elements(column_names, {2, "mr_palkin", 2});

    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 999);
    table.delete_elements(std::move(condition));

    auto result = table.select_elements(std::nullopt, nullptr);
    
    // Ничего не должно удалиться
    ASSERT_EQ(result.size(), 2);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}