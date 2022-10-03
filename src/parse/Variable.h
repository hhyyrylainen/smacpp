#pragma once

#include "rotate.h"

#include <clang/AST/Stmt.h>

#include <cstdint>
#include <string>
#include <optional>
#include <variant>

namespace smacpp {

class VariableValueProvider;
class VariableState;

//! \todo The values inside should be renamed to match naming convention
enum class COMPARISON {
    LESS_THAN,
    LESS_THAN_EQUAL,
    GREATER_THAN,
    GREATER_THAN_EQUAL,
    NOT_EQUAL,
    EQUAL
};

enum class OPERATOR { Add, Multiply, Subtract };


struct VariableIdentifier {
    VariableIdentifier(const std::string& name) : Name(name) {}

    VariableIdentifier(clang::VarDecl* var);

    std::string Dump() const
    {
        return Name;
    }

    bool operator==(const VariableIdentifier& other) const
    {
        return Name == other.Name;
    }

    //! \todo Implement proper scoping
    std::string Name;
};

struct BufferInfo {
public:
    BufferInfo(std::nullptr_t) : NullPtr(true) {}
    BufferInfo(size_t size) : AllocatedSize(size) {}

    std::string Dump() const;

    BufferInfo ApplyOperator(OPERATOR op, const BufferInfo& other) const;

    // TODO: relative pointer addresses aren't known

    bool operator==(const BufferInfo& other) const
    {
        return NullPtr == other.NullPtr && AllocatedSize == other.AllocatedSize;
    }

    bool NullPtr = false;
    size_t AllocatedSize = 0;
};

struct PrimitiveInfo {
public:
    using Integer = long long;

    PrimitiveInfo(Integer intValue) : Value(intValue) {}

    bool IsNonZero() const;
    Integer AsInteger() const;

    std::string Dump() const;

    bool CompareTo(COMPARISON op, const PrimitiveInfo& other) const;
    PrimitiveInfo ApplyOperator(OPERATOR op, const PrimitiveInfo& other) const;

    bool operator==(const PrimitiveInfo& other) const
    {
        return Value == other.Value;
    }

    std::variant<bool, Integer, double> Value;
};

struct VarCopyInfo {
    VarCopyInfo(VariableIdentifier source) : Source(source) {}

    std::string Dump() const;

    bool operator==(const VarCopyInfo& other) const
    {
        return Source == other.Source;
    }

    VariableIdentifier Source;
};

struct ComputeInfo {
    ComputeInfo(const VariableState& lhs, OPERATOR op, const VariableState& rhs);

    std::string Dump() const;

    bool operator==(const ComputeInfo& other) const
    {
        return Operation == other.Operation && LHS == other.LHS && RHS == other.RHS;
    }

    OPERATOR Operation;
    std::shared_ptr<VariableState> LHS;
    std::shared_ptr<VariableState> RHS;
};

class UnknownVariableStateException : public std::runtime_error {
public:
    UnknownVariableStateException(const char* what) : std::runtime_error(what) {}
};



class VariableState {
public:
    enum class STATE { Unknown, Primitive, Buffer, CopyVar, Compute };

public:
    VariableState() {}

    VariableState(const BufferInfo& data)
    {
        Set(data);
    }

    VariableState(const VarCopyInfo& data)
    {
        Set(data);
    }

    VariableState(const PrimitiveInfo& data)
    {
        Set(data);
    }

    VariableState(const ComputeInfo& compute)
    {
        Set(compute);
    }

    VariableState(const VariableState& other) : State(other.State), Value(other.Value) {}
    VariableState(VariableState&& other) noexcept :
        State(other.State), Value(std::move(other.Value))
    {}

    //! Sets from a buffer
    void Set(BufferInfo buffer)
    {
        State = STATE::Buffer;
        Value = buffer;
    }

    //! Copied from another var
    void Set(VarCopyInfo copyInfo)
    {
        State = STATE::CopyVar;
        Value = copyInfo;
    }

    //! Sets from a known primitive value
    void Set(PrimitiveInfo primitive)
    {
        State = STATE::Primitive;
        Value = primitive;
    }

    void Set(ComputeInfo compute)
    {
        State = STATE::Compute;
        Value = compute;
    }

    //! \brief Resolves the actual value if this state is copied from a variable
    VariableState Resolve(const VariableValueProvider& otherVariables) const;

    //! \brief Compares this variable to another with an operator
    bool CompareTo(COMPARISON op, const VariableState& other) const;

    //! \brief Returns a new state that is either a fully computed one or which will compute a
    //! value when resolving
    //! \todo If this contains calculations that all use known values this could be resolved
    //! already
    VariableState CreateOperatorApplyingState(OPERATOR op, const VariableState& other) const;

    //! \brief Converts this to a 0 or 1
    //! \exception UnknownVariableStateException if unknown
    int ToZeroOrNonZero() const;

    std::string Dump() const;
    std::string DumpValue() const;

    VariableState operator=(const VariableState& other)
    {
        State = other.State;
        Value = other.Value;
        return *this;
    }

    bool operator==(const VariableState& other) const
    {
        return State == other.State && Value == other.Value;
    }

    static VariableState PerformComputation(
        const ComputeInfo& computation, const VariableValueProvider& otherVariables);

    static VariableState ResolveValue(
        VariableState variable, const VariableValueProvider& otherVariables);

    STATE State = STATE::Unknown;

    std::variant<std::monostate, BufferInfo, PrimitiveInfo, VarCopyInfo, ComputeInfo> Value;
};

struct ValueRange {
public:
    enum class RANGE_CLASS { NotZero, Zero, Comparison, Constant };

public:
    ValueRange(RANGE_CLASS type) : Type(type) {}
    ValueRange(COMPARISON op, VariableIdentifier other) :
        Type(RANGE_CLASS::Comparison), Comparison(op), ComparedTo(other)
    {}

    ValueRange(COMPARISON op, VariableState constant) :
        Type(RANGE_CLASS::Constant), Comparison(op), ComparedConstant(constant)
    {}

    ValueRange Negate() const;

    //! \returns True if the provided variable state satisfies this range
    bool Matches(
        const VariableState& state, const VariableValueProvider& otherVariables) const;

    std::string Dump() const;

    RANGE_CLASS Type;
    COMPARISON Comparison;
    std::optional<VariableIdentifier> ComparedTo;
    std::optional<VariableState> ComparedConstant;
};

inline COMPARISON Negate(COMPARISON op)
{
    switch(op) {
    case COMPARISON::LESS_THAN: return COMPARISON::GREATER_THAN_EQUAL;
    case COMPARISON::LESS_THAN_EQUAL: return COMPARISON::GREATER_THAN;
    case COMPARISON::GREATER_THAN: return COMPARISON::LESS_THAN_EQUAL;
    case COMPARISON::GREATER_THAN_EQUAL: return COMPARISON::LESS_THAN;
    case COMPARISON::NOT_EQUAL: return COMPARISON::EQUAL;
    case COMPARISON::EQUAL: return COMPARISON::NOT_EQUAL;
    default: throw std::runtime_error("negate not implemented for this COMPARISON");
    }
}

inline const char* Dump(COMPARISON op)
{
    switch(op) {
    case COMPARISON::LESS_THAN: return "<";
    case COMPARISON::LESS_THAN_EQUAL: return "<=";
    case COMPARISON::GREATER_THAN: return ">";
    case COMPARISON::GREATER_THAN_EQUAL: return ">=";
    case COMPARISON::NOT_EQUAL: return "!=";
    case COMPARISON::EQUAL: return "==";
    default: throw std::runtime_error("dump not implemented for this COMPARISON");
    }
}

inline const char* Dump(OPERATOR op)
{
    switch(op) {
    case OPERATOR::Add: return "+";
    case OPERATOR::Multiply: return "*";
    case OPERATOR::Subtract: return "-";
    default: throw std::runtime_error("dump not implemented for this OPERATOR");
    }
}

} // namespace smacpp

namespace std {

template<>
struct hash<smacpp::VariableIdentifier> {
    std::size_t operator()(const smacpp::VariableIdentifier& k) const
    {
        return hash<std::string>()(k.Name);
    }
};

template<>
struct hash<smacpp::BufferInfo> {
    std::size_t operator()(const smacpp::BufferInfo& k) const
    {
        return hash()(k.AllocatedSize) ^ (hash()(k.NullPtr) << 1);
    }
};

template<>
struct hash<smacpp::PrimitiveInfo> {
    std::size_t operator()(const smacpp::PrimitiveInfo& k) const
    {
        return hash<std::variant<bool, smacpp::PrimitiveInfo::Integer, double>>()(k.Value);
    }
};

template<>
struct hash<smacpp::VarCopyInfo> {
    std::size_t operator()(const smacpp::VarCopyInfo& k) const
    {
        return hash<smacpp::VariableIdentifier>()(k.Source);
    }
};

template<>
struct hash<smacpp::ComputeInfo> {
    std::size_t operator()(const smacpp::ComputeInfo& k) const;
};

template<>
struct hash<smacpp::VariableState> {
    std::size_t operator()(const smacpp::VariableState& k) const
    {
        return hash<std::variant<std::monostate, smacpp::BufferInfo, smacpp::PrimitiveInfo,
                   smacpp::VarCopyInfo, smacpp::ComputeInfo>>()(k.Value) ^
               (hash<smacpp::VariableState::STATE>()(k.State) << 1);
    }
};

inline std::size_t hash<smacpp::ComputeInfo>::operator()(const smacpp::ComputeInfo& k) const
{
    return hash<smacpp::VariableState>()(*k.LHS) ^
           (hash<smacpp::VariableState>()(*k.LHS) << 1) ^
           (hash<smacpp::OPERATOR>()(k.Operation) << 2);
}

// For variable lists to work with hashing
template<>
struct hash<std::vector<smacpp::VariableState>> {
    std::size_t operator()(const std::vector<smacpp::VariableState>& k) const
    {
        std::size_t result = hash<size_t>()(k.size());

        for(size_t i = 0; i < k.size(); ++i) {

            result ^= ::rotateLeft(hash<smacpp::VariableState>()(k[i]), i);
        }

        return result;
    }
};

} // namespace std
