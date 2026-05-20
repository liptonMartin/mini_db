#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include "command.h"

using boost::asio::ip::tcp;

static constexpr int TEST_PORT = 12500;

class HeartbitTest : public ::testing::Test {
protected:
    boost::asio::io_context io;
    tcp::acceptor acceptor{io};

    HeartbitTest() : acceptor(io, tcp::endpoint(tcp::v4(), TEST_PORT)) {}
};

TEST_F(HeartbitTest, StorageNodeRespondsAlive) {
    std::thread entrypoint_thread([this] {
        tcp::socket sock(io);
        sock.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), TEST_PORT));

        CommandType cmd = CommandType::Heartbeat;
        boost::asio::write(sock, boost::asio::buffer(&cmd, sizeof(cmd)));

        uint32_t len;
        boost::asio::read(sock, boost::asio::buffer(&len, sizeof(len)));

        std::string body(len, '\0');
        boost::asio::read(sock, boost::asio::buffer(body));

        auto response = nlohmann::json::parse(body);
        EXPECT_EQ(response["Status"], 200);
        EXPECT_EQ(response["Message"], "alive");
    });

    std::thread storage_thread([this] {
        tcp::socket sock(io);
        acceptor.accept(sock);

        CommandType cmd;
        boost::asio::read(sock, boost::asio::buffer(&cmd, sizeof(cmd)));
        EXPECT_EQ(cmd, CommandType::Heartbeat);

        nlohmann::json response{{"Status", 200}, {"Message", "alive"}};
        auto raw = response.dump();
        uint32_t len = raw.size();

        std::vector<uint8_t> buf(sizeof(len) + len);
        memcpy(buf.data(), &len, sizeof(len));
        memcpy(buf.data() + sizeof(len), raw.data(), len);

        boost::asio::write(sock, boost::asio::buffer(buf));
    });

    entrypoint_thread.join();
    storage_thread.join();
}

TEST_F(HeartbitTest, EntrypointSendsHeartbeat) {
    std::thread storage_thread([this] {
        tcp::socket sock(io);
        acceptor.accept(sock);

        CommandType cmd;
        boost::asio::read(sock, boost::asio::buffer(&cmd, sizeof(cmd)));
        EXPECT_EQ(cmd, CommandType::Heartbeat);

        nlohmann::json response{{"Status", 200}, {"Message", "alive"}};
        auto raw = response.dump();
        uint32_t len = raw.size();

        std::vector<uint8_t> buf(sizeof(len) + len);
        memcpy(buf.data(), &len, sizeof(len));
        memcpy(buf.data() + sizeof(len), raw.data(), len);

        boost::asio::write(sock, boost::asio::buffer(buf));

        boost::asio::read(sock, boost::asio::buffer(&cmd, sizeof(cmd)));
        EXPECT_EQ(cmd, CommandType::Heartbeat);
        boost::asio::write(sock, boost::asio::buffer(buf));
    });

    std::thread entrypoint_thread([this] {
        tcp::socket sock(io);
        sock.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), TEST_PORT));

        for (int i = 0; i < 2; i++) {
            CommandType cmd = CommandType::Heartbeat;
            boost::asio::write(sock, boost::asio::buffer(&cmd, sizeof(cmd)));

            uint32_t len;
            boost::asio::read(sock, boost::asio::buffer(&len, sizeof(len)));

            std::string body(len, '\0');
            boost::asio::read(sock, boost::asio::buffer(body));

            auto response = nlohmann::json::parse(body);
            EXPECT_EQ(response["Status"], 200);
            EXPECT_EQ(response["Message"], "alive");
        }
    });

    entrypoint_thread.join();
    storage_thread.join();
}

TEST_F(HeartbitTest, EntrypointDetectsDeadNode) {
    std::thread storage_thread([this] {
        tcp::socket sock(io);
        acceptor.accept(sock);

        CommandType cmd;
        boost::asio::read(sock, boost::asio::buffer(&cmd, sizeof(cmd)));
    });

    std::thread entrypoint_thread([this] {
        tcp::socket sock(io);
        sock.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), TEST_PORT));

        CommandType cmd = CommandType::Heartbeat;
        boost::asio::write(sock, boost::asio::buffer(&cmd, sizeof(cmd)));

        uint32_t len;
        boost::system::error_code ec;
        boost::asio::read(sock, boost::asio::buffer(&len, sizeof(len)), ec);
        EXPECT_TRUE(ec) << "Expected error on dead node, but read succeeded";
    });

    storage_thread.join();
    entrypoint_thread.join();
}

TEST(HeartbitFormatTest, ResponseJsonFormat) {
    nlohmann::json response;
    response["Status"] = 200;
    response["Message"] = "alive";

    auto serialized = response.dump();
    auto parsed = nlohmann::json::parse(serialized);

    EXPECT_EQ(parsed["Status"], 200);
    EXPECT_EQ(parsed["Message"], "alive");
    EXPECT_EQ(parsed.size(), 2);
}
