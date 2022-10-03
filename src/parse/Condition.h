#pragma once

#include "Variable.h"

#include <clang/AST/Stmt.h>

#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>

namespace smacpp {

struct VariableValueCondition {
public:
    VariableValueCondition(const std::string& variable, ValueRange value) :
        Variable(variable), Value(value)
    {}

    VariableValueCondition(const VariableIdentifier& variable, ValueRange value) :
        Variable(variable), Value(value)
    {}

    VariableValueCondition Negate() const;

    std::string Dump() const;

    VariableIdentifier Variable;
    ValueRange Value;
};

struct VariableStateCondition {
public:
    VariableStateCondition(VariableState state, ValueRange value) : State(state), Value(value)
    {}

    VariableStateCondition Negate() const;

    std::string Dump() const;

    VariableState State;
    ValueRange Value;
};

//! Interface for providing variable values for evaluating a Condition
class VariableValueProvider {
public:
    virtual ~VariableValueProvider() = default;

    virtual VariableState GetVariableValue(const VariableIdentifier& variable) const = 0;
    //! \brief Variable value without any resolving, needed by resolving itself
    virtual VariableState GetVariableValueRaw(const VariableIdentifier& variable) const = 0;
};

enum class COMBINE_OPERATOR { And, Or };

inline const char* CombineOperatorToString(COMBINE_OPERATOR op)
{
    switch(op) {
    case COMBINE_OPERATOR::And: return "and";
    case COMBINE_OPERATOR::Or: return "or";
    }
    return "invalid";
}

inline COMBINE_OPERATOR NegateCombineOperator(COMBINE_OPERATOR op)
{
    switch(op) {
    case COMBINE_OPERATOR::And: return COMBINE_OPERATOR::Or;
    case COMBINE_OPERATOR::Or: return COMBINE_OPERATOR::And;
    }
    return COMBINE_OPERATOR::And;
}

//! A parsed condition type
class Condition {
public:
    struct Part {
        using CombinedParts =
            std::tuple<std::shared_ptr<Part>, COMBINE_OPERATOR, std::shared_ptr<Part>>;

        Part(VariableValueCondition value) : Value(value) {}
        Part(VariableStateCondition value) : Value(value) {}

        Part(std::shared_ptr<Part> lhs, COMBINE_OPERATOR op, std::shared_ptr<Part> rhs) :
            Value(std::make_tuple(lhs, op, rhs))
        {}

        bool Evaluate(const VariableValueProvider& values) const;

        //! \brief Tries to detect if this is always true
        //! \todo implement
        bool DetectTautology() const
        {
            return false;
        }

        Part Negate() const;

        std::string Dump() const;

        //! shared_ptrs are used here to make copying work
        std::variant<VariableValueCondition, VariableStateCondition, CombinedParts> Value;
    };



public:
    //! Creates an always true condition
    Condition() : Tautology(true) {}

    Condition(clang::Stmt* stmt);

    Condition(const Part& part);

    bool Evaluate(const VariableValueProvider& values) const;

    bool IsAlwaysTrue() const
    {
        return Tautology;
    }

    Condition Negate() const;

    Condition And(const Condition& other) const;
    Condition Or(const Condition& other) const;

    void SetTautology(bool value)
    {
        Tautology = value;
    }

    std::string Dump() const;

private:
    //! \todo Add contradiction
    bool Tautology = false;

    //! When not a tautology the parts are stored here
    std::optional<Part> VariableConditions;
};
} // namespace smacpp
