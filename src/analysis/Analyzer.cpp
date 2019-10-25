// ------------------------------------ //
#include "Analyzer.h"

#include "parse/CodeBlock.h"

#include <sstream>

// DEBUGGING CODE
#include <iostream>

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
    std::list<AnalysisOperation> toCheck;

    {
        AnalysisOperation entryAnalysis(entryPoint.GetActions());

        if(callParameters.size() != entryPoint.GetParameters().size()) {

            Problems.push_back(FoundProblem(FoundProblem::SEVERITY::Error,
                "given parameters count mismatches analysis entrypoint parameter count"));
            return false;
        }

        for(size_t i = 0; i < callParameters.size(); ++i) {
            entryAnalysis.State->CreateLocal(entryPoint.GetParameters()[i], callParameters[i]);
        }

        toCheck.push_back(std::move(entryAnalysis));
    }

    while(!toCheck.empty()) {

        const auto result = PerformAnalysisOperation(toCheck.front());

        if(!std::get<0>(result)) {
            Problems.push_back(
                FoundProblem(FoundProblem::SEVERITY::Error, "an analysis step failed"));
            return false;
        }

        const auto& newOps = std::get<1>(result);

        if(!newOps.empty()) {

            for(auto&& op : newOps)
                toCheck.push_back(std::move(op));
        }

        toCheck.pop_front();
    }

    return true;
}
// ------------------------------------ //
std::tuple<bool, std::list<Analyzer::AnalysisOperation>> Analyzer::PerformAnalysisOperation(
    AnalysisOperation& operation)
{
    std::list<AnalysisOperation> foundCalls;

    // TODO: when a conditional is uncertain the check needs to be split into two here to
    // independently check both uncertain outcomes

    for(auto iter = operation.Actions.begin(); iter != operation.Actions.end(); ++iter) {

        const ProcessedAction& action = **iter;

        std::cout << "analysis at step: " << action.Dump() << "\n";
    }

    return std::make_tuple(true, foundCalls);
}
