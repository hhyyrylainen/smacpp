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

            if(Parts) {
                llvm::outs() << "found multiple conditions, ConditionParseVisitor doesn't "
                                "know how to handle this yet\n";
            }

            Parts = Condition::Part(VariableValueCondition(
                VariableIdentifier(var), ValueRange(ValueRange::RANGE_CLASS::NotZero)));
        }

        return true;
    }

    std::optional<Condition::Part> Parts;
};
// ------------------------------------ //
// VariableValueCondition
VariableValueCondition VariableValueCondition::Negate() const
{
    return VariableValueCondition(Variable, Value.Negate());
}

std::string VariableValueCondition::Dump() const
{
    return Variable.Dump() + " " + Value.Dump();
}

// ------------------------------------ //
// Condition::Part
bool Condition::Part::Evaluate(const VariableValueProvider& values) const
{
    if(auto value = std::get_if<VariableValueCondition>(&Value); value) {

        const auto actualValue = values.GetVariableValue(value->Variable);

        // TODO: somehow pass that this is unknown to the top level (maybe an exception?)
        if(actualValue.State == VariableState::STATE::Unknown) {
            return false;
        }

        return value->Value.Matches(actualValue);

    } else if(auto combined = std::get_if<CombinedParts>(&Value); combined) {

        switch(std::get<1>(*combined)) {
        case COMBINE_OPERATOR::And:
            if(std::get<0>(*combined)->Evaluate(values) &&
                std::get<2>(*combined)->Evaluate(values))
                return true;
            return false;
        case COMBINE_OPERATOR::Or:
            if(std::get<0>(*combined)->Evaluate(values) ||
                std::get<2>(*combined)->Evaluate(values))
                return true;
            return false;
        }

        throw std::runtime_error("unimplemented combine operator in Evaluate");

    } else {
        throw std::runtime_error("evaluate not implemented for this variant type");
    }
}
// ------------------------------------ //
Condition::Part Condition::Part::Negate() const
{
    if(auto value = std::get_if<VariableValueCondition>(&Value); value) {

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
    if(auto value = std::get_if<VariableValueCondition>(&Value); value) {

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

    if(!visitor.Parts || visitor.Parts->DetectTautology()) {

        Tautology = true;
    } else {

        VariableConditions = std::move(visitor.Parts);
    }
}

Condition::Condition(const Part& part)
{
    if(part.DetectTautology()) {
        Tautology = true;
    } else {

        VariableConditions = part;
    }
}
// ------------------------------------ //
bool Condition::Evaluate(const VariableValueProvider& values) const
{
    if(IsAlwaysTrue())
        return true;

    return VariableConditions->Evaluate(values);
}
// ------------------------------------ //
Condition Condition::Negate() const
{
    if(Tautology) {
        Condition opposite;
        opposite.SetTautology(false);
        return opposite;
    }

    return Condition(VariableConditions->Negate());
}

Condition Condition::And(const Condition& other) const
{
    if(other.Tautology) {
        Condition combined;
        combined.SetTautology(true);
        return combined;
    }

    if(Tautology) {
        return Condition(*other.VariableConditions);
    }

    return Condition(Part(std::make_shared<Part>(*VariableConditions), COMBINE_OPERATOR::And,
        std::make_shared<Part>(*other.VariableConditions)));
}
// ------------------------------------ //
std::string Condition::Dump() const
{
    if(Tautology)
        return "tautology";

    if(!VariableConditions)
        return "invalid";

    return VariableConditions->Dump();
}
