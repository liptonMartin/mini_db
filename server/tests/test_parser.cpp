#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "command.h"

static std::unique_ptr<Command> parse_sql(const std::string& sql) {
    Lexer lexer;
    auto tokens = lexer.tokenize(sql);

    Parser parser;
    return parser.parse(tokens);
}

// ==================== CREATE DATABASE ====================

TEST(ParserTest, CreateDatabaseSimple) {
    auto cmd = parse_sql("CREATE DATABASE my_db;");
    ASSERT_NE(cmd, nullptr);
    auto* create_cmd = dynamic_cast<CreateDatabaseCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);

    auto json = create_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["database_name"], "my_db");
}

TEST(ParserTest, CreateDatabaseCaseInsensitive) {
    auto cmd = parse_sql("CrEaTe DaTaBaSe casedb;");
    ASSERT_NE(cmd, nullptr);
    auto* create_cmd = dynamic_cast<CreateDatabaseCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);

    auto json = create_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["database_name"], "casedb");
}

TEST(ParserTest, CreateDatabaseWithToken) {
    auto cmd = parse_sql("CREATE DATABASE securedb \"eyJhbGciOiJIUzI1NiJ9.abc.xyz\";");
    ASSERT_NE(cmd, nullptr);
    auto* create_cmd = dynamic_cast<CreateDatabaseCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);

    EXPECT_EQ(create_cmd->get_token(), "eyJhbGciOiJIUzI1NiJ9.abc.xyz");
    EXPECT_FALSE(create_cmd->get_token().empty());
}

// ==================== DROP DATABASE ====================

TEST(ParserTest, DropDatabase) {
    auto cmd = parse_sql("DROP DATABASE old_db;");
    ASSERT_NE(cmd, nullptr);
    auto* drop_cmd = dynamic_cast<DropDatabaseCommand*>(cmd.get());
    ASSERT_NE(drop_cmd, nullptr);

    auto json = drop_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["database_name"], "old_db");
}

TEST(ParserTest, DropDatabaseWithToken) {
    auto cmd = parse_sql("DROP DATABASE old_db \"token123\";");
    ASSERT_NE(cmd, nullptr);
    auto* drop_cmd = dynamic_cast<DropDatabaseCommand*>(cmd.get());
    ASSERT_NE(drop_cmd, nullptr);

    EXPECT_EQ(drop_cmd->get_token(), "token123");
}

// ==================== USE DATABASE ====================

TEST(ParserTest, UseDatabase) {
    auto cmd = parse_sql("USE active_db;");
    ASSERT_NE(cmd, nullptr);
    auto* use_cmd = dynamic_cast<UseDatabaseCommand*>(cmd.get());
    ASSERT_NE(use_cmd, nullptr);

    auto json = use_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["database_name"], "active_db");
}

TEST(ParserTest, UseDatabaseWithToken) {
    auto cmd = parse_sql("USE active_db \"jwt_use\";");
    ASSERT_NE(cmd, nullptr);
    auto* use_cmd = dynamic_cast<UseDatabaseCommand*>(cmd.get());
    ASSERT_NE(use_cmd, nullptr);

    EXPECT_EQ(use_cmd->get_token(), "jwt_use");
}

// ==================== CREATE TABLE ====================

TEST(ParserTest, CreateTableSimple) {
    auto cmd = parse_sql("CREATE TABLE users (id int, name string);");
    ASSERT_NE(cmd, nullptr);
    auto* create_cmd = dynamic_cast<CreateTableCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);

    auto json = create_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "users");
    EXPECT_EQ(parsed["columns"].size(), 2);
}

TEST(ParserTest, CreateTableWithConstraints) {
    auto cmd = parse_sql("CREATE TABLE products (id int INDEXED, name string NOT NULL, price int);");
    ASSERT_NE(cmd, nullptr);
    auto* create_cmd = dynamic_cast<CreateTableCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);

    auto json = create_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "products");
    EXPECT_EQ(parsed["columns"].size(), 3);
}

TEST(ParserTest, CreateTableWithToken) {
    auto cmd = parse_sql("CREATE TABLE t (x int) \"table_token\";");
    ASSERT_NE(cmd, nullptr);
    auto* create_cmd = dynamic_cast<CreateTableCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);

    EXPECT_EQ(create_cmd->get_token(), "table_token");
}

// ==================== DROP TABLE ====================

TEST(ParserTest, DropTable) {
    auto cmd = parse_sql("DROP TABLE old_table;");
    ASSERT_NE(cmd, nullptr);
    auto* drop_cmd = dynamic_cast<DropTableCommand*>(cmd.get());
    ASSERT_NE(drop_cmd, nullptr);

    auto json = drop_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "old_table");
}

TEST(ParserTest, DropTableWithToken) {
    auto cmd = parse_sql("DROP TABLE t \"drop_token\";");
    ASSERT_NE(cmd, nullptr);
    auto* drop_cmd = dynamic_cast<DropTableCommand*>(cmd.get());
    ASSERT_NE(drop_cmd, nullptr);

    EXPECT_EQ(drop_cmd->get_token(), "drop_token");
}

// ==================== INSERT INTO ====================

TEST(ParserTest, InsertIntoString) {
    auto cmd = parse_sql("INSERT INTO users (name, email) VALUE (\"Alice\", \"alice@mail.com\");");
    ASSERT_NE(cmd, nullptr);
    auto* insert_cmd = dynamic_cast<InsertIntoCommand*>(cmd.get());
    ASSERT_NE(insert_cmd, nullptr);

    auto json = insert_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "users");
    EXPECT_EQ(parsed["columns"].size(), 2);
    EXPECT_EQ(parsed["values"].size(), 2);
}

TEST(ParserTest, InsertIntoNumbers) {
    auto cmd = parse_sql("INSERT INTO products (id, price) VALUE (1, 999);");
    ASSERT_NE(cmd, nullptr);
    auto* insert_cmd = dynamic_cast<InsertIntoCommand*>(cmd.get());
    ASSERT_NE(insert_cmd, nullptr);

    auto json = insert_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "products");
    EXPECT_EQ(parsed["values"].size(), 2);
}

TEST(ParserTest, InsertIntoNull) {
    auto cmd = parse_sql("INSERT INTO users (name, phone) VALUE (\"Bob\", NULL);");
    ASSERT_NE(cmd, nullptr);
    auto* insert_cmd = dynamic_cast<InsertIntoCommand*>(cmd.get());
    ASSERT_NE(insert_cmd, nullptr);
}

TEST(ParserTest, InsertIntoWithToken) {
    auto cmd = parse_sql("INSERT INTO t (a) VALUE (1) \"insert_token\";");
    ASSERT_NE(cmd, nullptr);
    auto* insert_cmd = dynamic_cast<InsertIntoCommand*>(cmd.get());
    ASSERT_NE(insert_cmd, nullptr);

    EXPECT_EQ(insert_cmd->get_token(), "insert_token");
}

// ==================== UPDATE ====================

TEST(ParserTest, UpdateSimple) {
    auto cmd = parse_sql("UPDATE users SET name = \"Charlie\" WHERE id == 1;");
    ASSERT_NE(cmd, nullptr);
    auto* update_cmd = dynamic_cast<UpdateCommand*>(cmd.get());
    ASSERT_NE(update_cmd, nullptr);

    auto json = update_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "users");
    EXPECT_TRUE(parsed.contains("condition"));
}

TEST(ParserTest, UpdateMultipleSets) {
    auto cmd = parse_sql("UPDATE products SET price = 100, name = \"NewName\" WHERE id == 5;");
    ASSERT_NE(cmd, nullptr);
    auto* update_cmd = dynamic_cast<UpdateCommand*>(cmd.get());
    ASSERT_NE(update_cmd, nullptr);

    EXPECT_TRUE(update_cmd->get_token().empty());
}

TEST(ParserTest, UpdateWithToken) {
    auto cmd = parse_sql("UPDATE users SET name = \"Bob\" WHERE id == 1 \"update_token\";");
    ASSERT_NE(cmd, nullptr);
    auto* update_cmd = dynamic_cast<UpdateCommand*>(cmd.get());
    ASSERT_NE(update_cmd, nullptr);

    EXPECT_EQ(update_cmd->get_token(), "update_token");
}

// ==================== DELETE FROM ====================

TEST(ParserTest, DeleteSimple) {
    auto cmd = parse_sql("DELETE FROM users WHERE id == 10;");
    ASSERT_NE(cmd, nullptr);
    auto* delete_cmd = dynamic_cast<DeleteFromCommand*>(cmd.get());
    ASSERT_NE(delete_cmd, nullptr);

    auto json = delete_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "users");
    EXPECT_TRUE(parsed.contains("condition"));
}

TEST(ParserTest, DeleteWithToken) {
    auto cmd = parse_sql("DELETE FROM users WHERE id == 1 \"delete_token\";");
    ASSERT_NE(cmd, nullptr);
    auto* delete_cmd = dynamic_cast<DeleteFromCommand*>(cmd.get());
    ASSERT_NE(delete_cmd, nullptr);

    EXPECT_EQ(delete_cmd->get_token(), "delete_token");
}

// ==================== SELECT ====================

TEST(ParserTest, SelectStar) {
    auto cmd = parse_sql("SELECT * FROM users;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);

    auto json = select_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["table_name"], "users");
}

TEST(ParserTest, SelectColumns) {
    auto cmd = parse_sql("SELECT name, email FROM users;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);

    auto json = select_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_TRUE(parsed.contains("select_targets"));
    EXPECT_EQ(parsed["select_targets"].size(), 2);
    EXPECT_EQ(parsed["select_targets"][0]["type"], "column");
    EXPECT_EQ(parsed["select_targets"][0]["column_name"], "name");
    EXPECT_EQ(parsed["select_targets"][1]["type"], "column");
    EXPECT_EQ(parsed["select_targets"][1]["column_name"], "email");
}

TEST(ParserTest, SelectWithCondition) {
    auto cmd = parse_sql("SELECT * FROM users WHERE age >= 18;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);

    auto json = select_cmd->serialize_command();
    auto parsed = nlohmann::json::parse(json);
    EXPECT_TRUE(parsed.contains("condition"));
}

TEST(ParserTest, SelectWithAlias) {
    auto cmd = parse_sql("SELECT name AS n FROM users;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);
}

TEST(ParserTest, SelectWithToken) {
    auto cmd = parse_sql("SELECT * FROM t \"select_token\";");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);

    EXPECT_EQ(select_cmd->get_token(), "select_token");
}

TEST(ParserTest, SelectWithWhereAndToken) {
    auto cmd = parse_sql("SELECT * FROM t WHERE id > 5 \"select_where_token\";");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);

    EXPECT_EQ(select_cmd->get_token(), "select_where_token");
}

// ==================== Conditions (comparison operators) ====================

TEST(ParserTest, ConditionEqual) {
    auto cmd = parse_sql("SELECT * FROM t WHERE id == 1;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ConditionNotEqual) {
    auto cmd = parse_sql("SELECT * FROM t WHERE id != 1;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ConditionLess) {
    auto cmd = parse_sql("SELECT * FROM t WHERE id < 10;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ConditionGreater) {
    auto cmd = parse_sql("SELECT * FROM t WHERE id > 5;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ConditionLessEqual) {
    auto cmd = parse_sql("SELECT * FROM t WHERE id <= 100;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ConditionGreaterEqual) {
    auto cmd = parse_sql("SELECT * FROM t WHERE id >= 18;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ConditionStringValue) {
    auto cmd = parse_sql("SELECT * FROM t WHERE name == \"admin\";");
    ASSERT_NE(cmd, nullptr);
}

// ==================== Error cases ====================

TEST(ParserTest, ErrorEmptyInput) {
    EXPECT_THROW(parse_sql(""), ParserException);
}

TEST(ParserTest, ErrorUnknownCommand) {
    EXPECT_THROW(parse_sql("FOOBAR my_table;"), ParserException);
}

TEST(ParserTest, ErrorCreateTableMissingParentheses) {
    EXPECT_THROW(parse_sql("CREATE TABLE users id int;"), ParserException);
}

TEST(ParserTest, BetweenCondition) {
    auto cmd = parse_sql("SELECT * FROM t WHERE price BETWEEN 100 AND 500;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, LikeCondition) {
    auto cmd = parse_sql("SELECT * FROM t WHERE name LIKE \"A%\";");
    ASSERT_NE(cmd, nullptr);
}

// ==================== Logical AND/OR/NOT with parentheses ====================

TEST(ParserTest, AndCondition) {
    auto cmd = parse_sql("SELECT * FROM t WHERE age > 18 AND name == \"Alice\";");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, OrCondition) {
    auto cmd = parse_sql("SELECT * FROM t WHERE age < 10 OR age > 60;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, NotCondition) {
    auto cmd = parse_sql("SELECT * FROM t WHERE NOT admin;");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ParenthesesSimple) {
    auto cmd = parse_sql("SELECT * FROM t WHERE (age > 18);");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ParenthesesNested) {
    auto cmd = parse_sql("SELECT * FROM t WHERE (age > 18 AND (name == \"A\" OR name == \"B\"));");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ParenthesesPrecedence) {
    auto cmd = parse_sql("SELECT * FROM t WHERE age > 18 OR (admin == 1 AND name == \"root\");");
    ASSERT_NE(cmd, nullptr);
}

TEST(ParserTest, ParenthesesDeepNested) {
    auto cmd = parse_sql("SELECT * FROM t WHERE ((age >= 18 AND name == \"foo\") OR (NOT banned));");
    ASSERT_NE(cmd, nullptr);
}

// ==================== Aggregate functions ====================

TEST(ParserTest, SelectSum) {
    auto cmd = parse_sql("SELECT SUM(age) FROM users;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);
    auto targets = select_cmd->get_select_targets();
    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].column_name, "age");
    ASSERT_TRUE(targets[0].agg_func.has_value());
    EXPECT_EQ(targets[0].agg_func.value(), AggregateFunction::Sum);
}

TEST(ParserTest, SelectCount) {
    auto cmd = parse_sql("SELECT COUNT(*) FROM users;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);
    auto targets = select_cmd->get_select_targets();
    ASSERT_EQ(targets.size(), 1);
    EXPECT_TRUE(targets[0].column_name.empty());
    ASSERT_TRUE(targets[0].agg_func.has_value());
    EXPECT_EQ(targets[0].agg_func.value(), AggregateFunction::Count);
}

TEST(ParserTest, SelectAvg) {
    auto cmd = parse_sql("SELECT AVG(salary) FROM users;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);
    auto targets = select_cmd->get_select_targets();
    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].column_name, "salary");
    ASSERT_TRUE(targets[0].agg_func.has_value());
    EXPECT_EQ(targets[0].agg_func.value(), AggregateFunction::Avg);
}

TEST(ParserTest, SelectMixedAggregatesAndColumns) {
    auto cmd = parse_sql("SELECT name, SUM(age), COUNT(*), AVG(salary) FROM users;");
    ASSERT_NE(cmd, nullptr);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);
    auto targets = select_cmd->get_select_targets();
    ASSERT_EQ(targets.size(), 4);
    /* name */
    EXPECT_EQ(targets[0].column_name, "name");
    EXPECT_FALSE(targets[0].agg_func.has_value());
    /* SUM(age) */
    EXPECT_EQ(targets[1].column_name, "age");
    ASSERT_TRUE(targets[1].agg_func.has_value());
    EXPECT_EQ(targets[1].agg_func.value(), AggregateFunction::Sum);
    /* COUNT(*) */
    EXPECT_TRUE(targets[2].column_name.empty());
    ASSERT_TRUE(targets[2].agg_func.has_value());
    EXPECT_EQ(targets[2].agg_func.value(), AggregateFunction::Count);
    /* AVG(salary) */
    EXPECT_EQ(targets[3].column_name, "salary");
    ASSERT_TRUE(targets[3].agg_func.has_value());
    EXPECT_EQ(targets[3].agg_func.value(), AggregateFunction::Avg);
}

// ==================== Error cases ====================

TEST(ParserTest, ErrorMissingSemicolon) {
    EXPECT_THROW(parse_sql("SELECT * FROM t"), ParserException);
}

TEST(ParserTest, ErrorNoDatabaseName) {
    EXPECT_THROW(parse_sql("CREATE DATABASE;"), ParserException);
}

TEST(ParserTest, ErrorUnmatchedOpenParen) {
    EXPECT_THROW(parse_sql("SELECT * FROM t WHERE (age > 18;"), ParserException);
}

TEST(ParserTest, ErrorUnmatchedCloseParen) {
    EXPECT_THROW(parse_sql("SELECT * FROM t WHERE age > 18);"), ParserException);
}

TEST(ParserTest, ErrorEmptyParens) {
    EXPECT_THROW(parse_sql("SELECT * FROM t WHERE ();"), ParserException);
}

TEST(ParserTest, ErrorUnmatchedNestedParen) {
    EXPECT_THROW(parse_sql("SELECT * FROM t WHERE (age > 18 AND (name == \"A\");"), ParserException);
}

// ==================== JWT Token edge cases ====================

TEST(ParserTest, TokenEmptyWhenNoToken) {
    auto cmd = parse_sql("CREATE DATABASE mydb;");
    ASSERT_NE(cmd, nullptr);
    EXPECT_TRUE(cmd->get_token().empty());
}

TEST(ParserTest, TokenNotEmptyWhenTokenPresent) {
    auto cmd = parse_sql("DROP DATABASE mydb \"mytoken\";");
    ASSERT_NE(cmd, nullptr);
    EXPECT_FALSE(cmd->get_token().empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
