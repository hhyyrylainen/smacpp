#pragma once

#include "Condition.h"
#include "ProcessedAction.h"

#include <clang/AST/Stmt.h>

namespace smacpp {

//! Represents a block of source code that has properties extracted from it
class CodeBlock {
public:
    CodeBlock(const std::string& qualifiedName, clang::SourceLocation location) :
        Name(qualifiedName), Location(location)
    {}

    //! \brief All potentially unsafe calls, and variable state changes are added through this
    //!
    //! Everything is bunched together like this in order to be able to determine the variable
    //! states at the potentially unsafe operations in order to verify the conditions under
    //! which they are unsafe
    void AddProcessedAction(std::unique_ptr<ProcessedAction>&& action,
        clang::SourceLocation location = clang::SourceLocation{})
    {
        if(location.isValid())
            action->Location = location;
        Actions.push_back(std::move(action));
    }

    //! \brief Register function parameter
    void AddFunctionParameter(VariableIdentifier var)
    {
        FunctionParameters.push_back(var);
    }

    const auto& GetActions() const
    {
        return Actions;
    }

    const auto& GetParameters() const
    {
        return FunctionParameters;
    }

    // //! \brief Computes an overall Condition that if it matches this is unsafe to call
    // Condition ComputeUnsafeInput();

    const auto& GetName() const
    {
        return Name;
    }

    std::string Dump() const;

    const auto GetLocation() const
    {
        return Location;
    }

private:
    std::string Name;
    clang::SourceLocation Location;

    //! \todo Find default values
    std::vector<VariableIdentifier> FunctionParameters;

    //! All actions in chronological order in order to be able to do symbolic execution
    //! correctly
    std::vector<std::unique_ptr<ProcessedAction>> Actions;
};

} // namespace smacpp
