// ------------------------------------ //
#include "ProcessedAction.h"

#include "analysis/Analyzer.h"

using namespace smacpp;
using namespace smacpp::action;
// ------------------------------------ //
// ProcessedAction
std::string ProcessedAction::Dump() const
{
    std::stringstream sstream;

    sstream << If.Dump() << " ";

    DumpSpecialized(sstream);

    return sstream.str();
}
// ------------------------------------ //
// VarDeclared
void VarDeclared::Dispatch(AnalysisOperation& receiver) const
{
    receiver.HandleAction(this);
}

void VarDeclared::DumpSpecialized(std::stringstream& sstream) const
{
    sstream << "VarDeclared " << Variable.Dump() << " value: " << State.Dump();
}

// ------------------------------------ //
// VarAssigned
void VarAssigned::Dispatch(AnalysisOperation& receiver) const
{
    receiver.HandleAction(this);
}

void VarAssigned::DumpSpecialized(std::stringstream& sstream) const
{
    sstream << "VarAssigned " << Variable.Dump() << " = " << State.Dump();
}
// ------------------------------------ //
// ArrayIndexAccess
void ArrayIndexAccess::Dispatch(AnalysisOperation& receiver) const
{
    receiver.HandleAction(this);
}

void ArrayIndexAccess::DumpSpecialized(std::stringstream& sstream) const
{
    sstream << "ArrayIndexAccess " << Array.Dump() << "[" << Index.Dump() << "]";
}
// ------------------------------------ //
// FunctionCall
void FunctionCall::Dispatch(AnalysisOperation& receiver) const
{
    receiver.HandleAction(this);
}

void FunctionCall::DumpSpecialized(std::stringstream& sstream) const
{
    sstream << "FunctionCall " << Function << "(";

    bool first = true;

    for(const auto& param : Params) {
        if(!first)
            sstream << ", ";
        first = false;

        sstream << param.Dump();
    }
    sstream << ")";
}
