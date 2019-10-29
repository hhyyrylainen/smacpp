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
FoundProblem::FoundProblem(
    SEVERITY severity, const std::string& message, clang::SourceLocation loc) :
    Severity(severity),
    Message(message), Location(loc)
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
    // TODO: printing Location here needs a clang SourceManager reference
    return sstream.str();
}
// ------------------------------------ //
// ProgramState
void ProgramState::CreateLocal(VariableIdentifier identifier, VariableState initialState)
{
    // TODO: allow shadowing globals and locals defined in upper scope
    Variables[identifier] = initialState;
}

void ProgramState::Assign(VariableIdentifier identifier, VariableState state)
{
    Variables[identifier] = state;
}
// ------------------------------------ //
bool ProgramState::MatchesCondition(const Condition& condition) const
{
    return condition.Evaluate(*this);
}

VariableState ProgramState::GetVariableValue(const VariableIdentifier& variable) const
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
// DoneAnalysisRegistry
bool DoneAnalysisRegistry::HasBeenDone(
    const CodeBlock* func, const std::vector<VariableState>& params)
{
    const auto found = RecordedFunctionCalls.find(func->GetName());

    if(found == RecordedFunctionCalls.end()) {
        return false;
    }

    for(const auto& paramSet : found->second) {

        if(params == paramSet)
            return true;
    }

    return false;
}

void DoneAnalysisRegistry::Add(const CodeBlock* func, const std::vector<VariableState>& params)
{
    const auto existing = RecordedFunctionCalls.find(func->GetName());

    if(existing == RecordedFunctionCalls.end()) {
        RecordedFunctionCalls[func->GetName()] = {params};
    } else {
        existing->second.push_back(params);
    }
}

bool DoneAnalysisRegistry::CheckAndAdd(
    const CodeBlock* func, const std::vector<VariableState>& params)
{
    if(HasBeenDone(func, params))
        return false;

    Add(func, params);
    return true;
}

// ------------------------------------ //
// AnalysisOperation
void AnalysisOperation::HandleAction(const action::FunctionCall* call)
{
    const CodeBlock* calledFunction = AvailableFunctions->FindFunction(call->Function);

    if(calledFunction) {

        // if(calledFunction == CurrentFunction) {
        //     // TODO: add proper handling for recursion
        //     return;
        // }

        AnalysisOperation newOp(
            calledFunction->GetActions(), AvailableFunctions, Problems, DoneOperations);
        newOp.CurrentFunction = calledFunction;

        if(Analyzer::ResolveCallParameters(newOp, *calledFunction, call->Params)) {

            // TODO: this should be moved to use the resolved parameters
            if(DoneOperations.CheckAndAdd(calledFunction, call->Params)) {
                FoundCalls.push_back(std::move(newOp));
            }
        }
    }
}

void AnalysisOperation::HandleAction(const action::VarDeclared* var)
{
    // TODO: should resolve happen here?
    State->CreateLocal(var->Variable, var->State);
}

void AnalysisOperation::HandleAction(const action::VarAssigned* var)
{
    // TODO: should resolve happen here?
    State->Assign(var->Variable, var->State);
}

void AnalysisOperation::HandleAction(const action::ArrayIndexAccess* index)
{
    const auto array = State->GetVariableValue(index->Array).Resolve(*State);

    if(array.State == VariableState::STATE::Unknown)
        return;

    const auto indexVar = index->Index.Resolve(*State);

    if(indexVar.State == VariableState::STATE::Unknown)
        return;

    if(auto buf = std::get_if<BufferInfo>(&array.Value); buf) {

        // TODO: emit line numbers
        if(buf->NullPtr) {
            Problems.push_back(FoundProblem(
                FoundProblem::SEVERITY::Error, "Write to nullptr array", index->Location));
        } else {

            if(auto indexNumber = std::get_if<PrimitiveInfo>(&indexVar.Value); indexNumber) {
                if(buf->AllocatedSize <= indexNumber->AsInteger()) {

                    Problems.push_back(FoundProblem(FoundProblem::SEVERITY::Error,
                        "Buffer overflow: buffer size: " + std::to_string(buf->AllocatedSize) +
                            " used index: " + std::to_string(indexNumber->AsInteger()),
                        index->Location));
                }
            }
        }
    }
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
        AnalysisOperation entryAnalysis(
            entryPoint.GetActions(), availableFunctions, Problems, AlreadyQueuedOps);
        entryAnalysis.CurrentFunction = &entryPoint;

        if(!ResolveCallParameters(entryAnalysis, entryPoint, callParameters)) {
            Problems.push_back(FoundProblem(FoundProblem::SEVERITY::Error,
                "given parameters count mismatches analysis entrypoint parameter count",
                entryPoint.GetLocation()));
            return false;
        }

        toCheck.push_back(std::move(entryAnalysis));
        // TODO: this should be moved to use the resolved parameters
        AlreadyQueuedOps.Add(&entryPoint, callParameters);
    }

    while(!toCheck.empty()) {

        const auto result = PerformAnalysisOperation(toCheck.front());

        if(!std::get<0>(result)) {
            Problems.push_back(FoundProblem(FoundProblem::SEVERITY::Error,
                "an analysis step failed", clang::SourceLocation{}));
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

        const auto resolved = callParameters[i].Resolve(*operation.State);

        operation.State->CreateLocal(function.GetParameters()[i], resolved);
    }

    return true;
}
// ------------------------------------ //
std::tuple<bool, std::list<AnalysisOperation>> Analyzer::PerformAnalysisOperation(
    AnalysisOperation& operation)
{
    // TODO: when a conditional is uncertain the check needs to be split into two here to
    // independently check both uncertain outcomes

    for(auto iter = operation.Actions.begin(); iter != operation.Actions.end(); ++iter) {

        const ProcessedAction& action = **iter;

        if(operation.State->MatchesCondition(action.If)) {

            std::cout << "analysis at step: " << action.Dump() << "\n";
            action.Dispatch(operation);

        } else {
            // If the failure was due to unknown variable states execution should split here,
            // see the TODO above
        }
    }

    return std::make_tuple(true, operation.FoundCalls);
}
