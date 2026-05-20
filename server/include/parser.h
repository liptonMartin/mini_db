#ifndef MINIDB_PARSER_H
#define MINIDB_PARSER_H

#include <memory>
#include <vector>
#include <string>
#include "lexer.h"
#include "command.h"

class ParserException : public std::runtime_error {
public:
    explicit ParserException(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    std::unique_ptr<Command> parse(const std::vector<Token>& tokens);

private:
    size_t _pos = 0;
    std::string _jwt_token;
    const std::vector<Token>* _tokens = nullptr;

    const Token& current() const;
    const Token& peek(size_t offset = 0) const;
    const Token& advance();
    bool is_end() const;
    bool check(TypeToken type, const std::string& value = "") const;
    bool match(TypeToken type, const std::string& value = "");
    void expect(TypeToken type, const std::string& value = "");
    void expect_semicolon();
    bool match_keyword(const std::string& value);

    std::unique_ptr<Command> parse_command();
    std::unique_ptr<Command> parse_create_database();
    std::unique_ptr<Command> parse_drop_database();
    std::unique_ptr<Command> parse_use_database();
    std::unique_ptr<Command> parse_create_table();
    std::unique_ptr<Command> parse_drop_table();
    std::unique_ptr<Command> parse_insert_into();
    std::unique_ptr<Command> parse_update();
    std::unique_ptr<Command> parse_delete_from();
    std::unique_ptr<Command> parse_select();

    std::unique_ptr<Condition> parse_condition();
    std::unique_ptr<Condition> parse_or_expression();
    std::unique_ptr<Condition> parse_and_expression();
    std::unique_ptr<Condition> parse_unary_expression();
    std::unique_ptr<Condition> parse_primary_condition();
    Column parse_column_definition();
    Value parse_value();
    std::vector<std::string> parse_identifier_list();
    std::vector<Value> parse_value_list();
    std::string parse_aggregate_or_column();
};

#endif //MINIDB_PARSER_H