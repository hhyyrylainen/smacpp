// Tests for making sure that clang can load the plugins
#include "catch.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>

#include <algorithm>

constexpr auto SMACP_PATH = "src/smacpp";
constexpr auto SMACP_PLUGIN_PATH = "src/libsmacpp-clang-plugin.so";

namespace bp = boost::process;

TEST_CASE("Normal clang help print works", "[plugin]")
{
    boost::asio::io_service ios;

    std::future<std::string> data;

    CHECK(!boost::process::search_path("clang").empty());

    bp::child c(boost::process::search_path("clang"), "--help", bp::std_out > data, ios);

    ios.run();

    auto output = data.get();

    c.wait();
    int result = c.exit_code();

    CHECK(result == 0);
    CHECK(output.find_first_of("OPTIONS:") != std::string::npos);
}

TEST_CASE("Clang wrapper loads clang and prints help info", "[plugin]")
{
    REQUIRE(boost::filesystem::exists(SMACP_PATH));

    boost::asio::io_service ios;

    std::future<std::string> data;

    bp::child c(boost::filesystem::absolute(SMACP_PATH), "--help", bp::std_out > data, ios);

    ios.run();

    auto output = data.get();

    c.wait();
    int result = c.exit_code();

    CHECK(result == 0);
    CHECK(output.find_first_of("Help for smacpp plugin") != std::string::npos);
}

TEST_CASE("Normal plugin load syntax works", "[plugin]")
{
    REQUIRE(boost::filesystem::exists(SMACP_PLUGIN_PATH));

    boost::asio::io_service ios;

    std::future<std::string> data;

    CHECK(!boost::process::search_path("clang").empty());

    bp::child c(boost::process::search_path("clang"),
        "-fplugin=" + boost::filesystem::absolute(SMACP_PLUGIN_PATH).string(), "--help",
        bp::std_out > data, ios);

    ios.run();

    auto output = data.get();

    c.wait();
    int result = c.exit_code();

    CHECK(result == 0);
    CHECK(output.find_first_of("Help for smacpp plugin") != std::string::npos);
}

TEST_CASE("Manual plugin specifying (-Xclang) syntax works", "[plugin]")
{
    REQUIRE(boost::filesystem::exists(SMACP_PLUGIN_PATH));

    boost::asio::io_service ios;

    std::future<std::string> data;

    CHECK(!boost::process::search_path("clang").empty());

    bp::child c(boost::process::search_path("clang"), "-Xclang", "-load", "-Xclang",
        boost::filesystem::absolute(SMACP_PLUGIN_PATH).string(), "-Xclang", "-plugin",
        "-Xclang", "smacpp", "--help", bp::std_out > data, ios);

    ios.run();

    auto output = data.get();

    c.wait();
    int result = c.exit_code();

    CHECK(result == 0);
    CHECK(output.find_first_of("Help for smacpp plugin") != std::string::npos);
}
