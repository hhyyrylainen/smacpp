#pragma once

#include <string>

namespace smacpp {

//! \brief Tries to find the smacpp plugin for clang
//!
//! Searches first relative to the current path (if it isn't empty) and then globally
std::string FindSMACPPClangPlugin(const std::string& currentExecutable);

} // namespace smacpp
