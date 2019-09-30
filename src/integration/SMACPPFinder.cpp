// ------------------------------------ //
#include "SMACPPFinder.h"

#include <boost/filesystem.hpp>

using namespace smacpp;
// ------------------------------------ //
std::string smacpp::FindSMACPPClangPlugin(const std::string& currentExecutable)
{
#if _WIN32
    constexpr auto pluginFileName = "libsmacpp-clang-plugin.so";
#else
    constexpr auto pluginFileName = "libsmacpp-clang-plugin.so";
#endif

    auto check = boost::filesystem::path(currentExecutable).parent_path() / pluginFileName;

    if(boost::filesystem::exists(check))
        return check.string();

    const std::array<boost::filesystem::path, 4> searchPaths = {
        "/usr/bin", "/usr/local/bin", "/usr/lib64", "/usr/lib"};

    for(const auto search : searchPaths) {

        check = search / pluginFileName;

        if(boost::filesystem::exists(check))
            return check.string();
    }

    return "";
}
