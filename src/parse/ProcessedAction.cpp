// ------------------------------------ //
#include "ProcessedAction.h"

using namespace smacpp;
using namespace smacpp::action;
// ------------------------------------ //

// ------------------------------------ //
// BufferInfo
std::string BufferInfo::Dump() const
{
    if(NullPtr)
        return "nullptr";
    return "buffer of size " + std::to_string(AllocatedSize);
}
// ------------------------------------ //
// PrimitiveInfo
std::string PrimitiveInfo::Dump() const
{
    if(auto var = std::get_if<bool>(&Value); var) {
        if(*var == true)
            return "true";
        return "false";
    } else if(auto var = std::get_if<Integer>(&Value); var) {
        return std::to_string(*var);
    } else if(auto var = std::get_if<double>(&Value); var) {
        return std::to_string(*var);
    } else {
        return "ERROR: unhandled variant type";
    }
}
// ------------------------------------ //
// VariableState
std::string VariableState::Dump() const
{
    switch(State) {
    case STATE::Unknown: return "unknown";
    case STATE::Primitive: return std::get<PrimitiveInfo>(Value).Dump();
    case STATE::Buffer: return std::get<BufferInfo>(Value).Dump();
    }

    throw std::runtime_error("VariableState is in invalid state");
}
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
void VarDeclared::DumpSpecialized(std::stringstream& sstream) const
{
    sstream << "VarDeclared " << Variable.Dump() << " value: " << State.Dump();
}
