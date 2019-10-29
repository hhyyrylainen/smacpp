#pragma once

#include "parse/ProcessedAction.h"

#include <clang/Basic/SourceLocation.h>

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace smacpp {

class CodeBlock;
class BlockRegistry;

struct FoundProblem {
    enum class SEVERITY { Info, Warning, Error };

    //! Basic message with no location info
    FoundProblem(SEVERITY severity, const std::string& message, clang::SourceLocation loc);

    std::string FormatAsString() const;

    clang::SourceLocation Location;
    std::string Message;
    SEVERITY Severity;
};

//! Program state in analysis
class ProgramState : public VariableValueProvider {
public:
    void CreateLocal(VariableIdentifier identifier, VariableState initialState);
    void Assign(VariableIdentifier identifier, VariableState state);

    //! \returns True if current state matches Condition. Unknown variables are assumed to
    //! have the wrong value
    bool MatchesCondition(const Condition& condition) const;

    VariableState GetVariableValue(const VariableIdentifier& variable) const override;

    std::unordered_map<VariableIdentifier, VariableState> Variables;
};

//! Makes sure each codeblock is not analysed multiple times
class DoneAnalysisRegistry {
public:
    bool HasBeenDone(const CodeBlock* func, const std::vector<VariableState>& params);
    void Add(const CodeBlock* func, const std::vector<VariableState>& params);

    //! \brief Adds a call to the registry if it wasn't already added
    //! \returns True if the func call was not in the registry and was added
    bool CheckAndAdd(const CodeBlock* func, const std::vector<VariableState>& params);

protected:
    std::unordered_map<std::string, std::vector<std::vector<VariableState>>>
        RecordedFunctionCalls;
};

//! A single operation the analysis is split into
class AnalysisOperation {
public:
    AnalysisOperation(const std::vector<std::unique_ptr<ProcessedAction>>& actions,
        const BlockRegistry* availableFunctions, std::vector<FoundProblem>& reportProblems,
        DoneAnalysisRegistry& doneOps) :
        Actions(actions),
        State(std::make_shared<ProgramState>()), AvailableFunctions(availableFunctions),
        Problems(reportProblems), DoneOperations(doneOps)
    {}

    // Double dispatch
    void HandleAction(const action::FunctionCall* call);
    void HandleAction(const action::VarDeclared* var);
    void HandleAction(const action::VarAssigned* var);
    void HandleAction(const action::ArrayIndexAccess* index);

    //! Base action with no action
    void HandleAction(const ProcessedAction* action) {}

public:
    const std::vector<std::unique_ptr<ProcessedAction>>& Actions;
    std::shared_ptr<ProgramState> State;

    std::list<AnalysisOperation> FoundCalls;

    //! Used for recursion detection
    const CodeBlock* CurrentFunction = nullptr;
    const BlockRegistry* AvailableFunctions = nullptr;
    std::vector<FoundProblem>& Problems;
    DoneAnalysisRegistry& DoneOperations;
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

    static bool ResolveCallParameters(AnalysisOperation& operation, const CodeBlock& function,
        const std::vector<VariableState>& callParameters);

private:
    std::tuple<bool, std::list<AnalysisOperation>> PerformAnalysisOperation(
        AnalysisOperation& operation);

private:
    std::vector<FoundProblem>& Problems;
    DoneAnalysisRegistry AlreadyQueuedOps;
};

} // namespace smacpp
