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
    BETWEEN,
    ASSIGN
};

struct Token {
    TypeToken type; // тип токена - слово которое разбиваем
    std::string value;
};

// Ключевые слова с правилами
struct kw_create : TAO_PEGTL_STRING("CREATE") {
};

struct kw_database : TAO_PEGTL_STRING("DATABASE") {
};

struct kw_drop : TAO_PEGTL_STRING("DROP") {
};

struct kw_use : TAO_PEGTL_STRING("USE") {
};

struct kw_table : TAO_PEGTL_STRING("TABLE") {
};

struct kw_not : TAO_PEGTL_STRING("NOT") {
};

struct kw_null : TAO_PEGTL_STRING("NULL") {
};

struct kw_indexed : TAO_PEGTL_STRING("INDEXED") {
};

struct kw_insert : TAO_PEGTL_STRING("INSERT") {
};

struct kw_into : TAO_PEGTL_STRING("INTO") {
};

struct kw_value : TAO_PEGTL_STRING("VALUE") {
};

struct kw_update : TAO_PEGTL_STRING("UPDATE") {
};

struct kw_set : TAO_PEGTL_STRING("SET") {
};

struct kw_delete : TAO_PEGTL_STRING("DELETE") {
};

struct kw_from : TAO_PEGTL_STRING("FROM") {
};

struct kw_select : TAO_PEGTL_STRING("SELECT") {
};

struct kw_where : TAO_PEGTL_STRING("WHERE") {
};

struct kw_as : TAO_PEGTL_STRING("AS") {
};

struct kw_and : TAO_PEGTL_STRING("AND") {
};

struct kw_like : TAO_PEGTL_STRING("LIKE") {
};

struct kw_between : TAO_PEGTL_STRING("BETWEEN") {
};


struct keyword : pegtl::sor<
            kw_create, kw_database, kw_drop, kw_use, kw_table,
            kw_not, kw_null, kw_indexed, kw_insert, kw_into,
            kw_value, kw_update, kw_set, kw_delete, kw_from, kw_select,
            kw_where, kw_as, kw_and, kw_like, kw_between
        > {
};

/* Структура  идентификатора (Имя столбца либо название бд либо еще что)
 *Реализуем, что первый символ не цифра
 * ВАЖНО РЕАЛИЗОВАТЬ РАЗДЕЛЕНИЕ ПО ;
 */
struct identificator : pegtl::seq<
            pegtl::sor<pegtl::ascii::alpha, pegtl::one<'_'> >,
            pegtl::star<pegtl::sor<pegtl::ascii::alnum, pegtl::one<'_'> > >
        > {
};

// Cтруктура чисел
struct number : pegtl::plus<pegtl::ascii::digit> {
};

// строковый литерал заключается в двойные кавычки:
struct string_literal : pegtl::seq<
            pegtl::one<'"'>,
            pegtl::star<pegtl::not_one<'"'> >,
            pegtl::one<'"'>
        > {
};

// Спецсимволы
struct star_s : pegtl::one<'*'> {
};

struct comma_s : pegtl::one<','> {
};

struct lbracket_s : pegtl::one<'('> {
};

struct rbracket_s : pegtl::one<')'> {
};

struct dot_s : pegtl::one<'.'> {
};

struct semicolon_s : pegtl::one<';'> {
};

struct assign_s : pegtl::one<'='> {

};

//Сравнение
struct eq_s : TAO_PEGTL_STRING("==") {
};

struct ne_s : TAO_PEGTL_STRING("!=") {
};

struct greater_equal_s : TAO_PEGTL_STRING(">=") {
};

struct less_equal_s : TAO_PEGTL_STRING("<=") {
};

struct greater_s : pegtl::one<'>'> {
};

struct less_s : pegtl::one<'<'> {
};

// Итоговая структура сравнения
struct comparison : pegtl::sor<greater_equal_s, less_equal_s, eq_s, ne_s, greater_s, less_s> {
};

struct token : pegtl::sor<
            comparison,
            assign_s,
            string_literal,
            number,
            star_s,
            comma_s,
            lbracket_s,
            rbracket_s,
            dot_s,
            semicolon_s,
            keyword,
            identificator
        > {
};

// Лексер на вход получает строку и превращает ее в токен выше
class Lexer {
public:
    std::vector<Token> tokenize(const std::string &input);
};

#endif //MINI_DB_PARSER_H
