//
// Created by rvova on 26.04.2026.
//

#ifndef MINIDB_EXCEPTIONS_H
#define MINIDB_EXCEPTIONS_H
#include <exception>
#include <string>

class InvalidOpenFileException : public std::runtime_error {
public:
    explicit InvalidOpenFileException(const std::string &msg) : std::runtime_error(msg) {
    }
};


class InvalidWriteToPageException : public std::runtime_error {
public:
    explicit InvalidWriteToPageException(const std::string &msg) : std::runtime_error(msg) {
    }
};


class SlotNotFoundException : public std::runtime_error {
public:
    explicit SlotNotFoundException(const std::string &msg) : std::runtime_error(msg) {
    }
};


class InvalidSlotEraseException : public std::runtime_error {
public:
    explicit InvalidSlotEraseException(const std::string &msg) : std::runtime_error(msg) {
    }
};


class DatabaseIsNotChosenException : public std::exception {
public:
    const char *what() const noexcept override {
        return "Database is not selected, either USE before the query, or add the database name";
    }
};

class DatabaseHasAlreadyExistsException : public std::runtime_error {
public:
    explicit DatabaseHasAlreadyExistsException(const std::string &name) : std::runtime_error(
        "The database " + name + " already exists.") {
    }
};

class DatabaseDoesNotExistException : public std::runtime_error {
public:
    explicit DatabaseDoesNotExistException(const std::string &name) : std::runtime_error(
        "The database " + name + " doesn't exist.") {
    }
};

class LexerException : public std::runtime_error {
public:
    explicit LexerException(const std::string &msg) : std::runtime_error(msg) {
    }
};

class TableHasAlreadyExistsException : public std::runtime_error {
public:
    explicit TableHasAlreadyExistsException(const std::string &name) : std::runtime_error(
        "The table " + name + " already exists.") {
    }
};

class TableDoesNotExistException : public std::runtime_error {
public:
    explicit TableDoesNotExistException(const std::string &name) : std::runtime_error(
        "The table " + name + " does not exist.") {
    }
};

class FailedCreateTableException : public std::runtime_error {
public:
    explicit FailedCreateTableException(const std::string &msg) : std::runtime_error(msg) {
    }
};

class FailedInsertElementsToTableException : public std::runtime_error {
public:
    explicit FailedInsertElementsToTableException(const std::string &msg) : std::runtime_error(msg) {
    }
};

class FailedUpdateElementsToTableException : public std::runtime_error {
public:
    explicit FailedUpdateElementsToTableException(const std::string &msg) : std::runtime_error(msg) {
    }
};

class FailedDeleteElementsToTableException : public std::runtime_error {
public:
    explicit FailedDeleteElementsToTableException(const std::string &msg) : std::runtime_error(msg) {
    }
};

class FailedStartSocketException : public std::runtime_error {
public:
    explicit FailedStartSocketException() : std::runtime_error("Failed to start socket") {
    }
};

class FailedSendDataException : public std::runtime_error {
public:
    explicit FailedSendDataException() : std::runtime_error("Failed to send data") {
    }
};

class FailedReceiveDataException : public std::runtime_error {
public:
    explicit FailedReceiveDataException() : std::runtime_error("Failed to receive data") {
    }
};

class InternalServerErrorException : public std::runtime_error {
public:
    explicit InternalServerErrorException() : std::runtime_error("Internal error!") {
    }
};

class FailedStartEntrypointException : public std::runtime_error {
public:
    explicit FailedStartEntrypointException() : std::runtime_error("Failed to start entrypoint") {
    }
};

class ColumnNotFoundInTableException : public std::runtime_error {
public:
    explicit ColumnNotFoundInTableException(const std::string &column_name) : std::runtime_error(
        "Column " + column_name + "not found!") {
    }
};

class RedundantColumnException : public std::runtime_error {
public:
    explicit RedundantColumnException(const std::string &column_name) : std::runtime_error(
        "Column" + column_name + "repeated") {
    }
};

#endif //MINIDB_EXCEPTIONS_H
