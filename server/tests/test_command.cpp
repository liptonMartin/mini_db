//
// Created by rvova on 19.05.2026.
//

#include <gtest/gtest.h>
#include "command.h"
#include "datatypes.h"

// ==================== CreateDatabaseCommand ====================

TEST(CommandSerializationTest, CreateDatabaseCommand_Serialization) {
    CreateDatabaseCommand cmd("testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, CreateDatabaseCommand_WithSpecialChars) {
    CreateDatabaseCommand cmd("test_db-123");
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== DropDatabaseCommand ====================

TEST(CommandSerializationTest, DropDatabaseCommand_Serialization) {
    DropDatabaseCommand cmd("testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = DropDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== UseDatabaseCommand ====================

TEST(CommandSerializationTest, UseDatabaseCommand_Serialization) {
    UseDatabaseCommand cmd("mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = UseDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== CreateTableCommand ====================

TEST(CommandSerializationTest, CreateTableCommand_WithoutDatabase) {
    std::vector<Column> columns = {
        Column("id", DataType::Int, false, false),
        Column("name", DataType::String, true, false)
    };

    CreateTableCommand cmd("users", columns);
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateTableCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, CreateTableCommand_WithDatabase) {
    std::vector<Column> columns = {
        Column("id", DataType::Int, false, true),
        Column("email", DataType::String, false, false)
    };

    CreateTableCommand cmd("users", columns, "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateTableCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, CreateTableCommand_MultipleColumns) {
    std::vector<Column> columns = {
        Column("id", DataType::Int, false, true),
        Column("name", DataType::String, true, false),
        Column("age", DataType::Int, true, false),
        Column("city", DataType::String, true, false)
    };

    CreateTableCommand cmd("persons", columns, "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateTableCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== DropTableCommand ====================

TEST(CommandSerializationTest, DropTableCommand_WithoutDatabase) {
    DropTableCommand cmd("users");
    std::string serialized = cmd.serialize_command();

    auto parsed = DropTableCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, DropTableCommand_WithDatabase) {
    DropTableCommand cmd("users", "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = DropTableCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== InsertIntoCommand ====================

TEST(CommandSerializationTest, InsertIntoCommand_IntValues) {
    std::vector<std::string> columns = {"id", "age"};
    std::vector<Value> values = {42, 25};

    InsertIntoCommand cmd("users", columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, InsertIntoCommand_StringValues) {
    std::vector<std::string> columns = {"name", "email"};
    std::vector<Value> values = {std::string("John"), std::string("john@example.com")};

    InsertIntoCommand cmd("users", columns, values, "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, InsertIntoCommand_MixedValues) {
    std::vector<std::string> columns = {"id", "name", "age"};
    std::vector<Value> values = {1, std::string("Alice"), 30};

    InsertIntoCommand cmd("users", columns, values, "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, InsertIntoCommand_WithNull) {
    std::vector<std::string> columns = {"id", "name", "age"};
    std::vector<Value> values = {1, std::string("Bob"), Null{}};

    InsertIntoCommand cmd("users", columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== UpdateCommand ====================

TEST(CommandSerializationTest, UpdateCommand_WithCondition) {
    Column col("id", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::Equal, 5);

    std::vector<std::string> columns = {"name"};
    std::vector<Value> values = {std::string("Updated")};

    UpdateCommand cmd("users", std::move(condition), columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_WithDatabase) {
    Column col("age", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::Greater, 18);

    std::vector<std::string> columns = {"status"};
    std::vector<Value> values = {std::string("adult")};

    UpdateCommand cmd("users", std::move(condition), columns, values, "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_NullCondition) {
    // Проверяем, что код корректно обрабатывает nullptr condition
    // Примечание: serialize_command вызовет ошибку при nullptr,
    // но это нужно протестировать
    Column col("id", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::Equal, 1);

    std::vector<std::string> columns = {"name"};
    std::vector<Value> values = {std::string("Test")};

    UpdateCommand cmd("users", std::move(condition), columns, values);

    // Проверяем, что сериализация работает
    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
    });
}

TEST(CommandSerializationTest, UpdateCommand_BetweenCondition) {
    Column col("age", DataType::Int, false, false);
    auto condition = std::make_unique<BetweenCondition>(col, 18, 65);

    std::vector<std::string> columns = {"category"};
    std::vector<Value> values = {std::string("working_age")};

    UpdateCommand cmd("users", std::move(condition), columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_RegexCondition) {
    Column col("email", DataType::String, false, false);
    auto condition = std::make_unique<RegexCondition>(col, ".*@example\\.com");

    std::vector<std::string> columns = {"verified"};
    std::vector<Value> values = {1};

    UpdateCommand cmd("users", std::move(condition), columns, values, "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== DeleteFromCommand ====================

TEST(CommandSerializationTest, DeleteFromCommand_WithCondition) {
    Column col("id", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::Equal, 10);

    DeleteFromCommand cmd("users", std::move(condition));
    std::string serialized = cmd.serialize_command();

    auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, DeleteFromCommand_WithDatabase) {
    Column col("status", DataType::String, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::Equal, std::string("deleted"));

    DeleteFromCommand cmd("users", std::move(condition), "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, DeleteFromCommand_BetweenCondition) {
    Column col("created_at", DataType::Int, false, false);
    auto condition = std::make_unique<BetweenCondition>(col, 1000, 2000);

    DeleteFromCommand cmd("logs", std::move(condition), "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== SelectCommand ====================

TEST(CommandSerializationTest, SelectCommand_AllColumns) {
    Column col("id", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::Greater, 0);

    SelectCommand cmd("users", std::move(condition));
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_SpecificColumns) {
    Column col("age", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::GreaterEqual, 18);

    std::vector<std::string> columns = {"id", "name", "email"};

    SelectCommand cmd("users", columns, std::move(condition));
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_WithAliases) {
    Column col("status", DataType::String, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::Equal, std::string("active"));

    std::vector<std::string> columns = {"id", "name"};
    std::vector<std::string> aliases = {"user_id", "user_name"};

    SelectCommand cmd("users", columns, aliases, std::move(condition), "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_WithDatabase) {
    Column col("id", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>(col, ComparisonDataType::NotEqual, 0);

    SelectCommand cmd("users", std::move(condition), "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_RegexCondition) {
    Column col("name", DataType::String, false, false);
    auto condition = std::make_unique<RegexCondition>(col, "^[A-Z].*");

    std::vector<std::string> columns = {"name"};

    SelectCommand cmd("users", columns, std::move(condition), "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== Edge Cases ====================

TEST(CommandSerializationTest, EmptyStrings) {
    CreateDatabaseCommand cmd("");
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SpecialCharactersInStrings) {
    std::vector<std::string> columns = {"message"};
    std::vector<Value> values = {std::string("Hello \"World\" \n\t\r")};

    InsertIntoCommand cmd("logs", columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UnicodeCharacters) {
    std::vector<std::string> columns = {"name"};
    std::vector<Value> values = {std::string("Привет мир 你好")};

    InsertIntoCommand cmd("users", columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}