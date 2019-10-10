// Forwards program invocation to clang or clang++ with the extra needed flags to load the
// smacpp plugin

// Only for posix systems
#include <unistd.h>

#include "integration/SMACPPFinder.h"

#include <boost/process/search_path.hpp>

#include <iostream>
#include <string>


int main(int argc, char* argv[])
{
    if(argc < 1)
        return 1;

    const char* clangExecutable = "clang";

    if(std::string(argv[0]).find_last_of("++") != std::string::npos)
        clangExecutable = "clang++";

    const std::string clangPath = boost::process::search_path(clangExecutable).string();

    if(clangPath.empty()) {
        std::cout << "Could not find '" << clangExecutable << "' in PATH\n";
        return 2;
    }

    std::vector<char*> newArgs;
    newArgs.resize(argc);

    for(int i = 1; i < argc; ++i)
        newArgs[i] = argv[i];

    newArgs[0] = const_cast<char*>(clangPath.c_str());

    // Plugin loading arguments
    std::vector<std::string> newArgumentValues;

    bool pluginLoadingAlreadyPresent = false;

    // TODO: check if the plugin loading args already exist

    if(!pluginLoadingAlreadyPresent) {

        const auto plugin = smacpp::FindSMACPPClangPlugin(argv[0]);

        if(plugin.empty()) {
            std::cout
                << "Could not find Static Memory Analyzer For C++ clang plugin library\n";
            return 2;
        }

        newArgumentValues.push_back("-fplugin=" + plugin);
    }

    for(size_t i = 0; i < newArgumentValues.size(); ++i) {

        // Put the loading args before any other arguments
        newArgs.insert(newArgs.begin() + 1, const_cast<char*>(newArgumentValues[i].c_str()));
    }

    // Null to terminate the list
    newArgs.push_back(nullptr);

    // Execute clang
    int status = execv(clangPath.c_str(), newArgs.data());

    std::cout << "Error calling exec with clang executable, error: " << errno << "\n";
    return status;
}
