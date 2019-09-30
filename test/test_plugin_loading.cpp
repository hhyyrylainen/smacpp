// Tests for making sure that clang can load the plugins
#include "catch.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>

#include <algorithm>

constexpr auto SMACP_PATH = "src/smacpp";

namespace bp = boost::process;


TEST_CASE("Test clang wrapper loads clang and prints help info", "[plugin]")
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
