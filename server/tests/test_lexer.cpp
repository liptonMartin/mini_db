#include <gtest/gtest.h>
#include "lexer.h"
#include "exceptions.h"

// Пример теста для вашего лексера
TEST(LexerTest, HandlesBasicKeywords) {
    Lexer lexer;
    std::string input = "SELECT TABLE";
    auto tokens = lexer.tokenize(input);

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TypeToken::KEYWORD);
    EXPECT_EQ(tokens[0].value, "SELECT");
    EXPECT_EQ(tokens[1].value, "TABLE");
    EXPECT_EQ(tokens[1].type, TypeToken::KEYWORD);
}

TEST(LexerTest, HandleKeywordStringLiteral) {
    Lexer lexer;
    std::string input = "UPDATE \"vova\"";
    auto tokens = lexer.tokenize(input);

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TypeToken::KEYWORD);
    EXPECT_EQ(tokens[0].value, "UPDATE");
    EXPECT_EQ(tokens[1].value, "vova");
    EXPECT_EQ(tokens[1].type, TypeToken::STRING);
}

TEST(LexerTest, HandleKeywordIdentifiyer) {
    Lexer lexer;
    std::string input = "UPDATE vova4343 ;"; // TODO: СДЕЛАТЬ ОБРАБОТКУ ОШИБКИ КОГДА ВВОДЯТ ИДЕНТИФИКАТОР С ЦИФРОЙ В НАЧАЛЕ
    auto tokens = lexer.tokenize(input);

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TypeToken::KEYWORD);
    EXPECT_EQ(tokens[0].value, "UPDATE");
    EXPECT_EQ(tokens[1].value, "vova4343");
    EXPECT_EQ(tokens[1].type, TypeToken::IDENTIFIER);
    EXPECT_EQ(tokens[2].type, TypeToken::SEMICOLON);

}

// ===== Вспомогательная функция =====
std::vector<Token> lex(const std::string& input) {
    Lexer lexer;
    return lexer.tokenize(input);
}

// ===== Базовые токены =====

TEST(LexerTest, EmptyInput) {
    auto tokens = lex("");
    EXPECT_TRUE(tokens.empty());
}

TEST(LexerTest, SingleKeyword) {
    auto tokens = lex("CREATE");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TypeToken::KEYWORD);
    EXPECT_EQ(tokens[0].value, "CREATE");
}

TEST(LexerTest, SingleIdentifier) {
    auto tokens = lex("mydb");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TypeToken::IDENTIFIER);
    EXPECT_EQ(tokens[0].value, "mydb");
}

TEST(LexerTest, SingleNumber) {
    auto tokens = lex("12345");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TypeToken::NUMBER);
    EXPECT_EQ(tokens[0].value, "12345");
}

// ===== Простые команды =====

TEST(LexerTest, CreateDatabase) {
    auto tokens = lex("CREATE DATABASE mydb");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TypeToken::KEYWORD);
    EXPECT_EQ(tokens[0].value, "CREATE");
    EXPECT_EQ(tokens[1].type, TypeToken::KEYWORD);
    EXPECT_EQ(tokens[1].value, "DATABASE");
    EXPECT_EQ(tokens[2].type, TypeToken::IDENTIFIER);
    EXPECT_EQ(tokens[2].value, "mydb");
}

TEST(LexerTest, UseDatabase) {
    auto tokens = lex("USE mydb");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TypeToken::KEYWORD);
    EXPECT_EQ(tokens[0].value, "USE");
    EXPECT_EQ(tokens[1].type, TypeToken::IDENTIFIER);
    EXPECT_EQ(tokens[1].value, "mydb");
}

TEST(LexerTest, DropDatabase) {
    auto tokens = lex("DROP DATABASE mydb");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].value, "DROP");
    EXPECT_EQ(tokens[1].value, "DATABASE");
    EXPECT_EQ(tokens[2].value, "mydb");
}

TEST(LexerTest, Semicolon) {
    auto tokens = lex("CREATE DATABASE mydb;");
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[3].type, TypeToken::SEMICOLON);
}
// ===== DDL: CREATE TABLE =====

TEST(LexerTest, CreateTable) {
    auto tokens = lex("CREATE TABLE users ( id INT NOT NULL , name TEXT )");
    ASSERT_EQ(tokens.size(), 12);
    EXPECT_EQ(tokens[0].value, "CREATE");
    EXPECT_EQ(tokens[1].value, "TABLE");
    EXPECT_EQ(tokens[2].value, "users");
    EXPECT_EQ(tokens[3].type, TypeToken::LBRACKET);
    EXPECT_EQ(tokens[4].value, "id");
    EXPECT_EQ(tokens[5].value, "INT");
    EXPECT_EQ(tokens[6].value, "NOT");
    EXPECT_EQ(tokens[7].value, "NULL");
    EXPECT_EQ(tokens[8].type, TypeToken::COMMA);
    EXPECT_EQ(tokens[9].value, "name");
    EXPECT_EQ(tokens[10].value, "TEXT");
    EXPECT_EQ(tokens[11].type, TypeToken::RBRACKET);
}

TEST(LexerTest, CreateTableWithIndexed) {
    auto tokens = lex("CREATE TABLE t ( col INT INDEXED )");
    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[6].value, "INDEXED");
    EXPECT_EQ(tokens[6].type, TypeToken::KEYWORD);
}

// ===== Строковые литералы =====

TEST(LexerTest, StringLiteral) {
    auto tokens = lex("\"hello world\"");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TypeToken::STRING);
    EXPECT_EQ(tokens[0].value, "hello world");  // без кавычек
}

TEST(LexerTest, InsertWithStringValue) {
    auto tokens = lex("INSERT INTO users ( name ) VALUE ( \"John\" )");
    ASSERT_GE(tokens.size(), 1);
    // Ищем строковой токен
    bool found = false;
    for (const auto& t : tokens) {
        if (t.type == TypeToken::STRING && t.value == "John") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ===== Операторы сравнения =====

TEST(LexerTest, Operators) {
    auto tokens = lex("== != < > <= >=");
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].type, TypeToken::EQ);
    EXPECT_EQ(tokens[1].type, TypeToken::NE);
    EXPECT_EQ(tokens[2].type, TypeToken::LESSTHAN);
    EXPECT_EQ(tokens[3].type, TypeToken::GREATERTHAN);
    EXPECT_EQ(tokens[4].type, TypeToken::LESSEQUAL);
    EXPECT_EQ(tokens[5].type, TypeToken::GREATEREQUAL);
}

TEST(LexerTest, TwoCharOpsPriority) {
    auto tokens = lex(">= <=");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TypeToken::GREATEREQUAL);
    EXPECT_EQ(tokens[1].type, TypeToken::LESSEQUAL);
}

// ===== SELECT с условиями =====

TEST(LexerTest, SelectWithWhere) {
    auto tokens = lex("SELECT * FROM users WHERE id > 5");
    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].value, "SELECT");
    EXPECT_EQ(tokens[1].type, TypeToken::STAR);
    EXPECT_EQ(tokens[2].value, "FROM");
    EXPECT_EQ(tokens[3].value, "users");
    EXPECT_EQ(tokens[4].value, "WHERE");
    EXPECT_EQ(tokens[5].value, "id");
    EXPECT_EQ(tokens[6].type, TypeToken::GREATERTHAN);
    EXPECT_EQ(tokens[7].type, TypeToken::NUMBER);
}

TEST(LexerTest, SelectWithAlias) {
    auto tokens = lex("SELECT name AS n FROM users");
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].value, "SELECT");
    EXPECT_EQ(tokens[1].value, "name");
    EXPECT_EQ(tokens[2].value, "AS");
    EXPECT_EQ(tokens[3].value, "n");
    EXPECT_EQ(tokens[4].value, "FROM");
    EXPECT_EQ(tokens[5].value, "users");
}

// ===== INSERT =====

TEST(LexerTest, InsertInto) {
    auto tokens = lex("INSERT INTO users ( id , name ) VALUE ( 1 , \"Alice\" )");
    ASSERT_GE(tokens.size(), 14);
    EXPECT_EQ(tokens[0].value, "INSERT");
    EXPECT_EQ(tokens[1].value, "INTO");
    EXPECT_EQ(tokens[2].value, "users");
}

// ===== UPDATE =====

TEST(LexerTest, UpdateSet) {
    auto tokens = lex("UPDATE users SET name = \"Bob\" WHERE id == 1");
    ASSERT_GE(tokens.size(), 10);
    EXPECT_EQ(tokens[0].value, "UPDATE");
    EXPECT_EQ(tokens[1].value, "users");
    EXPECT_EQ(tokens[2].value, "SET");
    EXPECT_EQ(tokens[3].value, "name");
    EXPECT_EQ(tokens[4].type, TypeToken::ASSIGN);
    EXPECT_EQ(tokens[8].type, TypeToken::EQ);
}

// ===== DELETE =====

TEST(LexerTest, DeleteFrom) {
    auto tokens = lex("DELETE FROM users WHERE id == 1");
    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].value, "DELETE");
    EXPECT_EQ(tokens[1].value, "FROM");
}

// ===== BETWEEN и LIKE =====

TEST(LexerTest, Between) {
    auto tokens = lex("x BETWEEN 1 AND 10");
    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[1].value, "BETWEEN");
    EXPECT_EQ(tokens[3].value, "AND");
}

TEST(LexerTest, Like) {
    auto tokens = lex("name LIKE \"%test%\"");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[1].value, "LIKE");
    EXPECT_EQ(tokens[2].type, TypeToken::STRING);
}



// ===== Обработка ошибок =====

TEST(LexerTest, UnexpectedCharacter) {
    EXPECT_THROW(lex("CREATE @ DATABASE"), LexerException);
}

TEST(LexerTest, UnclosedString) {
    EXPECT_THROW(lex("\"unclosed string"), LexerException);
}

// ===== Идентификаторы с подчёркиванием =====

TEST(LexerTest, IdentifierWithUnderscore) {
    auto tokens = lex("my_db _test a1");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].value, "my_db");
    EXPECT_EQ(tokens[1].value, "_test");
    EXPECT_EQ(tokens[2].value, "a1");
}

// ===== Идентификатор не может начинаться с цифры =====

TEST(LexerTest, IdentifierCannotStartWithDigit) {
    auto tokens = lex("123abc");
    ASSERT_EQ(tokens.size(), 1);
    // Должен быть number + identifier? Или только number?
    // С текущей грамматикой number съест "123", потом "abc" отдельно
    // Проверим "123abc" как единую строку — должно быть number(123) + identifier(abc)
}
