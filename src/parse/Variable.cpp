// ------------------------------------ //
#include "Variable.h"

#include "parse/Condition.h"

#include <clang/AST/Decl.h>

using namespace smacpp;
// ------------------------------------ //
// VariableIdentifier
VariableIdentifier::VariableIdentifier(clang::VarDecl* var) :
    Name(var->getQualifiedNameAsString())
{
    // var->getGlobalID();
}
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
bool PrimitiveInfo::IsNonZero() const
{
    if(auto var = std::get_if<bool>(&Value); var) {
        return *var;
    } else if(auto var = std::get_if<Integer>(&Value); var) {
        return (*var) != 0;
    } else if(auto var = std::get_if<double>(&Value); var) {
        return (*var) != 0;
    } else {
        throw std::runtime_error("unhandled variant in PrimitiveInfo");
    }
}

PrimitiveInfo::Integer PrimitiveInfo::AsInteger() const
{
    if(auto var = std::get_if<bool>(&Value); var) {
        return *var;
    } else if(auto var = std::get_if<Integer>(&Value); var) {
        return *var;
    } else if(auto var = std::get_if<double>(&Value); var) {
        return *var;
    } else {
        throw std::runtime_error("unhandled variant in PrimitiveInfo");
    }
}
// ------------------------------------ //
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
std::string VarCopyInfo::Dump() const
{
    return "assign from " + Source.Dump();
}
// ------------------------------------ //
// VariableState
int VariableState::ToZeroOrNonZero() const
{
    switch(State) {
    case STATE::Unknown:
        throw UnknownVariableStateException("unknown variable in VariableState");
    case STATE::Primitive: return std::get<PrimitiveInfo>(Value).IsNonZero() ? 1 : 0;
    case STATE::Buffer: return std::get<BufferInfo>(Value).NullPtr ? 0 : 1;
    case STATE::CopyVar:
        throw UnknownVariableStateException(
            "copyvar must be resolved before call to ToZeroOrNonZero");
    }

    throw std::runtime_error("this should be unreachable");
}
// ------------------------------------ //
VariableState VariableState::Resolve(const VariableValueProvider& otherVariables) const
{
    VariableState result(*this);

    while(result.State == STATE::CopyVar) {

        result = otherVariables.GetVariableValue(std::get<VarCopyInfo>(Value).Source);
    }

    return result;
}
// ------------------------------------ //
std::string VariableState::Dump() const
{
    switch(State) {
    case STATE::Unknown: return "unknown";
    case STATE::Primitive: return std::get<PrimitiveInfo>(Value).Dump();
    case STATE::Buffer: return std::get<BufferInfo>(Value).Dump();
    case STATE::CopyVar: return std::get<VarCopyInfo>(Value).Dump();
    }

    throw std::runtime_error("VariableState is in invalid state");
}
// ------------------------------------ //
// ValueRange
bool ValueRange::Matches(const VariableState& state) const
{
    switch(Type) {
    case RANGE_CLASS::NotZero: return state.ToZeroOrNonZero() != 0;
    case RANGE_CLASS::Zero: return state.ToZeroOrNonZero() == 0;
    }

    throw std::runtime_error("this should be unreachable");
}
// ------------------------------------ //
ValueRange ValueRange::Negate() const
{
    switch(Type) {
    case RANGE_CLASS::NotZero: return ValueRange(RANGE_CLASS::Zero);
    case RANGE_CLASS::Zero: return ValueRange(RANGE_CLASS::NotZero);
    }

    throw std::runtime_error("negate not implemented for this ValueRange type");
}

std::string ValueRange::Dump() const
{
    switch(Type) {
    case RANGE_CLASS::NotZero: return "!= 0";
    case RANGE_CLASS::Zero: return "== 0";
    default: return "== ?";
    }
}
