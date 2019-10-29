#pragma once

#include <clang/AST/Stmt.h>

#include <cstdint>
#include <string>
#include <variant>

namespace smacpp {

class VariableValueProvider;

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

class UnknownVariableStateException : std::runtime_error {
public:
    UnknownVariableStateException(const char* what) : std::runtime_error(what) {}
};

class VariableState {
public:
    enum class STATE { Unknown, Primitive, Buffer, CopyVar };

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

    //! \brief Resolves the actual value if this state is copied from a variable
    VariableState Resolve(const VariableValueProvider& otherVariables) const;

    //! \brief Converts this to a 0 or 1
    //! \exception UnknownVariableStateException if unknown
    int ToZeroOrNonZero() const;

    std::string Dump() const;

    bool operator==(const VariableState& other) const
    {
        return State == other.State && Value == other.Value;
    }

    STATE State = STATE::Unknown;

    std::variant<std::monostate, BufferInfo, PrimitiveInfo, VarCopyInfo> Value;
};

struct ValueRange {
public:
    enum class RANGE_CLASS { NotZero, Zero };

public:
    ValueRange(RANGE_CLASS type) : Type(type) {}

    ValueRange Negate() const;

    //! \returns True if the provided variable state satisfies this range
    bool Matches(const VariableState& state) const;

    std::string Dump() const;

    RANGE_CLASS Type;
};



} // namespace smacpp
