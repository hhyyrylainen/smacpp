#pragma once

#include "Condition.h"

#include <clang/AST/Stmt.h>

namespace smacpp {

class VariableState {};

class VariableStates {
public:
    std::vector<std::tuple<Condition, VariableState>> StateChanges;
};

//! Represents a block of source code that has properties extracted from it
class CodeBlock {
public:
    std::vector<VariableIdentifier> FunctionParameters;
    std::vector<std::tuple<VariableIdentifier, VariableStates>> Variables;
};

} // namespace smacpp
