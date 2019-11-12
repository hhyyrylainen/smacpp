// ------------------------------------ //
#include "BlockRegistry.h"

using namespace smacpp;
// ------------------------------------ //
void BlockRegistry::AddBlock(CodeBlock&& block)
{
    if(FunctionBlocks.find(block.GetName()) != FunctionBlocks.end()) {
        // TODO: report warning
        // Function with conflicting name, overwriting previous data
    }

    FunctionBlocks.insert_or_assign(block.GetName(), std::move(block));
}
// ------------------------------------ //
std::vector<FoundProblem> BlockRegistry::PerformAnalysis(bool debug) const
{
    std::vector<FoundProblem> problems;

    const auto mainIter = FunctionBlocks.find("main");

    if(mainIter != FunctionBlocks.end()) {

        Analyzer analyzer(problems);
        analyzer.SetDebug(debug);

        std::vector<VariableState> params;

        if(mainIter->second.GetParameters().size() == 2 ||
            mainIter->second.GetParameters().size() == 3) {

            // TODO: these could be more intelligently done
            while(mainIter->second.GetParameters().size() != params.size()) {
                params.push_back(VariableState{});
            }
        }

        if(!analyzer.BeginAnalysis(mainIter->second, this, params)) {

            problems.push_back(FoundProblem(FoundProblem::SEVERITY::Error,
                "Analysis encountered a fatal error", mainIter->second.GetLocation()));
        }

    } else {
        // TODO: this should only be a warning / info if some other function that could be
        // started from is found
        problems.push_back(FoundProblem(FoundProblem::SEVERITY::Error,
            "'main' function was not found", clang::SourceLocation{}));
    }

    return problems;
}
// ------------------------------------ //
const CodeBlock* BlockRegistry::FindFunction(const std::string& name) const
{
    const auto found = FunctionBlocks.find(name);

    if(found == FunctionBlocks.end())
        return nullptr;

    return &found->second;
}
