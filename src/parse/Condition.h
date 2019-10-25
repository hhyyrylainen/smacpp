#pragma once

#include <clang/AST/Stmt.h>

#include <memory>
#include <unordered_map>
#include <variant>

namespace smacpp {

struct VariableIdentifier {
    VariableIdentifier(const std::string& name) : Name(name) {}

    VariableIdentifier(clang::VarDecl* var);

    std::string Dump() const
    {
        return Name;
    }

    //! \todo Implement proper scoping
    std::string Name;
};

struct ValueRange {
public:
    enum class RANGE_CLASS { NotZero, Zero };

public:
    ValueRange(RANGE_CLASS type) : Type(type) {}

    ValueRange Negate() const;

    std::string Dump() const;

    RANGE_CLASS Type;
};

struct VariableValue {
public:
    VariableValue(const std::string& variable, ValueRange value) :
        Variable(variable), Value(value)
    {}

    VariableValue(const VariableIdentifier& variable, ValueRange value) :
        Variable(variable), Value(value)
    {}

    VariableValue Negate() const;

    std::string Dump() const;

    VariableIdentifier Variable;
    ValueRange Value;
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

        Part(VariableValue value) : Value(value) {}

        Part(std::shared_ptr<Part> lhs, COMBINE_OPERATOR op, std::shared_ptr<Part> rhs) :
            Value(std::make_tuple(lhs, op, rhs))
        {}

        Part Negate() const;

        std::string Dump() const;

        //! shared_ptrs are used here to make copying work
        std::variant<VariableValue, CombinedParts> Value;
    };



public:
    //! Creates an always true condition
    Condition() : Tautology(true) {}

    Condition(clang::Stmt* stmt);

    Condition(const std::vector<std::tuple<Part, COMBINE_OPERATOR>>& parts);

    bool IsAlwaysTrue() const
    {
        return Tautology;
    }

    Condition Negate() const;

    Condition And(const Condition& other) const;

    void SetTautology(bool value)
    {
        Tautology = value;
    }

    std::string Dump() const;

    static bool DetectTautology(const std::vector<std::tuple<Part, COMBINE_OPERATOR>>& parts);

private:
    //! \todo Add contradiction
    bool Tautology = false;

    //! When not a tautology the parts are stored here
    std::vector<std::tuple<Part, COMBINE_OPERATOR>> Parts;
};
} // namespace smacpp


namespace std {

template<>
struct hash<smacpp::VariableIdentifier> {
    std::size_t operator()(const smacpp::VariableIdentifier& k) const
    {
        using std::hash;
        using std::size_t;

        return hash<std::string>()(k.Name);
    }
};

} // namespace std
