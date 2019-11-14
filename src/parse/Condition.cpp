// ------------------------------------ //
#include "Condition.h"

#include "LiteralStateVisitor.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <sstream>

using namespace smacpp;
// ------------------------------------ //
class ConditionParseVisitor : public clang::RecursiveASTVisitor<ConditionParseVisitor> {
public:
    // FunctionVisitor(clang::ASTContext& context) : ValueVisitBase(context) {}

    // No clue why traverse doesn't work here
    bool VisitBinaryOperator(clang::BinaryOperator* op)
    {
        clang::Expr* lhs = op->getLHS();
        clang::Expr* rhs = op->getRHS();

        if(!lhs || !rhs)
            return true;

        if(op->getReferencedDeclOfCallee()) {
            llvm::outs() << "binary operator has decl of callee, this is not handled: ";
            llvm::outs() << "decl of callee: ";
            op->getReferencedDeclOfCallee()->dump();
            return true;
        }

        // llvm::outs() << "found operator ";
        // llvm::outs() << "lhs: ";
        // lhs->dump();
        // llvm::outs() << "rhs: ";
        // rhs->dump();

        ConditionParseVisitor lhsVisitor;
        lhsVisitor.TraverseStmt(lhs);

        ConditionParseVisitor rhsVisitor;
        rhsVisitor.TraverseStmt(rhs);

        const auto lhsConstant = LiteralFromExpr(lhs);
        const auto rhsConstant = LiteralFromExpr(rhs);

        if((lhsVisitor.Variable || lhsConstant) && (rhsVisitor.Variable || rhsConstant)) {
            std::optional<COMPARISON> translatedOp;

            switch(op->getOpcode()) {
            case clang::BO_LT: translatedOp = COMPARISON::LESS_THAN; break;
            case clang::BO_LE: translatedOp = COMPARISON::LESS_THAN_EQUAL; break;
            case clang::BO_GT: translatedOp = COMPARISON::GREATER_THAN; break;
            case clang::BO_GE: translatedOp = COMPARISON::GREATER_THAN_EQUAL; break;
            case clang::BO_EQ: translatedOp = COMPARISON::EQUAL; break;
            case clang::BO_NE: translatedOp = COMPARISON::NOT_EQUAL; break;
            case clang::BO_LAnd: {
                llvm::outs() << "Condition binary and not implemented\n";
                return true;
            }
            default:
                llvm::outs() << "unknown binary operator opcode for two variables: "
                             << op->getOpcode() << "\n";
                return false;
            }

            if(!lhsVisitor.Variable) {

                llvm::outs() << "TODO: implement constant comparing to a variable (only "
                                "variable to constant comparison works)\n";
                return false;
            }

            if(rhsVisitor.Variable) {
                Parts = Condition::Part(VariableValueCondition(
                    *lhsVisitor.Variable, ValueRange(*translatedOp, *rhsVisitor.Variable)));
            } else {

                Parts = Condition::Part(VariableValueCondition(
                    *lhsVisitor.Variable, ValueRange(*translatedOp, *rhsConstant)));
            }
            return false;
        }

        llvm::outs() << "binary operator has lhs and rhs that couldn't be combined\n";

        return true;
    }

    bool VisitImplicitCastExpr(clang::ImplicitCastExpr* expr)
    {
        switch(expr->getCastKind()) {
        case clang::CK_FixedPointToBoolean:
        case clang::CK_FloatingComplexToBoolean:
        case clang::CK_FloatingToBoolean:
        case clang::CK_IntegralComplexToBoolean:
        case clang::CK_IntegralToBoolean:
        case clang::CK_MemberPointerToBoolean:
        case clang::CK_PointerToBoolean: {
            ConditionParseVisitor subVisitor;
            subVisitor.TraverseStmt(expr->getSubExpr());

            if(!subVisitor.Variable) {
                llvm::outs() << "could not find implicit cast referenced variable\n";
                return false;
            }

            Parts = Condition::Part(VariableValueCondition(
                *subVisitor.Variable, ValueRange(ValueRange::RANGE_CLASS::NotZero)));
            return false;
        }
        default: return true;
        }
    }

    bool VisitDeclRefExpr(clang::DeclRefExpr* ref)
    {
        if(clang::VarDecl* var = clang::dyn_cast<clang::VarDecl>(ref->getDecl()); var) {

            if(Variable) {
                llvm::outs() << "found multiple variables in condition, this should have been "
                                "handled by a higher level visitor\n";
            }

            Variable = VariableIdentifier(var);
        }

        return true;
    }

    std::optional<VariableState> LiteralFromExpr(clang::Expr* expr)
    {
        LiteralStateVisitor visitor;
        visitor.TraverseStmt(expr);

        return visitor.FoundValue;
    }

    void CheckIfOnlyVariable()
    {
        if(!Parts && Variable) {
            Parts = Condition::Part(VariableValueCondition(
                *Variable, ValueRange(ValueRange::RANGE_CLASS::NotZero)));
        }
    }

    std::optional<VariableIdentifier> Variable;
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
// VariableStateCondition
VariableStateCondition VariableStateCondition::Negate() const
{
    return VariableStateCondition(State, Value.Negate());
}

std::string VariableStateCondition::Dump() const
{
    return State.Dump() + " " + Value.Dump();
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

        return value->Value.Matches(actualValue, values);

    } else if(auto value = std::get_if<VariableStateCondition>(&Value); value) {

        const auto actualValue = value->State.Resolve(values);

        // TODO: somehow pass that this is unknown to the top level (maybe an exception?)
        if(actualValue.State == VariableState::STATE::Unknown) {
            return false;
        }

        return value->Value.Matches(actualValue, values);

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

    } else if(auto value = std::get_if<VariableStateCondition>(&Value); value) {

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

    } else if(auto value = std::get_if<VariableStateCondition>(&Value); value) {

        return value->Dump();

    } else if(auto combined = std::get_if<CombinedParts>(&Value); combined) {

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
    visitor.CheckIfOnlyVariable();

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

Condition Condition::Or(const Condition& other) const
{
    if(Tautology || other.Tautology) {
        Condition combined;
        combined.SetTautology(true);
        return combined;
    }

    return Condition(Part(std::make_shared<Part>(*VariableConditions), COMBINE_OPERATOR::Or,
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
