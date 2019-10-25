#pragma once

#include "parse/ProcessedAction.h"


#include <string>
#include <vector>

namespace smacpp {

class CodeBlock;
class BlockRegistry;

struct FoundProblem {
    enum class SEVERITY { Info, Warning, Error };

    //! Basic message with no location info
    FoundProblem(SEVERITY severity, const std::string& message);

    std::string FormatAsString() const;

    std::string Message;
    SEVERITY Severity;
};


//! Main class implementing the actual static analysis checks
class Analyzer {
public:
    Analyzer(std::vector<FoundProblem>& reportProblems);

    //! \brief Starts full analysis from the specified function
    //! \param availableFunctions If non-null this is used to find functions that entryPoint
    //! calls, and descend the analysis into them
    //! \param callParameters the parameters that are passed to entryPoint
    //! \returns True if analysis ran correctly. False if a fatal error was encountered
    bool BeginAnalysis(const CodeBlock& entryPoint, const BlockRegistry* availableFunctions,
        const std::vector<VariableState>& callParameters);

private:
    std::vector<FoundProblem>& Problems;
};

} // namespace smacpp
