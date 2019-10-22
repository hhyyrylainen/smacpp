#pragma once

#include "Condition.h"

#include <sstream>
#include <variant>

namespace smacpp {

struct BufferInfo {
public:
    BufferInfo(std::nullptr_t) : NullPtr(true) {}
    BufferInfo(size_t size) : AllocatedSize(size) {}

    std::string Dump() const;

    bool NullPtr = false;
    size_t AllocatedSize = 0;
};

struct PrimitiveInfo {
public:
    using Integer = long long;

    PrimitiveInfo(Integer intValue) : Value(intValue) {}

    std::string Dump() const;

    std::variant<bool, Integer, double> Value;
};

struct VarCopyInfo {
    VarCopyInfo(VariableIdentifier source) : Source(source) {}

    std::string Dump() const;

    VariableIdentifier Source;
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

    std::string Dump() const;

    STATE State = STATE::Unknown;

    std::variant<std::monostate, BufferInfo, PrimitiveInfo, VarCopyInfo> Value;
};

//! \brief Some action the program takes that is relevant for static analysis
class ProcessedAction {
public:
    ProcessedAction(Condition condition) : If(condition) {}
    virtual ~ProcessedAction() = default;

    virtual std::string Dump() const;

protected:
    virtual void DumpSpecialized(std::stringstream& sstream) const = 0;

public:
    //! Action is taken when this condition is true
    const Condition If;
};

namespace action {
class VarDeclared : public ProcessedAction {
public:
    VarDeclared(Condition condition, VariableIdentifier var, VariableState state) :
        ProcessedAction(condition), Variable(var), State(state)
    {}

protected:
    void DumpSpecialized(std::stringstream& sstream) const override;

public:
    const VariableIdentifier Variable;
    const VariableState State;
};

class VarAssigned : public ProcessedAction {
public:
    VarAssigned(Condition condition, VariableIdentifier var, VariableState state) :
        ProcessedAction(condition), Variable(var), State(state)
    {}

protected:
    void DumpSpecialized(std::stringstream& sstream) const override;

public:
    const VariableIdentifier Variable;
    const VariableState State;
};

//! \brief Array index read that should be checked to be within the buffer size
class ArrayIndexAccess : public ProcessedAction {
public:
    ArrayIndexAccess(Condition condition, VariableIdentifier array, VariableState index) :
        ProcessedAction(condition), Array(array), Index(index)
    {}

protected:
    void DumpSpecialized(std::stringstream& sstream) const override;

public:
    const VariableIdentifier Array;
    const VariableState Index;
};

}; // namespace action


} // namespace smacpp
