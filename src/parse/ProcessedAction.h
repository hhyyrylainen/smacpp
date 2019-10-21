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

    std::string Dump() const;

    std::variant<bool, Integer, double> Value;
};

class VariableState {
public:
    enum class STATE { Unknown, Primitive, Buffer };

    //! Sets from a buffer
    void Set(BufferInfo buffer)
    {
        State = STATE::Buffer;
        Value = buffer;
    }

    std::string Dump() const;

    STATE State = STATE::Unknown;

    std::variant<std::monostate, BufferInfo, PrimitiveInfo> Value;
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

}; // namespace action


} // namespace smacpp
