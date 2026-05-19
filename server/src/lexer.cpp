#include "lexer.h"
#include <tao/pegtl.hpp>
#include "exceptions.h"
namespace pegtl = tao::pegtl;


template<typename Rule>
struct action : pegtl::nothing<Rule> {};  // ЗАГЛУШКА - ДЕЛАТЬ НИЧЕГО ЕСЛИ ЧИТАЕМ

template<> struct action<keyword> { // ACTION для KEYWORD, видим KEYWORD тогда вызываем apply и добаляем объект токен в конец вкетора
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::KEYWORD, in.string()});
    }
};

template<> struct action<identificator> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::IDENTIFIER, in.string()});
    }
};

template<> struct action<string_literal> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        std::string s = in.string();
        v.push_back({TypeToken::STRING, s.substr(1, s.size() - 2)});
    }
};

template<> struct action<number> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::NUMBER, in.string()});
    }
};

template<> struct action<star_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::STAR, "*"});
    }
};

template<> struct action<comma_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::COMMA, ","});
    }
};

template<> struct action<lbracket_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::LBRACKET, "("});
    }
};

template<> struct action<rbracket_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::RBRACKET, ")"});
    }
};

template<> struct action<dot_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::DOT, "."});
    }
};

template<> struct action<semicolon_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::SEMICOLON, ";"});
    }
};

template<> struct action<eq_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::EQ, "=="});
    }
};

template<> struct action<ne_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::NE, "!="});
    }
};

template<> struct action<less_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::LESSTHAN, "<"});
    }
};

template<> struct action<greater_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::GREATERTHAN, ">"});
    }
};

template<> struct action<less_equal_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::LESSEQUAL, "<="});
    }
};

template<> struct action<greater_equal_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::GREATEREQUAL, ">="});
    }
};

template<> struct action<assign_s> {
    template<typename Input>
    static void apply(const Input& in, std::vector<Token>& v) {
        v.push_back({TypeToken::ASSIGN, "="});
    }
};


struct grammar : pegtl::seq<
    pegtl::star<pegtl::space>,
    pegtl::until<pegtl::eof, pegtl::must<pegtl::sor<token, pegtl::space>>>,
    pegtl::eof
> {};

std::vector<Token> Lexer::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    if (input.empty()) return tokens;
    try {
        pegtl::memory_input<> in(input, "lexer_sql");
        pegtl::parse<grammar, action>(in, tokens);
    } catch (const pegtl::parse_error& e) {

        throw LexerException(e.what());
    }
    return tokens;
}