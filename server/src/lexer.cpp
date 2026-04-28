#include "lexer.h"
#include <tao/pegtl.hpp>

namespace pegtl = tao::pegtl;


template<typename Rule>
struct action : pegtl::nothing<Rule> {};  // ЗАГЛУШКА - ДЕЛАТЬ НИЧЕГО ЕСЛИ ЧИТАЕМ

template<> struct action<keyword> { // ACTION для KEYWORD, видим KEYWORD тогда вызываем apply и добаляем объект токен в конец вкетора
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::KEYWORD, in.string()});
    }
};

// TODO: НАКЛЕПАТЬ ТАКИХ ЖЕ ЭКШНОВ ДЛЯ КАЖДОГО ТОКЕНТАЙПА - обработать их ошибки, В ТОКЕНАЙЗ И ПРИДУМАТЬ ЧТО ДЕЛАТЬ ДАЛЬШЕ
struct spaces : pegtl::star<pegtl::space> {};
struct grammar : pegtl::list<token, spaces> {};

std::vector<Token> Lexer::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    try {
        pegtl::memory_input<> in(input, "lexer_sql");
        pegtl::parse<grammar, action>(in, tokens);
    } catch (const pegtl::parse_error& e) {
        throw LexerException(
            "Unexpected character at position " +
            std::to_string(e.positions.front().byte)
        );
    }
    return tokens;
}