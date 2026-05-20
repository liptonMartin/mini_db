#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "command.h"

using json = nlohmann::json;

static json parse_to_json(const std::string& sql) {
    Lexer lexer;
    auto tokens = lexer.tokenize(sql);
    Parser parser;
    auto cmd = parser.parse(tokens);
    return json::parse(cmd->serialize_command());
}

static json cond(const std::string& sql) {
    return parse_to_json(sql)["condition"];
}

// ==================== CREATE DATABASE ====================

TEST(ParserTest, CreateDatabaseSimple) {
    auto actual = parse_to_json("CREATE DATABASE my_db;");
    auto expected = R"({"database_name":"my_db"})"_json;
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, CreateDatabaseCaseInsensitive) {
    auto actual = parse_to_json("CrEaTe DaTaBaSe casedb;");
    auto expected = R"({"database_name":"casedb"})"_json;
    EXPECT_EQ(actual, expected);
}

// ==================== DROP DATABASE ====================

TEST(ParserTest, DropDatabase) {
    auto actual = parse_to_json("DROP DATABASE old_db;");
    auto expected = R"({"database_name":"old_db"})"_json;
    EXPECT_EQ(actual, expected);
}

// ==================== USE DATABASE ====================

TEST(ParserTest, UseDatabase) {
    auto actual = parse_to_json("USE active_db;");
    auto expected = R"({"database_name":"active_db"})"_json;
    EXPECT_EQ(actual, expected);
}

// ==================== CREATE TABLE ====================

TEST(ParserTest, CreateTableSimple) {
    auto actual = parse_to_json("CREATE TABLE users (id int, name string);");
    auto expected = json{
        {"table_name", "users"},
        {"columns", json::array({
            json{{"column_id", -1}, {"name", "id"}, {"type", 0}, {"is_nullable", true}, {"is_indexed", false}},
            json{{"column_id", -1}, {"name", "name"}, {"type", 1}, {"is_nullable", true}, {"is_indexed", false}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, CreateTableWithConstraints) {
    auto actual = parse_to_json("CREATE TABLE products (id int INDEXED, name string NOT NULL, price int);");
    auto expected = json{
        {"table_name", "products"},
        {"columns", json::array({
            json{{"column_id", -1}, {"name", "id"}, {"type", 0}, {"is_nullable", false}, {"is_indexed", true}},
            json{{"column_id", -1}, {"name", "name"}, {"type", 1}, {"is_nullable", false}, {"is_indexed", false}},
            json{{"column_id", -1}, {"name", "price"}, {"type", 0}, {"is_nullable", true}, {"is_indexed", false}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

// ==================== DROP TABLE ====================

TEST(ParserTest, DropTable) {
    auto actual = parse_to_json("DROP TABLE old_table;");
    auto expected = R"({"table_name":"old_table"})"_json;
    EXPECT_EQ(actual, expected);
}

// ==================== INSERT INTO ====================

TEST(ParserTest, InsertIntoString) {
    auto actual = parse_to_json("INSERT INTO users (name, email) VALUE (\"Alice\", \"alice@mail.com\");");
    auto expected = json{
        {"table_name", "users"},
        {"columns", json::array({"name", "email"})},
        {"values", json::array({
            json{{"type", "string"}, {"value", "Alice"}},
            json{{"type", "string"}, {"value", "alice@mail.com"}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, InsertIntoNumbers) {
    auto actual = parse_to_json("INSERT INTO products (id, price) VALUE (1, 999);");
    auto expected = json{
        {"table_name", "products"},
        {"columns", json::array({"id", "price"})},
        {"values", json::array({
            json{{"type", "int"}, {"value", 1}},
            json{{"type", "int"}, {"value", 999}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, InsertIntoNull) {
    auto actual = parse_to_json("INSERT INTO users (name, phone) VALUE (\"Bob\", NULL);");
    auto expected = json{
        {"table_name", "users"},
        {"columns", json::array({"name", "phone"})},
        {"values", json::array({
            json{{"type", "string"}, {"value", "Bob"}},
            json{{"type", "null"}, {"value", nullptr}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

// ==================== UPDATE ====================

TEST(ParserTest, UpdateSimple) {
    auto actual = parse_to_json("UPDATE users SET name = \"Charlie\" WHERE id == 1;");
    EXPECT_EQ(actual["table_name"], "users");
    EXPECT_EQ(actual["columns"], json::array({"name"}));
    EXPECT_EQ(actual["values"], json::array({R"({"type":"string","value":"Charlie"})"_json}));
    EXPECT_EQ(actual["condition"]["type"], "comparison");
    EXPECT_EQ(actual["condition"]["column_name"], "id");
    EXPECT_EQ(actual["condition"]["comparison_type"], 0);
    EXPECT_EQ(actual["condition"]["value"], R"({"type":"int","value":1})"_json);
}

TEST(ParserTest, UpdateMultipleSets) {
    auto actual = parse_to_json("UPDATE products SET price = 100, name = \"NewName\" WHERE id == 5;");
    EXPECT_EQ(actual["table_name"], "products");
    EXPECT_EQ(actual["columns"], json::array({"price", "name"}));
    EXPECT_EQ(actual["values"], json::array({
        json{{"type", "int"}, {"value", 100}},
        json{{"type", "string"}, {"value", "NewName"}}
    }));
}

// ==================== DELETE FROM ====================

TEST(ParserTest, DeleteSimple) {
    auto actual = parse_to_json("DELETE FROM users WHERE id == 10;");
    EXPECT_EQ(actual["table_name"], "users");
    EXPECT_EQ(actual["condition"]["type"], "comparison");
    EXPECT_EQ(actual["condition"]["column_name"], "id");
    EXPECT_EQ(actual["condition"]["comparison_type"], 0);
    EXPECT_EQ(actual["condition"]["value"], R"({"type":"int","value":10})"_json);
}

// ==================== SELECT ====================

TEST(ParserTest, SelectStar) {
    auto actual = parse_to_json("SELECT * FROM users;");
    auto expected = json{{"table_name", "users"}, {"select_targets", json::array()}};
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, SelectColumns) {
    auto actual = parse_to_json("SELECT name, email FROM users;");
    auto expected = json{
        {"table_name", "users"},
        {"select_targets", json::array({
            json{{"type", "column"}, {"column_name", "name"}},
            json{{"type", "column"}, {"column_name", "email"}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, SelectWithAlias) {
    auto actual = parse_to_json("SELECT name AS n FROM users;");
    auto expected = json{
        {"table_name", "users"},
        {"select_targets", json::array({
            json{{"type", "column"}, {"column_name", "name"}, {"alias", "n"}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, SelectWithCondition) {
    auto actual = parse_to_json("SELECT * FROM users WHERE age >= 18;");
    EXPECT_EQ(actual["table_name"], "users");
    EXPECT_EQ(actual["select_targets"], json::array());
    EXPECT_EQ(actual["condition"]["type"], "comparison");
    EXPECT_EQ(actual["condition"]["column_name"], "age");
    EXPECT_EQ(actual["condition"]["comparison_type"], 3);  // GreaterEqual
    EXPECT_EQ(actual["condition"]["value"], R"({"type":"int","value":18})"_json);
}

// ==================== Conditions (comparison operators) ====================

TEST(ParserTest, ConditionEqual) {
    auto c = cond("SELECT * FROM t WHERE id == 1;");
    EXPECT_EQ(c["type"], "comparison");
    EXPECT_EQ(c["column_name"], "id");
    EXPECT_EQ(c["comparison_type"], 0);  // Equal
    EXPECT_EQ(c["value"], R"({"type":"int","value":1})"_json);
}

TEST(ParserTest, ConditionNotEqual) {
    auto c = cond("SELECT * FROM t WHERE id != 1;");
    EXPECT_EQ(c["type"], "comparison");
    EXPECT_EQ(c["column_name"], "id");
    EXPECT_EQ(c["comparison_type"], 1);  // NotEqual
    EXPECT_EQ(c["value"], R"({"type":"int","value":1})"_json);
}

TEST(ParserTest, ConditionLess) {
    auto c = cond("SELECT * FROM t WHERE id < 10;");
    EXPECT_EQ(c["comparison_type"], 4);  // Less
}

TEST(ParserTest, ConditionGreater) {
    auto c = cond("SELECT * FROM t WHERE id > 5;");
    EXPECT_EQ(c["comparison_type"], 2);  // Greater
}

TEST(ParserTest, ConditionLessEqual) {
    auto c = cond("SELECT * FROM t WHERE id <= 100;");
    EXPECT_EQ(c["comparison_type"], 5);  // LessEqual
}

TEST(ParserTest, ConditionGreaterEqual) {
    auto c = cond("SELECT * FROM t WHERE id >= 18;");
    EXPECT_EQ(c["comparison_type"], 3);  // GreaterEqual
}

TEST(ParserTest, ConditionStringValue) {
    auto c = cond("SELECT * FROM t WHERE name == \"admin\";");
    EXPECT_EQ(c["type"], "comparison");
    EXPECT_EQ(c["column_name"], "name");
    EXPECT_EQ(c["comparison_type"], 0);
    EXPECT_EQ(c["value"], R"({"type":"string","value":"admin"})"_json);
}

// ==================== Between / Like ====================

TEST(ParserTest, BetweenCondition) {
    auto c = cond("SELECT * FROM t WHERE price BETWEEN 100 AND 500;");
    EXPECT_EQ(c["type"], "between");
    EXPECT_EQ(c["column_name"], "price");
    EXPECT_EQ(c["start"], R"({"type":"int","value":100})"_json);
    EXPECT_EQ(c["end"], R"({"type":"int","value":500})"_json);
}

TEST(ParserTest, LikeCondition) {
    auto c = cond("SELECT * FROM t WHERE name LIKE \"A%\";");
    EXPECT_EQ(c["type"], "regex");
    EXPECT_EQ(c["column_name"], "name");
    EXPECT_EQ(c["regex"], "A.*");
}

// ==================== Logical AND/OR/NOT with parentheses ====================

TEST(ParserTest, AndCondition) {
    auto c = cond("SELECT * FROM t WHERE age > 18 AND name == \"Alice\";");
    EXPECT_EQ(c["type"], "and");
    EXPECT_EQ(c["left"]["column_name"], "age");
    EXPECT_EQ(c["right"]["column_name"], "name");
}

TEST(ParserTest, OrCondition) {
    auto c = cond("SELECT * FROM t WHERE age < 10 OR age > 60;");
    EXPECT_EQ(c["type"], "or");
    EXPECT_EQ(c["left"]["column_name"], "age");
    EXPECT_EQ(c["left"]["comparison_type"], 4);
    EXPECT_EQ(c["right"]["column_name"], "age");
    EXPECT_EQ(c["right"]["comparison_type"], 2);
}

TEST(ParserTest, ParenthesesSimple) {
    auto c = cond("SELECT * FROM t WHERE (age > 18);");
    // скобки не создают отдельного узла
    EXPECT_EQ(c["type"], "comparison");
    EXPECT_EQ(c["column_name"], "age");
    EXPECT_EQ(c["comparison_type"], 2);
}

TEST(ParserTest, ParenthesesNested) {
    auto c = cond("SELECT * FROM t WHERE (age > 18 AND (name == \"A\" OR name == \"B\"));");
    EXPECT_EQ(c["type"], "and");
    EXPECT_EQ(c["left"]["column_name"], "age");
    EXPECT_EQ(c["right"]["type"], "or");
    EXPECT_EQ(c["right"]["left"]["column_name"], "name");
    EXPECT_EQ(c["right"]["right"]["column_name"], "name");
}

TEST(ParserTest, ParenthesesPrecedence) {
    auto c = cond("SELECT * FROM t WHERE age > 18 OR (admin == 1 AND name == \"root\");");
    EXPECT_EQ(c["type"], "or");
    EXPECT_EQ(c["left"]["type"], "comparison");
    EXPECT_EQ(c["right"]["type"], "and");
}

TEST(ParserTest, NotCondition) {
    auto c = cond("SELECT * FROM t WHERE NOT admin;");
    EXPECT_EQ(c["type"], "not");
    EXPECT_EQ(c["operand"]["type"], "comparison");
    EXPECT_EQ(c["operand"]["column_name"], "admin");
    EXPECT_EQ(c["operand"]["comparison_type"], 1);  // NotEqual(admin, 0)
}

TEST(ParserTest, ParenthesesDeepNested) {
    auto c = cond("SELECT * FROM t WHERE ((age >= 18 AND name == \"foo\") OR (NOT banned));");
    EXPECT_EQ(c["type"], "or");
    EXPECT_EQ(c["left"]["type"], "and");
    EXPECT_EQ(c["left"]["left"]["column_name"], "age");
    EXPECT_EQ(c["left"]["right"]["column_name"], "name");
    EXPECT_EQ(c["right"]["type"], "not");
    EXPECT_EQ(c["right"]["operand"]["column_name"], "banned");
}

// ==================== Aggregate functions ====================

TEST(ParserTest, SelectSum) {
    auto actual = parse_to_json("SELECT SUM(age) FROM users;");
    auto expected = json{
        {"table_name", "users"},
        {"select_targets", json::array({
            json{{"type", "aggregate"}, {"func", "sum"}, {"column_name", "age"}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, SelectCount) {
    auto actual = parse_to_json("SELECT COUNT(*) FROM users;");
    auto expected = json{
        {"table_name", "users"},
        {"select_targets", json::array({
            json{{"type", "aggregate"}, {"func", "count"}, {"column_name", ""}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, SelectAvg) {
    auto actual = parse_to_json("SELECT AVG(salary) FROM users;");
    auto expected = json{
        {"table_name", "users"},
        {"select_targets", json::array({
            json{{"type", "aggregate"}, {"func", "avg"}, {"column_name", "salary"}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, SelectMixedAggregatesAndColumns) {
    auto actual = parse_to_json("SELECT name, SUM(age), COUNT(*), AVG(salary) FROM users;");
    auto expected = json{
        {"table_name", "users"},
        {"select_targets", json::array({
            json{{"type", "column"}, {"column_name", "name"}},
            json{{"type", "aggregate"}, {"func", "sum"}, {"column_name", "age"}},
            json{{"type", "aggregate"}, {"func", "count"}, {"column_name", ""}},
            json{{"type", "aggregate"}, {"func", "avg"}, {"column_name", "salary"}}
        })}
    };
    EXPECT_EQ(actual, expected);
}

// ==================== Order of operations (AND/OR precedence) ====================

TEST(ParserTest, PrecedenceAndBindsTighterThanOr) {
    auto c = cond("SELECT * FROM t WHERE age > 10 AND name == \"Alice\" OR id == 1;");
    EXPECT_EQ(c["type"], "or");
    EXPECT_EQ(c["left"]["type"], "and");
    EXPECT_EQ(c["right"]["column_name"], "id");
}

TEST(ParserTest, PrecedenceOrWithAndOnRight) {
    auto c = cond("SELECT * FROM t WHERE age > 10 OR name == \"Alice\" AND id == 1;");
    EXPECT_EQ(c["type"], "or");
    EXPECT_EQ(c["left"]["type"], "comparison");
    EXPECT_EQ(c["left"]["column_name"], "age");
    EXPECT_EQ(c["right"]["type"], "and");
}

TEST(ParserTest, PrecedenceParensOverride) {
    auto c = cond("SELECT * FROM t WHERE (age > 10 OR name == \"Alice\") AND id == 1;");
    EXPECT_EQ(c["type"], "and");
    EXPECT_EQ(c["left"]["type"], "or");
    EXPECT_EQ(c["right"]["column_name"], "id");
}

TEST(ParserTest, PrecedenceParensBeforeAnd) {
    auto c = cond("SELECT * FROM t WHERE age > 10 AND (name == \"Alice\" OR id == 1);");
    EXPECT_EQ(c["type"], "and");
    EXPECT_EQ(c["left"]["type"], "comparison");
    EXPECT_EQ(c["left"]["column_name"], "age");
    EXPECT_EQ(c["right"]["type"], "or");
}

TEST(ParserTest, PrecedenceVovaExample) {
    auto c = cond("SELECT * FROM t WHERE (id == 1 AND name == \"vova\") OR id == 3;");
    EXPECT_EQ(c["type"], "or");
    EXPECT_EQ(c["left"]["type"], "and");
    EXPECT_EQ(c["left"]["left"]["column_name"], "id");
    EXPECT_EQ(c["left"]["left"]["comparison_type"], 0);
    EXPECT_EQ(c["left"]["right"]["column_name"], "name");
    EXPECT_EQ(c["right"]["type"], "comparison");
    EXPECT_EQ(c["right"]["column_name"], "id");
    EXPECT_EQ(c["right"]["comparison_type"], 0);
}

TEST(ParserTest, PrecedenceNestedParens) {
    auto c = cond("SELECT * FROM t WHERE (age > 10 AND (name == \"Alice\" OR id == 1)) OR admin == 1;");
    EXPECT_EQ(c["type"], "or");
    EXPECT_EQ(c["left"]["type"], "and");
    EXPECT_EQ(c["left"]["right"]["type"], "or");
    EXPECT_EQ(c["right"]["type"], "comparison");
    EXPECT_EQ(c["right"]["column_name"], "admin");
}

// ==================== Error cases ====================

TEST(ParserTest, ErrorEmptyInput) {
    EXPECT_THROW(parse_to_json(""), ParserException);
}

TEST(ParserTest, ErrorUnknownCommand) {
    EXPECT_THROW(parse_to_json("FOOBAR my_table;"), ParserException);
}

TEST(ParserTest, ErrorCreateTableMissingParentheses) {
    EXPECT_THROW(parse_to_json("CREATE TABLE users id int;"), ParserException);
}

TEST(ParserTest, ErrorMissingSemicolon) {
    EXPECT_THROW(parse_to_json("SELECT * FROM t"), ParserException);
}

TEST(ParserTest, ErrorNoDatabaseName) {
    EXPECT_THROW(parse_to_json("CREATE DATABASE;"), ParserException);
}

TEST(ParserTest, ErrorUnmatchedOpenParen) {
    EXPECT_THROW(parse_to_json("SELECT * FROM t WHERE (age > 18;"), ParserException);
}

TEST(ParserTest, ErrorUnmatchedCloseParen) {
    EXPECT_THROW(parse_to_json("SELECT * FROM t WHERE age > 18);"), ParserException);
}

TEST(ParserTest, ErrorEmptyParens) {
    EXPECT_THROW(parse_to_json("SELECT * FROM t WHERE ();"), ParserException);
}

TEST(ParserTest, ErrorUnmatchedNestedParen) {
    EXPECT_THROW(parse_to_json("SELECT * FROM t WHERE (age > 18 AND (name == \"A\");"), ParserException);
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
    Lexer lexer;
    auto tokens = lexer.tokenize("CREATE DATABASE mydb;");
    Parser parser;
    auto cmd = parser.parse(tokens);
    EXPECT_TRUE(cmd->get_token().empty());
}

TEST(ParserTest, TokenNotEmptyWhenTokenPresent) {
    Lexer lexer;
    auto tokens = lexer.tokenize("DROP DATABASE mydb \"mytoken\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    EXPECT_FALSE(cmd->get_token().empty());
}

// ==================== Token serialization tests ====================

TEST(ParserTest, CreateDatabaseWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("CREATE DATABASE securedb \"eyJhbGciOiJIUzI1NiJ9.abc.xyz\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* create_cmd = dynamic_cast<CreateDatabaseCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);
    EXPECT_EQ(create_cmd->get_token(), "eyJhbGciOiJIUzI1NiJ9.abc.xyz");
    EXPECT_FALSE(create_cmd->get_token().empty());

    auto actual = json::parse(cmd->serialize_command());
    auto expected = R"({"database_name":"securedb"})"_json;
    EXPECT_EQ(actual, expected);
}

TEST(ParserTest, DropDatabaseWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("DROP DATABASE old_db \"token123\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* drop_cmd = dynamic_cast<DropDatabaseCommand*>(cmd.get());
    ASSERT_NE(drop_cmd, nullptr);
    EXPECT_EQ(drop_cmd->get_token(), "token123");
}

TEST(ParserTest, UseDatabaseWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("USE active_db \"jwt_use\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* use_cmd = dynamic_cast<UseDatabaseCommand*>(cmd.get());
    ASSERT_NE(use_cmd, nullptr);
    EXPECT_EQ(use_cmd->get_token(), "jwt_use");
}

TEST(ParserTest, CreateTableWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("CREATE TABLE t (x int) \"table_token\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* create_cmd = dynamic_cast<CreateTableCommand*>(cmd.get());
    ASSERT_NE(create_cmd, nullptr);
    EXPECT_EQ(create_cmd->get_token(), "table_token");

    auto actual = json::parse(cmd->serialize_command());
    EXPECT_EQ(actual["table_name"], "t");
}

TEST(ParserTest, DropTableWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("DROP TABLE t \"drop_token\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* drop_cmd = dynamic_cast<DropTableCommand*>(cmd.get());
    ASSERT_NE(drop_cmd, nullptr);
    EXPECT_EQ(drop_cmd->get_token(), "drop_token");
}

TEST(ParserTest, InsertIntoWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("INSERT INTO t (a) VALUE (1) \"insert_token\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* insert_cmd = dynamic_cast<InsertIntoCommand*>(cmd.get());
    ASSERT_NE(insert_cmd, nullptr);
    EXPECT_EQ(insert_cmd->get_token(), "insert_token");
}

TEST(ParserTest, UpdateWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("UPDATE users SET name = \"Bob\" WHERE id == 1 \"update_token\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* update_cmd = dynamic_cast<UpdateCommand*>(cmd.get());
    ASSERT_NE(update_cmd, nullptr);
    EXPECT_EQ(update_cmd->get_token(), "update_token");
}

TEST(ParserTest, DeleteWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("DELETE FROM users WHERE id == 1 \"delete_token\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* delete_cmd = dynamic_cast<DeleteFromCommand*>(cmd.get());
    ASSERT_NE(delete_cmd, nullptr);
    EXPECT_EQ(delete_cmd->get_token(), "delete_token");
}

TEST(ParserTest, SelectWithToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("SELECT * FROM t \"select_token\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);
    EXPECT_EQ(select_cmd->get_token(), "select_token");
}

TEST(ParserTest, SelectWithWhereAndToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("SELECT * FROM t WHERE id > 5 \"select_where_token\";");
    Parser parser;
    auto cmd = parser.parse(tokens);
    auto* select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    ASSERT_NE(select_cmd, nullptr);
    EXPECT_EQ(select_cmd->get_token(), "select_where_token");
}

TEST(ParserTest, UpdateMultipleSetsNoToken) {
    Lexer lexer;
    auto tokens = lexer.tokenize("UPDATE products SET price = 100, name = \"NewName\" WHERE id == 5;");
    Parser parser;
    auto cmd = parser.parse(tokens);
    EXPECT_TRUE(cmd->get_token().empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
