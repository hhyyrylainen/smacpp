// Tests for making sure that clang can load the plugins
#include "catch.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>

#include <algorithm>

constexpr auto SMACPP_PATH = "src/smacpp";
constexpr auto SMACPP_PLUGIN_PATH = "src/libsmacpp-clang-plugin.so";
constexpr auto SMACPP_ANALYZER_PLUGIN = "src/libsmacpp-clang-analyzer.so";

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
    REQUIRE(boost::filesystem::exists(SMACPP_PATH));

    boost::asio::io_service ios;

    std::future<std::string> data;

    bp::child c(boost::filesystem::absolute(SMACPP_PATH), "--help", bp::std_out > data, ios);

    ios.run();

    auto output = data.get();

    c.wait();
    int result = c.exit_code();

    CHECK(result == 0);
    CHECK(output.find_first_of("Help for smacpp plugin") != std::string::npos);
}

TEST_CASE("Normal plugin load syntax works", "[plugin]")
{
    REQUIRE(boost::filesystem::exists(SMACPP_PLUGIN_PATH));

    boost::asio::io_service ios;

    std::future<std::string> data;

    CHECK(!boost::process::search_path("clang").empty());

    bp::child c(boost::process::search_path("clang"),
        "-fplugin=" + boost::filesystem::absolute(SMACPP_PLUGIN_PATH).string(), "--help",
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
    REQUIRE(boost::filesystem::exists(SMACPP_PLUGIN_PATH));

    boost::asio::io_service ios;

    std::future<std::string> data;

    CHECK(!boost::process::search_path("clang").empty());

    bp::child c(boost::process::search_path("clang"), "-Xclang", "-load", "-Xclang",
        boost::filesystem::absolute(SMACPP_PLUGIN_PATH).string(), "-Xclang", "-plugin",
        "-Xclang", "smacpp", "--help", bp::std_out > data, ios);

    ios.run();

    auto output = data.get();

    c.wait();
    int result = c.exit_code();

    CHECK(result == 0);
    CHECK(output.find_first_of("Help for smacpp plugin") != std::string::npos);
}

TEST_CASE("Loading static analyzer plugin works", "[plugin]")
{
    REQUIRE(boost::filesystem::exists(SMACPP_ANALYZER_PLUGIN));

    boost::asio::io_service ios;

    std::future<std::string> data;

    CHECK(!boost::process::search_path("clang").empty());

    bp::child c(boost::process::search_path("clang"), "-cc1", "-load",
        boost::filesystem::absolute(SMACPP_PLUGIN_PATH).string(), "-analyzer-checker-help",
        bp::std_out > data, ios);

    ios.run();

    auto output = data.get();

    c.wait();
    int result = c.exit_code();

    CHECK(result == 0);
    CHECK(output.find_first_of("SMACPP") != std::string::npos);
}
