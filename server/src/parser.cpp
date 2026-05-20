#include "parser.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include "lexer.h"

const Token& Parser::current() const {
    if (_pos >= _tokens->size()) {
        throw ParserException("Unexpected end of input");
    }
    return (*_tokens)[_pos];
}

const Token& Parser::peek(size_t offset) const {
    size_t idx = _pos + offset;
    if (idx >= _tokens->size()) {
        throw ParserException("Unexpected end of input");
    }
    return (*_tokens)[idx];
}

const Token& Parser::advance() {
    if (_pos >= _tokens->size()) {
        throw ParserException("Unexpected end of input");
    }
    return (*_tokens)[_pos++];
}

bool Parser::is_end() const {
    return _pos >= _tokens->size();
}

bool Parser::check(TypeToken type, const std::string& value) const {
    if (is_end()) return false;
    const Token& t = current();
    if (t.type != type) return false;
    if (!value.empty()) {
        std::string token_upper = t.value;
        std::string expected_upper = value;
        std::transform(token_upper.begin(), token_upper.end(), token_upper.begin(), ::toupper);
        std::transform(expected_upper.begin(), expected_upper.end(), expected_upper.begin(), ::toupper);
        return token_upper == expected_upper;
    }
    return true;
}

bool Parser::match(TypeToken type, const std::string& value) {
    if (check(type, value)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match_keyword(const std::string& value) {
    if (is_end()) return false;

    const Token& t = current();
    if (t.type != TypeToken::KEYWORD && t.type != TypeToken::IDENTIFIER) return false;

    std::string token_upper = t.value;
    std::string expected_upper = value;
    std::transform(token_upper.begin(), token_upper.end(), token_upper.begin(), ::toupper);
    std::transform(expected_upper.begin(), expected_upper.end(), expected_upper.begin(), ::toupper);

    if (token_upper == expected_upper) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TypeToken type, const std::string& value) {
    if (!match(type, value)) {
        std::string expected = value.empty() ? "token type " + std::to_string(static_cast<int>(type)) : value;
        std::string got = is_end() ? "end of input" : current().value;
        throw ParserException("Expected '" + expected + "', but got '" + got + "'");
    }
}

void Parser::expect_semicolon() {
    if (check(TypeToken::STRING)) {
        _jwt_token = advance().value;
    }
    expect(TypeToken::SEMICOLON, ";");
}

static DataType string_to_datatype(const std::string& type_str) {
    std::string upper = type_str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "INT" || upper == "INTEGER") return DataType::Int;
    if (upper == "STRING" || upper == "TEXT" || upper == "VARCHAR") return DataType::String;
    throw ParserException("Unknown data type: " + type_str);
}

static ComparisonDataType string_to_comparison(const std::string& op) {
    if (op == "==") return ComparisonDataType::Equal;
    if (op == "!=") return ComparisonDataType::NotEqual;
    if (op == "<")  return ComparisonDataType::Less;
    if (op == ">")  return ComparisonDataType::Greater;
    if (op == "<=") return ComparisonDataType::LessEqual;
    if (op == ">=") return ComparisonDataType::GreaterEqual;
    throw ParserException("Unknown comparison operator: " + op);
}

std::unique_ptr<Command> Parser::parse(const std::vector<Token>& tokens) {
    _tokens = &tokens;
    _pos = 0;
    _jwt_token.clear();

    if (tokens.empty()) {
        throw ParserException("Empty input");
    }

    auto command = parse_command();

    if (!_jwt_token.empty()) {
        command->set_token(_jwt_token);
    }

    return command;
}

std::unique_ptr<Command> Parser::parse_command() {
    if (is_end()) {
        throw ParserException("Expected a command");
    }

    if (match_keyword("CREATE")) {
        if (match_keyword("DATABASE")) {
            return parse_create_database();
        }
        else if (match_keyword("TABLE")) {
            return parse_create_table();
        }
        else {
            throw ParserException("Expected DATABASE or TABLE after CREATE, got '" + current().value + "'");
        }
    }

    if (match_keyword("DROP")) {
        if (match_keyword("DATABASE")) {
            return parse_drop_database();
        }
        else if (match_keyword("TABLE")) {
            return parse_drop_table();
        }
        else {
            throw ParserException("Expected DATABASE or TABLE after DROP, got '" + current().value + "'");
        }
    }

    if (match_keyword("USE")) {
        return parse_use_database();
    }

    if (match_keyword("INSERT")) {
        return parse_insert_into();
    }

    if (match_keyword("UPDATE")) {
        return parse_update();
    }

    if (match_keyword("DELETE")) {
        return parse_delete_from();
    }

    if (match_keyword("SELECT")) {
        return parse_select();
    }

    throw ParserException("Unknown command starting with '" + current().value + "'");
}

std::unique_ptr<Command> Parser::parse_create_database() {
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected database name");
    }
    std::string db_name = advance().value;

    expect_semicolon();

    return std::make_unique<CreateDatabaseCommand>(db_name);
}

std::unique_ptr<Command> Parser::parse_drop_database() {
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected database name");
    }
    std::string db_name = advance().value;

    expect_semicolon();

    return std::make_unique<DropDatabaseCommand>(db_name);
}

std::unique_ptr<Command> Parser::parse_use_database() {
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected database name");
    }
    std::string db_name = advance().value;

    expect_semicolon();

    return std::make_unique<UseDatabaseCommand>(db_name);
}

std::unique_ptr<Command> Parser::parse_create_table() {
    std::string table_name;
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected table name");
    }
    table_name = advance().value;

    if (match(TypeToken::DOT, ".")) {
        if (!check(TypeToken::IDENTIFIER)) {
            throw ParserException("Expected table name after dot");
        }
        table_name = advance().value;
    }

    expect(TypeToken::LBRACKET, "(");

    std::vector<Column> columns;

    if (!check(TypeToken::RBRACKET)) {
        do {
            columns.push_back(parse_column_definition());
        } while (match(TypeToken::COMMA, ","));

        expect(TypeToken::RBRACKET, ")");
    } else {
        advance();
    }

    expect_semicolon();

    return std::make_unique<CreateTableCommand>(table_name, columns);
}

Column Parser::parse_column_definition() {
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected column name, got '" + current().value + "'");
    }
    std::string col_name = advance().value;

    if (!check(TypeToken::IDENTIFIER) && !check(TypeToken::KEYWORD)) {
        throw ParserException("Expected column type for column '" + col_name + "'");
    }
    std::string type_str = advance().value;
    DataType type = string_to_datatype(type_str);

    bool is_nullable = true;
    bool is_indexed = false;

    while (!is_end() && check(TypeToken::KEYWORD)) {
        if (match_keyword("NOT")) {
            if (match_keyword("NULL")) {
                is_nullable = false;
            } else {
                throw ParserException("Expected NULL after NOT");
            }
        }
        else if (match_keyword("INDEXED")) {
            is_indexed = true;
            is_nullable = false;
        }
        else {
            break;
        }
    }

    return Column(col_name, type, is_nullable, is_indexed);
}

std::unique_ptr<Command> Parser::parse_drop_table() {
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected table name");
    }
    std::string table_name = advance().value;

    if (match(TypeToken::DOT, ".")) {
        if (!check(TypeToken::IDENTIFIER)) {
            throw ParserException("Expected table name after dot");
        }
        table_name = advance().value;
    }

    expect_semicolon();

    return std::make_unique<DropTableCommand>(table_name);
}

std::unique_ptr<Command> Parser::parse_insert_into() {
    expect(TypeToken::KEYWORD, "INTO");

    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected table name");
    }
    std::string table_name = advance().value;

    expect(TypeToken::LBRACKET, "(");
    std::vector<std::string> col_names = parse_identifier_list();
    expect(TypeToken::RBRACKET, ")");

    expect(TypeToken::KEYWORD, "VALUE");

    expect(TypeToken::LBRACKET, "(");
    std::vector<Value> values = parse_value_list();
    expect(TypeToken::RBRACKET, ")");

    expect_semicolon();

    return std::make_unique<InsertIntoCommand>(table_name, col_names, values);
}

std::unique_ptr<Command> Parser::parse_update() {
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected table name");
    }
    std::string table_name = advance().value;

    expect(TypeToken::KEYWORD, "SET");

    std::vector<std::string> col_names;
    std::vector<Value> values;

    do {
        if (!check(TypeToken::IDENTIFIER)) {
            throw ParserException("Expected column name in SET clause");
        }
        std::string col_name = advance().value;

        expect(TypeToken::ASSIGN, "=");

        Value val = parse_value();

        col_names.push_back(col_name);
        values.push_back(std::move(val));

    } while (match(TypeToken::COMMA, ","));

    expect(TypeToken::KEYWORD, "WHERE");
    auto condition = parse_condition();

    expect_semicolon();

    return std::make_unique<UpdateCommand>(table_name, std::move(condition), col_names, values);
}

std::unique_ptr<Command> Parser::parse_delete_from() {
    expect(TypeToken::KEYWORD, "FROM");

    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected table name");
    }
    std::string table_name = advance().value;

    expect(TypeToken::KEYWORD, "WHERE");
    auto condition = parse_condition();

    expect_semicolon();

    return std::make_unique<DeleteFromCommand>(table_name, std::move(condition));
}

std::unique_ptr<Command> Parser::parse_select() {
    std::optional<std::unordered_map<std::string, Alias>> columns_opt;

    if (match(TypeToken::STAR, "*")) {
    }
    else {
        std::unordered_map<std::string, Alias> column_map;

        do {
            std::string col_expr = parse_aggregate_or_column();

            Alias alias = std::nullopt;
            if (match_keyword("AS")) {
                if (!check(TypeToken::IDENTIFIER)) {
                    throw ParserException("Expected alias name after AS");
                }
                alias = advance().value;
            }
            column_map[col_expr] = alias;

        } while (match(TypeToken::COMMA, ","));

        columns_opt = std::move(column_map);
    }

    expect(TypeToken::KEYWORD, "FROM");

    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected table name after FROM");
    }
    std::string table_name = advance().value;

    std::unique_ptr<Condition> condition = nullptr;
    if (match_keyword("WHERE")) {
        condition = parse_condition();
    }

    expect_semicolon();

    if (columns_opt.has_value()) {
        return std::make_unique<SelectCommand>(table_name, std::move(condition), columns_opt);
    }
    else {
        return std::make_unique<SelectCommand>(table_name, std::move(condition));
    }
}

/* Парсит имя колонки или вызов агрегатной функции SUM/COUNT/AVG.
   Возвращает строку — для агрегатов в формате "FUNC(arg)", для колонки просто имя. */
std::string Parser::parse_aggregate_or_column() {
    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected column name or aggregate function, got '" + current().value + "'");
    }

    std::string name = advance().value;

    /* Проверяем, не агрегатная ли это функция */
    std::string upper;
    upper.resize(name.size());
    std::transform(name.begin(), name.end(), upper.begin(), ::toupper);

    if ((upper == "SUM" || upper == "COUNT" || upper == "AVG") && check(TypeToken::LBRACKET)) {
        advance(); /* ( */
        std::string arg;
        if (match(TypeToken::STAR, "*")) {
            arg = "*";
        } else if (check(TypeToken::IDENTIFIER)) {
            arg = advance().value;
        } else {
            throw ParserException("Expected column name or * inside aggregate function");
        }
        expect(TypeToken::RBRACKET, ")");

        /* сохраняем в формате "SUM(age)", "COUNT(*)", "AVG(salary)" */
        return upper + "(" + arg + ")";
    }

    return name;
}

/* ==================== Conditions (precedence: NOT > AND > OR) ==================== */

std::unique_ptr<Condition> Parser::parse_condition() {
    return parse_or_expression();
}

/* OR — lowest precedence */
std::unique_ptr<Condition> Parser::parse_or_expression() {
    auto left = parse_and_expression();

    while (match_keyword("OR")) {
        auto right = parse_and_expression();
        left = std::make_unique<OrCondition>(std::move(left), std::move(right));
    }

    return left;
}

/* AND — medium precedence */
std::unique_ptr<Condition> Parser::parse_and_expression() {
    auto left = parse_unary_expression();

    while (match_keyword("AND")) {
        auto right = parse_unary_expression();
        left = std::make_unique<AndCondition>(std::move(left), std::move(right));
    }

    return left;
}

/* NOT — highest precedence (unary prefix) */
std::unique_ptr<Condition> Parser::parse_unary_expression() {
    if (match_keyword("NOT")) {
        auto operand = parse_unary_expression();
        return std::make_unique<NotCondition>(std::move(operand));
    }

    return parse_primary_condition();
}

/* Primary: ( expr ) or column op value / column BETWEEN / column LIKE */
std::unique_ptr<Condition> Parser::parse_primary_condition() {
    if (match(TypeToken::LBRACKET, "(")) {
        auto expr = parse_or_expression();
        expect(TypeToken::RBRACKET, ")");
        return expr;
    }

    if (!check(TypeToken::IDENTIFIER)) {
        throw ParserException("Expected column name or '(' in condition, got '" + current().value + "'");
    }
    std::string col_name = advance().value;

    if (match_keyword("BETWEEN")) {
        Value start = parse_value();
        expect(TypeToken::KEYWORD, "AND");
        Value end = parse_value();
        return std::make_unique<BetweenCondition>(col_name, std::move(start), std::move(end));
    }

    if (match_keyword("LIKE")) {
        if (!check(TypeToken::STRING)) {
            throw ParserException("Expected string pattern after LIKE");
        }
        std::string pattern = advance().value;

        /* Convert SQL LIKE pattern to regex: % → .*, _ → . */
        std::string regex_pattern;
        regex_pattern.reserve(pattern.size());
        for (char c : pattern) {
            if (c == '%') regex_pattern += ".*";
            else if (c == '_') regex_pattern += ".";
            else if (c == '.' || c == '*' || c == '+' || c == '?' || c == '(' || c == ')' ||
                     c == '[' || c == ']' || c == '^' || c == '$' || c == '|' || c == '\\')
                regex_pattern += '\\', regex_pattern += c;
            else regex_pattern += c;
        }

        return std::make_unique<RegexCondition>(col_name, regex_pattern);
    }

    /* bare column name without operator — treat as "column is truthy" (col != 0) */
    if (is_end() || check(TypeToken::RBRACKET) || check(TypeToken::SEMICOLON) ||
        check(TypeToken::KEYWORD, "AND") || check(TypeToken::KEYWORD, "OR")) {
        return std::make_unique<ComparisonCondition>(col_name, ComparisonDataType::NotEqual, Value(0));
    }

    std::string op;
    if (match(TypeToken::EQ, "==")) op = "==";
    else if (match(TypeToken::NE, "!=")) op = "!=";
    else if (match(TypeToken::LESSTHAN, "<")) op = "<";
    else if (match(TypeToken::GREATERTHAN, ">")) op = ">";
    else if (match(TypeToken::LESSEQUAL, "<=")) op = "<=";
    else if (match(TypeToken::GREATEREQUAL, ">=")) op = ">=";
    else {
        throw ParserException("Expected comparison operator, got '" + current().value + "'");
    }

    ComparisonDataType comp_type = string_to_comparison(op);
    Value val = parse_value();

    return std::make_unique<ComparisonCondition>(col_name, comp_type, val);
}

Value Parser::parse_value() {
    if (check(TypeToken::STRING)) {
        return Value(advance().value);
    }
    else if (check(TypeToken::NUMBER)) {
        std::string num_str = advance().value;
        try {
            return Value(std::stoi(num_str));
        } catch (...) {
            throw ParserException("Invalid number: " + num_str);
        }
    }
    else if (match_keyword("NULL")) {
        return Null{};
    }
    else {
        throw ParserException("Expected a value (string, number, or NULL), got '" + current().value + "'");
    }
}

std::vector<std::string> Parser::parse_identifier_list() {
    std::vector<std::string> ids;

    if (!check(TypeToken::IDENTIFIER)) {
        return ids;
    }

    do {
        ids.push_back(advance().value);
    } while (match(TypeToken::COMMA, ","));

    return ids;
}

std::vector<Value> Parser::parse_value_list() {
    std::vector<Value> values;

    if (is_end() || check(TypeToken::RBRACKET)) {
        return values;
    }

    do {
        values.push_back(parse_value());
    } while (match(TypeToken::COMMA, ","));

    return values;
}
