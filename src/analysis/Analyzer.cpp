// ------------------------------------ //
#include "Analyzer.h"

#include "parse/CodeBlock.h"

#include <sstream>

using namespace smacpp;
// ------------------------------------ //
// FoundProblem
FoundProblem::FoundProblem(SEVERITY severity, const std::string& message) :
    Severity(severity), Message(message)
{}
// ------------------------------------ //
std::string FoundProblem::FormatAsString() const
{
    std::stringstream sstream;

    switch(Severity) {
    case SEVERITY::Info: sstream << "info:"; break;
    case SEVERITY::Warning: sstream << "warning:"; break;
    case SEVERITY::Error: sstream << "error:"; break;
    }

    sstream << " " << Message;
    return sstream.str();
}
// ------------------------------------ //
// Analyzer
Analyzer::Analyzer(std::vector<FoundProblem>& reportProblems) : Problems(reportProblems) {}
// ------------------------------------ //
bool Analyzer::BeginAnalysis(const CodeBlock& entryPoint,
    const BlockRegistry* availableFunctions, const std::vector<VariableState>& callParameters)
{

    return true;
}
