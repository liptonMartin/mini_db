//
// Created by MialistaPC on 26.04.2026.
//

#ifndef MINI_DB_PARSER_H
#define MINI_DB_PARSER_H

#include <tao/pegtl.hpp>
#include <vector>
#include <string>

namespace pegtl = tao::pegtl;

// CREATE DATABASE my_db; -> Vector ["create', 'Database', 'my_db;]
enum class TypeToken {
    KEYWORD, // Сюда пишем CREATE DROP INSERT UPDATE DELETE SELECT и тд
    IDENTIFIER, // имена, столбцы
    //спецсимволы
    STRING,
    NUMBER,
    STAR,
    COMMA,
    LBRACKET,
    RBRACKET,
    DOT,
    SEMICOLON,
    //Операторы сравнения
    EQ,
    NE,
    LESSTHAN,
    GREATERTHAN,
    LESSEQUAL,
    GREATEREQUAL,
    BETWEEN
};

struct Token {
    TypeToken type; // тип токена - слово которое разбиваем
    std::string value;
};


#endif //MINI_DB_PARSER_H
