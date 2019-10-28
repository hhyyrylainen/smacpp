#pragma once

#include "Condition.h"
#include "Variable.h"

#include <sstream>

namespace smacpp {

class AnalysisOperation;

//! \brief Some action the program takes that is relevant for static analysis
class ProcessedAction {
public:
    ProcessedAction(Condition condition) : If(condition) {}
    virtual ~ProcessedAction() = default;

    virtual std::string Dump() const;

    virtual void Dispatch(AnalysisOperation& receiver) const = 0;

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

    void Dispatch(AnalysisOperation& receiver) const override;

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

    void Dispatch(AnalysisOperation& receiver) const override;

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

    void Dispatch(AnalysisOperation& receiver) const override;

protected:
    void DumpSpecialized(std::stringstream& sstream) const override;

public:
    const VariableIdentifier Array;
    const VariableState Index;
};

class FunctionCall : public ProcessedAction {
public:
    FunctionCall(Condition condition, const std::string& function,
        const std::vector<VariableState>& params) :
        ProcessedAction(condition),
        Function(function), Params(params)
    {}

    void Dispatch(AnalysisOperation& receiver) const override;

protected:
    void DumpSpecialized(std::stringstream& sstream) const override;

public:
    const std::string Function;
    const std::vector<VariableState> Params;
};



}; // namespace action


} // namespace smacpp
