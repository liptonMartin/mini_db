//
// Created by rvova on 19.05.2026.
//

#include <gtest/gtest.h>
#include "command.h"
#include "datatypes.h"

// ==================== CreateDatabaseCommand ====================

TEST(CommandSerializationTest, CreateDatabaseCommand_Basic) {
    CreateDatabaseCommand cmd("testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, CreateDatabaseCommand_SpecialChars) {
    CreateDatabaseCommand cmd("test_db-123");
    std::string serialized = cmd.serialize_command();

    auto parsed = CreateDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== DropDatabaseCommand ====================

TEST(CommandSerializationTest, DropDatabaseCommand_Basic) {
    DropDatabaseCommand cmd("testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = DropDatabaseCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== UseDatabaseCommand ====================

TEST(CommandSerializationTest, UseDatabaseCommand_Basic) {
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

TEST(CommandSerializationTest, InsertIntoCommand_AllNulls) {
    std::vector<std::string> columns = {"col1", "col2", "col3"};
    std::vector<Value> values = {Null{}, Null{}, Null{}};

    InsertIntoCommand cmd("test_table", columns, values, "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== UpdateCommand ====================

TEST(CommandSerializationTest, UpdateCommand_WithComparisonCondition) {
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 5);

    std::vector<std::string> columns = {"name"};
    std::vector<Value> values = {std::string("Updated")};

    UpdateCommand cmd("users", std::move(condition), columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_WithDatabase) {
    auto condition = std::make_unique<ComparisonCondition>("age", ComparisonDataType::Greater, 18);

    std::vector<std::string> columns = {"status"};
    std::vector<Value> values = {std::string("adult")};

    UpdateCommand cmd("users", std::move(condition), columns, values, "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_BetweenCondition) {
    auto condition = std::make_unique<BetweenCondition>("age", 18, 65);

    std::vector<std::string> columns = {"category"};
    std::vector<Value> values = {std::string("working_age")};

    UpdateCommand cmd("users", std::move(condition), columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_RegexCondition) {
    auto condition = std::make_unique<RegexCondition>("email", ".*@example\\.com");

    std::vector<std::string> columns = {"verified"};
    std::vector<Value> values = {1};

    UpdateCommand cmd("users", std::move(condition), columns, values, "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_MultipleColumns) {
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 1);

    std::vector<std::string> columns = {"name", "age", "city"};
    std::vector<Value> values = {std::string("John"), 25, std::string("NYC")};

    UpdateCommand cmd("users", std::move(condition), columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, UpdateCommand_WithNullValue) {
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 10);

    std::vector<std::string> columns = {"optional_field"};
    std::vector<Value> values = {Null{}};

    UpdateCommand cmd("users", std::move(condition), columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = UpdateCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== DeleteFromCommand ====================

TEST(CommandSerializationTest, DeleteFromCommand_WithComparisonCondition) {
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 10);

    DeleteFromCommand cmd("users", std::move(condition));
    std::string serialized = cmd.serialize_command();

    auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, DeleteFromCommand_WithDatabase) {
    auto condition = std::make_unique<ComparisonCondition>("status", ComparisonDataType::Equal, std::string("deleted"));

    DeleteFromCommand cmd("users", std::move(condition), "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, DeleteFromCommand_BetweenCondition) {
    auto condition = std::make_unique<BetweenCondition>("created_at", 1000, 2000);

    DeleteFromCommand cmd("logs", std::move(condition), "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, DeleteFromCommand_RegexCondition) {
    Column col("email", DataType::String, false, false);
    auto condition = std::make_unique<RegexCondition>("email", ".*@spam\\.com");

    DeleteFromCommand cmd("users", std::move(condition));
    std::string serialized = cmd.serialize_command();

    auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== DeleteFromCommand с nullptr condition ====================

TEST(CommandSerializationTest, DeleteFromCommand_NullCondition) {
    // Тест для случая когда condition = nullptr
    DeleteFromCommand cmd("users", nullptr);

    // Проверяем что сериализация не падает
    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    });
}

TEST(CommandSerializationTest, DeleteFromCommand_NullConditionWithDatabase) {
    DeleteFromCommand cmd("logs", nullptr, "testdb");

    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
    });
}

// ==================== SelectCommand ====================

TEST(CommandSerializationTest, SelectCommand_AllColumnsNoCondition) {
    SelectCommand cmd("users");
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_WithConditionNoColumns) {
    Column col("id", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Greater, 0);

    SelectCommand cmd("users", std::move(condition));
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_WithColumnsNoAliases) {
    Column col("age", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>("age", ComparisonDataType::GreaterEqual, 18);

    std::vector<SelectTarget> targets = {
        {"id", std::nullopt, std::nullopt},
        {"name", std::nullopt, std::nullopt},
        {"email", std::nullopt, std::nullopt}
    };

    SelectCommand cmd("users", std::move(condition), targets);
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_WithAliases) {
    Column col("status", DataType::String, false, false);
    auto condition = std::make_unique<ComparisonCondition>("status", ComparisonDataType::Equal, std::string("active"));

    std::vector<SelectTarget> targets = {
        {"id", std::string("user_id"), std::nullopt},
        {"name", std::string("user_name"), std::nullopt},
        {"email", std::nullopt, std::nullopt}
    };

    SelectCommand cmd("users", std::move(condition), targets, "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_WithDatabase) {
    Column col("id", DataType::Int, false, false);
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::NotEqual, 0);

    SelectCommand cmd("users", std::move(condition), std::vector<SelectTarget>{}, "mydb");
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_RegexCondition) {
    Column col("name", DataType::String, false, false);
    auto condition = std::make_unique<RegexCondition>("name", "^[A-Z].*");

    std::vector<SelectTarget> targets = {
        {"name", std::nullopt, std::nullopt}
    };

    SelectCommand cmd("users", std::move(condition), targets, "testdb");
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, SelectCommand_BetweenCondition) {
    Column col("age", DataType::Int, false, false);
    auto condition = std::make_unique<BetweenCondition>("age", 18, 65);

    SelectCommand cmd("users", std::move(condition));
    std::string serialized = cmd.serialize_command();

    auto parsed = SelectCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

// ==================== SelectCommand с nullptr condition ====================

TEST(CommandSerializationTest, SelectCommand_NullCondition) {
    SelectCommand cmd("users", nullptr);

    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
        auto parsed = SelectCommand::parse_from_bytes(serialized);
    });
}

TEST(CommandSerializationTest, SelectCommand_NullConditionWithColumns) {
    std::vector<SelectTarget> targets = {
        {"id", std::nullopt, std::nullopt},
        {"name", std::string("username"), std::nullopt}
    };

    SelectCommand cmd("users", nullptr, targets);

    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
        auto parsed = SelectCommand::parse_from_bytes(serialized);
    });
}

TEST(CommandSerializationTest, SelectCommand_NullConditionWithDatabase) {
    SelectCommand cmd("users", nullptr, std::vector<SelectTarget>{}, "testdb");

    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
        auto parsed = SelectCommand::parse_from_bytes(serialized);
    });
}

// ==================== UpdateCommand с nullptr condition ====================

TEST(CommandSerializationTest, UpdateCommand_NullCondition) {
    std::vector<std::string> columns = {"status"};
    std::vector<Value> values = {std::string("updated")};

    UpdateCommand cmd("users", nullptr, columns, values);

    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
        auto parsed = UpdateCommand::parse_from_bytes(serialized);
    });
}

TEST(CommandSerializationTest, UpdateCommand_NullConditionWithDatabase) {
    std::vector<std::string> columns = {"counter"};
    std::vector<Value> values = {0};

    UpdateCommand cmd("stats", nullptr, columns, values, "testdb");

    EXPECT_NO_THROW({
        std::string serialized = cmd.serialize_command();
        auto parsed = UpdateCommand::parse_from_bytes(serialized);
    });
}

// ==================== Edge Cases ====================

TEST(CommandSerializationTest, EmptyDatabaseName) {
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

TEST(CommandSerializationTest, LongStrings) {
    std::string long_string(10000, 'x');
    std::vector<std::string> columns = {"data"};
    std::vector<Value> values = {long_string};

    InsertIntoCommand cmd("test", columns, values);
    std::string serialized = cmd.serialize_command();

    auto parsed = InsertIntoCommand::parse_from_bytes(serialized);
    std::string reserialized = parsed.serialize_command();

    EXPECT_EQ(serialized, reserialized);
}

TEST(CommandSerializationTest, ComparisonCondition_AllOperators) {
    Column col("value", DataType::Int, false, false);

    // Equal
    {
        auto condition = std::make_unique<ComparisonCondition>("value", ComparisonDataType::Equal, 5);
        DeleteFromCommand cmd("test", std::move(condition));
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
        EXPECT_NO_THROW(parsed.serialize_command());
    }

    // NotEqual
    {
        auto condition = std::make_unique<ComparisonCondition>("value", ComparisonDataType::NotEqual, 5);
        DeleteFromCommand cmd("test", std::move(condition));
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
        EXPECT_NO_THROW(parsed.serialize_command());
    }

    // Greater
    {
        auto condition = std::make_unique<ComparisonCondition>("value", ComparisonDataType::Greater, 5);
        DeleteFromCommand cmd("test", std::move(condition));
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
        EXPECT_NO_THROW(parsed.serialize_command());
    }

    // GreaterEqual
    {
        auto condition = std::make_unique<ComparisonCondition>("value", ComparisonDataType::GreaterEqual, 5);
        DeleteFromCommand cmd("test", std::move(condition));
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
        EXPECT_NO_THROW(parsed.serialize_command());
    }

    // Less
    {
        auto condition = std::make_unique<ComparisonCondition>("value", ComparisonDataType::Less, 5);
        DeleteFromCommand cmd("test", std::move(condition));
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
        EXPECT_NO_THROW(parsed.serialize_command());
    }

    // LessEqual
    {
        auto condition = std::make_unique<ComparisonCondition>("value", ComparisonDataType::LessEqual, 5);
        DeleteFromCommand cmd("test", std::move(condition));
        std::string serialized = cmd.serialize_command();
        auto parsed = DeleteFromCommand::parse_from_bytes(serialized);
        EXPECT_NO_THROW(parsed.serialize_command());
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}