// ------------------------------------ //
#include "CodeBlock.h"

#include <sstream>

using namespace smacpp;
// ------------------------------------ //

// ------------------------------------ //
// CodeBlock
std::string CodeBlock::Dump() const
{
    std::stringstream sstream;

    sstream << "CodeBlock(" << Name << "):\n";

    sstream << " params: ";
    for(const auto& param : FunctionParameters)
        sstream << param.Dump() << " ";
    sstream << "\n";

    sstream << "actions:\n";
    for(const auto& action : Actions)
        sstream << " " << action->Dump() << "\n";

    sstream << "block end\n";
    return sstream.str();
}
