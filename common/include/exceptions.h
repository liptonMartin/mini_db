//
// Created by rvova on 26.04.2026.
//

#ifndef MINIDB_EXCEPTIONS_H
#define MINIDB_EXCEPTIONS_H
#include <exception>
#include <string>

class InvalidOpenFileException : public std::exception {
    std::string message;

public:
    explicit InvalidOpenFileException(const std::string& msg) : message(msg) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};

#endif //MINIDB_EXCEPTIONS_H
