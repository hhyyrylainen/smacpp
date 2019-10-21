// ------------------------------------ //
#include "Condition.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <sstream>

using namespace smacpp;
// ------------------------------------ //
class ConditionParseVisitor : public clang::RecursiveASTVisitor<ConditionParseVisitor> {
public:
    // FunctionVisitor(clang::ASTContext& context) : ValueVisitBase(context) {}

    bool VisitDeclRefExpr(clang::DeclRefExpr* ref)
    {
        if(clang::VarDecl* var = clang::dyn_cast<clang::VarDecl>(ref->getDecl()); var) {

            Parts.push_back(
                std::make_tuple(Condition::Part(VariableValue(VariableIdentifier(var),
                                    ValueRange(ValueRange::RANGE_CLASS::NotZero))),
                    COMBINE_OPERATOR::And));
        }

        return true;
    }

    std::vector<std::tuple<Condition::Part, COMBINE_OPERATOR>> Parts;
};
// ------------------------------------ //
// VariableIdentifier
VariableIdentifier::VariableIdentifier(clang::VarDecl* var) :
    Name(var->getQualifiedNameAsString())
{
    // var->getGlobalID();
}
// ------------------------------------ //
// ValueRange
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

// ------------------------------------ //
// VariableValue
VariableValue VariableValue::Negate() const
{
    return VariableValue(Variable, Value.Negate());
}

std::string VariableValue::Dump() const
{
    return Variable.Dump() + " " + Value.Dump();
}

// ------------------------------------ //
// Condition::Part
Condition::Part Condition::Part::Negate() const
{
    if(auto value = std::get_if<VariableValue>(&Value); value) {

        return Part(value->Negate());

    } else if(auto combined = std::get_if<CombinedParts>(&Value); value) {
        return Part(std::make_shared<Part>(std::get<0>(*combined)->Negate()),
            NegateCombineOperator(std::get<1>(*combined)),
            std::make_shared<Part>(std::get<2>(*combined)->Negate()));
    } else {
        throw std::runtime_error("negate not implemented for this variant type");
    }
}

std::string Condition::Part::Dump() const
{
    if(auto value = std::get_if<VariableValue>(&Value); value) {

        return value->Dump();

    } else if(auto combined = std::get_if<CombinedParts>(&Value); value) {

        std::stringstream sstream;
        sstream << "(";

        sstream << std::get<0>(*combined)->Dump() << ") "
                << CombineOperatorToString(std::get<1>(*combined)) << " (";

        sstream << std::get<2>(*combined)->Dump() << ")";
        return sstream.str();
    } else {
        return "invalid";
    }
}
// ------------------------------------ //
// Condition
Condition::Condition(clang::Stmt* stmt)
{
    ConditionParseVisitor visitor;
    visitor.TraverseStmt(stmt);

    if(DetectTautology(visitor.Parts)) {

        Tautology = true;
    } else {

        Parts = std::move(visitor.Parts);
    }
}

Condition::Condition(const std::vector<std::tuple<Part, COMBINE_OPERATOR>>& parts) :
    Parts(parts)
{
    Tautology = DetectTautology(Parts);

    if(Tautology)
        Parts.clear();
}
// ------------------------------------ //
Condition Condition::Negate() const
{
    if(Tautology) {
        Condition opposite;
        opposite.SetTautology(false);
        return opposite;
    }

    decltype(Parts) negatedParts;

    for(const auto& part : Parts) {

        negatedParts.push_back(std::make_tuple(
            std::get<0>(part).Negate(), NegateCombineOperator(std::get<1>(part))));
    }

    return Condition(negatedParts);
}

Condition Condition::And(const Condition& other) const
{
    if(other.Tautology) {
        Condition combined;
        combined.SetTautology(true);
        return combined;
    }

    decltype(Parts) combinedParts;

    // TODO: make this combine the conditions properly
    for(const auto& part : Parts) {
        combinedParts.push_back(part);
    }

    for(const auto& part : other.Parts) {
        combinedParts.push_back(part);
    }

    return Condition(combinedParts);
}
// ------------------------------------ //
std::string Condition::Dump() const
{
    if(Tautology)
        return "tautology";

    std::stringstream sstream;

    bool first = true;

    for(const auto& part : Parts) {

        if(first)
            sstream << " ";

        first = false;
        sstream << "(" << CombineOperatorToString(std::get<1>(part)) << " "
                << std::get<0>(part).Dump() << ")";
    }

    return sstream.str();
}
// ------------------------------------ //
bool Condition::DetectTautology(const std::vector<std::tuple<Part, COMBINE_OPERATOR>>& parts)
{
    if(parts.empty())
        return true;
    return false;
}
