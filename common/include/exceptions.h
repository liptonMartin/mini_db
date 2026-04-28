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
    explicit InvalidOpenFileException(const std::string &msg) : message(msg) {
    }

    const char *what() const noexcept override {
        return message.c_str();
    }
};


class InvalidWriteToPageException : public std::exception {
    std::string message;

public:
    explicit InvalidWriteToPageException(const std::string &msg) : message(msg) {
    }

    const char *what() const noexcept override {
        return message.c_str();
    }
};


class SlotNotFoundException : public std::exception {
    std::string message;

public:
    explicit SlotNotFoundException(const std::string &msg) : message(msg) {
    }

    const char *what() const noexcept override {
        return message.c_str();
    }
};


class InvalidSlotEraseException : public std::exception {
    std::string message;

public:
    explicit InvalidSlotEraseException(const std::string &msg) : message(msg) {
    }

    const char *what() const noexcept override {
        return message.c_str();
    }
};


class DatabaseIsNotChosenException : public std::exception {

public:
    const char *what() const noexcept override {
        return "Database is not selected, either USE before the query, or add the database name";
    }
};

class DatabaseHasAlreadyExistsException : public std::exception {
    std::string name;

    public:
    explicit DatabaseHasAlreadyExistsException(const std::string &name) : name(name) {}

    const char *what() const noexcept override {
        return ("The database " + name + " already exists.").c_str();
    }
};

class LexerException : public std::exception {
    std::string message;

public:
    explicit LexerException(const std::string& msg) : message(msg) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

};


#endif //MINIDB_EXCEPTIONS_H
