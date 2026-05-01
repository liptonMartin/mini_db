//
// Created by rvova on 01.05.2026.
//
#include <gtest/gtest.h>
#include "request_handler.h"


class RequestHandlerFixture : public ::testing::Test {
protected:
    StorageEngine engine_;
    RequestHandler request_handler{std::move(engine_)};
};

TEST_F(RequestHandlerFixture, TestRequestHandler) {
    auto create_database_command = std::make_unique<CreateDatabaseCommand>(CreateDatabaseCommand("test_database"));
    auto drop_database_command = std::make_unique<DropDatabaseCommand>(DropDatabaseCommand("test_database"));

    std::vector<std::unique_ptr<Command>> commands;
    commands.push_back(std::move(create_database_command));
    commands.push_back(std::move(drop_database_command));

    auto result = request_handler.process_request(commands);

    std::cout << result << std::endl;
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}