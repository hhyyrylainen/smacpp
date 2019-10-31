#pragma once

#include "Analyzer.h"
#include "parse/CodeBlock.h"

#include <unordered_map>
#include <vector>

namespace smacpp {

//! \brief Storage for all parsed CodeBlocks and running analysis on them
class BlockRegistry {
public:
    //! \brief Adds a block to this registry
    void AddBlock(CodeBlock&& block);

    const CodeBlock* FindFunction(const std::string& name) const;

    //! \brief Performs the static analysis starting from "main" and other good candidate
    //! functions
    std::vector<FoundProblem> PerformAnalysis(bool debug) const;

private:
    std::unordered_map<std::string, CodeBlock> FunctionBlocks;
};

} // namespace smacpp
