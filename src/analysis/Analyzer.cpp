// ------------------------------------ //
#include "Analyzer.h"

#include "BlockRegistry.h"
#include "parse/CodeBlock.h"
#include "parse/ProcessedAction.h"

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
// Analyzer::ProgramState
void Analyzer::ProgramState::CreateLocal(
    VariableIdentifier identifier, VariableState initialState)
{
    // TODO: allow shadowing globals and locals defined in upper scope
    Variables[identifier] = initialState;
}
// ------------------------------------ //
bool Analyzer::ProgramState::MatchesCondition(const Condition& condition) const
{
    return condition.Evaluate(*this);
}

VariableState Analyzer::ProgramState::GetVariableValue(
    const VariableIdentifier& variable) const
{
    const auto found = Variables.find(variable);

    if(found == Variables.end()) {
        return VariableState();
    }

    // TODO: resolve copyvar
    if(found->second.State == VariableState::STATE::CopyVar) {
    }

    return found->second;
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
        AnalysisOperation entryAnalysis(entryPoint.GetActions(), availableFunctions);

        if(!ResolveCallParameters(entryAnalysis, entryPoint, callParameters)) {
            Problems.push_back(FoundProblem(FoundProblem::SEVERITY::Error,
                "given parameters count mismatches analysis entrypoint parameter count"));
            return false;
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
bool Analyzer::ResolveCallParameters(AnalysisOperation& operation, const CodeBlock& function,
    const std::vector<VariableState>& callParameters)
{
    if(callParameters.size() != function.GetParameters().size()) {
        return false;
    }

    for(size_t i = 0; i < callParameters.size(); ++i) {
        operation.State->CreateLocal(function.GetParameters()[i], callParameters[i]);
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

        if(operation.State->MatchesCondition(action.If)) {

            std::cout << "analysis at step: " << action.Dump() << "\n";

            const action::FunctionCall* func =
                dynamic_cast<const action::FunctionCall*>(&action);

            if(func) {

                const CodeBlock* calledFunction =
                    operation.AvailableFunctions->FindFunction(func->Function);

                if(calledFunction) {

                    AnalysisOperation newOp(
                        calledFunction->GetActions(), operation.AvailableFunctions);

                    if(ResolveCallParameters(newOp, *calledFunction, func->Params)) {
                        foundCalls.push_back(std::move(newOp));
                    }
                }
            }

        } else {
            // If the failure was due to unknown variable states execution should split here,
            // see the TODO above
        }
    }

    return std::make_tuple(true, foundCalls);
}
