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

bool PrimitiveInfo::CompareTo(COMPARISON op, const PrimitiveInfo& other) const
{
    // Direct operator calls work here
    if(Value.index() == other.Value.index()) {
        switch(op) {
        case COMPARISON::LESS_THAN: return Value < other.Value;
        case COMPARISON::LESS_THAN_EQUAL: return Value <= other.Value;
        case COMPARISON::GREATER_THAN: return Value > other.Value;
        case COMPARISON::GREATER_THAN_EQUAL: return Value >= other.Value;
        case COMPARISON::NOT_EQUAL: return Value != other.Value;
        case COMPARISON::EQUAL: return Value == other.Value;
        }

        throw std::runtime_error("unhandled COMPARISON in PrimitiveInfo");
    }

    // A really basic comparison which loses information
    const auto first = AsInteger();
    const auto second = other.AsInteger();

    switch(op) {
    case COMPARISON::LESS_THAN: return first < second;
    case COMPARISON::LESS_THAN_EQUAL: return first <= second;
    case COMPARISON::GREATER_THAN: return first > second;
    case COMPARISON::GREATER_THAN_EQUAL: return first >= second;
    case COMPARISON::NOT_EQUAL: return first != second;
    case COMPARISON::EQUAL: return first == second;
    }

    throw std::runtime_error("unhandled COMPARISON in PrimitiveInfo");
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

bool VariableState::CompareTo(COMPARISON op, const VariableState& other) const
{
    // TODO: maybe throw here to allow detecting this
    if(State == STATE::Unknown || other.State == STATE::Unknown)
        return false;

    // TODO: some different states could probably be compared. Like Buffer not 0
    if(State != other.State)
        return false;

    switch(State) {
    case STATE::Primitive:
        return std::get<PrimitiveInfo>(Value).CompareTo(
            op, std::get<PrimitiveInfo>(other.Value));
        // These don't store enough info to be comparable
    // case STATE::Buffer: return std::get<BufferInfo>(Value).Dump();
    default: return false;
    }
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
bool ValueRange::Matches(
    const VariableState& state, const VariableValueProvider& otherVariables) const
{
    switch(Type) {
    case RANGE_CLASS::NotZero: return state.ToZeroOrNonZero() != 0;
    case RANGE_CLASS::Zero: return state.ToZeroOrNonZero() == 0;
    case RANGE_CLASS::Comparison:
        return state.CompareTo(Comparison, otherVariables.GetVariableValue(*ComparedTo));
    case RANGE_CLASS::Constant: return state.CompareTo(Comparison, *ComparedConstant);
    }

    throw std::runtime_error("this should be unreachable");
}
// ------------------------------------ //
ValueRange ValueRange::Negate() const
{
    switch(Type) {
    case RANGE_CLASS::NotZero: return ValueRange(RANGE_CLASS::Zero);
    case RANGE_CLASS::Zero: return ValueRange(RANGE_CLASS::NotZero);
    case RANGE_CLASS::Comparison: return ValueRange(::Negate(Comparison), *ComparedTo);
    case RANGE_CLASS::Constant: return ValueRange(::Negate(Comparison), *ComparedConstant);
    }

    throw std::runtime_error("negate not implemented for this ValueRange type");
}

std::string ValueRange::Dump() const
{
    switch(Type) {
    case RANGE_CLASS::NotZero: return "!= 0";
    case RANGE_CLASS::Zero: return "== 0";
    case RANGE_CLASS::Comparison:
        return ::Dump(Comparison) + std::string(" ") + ComparedTo->Dump();
    case RANGE_CLASS::Constant:
        return ::Dump(Comparison) + std::string(" ") + ComparedConstant->Dump();
    default: return "== ?";
    }
}
