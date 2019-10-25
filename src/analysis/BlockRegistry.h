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

    //! \brief Performs the static analysis starting from "main" and other good candidate
    //! functions
    std::vector<FoundProblem> PerformAnalysis() const;

private:
    std::unordered_map<std::string, CodeBlock> FunctionBlocks;
};

} // namespace smacpp