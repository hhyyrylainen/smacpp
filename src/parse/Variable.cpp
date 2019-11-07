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

BufferInfo BufferInfo::ApplyOperator(OPERATOR op, const BufferInfo& other) const
{
    switch(op) {
        // TODO: these should really return some value indicating these are unknown
    case OPERATOR::Add: return BufferInfo(nullptr);
    case OPERATOR::Multiply: return BufferInfo(nullptr);
    case OPERATOR::Subtract: return BufferInfo(nullptr);
    default: throw std::runtime_error("operator not supported on BufferInfo");
    }
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
template<class T>
std::optional<PrimitiveInfo> ApplyHelperOne(
    OPERATOR op, const PrimitiveInfo& first, const PrimitiveInfo& second)
{
    if(auto val = std::get_if<T>(&first.Value); val) {
        const auto otherVal = std::get<T>(second.Value);
        switch(op) {
        case OPERATOR::Add: return *val + otherVal;
        case OPERATOR::Multiply: return *val * otherVal;
        case OPERATOR::Subtract: return *val - otherVal;
        }

        throw std::runtime_error("unhandled OPERATOR in PrimitiveInfo");
    }

    return std::optional<PrimitiveInfo>{};
}

template<class T, class... Extra>
PrimitiveInfo ApplyHelper(OPERATOR op, const PrimitiveInfo& first, const PrimitiveInfo& second)
{
    auto current = ApplyHelperOne<T>(op, first, second);

    if(current)
        return *current;

    if constexpr(sizeof...(Extra) > 0) {
        return ApplyHelper<Extra...>(op, first, second);
    } else {
        throw std::runtime_error("template expansion reached end, this shouldn't happen");
    }
}

PrimitiveInfo PrimitiveInfo::ApplyOperator(OPERATOR op, const PrimitiveInfo& other) const
{
    if(Value.index() == other.Value.index()) {

        return ApplyHelper<bool, Integer, double>(op, *this, other);
    }

    // A really basic operator which loses information
    const auto first = AsInteger();
    const auto second = other.AsInteger();

    switch(op) {
    case OPERATOR::Add: return first + second;
    case OPERATOR::Multiply: return first * second;
    case OPERATOR::Subtract: return first - second;
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
// VarCopyInfo
std::string VarCopyInfo::Dump() const
{
    return "assign from " + Source.Dump();
}
// ------------------------------------ //
// ComputeInfo
ComputeInfo::ComputeInfo(const VariableState& lhs, OPERATOR op, const VariableState& rhs) :
    Operation(op), LHS(std::make_shared<VariableState>(lhs)),
    RHS(std::make_shared<VariableState>(rhs))
{}
// ------------------------------------ //
std::string ComputeInfo::Dump() const
{
    return LHS->Dump() + " " + ::Dump(Operation) + " " + RHS->Dump();
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
    case STATE::Compute:
    case STATE::CopyVar:
        throw UnknownVariableStateException(
            "copyvar must be resolved before call to ToZeroOrNonZero");
    }

    throw std::runtime_error("this should be unreachable");
}
// ------------------------------------ //
VariableState VariableState::Resolve(const VariableValueProvider& otherVariables) const
{
    return VariableState::ResolveValue(*this, otherVariables);
}

VariableState VariableState::ResolveValue(
    VariableState variable, const VariableValueProvider& otherVariables)
{
    while(variable.State == STATE::CopyVar) {

        variable =
            otherVariables.GetVariableValueRaw(std::get<VarCopyInfo>(variable.Value).Source);
    }

    if(variable.State == STATE::Compute) {
        variable = PerformComputation(std::get<ComputeInfo>(variable.Value), otherVariables);
    }

    return variable;
}
// ------------------------------------ //
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
VariableState VariableState::CreateOperatorApplyingState(
    OPERATOR op, const VariableState& other) const
{
    if(State == STATE::Unknown || other.State == STATE::Unknown)
        return VariableState();

    return VariableState(ComputeInfo(*this, op, other));
}
// ------------------------------------ //
VariableState VariableState::PerformComputation(
    const ComputeInfo& computation, const VariableValueProvider& otherVariables)
{
    const auto lhs = computation.LHS->Resolve(otherVariables);
    const auto rhs = computation.RHS->Resolve(otherVariables);

    if(lhs.State == STATE::Unknown || rhs.State == STATE::Unknown)
        return VariableState();

    // TODO: some different states could probably be applied an operator to. Like
    // Buffer and 1
    if(lhs.State != rhs.State)
        return VariableState();

    switch(lhs.State) {
    case STATE::Primitive:
        return std::get<PrimitiveInfo>(lhs.Value).ApplyOperator(
            computation.Operation, std::get<PrimitiveInfo>(rhs.Value));
    case STATE::Buffer:
        return std::get<BufferInfo>(lhs.Value).ApplyOperator(
            computation.Operation, std::get<BufferInfo>(rhs.Value));
    case STATE::Compute:
    case STATE::CopyVar:
        throw UnknownVariableStateException(
            "copyvar / recursive compute should have been solved (PerformComputation)");
    default: throw std::runtime_error("this should be unreachable");
    }
}
// ------------------------------------ //
std::string VariableState::Dump() const
{
    switch(State) {
    case STATE::Unknown: return "unknown";
    case STATE::Primitive:
    case STATE::Buffer:
    case STATE::CopyVar: return DumpValue();
    case STATE::Compute: return "(compute " + std::get<ComputeInfo>(Value).Dump() + ")";
    }

    throw std::runtime_error("VariableState is in invalid state");
}

std::string VariableState::DumpValue() const
{
    if(auto value = std::get_if<PrimitiveInfo>(&Value); value) {
        return value->Dump();
    } else if(auto value = std::get_if<BufferInfo>(&Value); value) {
        return value->Dump();
    } else if(auto value = std::get_if<VarCopyInfo>(&Value); value) {
        return value->Dump();
    } else {
        throw std::runtime_error("VariableState Value has unprintable type");
    }
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
